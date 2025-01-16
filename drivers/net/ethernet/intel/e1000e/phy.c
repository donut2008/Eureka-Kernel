cted\n");
			return;
		}

		cpuid(CPUID_FREQ_VOLT_CAPABILITIES, &eax, &ebx, &ecx, &edx);
		if ((edx & P_STATE_TRANSITION_CAPABLE)
			!= P_STATE_TRANSITION_CAPABLE) {
			pr_info("Power state transitions not supported\n");
			return;
		}
		*rc = 0;
	}
}

static int check_pst_table(struct powernow_k8_data *data, struct pst_s *pst,
		u8 maxvid)
{
	unsigned int j;
	u8 lastfid = 0xff;

	for (j = 0; j < data->numps; j++) {
		if (pst[j].vid > LEAST_VID) {
			pr_err(FW_BUG "vid %d invalid : 0x%x\n", j,
				pst[j].vid);
			return -EINVAL;
		}
		if (pst[j].vid < data->rvo) {
			/* vid + rvo >= 0 */
			pr_err(FW_BUG "0 vid exceeded with pstate %d\n", j);
			return -ENODEV;
		}
		if (pst[j].vid < maxvid + data->rvo) {
			/* vid + rvo >= maxvid */
			pr_err(FW_BUG "maxvid exceeded with pstate %d\n", j);
			return -ENODEV;
		}
		if (pst[j].fid > MAX_FID) {
			pr_err(FW_BUG "maxfid exceeded with pstate %d\n", j);
			return -ENODEV;
		}
		if (j && (pst[j].fid < HI_FID_TABLE_BOTTOM)) {
			/* Only first fid is allowed to be in "low" range */
			pr_err(FW_BUG "two low fids - %d : 0x%x\n", j,
				pst[j].fid);
			return -EINVAL;
		}
		if (pst[j].fid < lastfid)
			lastfid = pst[j].fid;
	}
	if (lastfid & 1) {
		pr_err(FW_BUG "lastfid invalid\n");
		return -EINVAL;
	}
	if (lastfid > LO_FID_TABLE_TOP)
		pr_info(FW_BUG "first fid not from lo freq table\n");

	return 0;
}

static void invalidate_entry(struct cpufreq_frequency_table *powernow_table,
		unsigned int entry)
{
	powernow_table[entry].frequency = CPUFREQ_ENTRY_INVALID;
}

static void print_basics(struct powernow_k8_data *data)
{
	int j;
	for (j = 0; j < data->numps; j++) {
		if (data->powernow_table[j].frequency !=
				CPUFREQ_ENTRY_INVALID) {
			pr_info("fid 0x%x (%d MHz), vid 0x%x\n",
				data->powernow_table[j].driver_data & 0xff,
				data->powernow_table[j].frequency/1000,
				data->powernow_table[j].driver_data >> 8);
		}
	}
	if (data->batps)
		pr_info("Only %d pstates on battery\n", data->batps);
}

static int fill_powernow_table(struct powernow_k8_data *data,
		struct pst_s *pst, u8 maxvid)
{
	struct cpufreq_frequency_table *powernow_table;
	unsigned int j;

	if (data->batps) {
		/* use ACPI support to get full speed on mains power */
		pr_warn("Only %d pstates usable (use ACPI driver for full range\n",
			data->batps);
		data->numps = data->batps;
	}

	for (j = 1; j < data->numps; j++) {
		if (pst[j-1].fid >= pst[j].fid) {
			pr_err("PST out of sequence\n");
			return -EINVAL;
		}
	}

	if (data->numps < 2) {
		pr_err("no p states to transition\n");
		return -ENODEV;
	}

	if (check_pst_table(data, pst, maxvid))
		return -EINVAL;

	powernow_table = kzalloc((sizeof(*powernow_table)
		* (data->numps + 1)), GFP_KERNEL);
	if (!powernow_table) {
		pr_err("powernow_table memory alloc failure\n");
		return -ENOMEM;
	}

	for (j = 0; j < data->numps; j++) {
		int freq;
		powernow_table[j].driver_data = pst[j].fid; /* lower 8 bits */
		powernow_table[j].driver_data |= (pst[j].vid << 8); /* upper 8 bits */
		freq = find_khz_freq_from_fid(pst[j].fid);
		powernow_table[j].frequency = freq;
	}
	powernow_table[data->numps].frequency = CPUFREQ_TABLE_END;
	powernow_table[data->numps].driver_data = 0;

	if (query_current_values_with_pending_wait(data)) {
		kfree(powernow_table);
		return -EIO;
	}

	pr_debug("cfid 0x%x, cvid 0x%x\n", data->currfid, data->currvid);
	data->powernow_table = powernow_table;
	if (cpumask_first(topology_core_cpumask(data->cpu)) == data->cpu)
		print_basics(data);

	for (j = 0; j < data->numps; j++)
		if ((pst[j].fid == data->currfid) &&
		    (pst[j].vid == data->currvid))
			return 0;

	pr_debug("currfid/vid do not match PST, ignoring\n");
	return 0;
}

/* Find and validate the PSB/PST table in BIOS. */
static int find_psb_table(struct powernow_k8_data *data)
{
	struct psb_s *psb;
	unsigned int i;
	u32 mvs;
	u8 maxvid;
	u32 cpst = 0;
	u32 thiscpuid;

	for (i = 0xc0000; i < 0xffff0; i += 0x10) {
		/* Scan BIOS looking for the signature. */
		/* It can not be at ffff0 - it is too big. */

		psb = phys_to_virt(i);
		if (memcmp(psb, PSB_ID_STRING, PSB_ID_STRING_LEN) != 0)
			continue;

		pr_debug("found PSB header at 0x%p\n", psb);

		pr_debug("table vers: 0x%x\n", psb->tableversion);
		if (psb->tableversion != PSB_VERSION_1_4) {
			pr_err(FW_BUG "PSB table is not v1.4\n");
			return -ENODEV;
		}

		pr_debug("flags: 0x%x\n", psb->flags1);
		if (psb->flags1) {
			pr_err(FW_BUG "unknown flags\n");
			return -ENODEV;
		}

		data->vstable = psb->vstable;
		pr_debug("voltage stabilization time: %d(*20us)\n",
				data->vstable);

		pr_debug("flags2: 0x%x\n", psb->flags2);
		data->rvo = psb->flags2 & 3;
		data->irt = ((psb->flags2) >> 2) & 3;
		mvs = ((psb->flags2) >> 4) & 3;
		data->vidmvs = 1 << mvs;
		data->batps = ((psb->flags2) >> 6) & 3;

		pr_debug("ramp voltage offset: %d\n", data->rvo);
		pr_debug("isochronous relief time: %d\n", data->irt);
		pr_debug("maximum voltage step: %d - 0x%x\n", mvs, data->vidmvs);

		pr_debug("numpst: 0x%x\n", psb->num_tables);
		cpst = psb->num_tables;
		if ((psb->cpuid == 0x00000fc0) ||
		    (psb->cpuid == 0x00000fe0)) {
			thiscpuid = cpuid_eax(CPUID_PROCESSOR_SIGNATURE);
			if ((thiscpuid == 0x00000fc0) ||
			    (thiscpuid == 0x00000fe0))
				cpst = 1;
		}
		if (cpst != 1) {
			pr_err(FW_BUG "numpst must be 1\n");
			return -ENODEV;
		}

		data->plllock = psb->plllocktime;
		pr_debug("plllocktime: 0x%x (units 1us)\n", psb->plllocktime);
		pr_debug("maxfid: 0x%x\n", psb->maxfid);
		pr_debug("maxvid: 0x%x\n", psb->maxvid);
		maxvid = psb->maxvid;

		data->numps = psb->numps;
		pr_debug("numpstates: 0x%x\n", data->numps);
		return fill_powernow_table(data,
				(struct pst_s *)(psb+1), maxvid);
	}
	/*
	 * If you see this message, complain to BIOS manufacturer. If
	 * he tells you "we do not support Linux" or some similar
	 * nonsense, remember that Windows 2000 uses the same legacy
	 * mechanism that the old Linux PSB driver uses. Tell them it
	 * is broken with Windows 2000.
	 *
	 * The reference to the AMD documentation is chapter 9 in the
	 * BIOS and Kernel Developer's Guide, which is available on
	 * www.amd.com
	 */
	pr_err(FW_BUG "No PSB or ACPI _PSS objects\n");
	pr_err("Make sure that your BIOS is up to date and Cool'N'Quiet support is enabled in BIOS setup\n");
	return -ENODEV;
}

static void powernow_k8_acpi_pst_values(struct powernow_k8_data *data,
		unsigned int index)
{
	u64 control;

	if (!data->acpi_data.state_count)
		return;

	control = data->acpi_data.states[index].control;
	data->irt = (control >> IRT_SHIFT) & IRT_MASK;
	data->rvo = (control >> RVO_SHIFT) & RVO_MASK;
	data->exttype = (control >> EXT_TYPE_SHIFT) & EXT_TYPE_MASK;
	data->plllock = (control >> PLL_L_SHIFT) & PLL_L_MASK;
	data->vidmvs = 1 << ((control >> MVS_SHIFT) & MVS_MASK);
	data->vstable = (control >> VST_SHIFT) & VST_MASK;
}

static int powernow_k8_cpu_init_acpi(struct powernow_k8_data *data)
{
	struct cpufreq_frequency_table *powernow_table;
	int ret_val = -ENODEV;
	u64 control, status;

	if (acpi_processor_register_performance(&data->acpi_data, data->cpu)) {
		pr_debug("register performance failed: bad ACPI data\n");
		return -EIO;
	}

	/* verify the data contained in the ACPI structures */
	if (data->acpi_data.state_count <= 1) {
		pr_debug("No ACPI P-States\n");
		goto err_out;
	}

	control = data->acpi_data.control_register.space_id;
	status = data->acpi_data.status_register.space_id;

	if ((control != ACPI_ADR_SPACE_FIXED_HARDWARE) ||
	    (status != ACPI_ADR_SPACE_FIXED_HARDWARE)) {
		pr_debug("Invalid control/status registers (%llx - %llx)\n",
			control, status);
		goto err_out;
	}

	/* fill in data->powernow_table */
	powernow_table = kzalloc((sizeof(*powernow_table)
		* (data->acpi_data.state_count + 1)), GFP_KERNEL);
	if (!powernow_table) {
		pr_debug("powernow_table memory alloc failure\n");
		goto err_out;
	}

	/* fill in data */
	data->numps = data->acpi_data.state_count;
	powernow_k8_acpi_pst_values(data, 0);

	ret_val = fill_powernow_table_fidvid(data, powernow_table);
	if (ret_val)
		goto err_out_mem;

	powernow_table[data->acpi_data.state_count].frequency =
		CPUFREQ_TABLE_END;
	data->powernow_table = powernow_table;

	if (cpumask_first(topology_core_cpumask(data->cpu)) == data->cpu)
		print_basics(data);

	/* notify BIOS that we exist */
	acpi_processor_notify_smm(THIS_MODULE);

	if (!zalloc_cpumask_var(&data->acpi_data.shared_cpu_map, GFP_KERNEL)) {
		pr_err("unable to alloc powernow_k8_data cpumask\n");
		ret_val = -ENOMEM;
		goto err_out_mem;
	}

	return 0;

err_out_mem:
	kfree(powernow_table);

err_out:
	acpi_processor_unregister_performance(data->cpu);

	/* data->acpi_data.state_count informs us at ->exit()
	 * whether ACPI was used */
	data->acpi_data.state_count = 0;

	return ret_val;
}

static int fill_powernow_table_fidvid(struct powernow_k8_data *data,
		struct cpufreq_frequency_table *powernow_table)
{
	int i;

	for (i = 0; i < data->acpi_data.state_count; i++) {
		u32 fid;
		u32 vid;
		u32 freq, index;
		u64 status, control;

		if (data->exttype) {
			status =  data->acpi_data.states[i].status;
			fid = status & EXT_FID_MASK;
			vid = (status >> VID_SHIFT) & EXT_VID_MASK;
		} else {
			control =  data->acpi_data.states[i].control;
			fid = control & FID_MASK;
			vid = (control >> VID_SHIFT) & VID_MASK;
		}

		pr_debug("   %d : fid 0x%x, vid 0x%x\n", i, fid, vid);

		index = fid | (vid<<8);
		powernow_table[i].driver_data = index;

		freq = find_khz_freq_from_fid(fid);
		powernow_table[i].frequency = freq;

		/* verify frequency is OK */
		if ((freq > (MAX_FREQ * 1000)) || (freq < (MIN_FREQ * 1000))) {
			pr_debug("invalid freq %u kHz, ignoring\n", freq);
			invalidate_entry(powernow_table, i);
			continue;
		}

		/* verify voltage is OK -
		 * BIOSs are using "off" to indicate invalid */
		if (vid == VID_OFF) {
			pr_debug("invalid vid %u, ignoring\n", vid);
			invalidate_entry(powernow_table, i);
			continue;
		}

		if (freq != (data->acpi_data.states[i].core_frequency * 1000)) {
			pr_info("invalid freq entries %u kHz vs. %u kHz\n",
				freq, (unsigned int)
				(data->acpi_data.states[i].core_frequency
				 * 1000));
			invalidate_entry(powernow_table, i);
			continue;
		}
	}
	return 0;
}

static void powernow_k8_cpu_exit_acpi(struct powernow_k8_data *data)
{
	if (data->acpi_data.state_count)
		acpi_processor_unregister_performance(data->cpu);
	free_cpumask_var(data->acpi_data.shared_cpu_map);
}

static int get_transition_latency(struct powernow_k8_data *data)
{
	int max_latency = 0;
	int i;
	for (i = 0; i < data->acpi_data.state_count; i++) {
		int cur_latency = data->acpi_data.states[i].transition_latency
			+ data->acpi_data.states[i].bus_master_latency;
		if (cur_latency > max_latency)
			max_latency = cur_latency;
	}
	if (max_latency == 0) {
		pr_err(FW_WARN "Invalid zero transition latency\n");
		max_latency = 1;
	}
	/* value in usecs, needs to be in nanoseconds */
	return 1000 * max_latency;
}

/* Take a frequency, and issue the fid/vid transition command */
static int transition_frequency_fidvid(struct powernow_k8_data *data,
		unsigned int index,
		struct cpufreq_policy *policy)
{
	u32 fid = 0;
	u32 vid = 0;
	int res;
	struct cpufreq_freqs freqs;

	pr_debug("cpu %d transition to index %u\n", smp_processor_id(), index);

	/* fid/vid correctness check for k8 */
	/* fid are the lower 8 bits of the index we stored into
	 * the cpufreq frequency table in find_psb_table, vid
	 * are the upper 8 bits.
	 */
	fid = data->powernow_table[index].driver_data & 0xFF;
	vid = (data->powernow_table[index].driver_data & 0xFF00) >> 8;

	pr_debug("table matched fid 0x%x, giving vid 0x%x\n", fid, vid);

	if (query_current_values_with_pending_wait(data))
		return 1;

	if ((data->currvid == vid) && (data->currfid == fid)) {
		pr_debug("target matches current values (fid 0x%x, vid 0x%x)\n",
			fid, vid);
		return 0;
	}

	pr_debug("cpu %d, changing to fid 0x%x, vid 0x%x\n",
		smp_processor_id(), fid, vid);
	freqs.old = find_khz_freq_from_fid(data->currfid);
	freqs.new = find_khz_freq_from_fid(fid);

	cpufreq_freq_transition_begin(policy, &freqs);
	res = transition_fid_vid(data, fid, vid);
	cpufreq_freq_transition_end(policy, &freqs, res);

	return res;
}

struct powernowk8_target_arg {
	struct cpufreq_policy		*pol;
	unsigned			newstate;
};

static long powernowk8_target_fn(void *arg)
{
	struct powernowk8_target_arg *pta = arg;
	struct cpufreq_policy *pol = pta->pol;
	unsigned newstate = pta->newstate;
	struct powernow_k8_data *data = per_cpu(powernow_data, pol->cpu);
	u32 checkfid;
	u32 checkvid;
	int ret;

	if (!data)
		return -EINVAL;

	checkfid = data->currfid;
	checkvid = data->currvid;

	if (pending_bit_stuck()) {
		pr_err("failing targ, change pending bit set\n");
		return -EIO;
	}

	pr_debug("targ: cpu %d, %d kHz, min %d, max %d\n",
		pol->cpu, data->powernow_table[newstate].frequency, pol->min,
		pol->max);

	if (query_current_values_with_pending_wait(data))
		return -EIO;

	pr_debug("targ: curr fid 0x%x, vid 0x%x\n",
		data->currfid, data->currvid);

	if ((checkvid != data->currvid) ||
	    (checkfid != data->currfid)) {
		pr_info("error - out of sync, fix 0x%x 0x%x, vid 0x%x 0x%x\n",
		       checkfid, data->currfid,
		       checkvid, data->currvid);
	}

	mutex_lock(&fidvid_mutex);

	powernow_k8_acpi_pst_values(data, newstate);

	ret = transition_frequency_fidvid(data, newstate, pol);

	if (ret) {
		pr_err("transition frequency failed\n");
		mutex_unlock(&fidvid_mutex);
		return 1;
	}
	mutex_unlock(&fidvid_mutex);

	pol->cur = find_khz_freq_from_fid(data->currfid);

	return 0;
}

/* Driver entry point to switch to the target frequency */
static int powernowk8_target(struct cpufreq_policy *pol, unsigned index)
{
	struct powernowk8_target_arg pta = { .pol = pol, .newstate = index };

	return work_on_cpu(pol->cpu, powernowk8_target_fn, &pta);
}

struct init_on_cpu {
	struct powernow_k8_data *data;
	int rc;
};

static void powernowk8_cpu_init_on_cpu(void *_init_on_cpu)
{
	struct init_on_cpu *init_on_cpu = _init_on_cpu;

	if (pending_bit_stuck()) {
		pr_err("failing init, change pending bit set\n");
		init_on_cpu->rc = -ENODEV;
		return;
	}

	if (query_current_values_with_pending_wait(init_on_cpu->data)) {
		init_on_cpu->rc = -ENODEV;
		return;
	}

	fidvid_msr_init();

	init_on_cpu->rc = 0;
}

#define MISSING_PSS_MSG \
	FW_BUG "No compatible ACPI _PSS objects found.\n" \
	FW_BUG "First, make sure Cool'N'Quiet is enabled in the BIOS.\n" \
	FW_BUG "If that doesn't help, try upgrading your BIOS.\n"

/* per CPU init entry point to the driver */
static int powernowk8_cpu_init(struct cpufreq_policy *pol)
{
	struct powernow_k8_data *data;
	struct init_on_cpu init_on_cpu;
	int rc, cpu;

	smp_call_function_single(pol->cpu, check_supported_cpu, &rc, 1);
	if (rc)
		return -ENODEV;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data) {
		pr_err("unable to alloc powernow_k8_data");
		return -ENOMEM;
	}

	data->cpu = pol->cpu;

	if (powernow_k8_cpu_init_acpi(data)) {
		/*
		 * Use the PSB BIOS structure. This is only available on
		 * an UP version, and is deprecated by AMD.
		 */
		if (num_online_cpus() != 1) {
			pr_err_once(MISSING_PSS_MSG);
			goto err_out;
		}
		if (pol->cpu != 0) {
			pr_err(FW_BUG "No ACPI _PSS objects for CPU other than CPU0. Complain to your BIOS vendor.\n");
			goto err_out;
		}
		rc = find_psb_table(data);
		if (rc)
			goto err_out;

		/* Take a crude guess here.
		 * That guess was in microseconds, so multiply with 1000 */
		pol->cpuinfo.transition_latency = (
			 ((data->rvo + 8) * data->vstable * VST_UNITS_20US) +
			 ((1 << data->irt) * 30)) * 1000;
	} else /* ACPI _PSS objects available */
		pol->cpuinfo.transition_latency = get_transition_latency(data);

	/* only run on specific CPU from here on */
	init_on_cpu.data = data;
	smp_call_function_single(data->cpu, powernowk8_cpu_init_on_cpu,
				 &init_on_cpu, 1);
	rc = init_on_cpu.rc;
	if (rc != 0)
		goto err_out_exit_acpi;

	cpumask_copy(pol->cpus, topology_core_cpumask(pol->cpu));
	data->available_cores = pol->cpus;

	/* min/max the cpu is capable of */
	if (cpufreq_table_validate_and_show(pol, data->powernow_table)) {
		pr_err(FW_BUG "invalid powernow_table\n");
		powernow_k8_cpu_exit_acpi(data);
		kfree(data->powernow_table);
		kfree(data);
		return -EINVAL;
	}

	pr_debug("cpu_init done, current fid 0x%x, vid 0x%x\n",
		data->currfid, data->currvid);

	/* Point all the CPUs in this policy to the same data */
	for_each_cpu(cpu, pol->cpus)
		per_cpu(powernow_data, cpu) = data;

	return 0;

err_out_exit_acpi:
	powernow_k8_cpu_exit_acpi(data);

err_out:
	kfree(data);
	return -ENODEV;
}

static int powernowk8_cpu_exit(struct cpufreq_policy *pol)
{
	struct powernow_k8_data *data = per_cpu(powernow_data, pol->cpu);
	int cpu;

	if (!data)
		return -EINVAL;

	powernow_k8_cpu_exit_acpi(data);

	kfree(data->powernow_table);
	kfree(data);
	/* pol->cpus will be empty here, use related_cpus instead. */
	for_each_cpu(cpu, pol->related_cpus)
		per_cpu(powernow_data, cpu) = NULL;

	return 0;
}

static void query_values_on_cpu(void *_err)
{
	int *err = _err;
	struct powernow_k8_data *data = __this_cpu_read(powernow_data);

	*err = query_current_values_with_pending_wait(data);
}

static unsigned int powernowk8_get(unsigned int cpu)
{
	struct powernow_k8_data *data = per_cpu(powernow_data, cpu);
	unsigned int khz = 0;
	int err;

	if (!data)
		return 0;

	smp_call_function_single(cpu, query_values_on_cpu, &err, true);
	if (err)
		goto out;

	khz = find_khz_freq_from_fid(data->currfid);


out:
	return khz;
}

static struct cpufreq_driver cpufreq_amd64_driver = {
	.flags		= CPUFREQ_ASYNC_NOTIFICATION,
	.verify		= cpufreq_generic_frequency_table_verify,
	.target_index	= powernowk8_target,
	.bios_limit	= acpi_processor_get_bios_limit,
	.init		= powernowk8_cpu_init,
	.exit		= powernowk8_cpu_exit,
	.get		= powernowk8_get,
	.name		= "powernow-k8",
	.attr		= cpufreq_generic_attr,
};

static void __request_acpi_cpufreq(void)
{
	const char *cur_drv, *drv = "acpi-cpufreq";

	cur_drv = cpufreq_get_current_driver();
	if (!cur_drv)
		goto request;

	if (strncmp(cur_drv, drv, min_t(size_t, strlen(cur_drv), strlen(drv))))
		pr_warn("WTF driver: %s\n", cur_drv);

	return;

 request:
	pr_warn("This CPU is not supported anymore, using acpi-cpufreq instead.\n");
	request_module(drv);
}

/* driver entry point for init */
static int powernowk8_init(void)
{
	unsigned int i, supported_cpus = 0;
	int ret;

	if (static_cpu_has(X86_FEATURE_HW_PSTATE)) {
		__request_acpi_cpufreq();
		return -ENODEV;
	}

	if (!x86_match_cpu(powernow_k8_ids))
		return -ENODEV;

	get_online_cpus();
	for_each_online_cpu(i) {
		smp_call_function_single(i, check_supported_cpu, &ret, 1);
		if (!ret)
			supported_cpus++;
	}

	if (supported_cpus != num_online_cpus()) {
		put_online_cpus();
		return -ENODEV;
	}
	put_online_cpus();

	ret = cpufreq_register_driver(&cpufreq_amd64_driver);
	if (ret)
		return ret;

	pr_info("Found %d %s (%d cpu cores) (" VERSION ")\n",
		num_online_nodes(), boot_cpu_data.x86_model_id, supported_cpus);

	return ret;
}

/* driver entry point for term */
static void __exit powernowk8_exit(void)
{
	pr_debug("exit\n");

	cpufreq_unregister_driver(&cpufreq_amd64_driver);
}

MODULE_AUTHOR("Paul Devriendt <paul.devriendt@amd.com>");
MODULE_AUTHOR("Mark Langsdorf <mark.langsdorf@amd.com>");
MODULE_DESCRIPTION("AMD Athlon 64 and Opteron processor frequency driver.");
MODULE_LICENSE("GPL");

late_initcall(powernowk8_init);
module_exit(powernowk8_exit);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       /*
 *  (c) 2003-2006 Advanced Micro Devices, Inc.
 *  Your use of this code is subject to the terms and conditions of the
 *  GNU general public license version 2. See "COPYING" or
 *  http://www.gnu.org/licenses/gpl.html
 */

struct powernow_k8_data {
	unsigned int cpu;

	u32 numps;  /* number of p-states */
	u32 batps;  /* number of p-states supported on battery */

	/* these values are constant when the PSB is used to determine
	 * vid/fid pairings, but are modified during the ->target() call
	 * when ACPI is used */
	u32 rvo;     /* ramp voltage offset */
	u32 irt;     /* isochronous relief time */
	u32 vidmvs;  /* usable value calculated from mvs */
	u32 vstable; /* voltage stabilization time, units 20 us */
	u32 plllock; /* pll lock time, units 1 us */
	u32 exttype; /* extended interface = 1 */

	/* keep track of the current fid / vid or pstate */
	u32 currvid;
	u32 currfid;

	/* the powernow_table includes all frequency and vid/fid pairings:
	 * fid are the lower 8 bits of the index, vid are the upper 8 bits.
	 * frequency is in kHz */
	struct cpufreq_frequency_table  *powernow_table;

	/* the acpi table needs to be kept. it's only available if ACPI was
	 * used to determine valid frequency/vid/fid states */
	struct acpi_processor_performance acpi_data;

	/* we need to keep track of associated cores, but let cpufreq
	 * handle hotplug events - so just point at cpufreq pol->cpus
	 * structure */
	struct cpumask *available_cores;
};

/* processor's cpuid instruction support */
#define CPUID_PROCESSOR_SIGNATURE	1	/* function 1 */
#define CPUID_XFAM			0x0ff00000	/* extended family */
#define CPUID_XFAM_K8			0
#define CPUID_XMOD			0x000f0000	/* extended model */
#define CPUID_XMOD_REV_MASK		0x000c0000
#define CPUID_XFAM_10H			0x00100000	/* family 0x10 */
#define CPUID_USE_XFAM_XMOD		0x00000f00
#define CPUID_GET_MAX_CAPABILITIES	0x80000000
#define CPUID_FREQ_VOLT_CAPABILITIES	0x80000007
#define P_STATE_TRANSITION_CAPABLE	6

/* Model Specific Registers for p-state transitions. MSRs are 64-bit. For     */
/* writes (wrmsr - opcode 0f 30), the register number is placed in ecx, and   */
/* the value to write is placed in edx:eax. For reads (rdmsr - opcode 0f 32), */
/* the register number is placed in ecx, and the data is returned in edx:eax. */

#define MSR_FIDVID_CTL      0xc0010041
#define MSR_FIDVID_STATUS   0xc0010042

/* Field definitions within the FID VID Low Control MSR : */
#define MSR_C_LO_INIT_FID_VID     0x00010000
#define MSR_C_LO_NEW_VID          0x00003f00
#define MSR_C_LO_NEW_FID          0x0000003f
#define MSR_C_LO_VID_SHIFT        8

/* Field definitions within the FID VID High Control MSR : */
#define MSR_C_HI_STP_GNT_TO	  0x000fffff

/* Field definitions within the FID VID Low Status MSR : */
#define MSR_S_LO_CHANGE_PENDING   0x80000000   /* cleared when completed */
#define MSR_S_LO_MAX_RAMP_VID     0x3f000000
#define MSR_S_LO_MAX_FID          0x003f0000
#define MSR_S_LO_START_FID        0x00003f00
#define MSR_S_LO_CURRENT_FID      0x0000003f

/* Field definitions within the FID VID High Status MSR : */
#define MSR_S_HI_MIN_WORKING_VID  0x3f000000
#define MSR_S_HI_MAX_WORKING_VID  0x003f0000
#define MSR_S_HI_START_VID        0x00003f00
#define MSR_S_HI_CURRENT_VID      0x0000003f
#define MSR_C_HI_STP_GNT_BENIGN	  0x00000001

/*
 * There are restrictions frequencies have to follow:
 * - only 1 entry in the low fid table ( <=1.4GHz )
 * - lowest entry in the high fid table must be >= 2 * the entry in the
 *   low fid table
 * - lowest entry in the high fid table must be a <= 200MHz + 2 * the entry
 *   in the low fid table
 * - the parts can only step at <= 200 MHz intervals, odd fid values are
 *   supported in revision G and later revisions.
 * - lowest frequency must be >= interprocessor hypertransport link speed
 *   (only applies to MP systems obviously)
 */

/* fids (frequency identifiers) are arranged in 2 tables - lo and hi */
#define LO_FID_TABLE_TOP     7	/* fid values marking the boundary    */
#define HI_FID_TABLE_BOTTOM  8	/* between the low and high tables    */

#define LO_VCOFREQ_TABLE_TOP    1400	/* corresponding vco frequency values */
#define HI_VCOFREQ_TABLE_BOTTOM 1600

#define MIN_FREQ_RESOLUTION  200 /* fids jump by 2 matching freq jumps by 200 */

#define MAX_FID 0x2a	/* Spec only gives FID values as far as 5 GHz */
#define LEAST_VID 0x3e	/* Lowest (numerically highest) useful vid value */

#define MIN_FREQ 800	/* Min and max freqs, per spec */
#define MAX_FREQ 5000

#define INVALID_FID_MASK 0xffffffc0  /* not a valid fid if these bits are set */
#define INVALID_VID_MASK 0xffffffc0  /* not a valid vid if these bits are set */

#define VID_OFF 0x3f

#define STOP_GRANT_5NS 1 /* min poss memory access latency for voltage change */

#define PLL_LOCK_CONVERSION (1000/5) /* ms to ns, then divide by clock period */

#define MAXIMUM_VID_STEPS 1  /* Current cpus only allow a single step of 25mV */
#define VST_UNITS_20US 20   /* Voltage Stabilization Time is in units of 20us */

/*
 * Most values of interest are encoded in a single field of the _PSS
 * entries: the "control" value.
 */

#define IRT_SHIFT      30
#define RVO_SHIFT      28
#define EXT_TYPE_SHIFT 27
#define PLL_L_SHIFT    20
#define MVS_SHIFT      18
#define VST_SHIFT      11
#define VID_SHIFT       6
#define IRT_MASK        3
#define RVO_MASK        3
#define EXT_TYPE_MASK   1
#define PLL_L_MASK   0x7f
#define MVS_MASK        3
#define VST_MASK     0x7f
#define VID_MASK     0x1f
#define FID_MASK     0x1f
#define EXT_VID_MASK 0x3f
#define EXT_FID_MASK 0x3f


/*
 * Version 1.4 of the PSB table. This table is constructed by BIOS and is
 * to tell the OS's power management driver which VIDs and FIDs are
 * supported by this particular processor.
 * If the data in the PSB / PST is wrong, then this driver will program the
 * wrong values into hardware, which is very likely to lead to a crash.
 */

#define PSB_ID_STRING      "AMDK7PNOW!"
#define PSB_ID_STRING_LEN  10

#define PSB_VERSION_1_4  0x14

struct psb_s {
	u8 signature[10];
	u8 tableversion;
	u8 flags1;
	u16 vstable;
	u8 flags2;
	u8 num_tables;
	u32 cpuid;
	u8 plllocktime;
	u8 maxfid;
	u8 maxvid;
	u8 numps;
};

/* Pairs of fid/vid values are appended to the version 1.4 PSB table. */
struct pst_s {
	u8 fid;
	u8 vid;
};

static int core_voltage_pre_transition(struct powernow_k8_data *data,
	u32 reqvid, u32 regfid);
static int core_voltage_post_transition(struct powernow_k8_data *data, u32 reqvid);
static int core_frequency_transition(struct powernow_k8_data *data, u32 reqfid);

static void powernow_k8_acpi_pst_values(struct powernow_k8_data *data, unsigned int index);

static int fill_powernow_table_fidvid(struct powernow_k8_data *data, struct cpufreq_frequency_table *powernow_table);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           /*
 * POWERNV cpufreq driver for the IBM POWER processors
 *
 * (C) Copyright IBM 2014
 *
 * Author: Vaidyanathan Srinivasan <svaidy at linux.vnet.ibm.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define pr_fmt(fmt)	"powernv-cpufreq: " fmt

#include <linux/kernel.h>
#include <linux/sysfs.h>
#include <linux/cpumask.h>
#include <linux/module.h>
#include <linux/cpufreq.h>
#include <linux/smp.h>
#include <linux/of.h>
#include <linux/reboot.h>
#include <linux/slab.h>

#include <asm/cputhreads.h>
#include <asm/firmware.h>
#include <asm/reg.h>
#include <asm/smp.h> /* Required for cpu_sibling_mask() in UP configs */
#include <asm/opal.h>

#define POWERNV_MAX_PSTATES	256
#define PMSR_PSAFE_ENABLE	(1UL << 30)
#define PMSR_SPR_EM_DISABLE	(1UL << 31)
#define PMSR_MAX(x)		((x >> 32) & 0xFF)

static struct cpufreq_frequency_table powernv_freqs[POWERNV_MAX_PSTATES+1];
static bool rebooting, throttled, occ_reset;

static struct chip {
	unsigned int id;
	bool throttled;
	cpumask_t mask;
	struct work_struct throttle;
	bool restore;
} *chips;

static int nr_chips;

/*
 * Note: The set of pstates consists of contiguous integers, the
 * smallest of which is indicated by powernv_pstate_info.min, the
 * largest of which is indicated by powernv_pstate_info.max.
 *
 * The nominal pstate is the highest non-turbo pstate in this
 * platform. This is indicated by powernv_pstate_info.nominal.
 */
static struct powernv_pstate_info {
	int min;
	int max;
	int nominal;
	int nr_pstates;
} powernv_pstate_info;

/*
 * Initialize the freq table based on data obtained
 * from the firmware passed via device-tree
 */
static int init_powernv_pstates(void)
{
	struct device_node *power_mgt;
	int i, pstate_min, pstate_max, pstate_nominal, nr_pstates = 0;
	const __be32 *pstate_ids, *pstate_freqs;
	u32 len_ids, len_freqs;

	power_mgt = of_find_node_by_path("/ibm,opal/power-mgt");
	if (!power_mgt) {
		pr_warn("power-mgt node not found\n");
		return -ENODEV;
	}

	if (of_property_read_u32(power_mgt, "ibm,pstate-min", &pstate_min)) {
		pr_warn("ibm,pstate-min node not found\n");
		return -ENODEV;
	}

	if (of_property_read_u32(power_mgt, "ibm,pstate-max", &pstate_max)) {
		pr_warn("ibm,pstate-max node not found\n");
		return -ENODEV;
	}

	if (of_property_read_u32(power_mgt, "ibm,pstate-nominal",
				 &pstate_nominal)) {
		pr_warn("ibm,pstate-nominal not found\n");
		return -ENODEV;
	}
	pr_info("cpufreq pstate min %d nominal %d max %d\n", pstate_min,
		pstate_nominal, pstate_max);

	pstate_ids = of_get_property(power_mgt, "ibm,pstate-ids", &len_ids);
	if (!pstate_ids) {
		pr_warn("ibm,pstate-ids not found\n");
		return -ENODEV;
	}

	pstate_freqs = of_get_property(power_mgt, "ibm,pstate-frequencies-mhz",
				      &len_freqs);
	if (!pstate_freqs) {
		pr_warn("ibm,pstate-frequencies-mhz not found\n");
		return -ENODEV;
	}

	if (len_ids != len_freqs) {
		pr_warn("Entries in ibm,pstate-ids and "
			"ibm,pstate-frequencies-mhz does not match\n");
	}

	nr_pstates = min(len_ids, len_freqs) / sizeof(u32);
	if (!nr_pstates) {
		pr_warn("No PStates found\n");
		return -ENODEV;
	}

	pr_debug("NR PStates %d\n", nr_pstates);
	for (i = 0; i < nr_pstates; i++) {
		u32 id = be32_to_cpu(pstate_ids[i]);
		u32 freq = be32_to_cpu(pstate_freqs[i]);

		pr_debug("PState id %d freq %d MHz\n", id, freq);
		powernv_freqs[i].frequency = freq * 1000; /* kHz */
		powernv_freqs[i].driver_data = id;
	}
	/* End of list marker entry */
	powernv_freqs[i].frequency = CPUFREQ_TABLE_END;

	powernv_pstate_info.min = pstate_min;
	powernv_pstate_info.max = pstate_max;
	powernv_pstate_info.nominal = pstate_nominal;
	powernv_pstate_info.nr_pstates = nr_pstates;

	return 0;
}

/* Returns the CPU frequency corresponding to the pstate_id. */
static unsigned int pstate_id_to_freq(int pstate_id)
{
	int i;

	i = powernv_pstate_info.max - pstate_id;
	if (i >= powernv_pstate_info.nr_pstates || i < 0) {
		pr_warn("PState id %d outside of PState table, "
			"reporting nominal id %d instead\n",
			pstate_id, powernv_pstate_info.nominal);
		i = powernv_pstate_info.max - powernv_pstate_info.nominal;
	}

	return powernv_freqs[i].frequency;
}

/*
 * cpuinfo_nominal_freq_show - Show the nominal CPU frequency as indicated by
 * the firmware
 */
static ssize_t cpuinfo_nominal_freq_show(struct cpufreq_policy *policy,
					char *buf)
{
	return sprintf(buf, "%u\n",
		pstate_id_to_freq(powernv_pstate_info.nominal));
}

struct freq_attr cpufreq_freq_attr_cpuinfo_nominal_freq =
	__ATTR_RO(cpuinfo_nominal_freq);

static struct freq_attr *powernv_cpu_freq_attr[] = {
	&cpufreq_freq_attr_scaling_available_freqs,
	&cpufreq_freq_attr_cpuinfo_nominal_freq,
	NULL,
};

/* Helper routines */

/* Access helpers to power mgt SPR */

static inline unsigned long get_pmspr(unsigned long sprn)
{
	switch (sprn) {
	case SPRN_PMCR:
		return mfspr(SPRN_PMCR);

	case SPRN_PMICR:
		return mfspr(SPRN_PMICR);

	case SPRN_PMSR:
		return mfspr(SPRN_PMSR);
	}
	BUG();
}

static inline void set_pmspr(unsigned long sprn, unsigned long val)
{
	switch (sprn) {
	case SPRN_PMCR:
		mtspr(SPRN_PMCR, val);
		return;

	case SPRN_PMICR:
		mtspr(SPRN_PMICR, val);
		return;
	}
	BUG();
}

/*
 * Use objects of this type to query/update
 * pstates on a remote CPU via smp_call_function.
 */
struct powernv_smp_call_data {
	unsigned int freq;
	int pstate_id;
};

/*
 * powernv_read_cpu_freq: Reads the current frequency on this CPU.
 *
 * Called via smp_call_function.
 *
 * Note: The caller of the smp_call_function should pass an argument of
 * the type 'struct powernv_smp_call_data *' along with this function.
 *
 * The current frequency on this CPU will be returned via
 * ((struct powernv_smp_call_data *)arg)->freq;
 */
static void powernv_read_cpu_freq(void *arg)
{
	unsigned long pmspr_val;
	s8 local_pstate_id;
	struct powernv_smp_call_data *freq_data = arg;

	pmspr_val = get_pmspr(SPRN_PMSR);

	/*
	 * The local pstate id corresponds bits 48..55 in the PMSR.
	 * Note: Watch out for the sign!
	 */
	local_pstate_id = (pmspr_val >> 48) & 0xFF;
	freq_data->pstate_id = local_pstate_id;
	freq_data->freq = pstate_id_to_freq(freq_data->pstate_id);

	pr_debug("cpu %d pmsr %016lX pstate_id %d frequency %d kHz\n",
		raw_smp_processor_id(), pmspr_val, freq_data->pstate_id,
		freq_data->freq);
}

/*
 * powernv_cpufreq_get: Returns the CPU frequency as reported by the
 * firmware for CPU 'cpu'. This value is reported through the sysfs
 * file cpuinfo_cur_freq.
 */
static unsigned int powernv_cpufreq_get(unsigned int cpu)
{
	struct powernv_smp_call_data freq_data;

	smp_call_function_any(cpu_sibling_mask(cpu), powernv_read_cpu_freq,
			&freq_data, 1);

	return freq_data.freq;
}

/*
 * set_pstate: Sets the pstate on this CPU.
 *
 * This is called via an smp_call_function.
 *
 * The caller must ensure that freq_data is of the type
 * (struct powernv_smp_call_data *) and the pstate_id which needs to be set
 * on this CPU should be present in freq_data->pstate_id.
 */
static void set_pstate(void *freq_data)
{
	unsigned long val;
	unsigned long pstate_ul =
		((struct powernv_smp_call_data *) freq_data)->pstate_id;

	val = get_pmspr(SPRN_PMCR);
	val = val & 0x0000FFFFFFFFFFFFULL;

	pstate_ul = pstate_ul & 0xFF;

	/* Set both global(bits 56..63) and local(bits 48..55) PStates */
	val = val | (pstate_ul << 56) | (pstate_ul << 48);

	pr_debug("Setting cpu %d pmcr to %016lX\n",
			raw_smp_processor_id(), val);
	set_pmspr(SPRN_PMCR, val);
}

/*
 * get_nominal_index: Returns the index corresponding to the nominal
 * pstate in the cpufreq table
 */
static inline unsigned int get_nominal_index(void)
{
	return powernv_pstate_info.max - powernv_pstate_info.nominal;
}

static void powernv_cpufreq_throttle_check(void *data)
{
	unsigned int cpu = smp_processor_id();
	unsigned long pmsr;
	int pmsr_pmax, i;

	pmsr = get_pmspr(SPRN_PMSR);

	for (i = 0; i < nr_chips; i++)
		if (chips[i].id == cpu_to_chip_id(cpu))
			break;

	/* Check for Pmax Capping */
	pmsr_pmax = (s8)PMSR_MAX(pmsr);
	if (pmsr_pmax != powernv_pstate_info.max) {
		if (chips[i].throttled)
			goto next;
		chips[i].throttled = true;
		if (pmsr_pmax < powernv_pstate_info.nominal)
			pr_crit("CPU %d on Chip %u has Pmax reduced below nominal frequency (%d < %d)\n",
				cpu, chips[i].id, pmsr_pmax,
				powernv_pstate_info.nominal);
		else
			pr_info("CPU %d on Chip %u has Pmax reduced below turbo frequency (%d < %d)\n",
				cpu, chips[i].id, pmsr_pmax,
				powernv_pstate_info.max);
	} else if (chips[i].throttled) {
		chips[i].throttled = false;
		pr_info("CPU %d on Chip %u has Pmax restored to %d\n", cpu,
			chips[i].id, pmsr_pmax);
	}

	/* Check if Psafe_mode_active is set in PMSR. */
next:
	if (pmsr & PMSR_PSAFE_ENABLE) {
		throttled = true;
		pr_info("Pstate set to safe frequency\n");
	}

	/* Check if SPR_EM_DISABLE is set in PMSR */
	if (pmsr & PMSR_SPR_EM_DISABLE) {
		throttled = true;
		pr_info("Frequency Control disabled from OS\n");
	}

	if (throttled) {
		pr_info("PMSR = %16lx\n", pmsr);
		pr_crit("CPU Frequency could be throttled\n");
	}
}

/*
 * powernv_cpufreq_target_index: Sets the frequency corresponding to
 * the cpufreq table entry indexed by new_index on the cpus in the
 * mask policy->cpus
 */
static int powernv_cpufreq_target_index(struct cpufreq_policy *policy,
					unsigned int new_index)
{
	struct powernv_smp_call_data freq_data;

	if (unlikely(rebooting) && new_index != get_nominal_index())
		return 0;

	if (!throttled) {
		/* we don't want to be preempted while
		 * checking if the CPU frequency has been throttled
		 */
		preempt_disable();
		powernv_cpufreq_throttle_check(NULL);
		preempt_enable();
	}

	freq_data.pstate_id = powernv_freqs[new_index].driver_data;

	/*
	 * Use smp_call_function to send IPI and execute the
	 * mtspr on target CPU.  We could do that without IPI
	 * if current CPU is within policy->cpus (core)
	 */
	smp_call_function_any(policy->cpus, set_pstate, &freq_data, 1);

	return 0;
}

static int powernv_cpufreq_cpu_init(struct cpufreq_policy *policy)
{
	int base, i;

	base = cpu_first_thread_sibling(policy->cpu);

	for (i = 0; i < threads_per_core; i++)
		cpumask_set_cpu(base + i, policy->cpus);

	return cpufreq_table_validate_and_show(policy, powernv_freqs);
}

static int powernv_cpufreq_reboot_notifier(struct notifier_block *nb,
				unsigned long action, void *unused)
{
	int cpu;
	struct cpufreq_policy *cpu_policy;

	rebooting = true;
	for_each_online_cpu(cpu) {
		cpu_policy = cpufreq_cpu_get(cpu);
		if (!cpu_policy)
			continue;
		powernv_cpufreq_target_index(cpu_policy, get_nominal_index());
		cpufreq_cpu_put(cpu_policy);
	}

	return NOTIFY_DONE;
}

static struct notifier_block powernv_cpufreq_reboot_nb = {
	.notifier_call = powernv_cpufreq_reboot_notifier,
};

void powernv_cpufreq_work_fn(struct work_struct *work)
{
	struct chip *chip = container_of(work, struct chip, throttle);
	unsigned int cpu;
	cpumask_var_t mask;

	smp_call_function_any(&chip->mask,
			      powernv_cpufreq_throttle_check, NULL, 0);

	if (!chip->restore)
		return;

	chip->restore = false;
	cpumask_copy(mask, &chip->mask);
	for_each_cpu_and(cpu, mask, cpu_online_mask) {
		int index, tcpu;
		struct cpufreq_policy policy;

		cpufreq_get_policy(&policy, cpu);
		cpufreq_frequency_table_target(&policy, policy.freq_table,
					       policy.cur,
					       CPUFREQ_RELATION_C, &index);
		powernv_cpufreq_target_index(&policy, index);
		for_each_cpu(tcpu, policy.cpus)
			cpumask_clear_cpu(tcpu, mask);
	}
}

static char throttle_reason[][30] = {
					"No throttling",
					"Power Cap",
					"Processor Over Temperature",
					"Power Supply Failure",
					"Over Current",
					"OCC Reset"
				     };

static int powernv_cpufreq_occ_msg(struct notifier_block *nb,
				   unsigned long msg_type, void *_msg)
{
	struct opal_msg *msg = _msg;
	struct opal_occ_msg omsg;
	int i;

	if (msg_type != OPAL_MSG_OCC)
		return 0;

	omsg.type = be64_to_cpu(msg->params[0]);

	switch (omsg.type) {
	case OCC_RESET:
		occ_reset = true;
		pr_info("OCC (On Chip Controller - enforces hard thermal/power limits) Resetting\n");
		/*
		 * powernv_cpufreq_throttle_check() is called in
		 * target() callback which can detect the throttle state
		 * for governors like ondemand.
		 * But static governors will not call target() often thus
		 * report throttling here.
		 */
		if (!throttled) {
			throttled = true;
			pr_crit("CPU frequency is throttled for duration\n");
		}

		break;
	case OCC_LOAD:
		pr_info("OCC Loading, CPU frequency is throttled until OCC is started\n");
		break;
	case OCC_THROTTLE:
		omsg.chip = be64_to_cpu(msg->params[1]);
		omsg.throttle_status = be64_to_cpu(msg->params[2]);

		if (occ_reset) {
			occ_reset = false;
			throttled = false;
			pr_info("OCC Active, CPU frequency is no longer throttled\n");

			for (i = 0; i < nr_chips; i++) {
				chips[i].restore = true;
				schedule_work(&chips[i].throttle);
			}

			return 0;
		}

		if (omsg.throttle_status &&
		    omsg.throttle_status <= OCC_MAX_THROTTLE_STATUS)
			pr_info("OCC: Chip %u Pmax reduced due to %s\n",
				(unsigned int)omsg.chip,
				throttle_reason[omsg.throttle_status]);
		else if (!omsg.throttle_status)
			pr_info("OCC: Chip %u %s\n", (unsigned int)omsg.chip,
				throttle_reason[omsg.throttle_status]);
		else
			return 0;

		for (i = 0; i < nr_chips; i++)
			if (chips[i].id == omsg.chip) {
				if (!omsg.throttle_status)
					chips[i].restore = true;
				schedule_work(&chips[i].throttle);
			}
	}
	return 0;
}

static struct notifier_block powernv_cpufreq_opal_nb = {
	.notifier_call	= powernv_cpufreq_occ_msg,
	.next		= NULL,
	.priority	= 0,
};

static void powernv_cpufreq_stop_cpu(struct cpufreq_policy *policy)
{
	struct powernv_smp_call_data freq_data;

	freq_data.pstate_id = powernv_pstate_info.min;
	smp_call_function_single(policy->cpu, set_pstate, &freq_data, 1);
}

static struct cpufreq_driver powernv_cpufreq_driver = {
	.name		= "powernv-cpufreq",
	.flags		= CPUFREQ_CONST_LOOPS,
	.init		= powernv_cpufreq_cpu_init,
	.verify		= cpufreq_generic_frequency_table_verify,
	.target_index	= powernv_cpufreq_target_index,
	.get		= powernv_cpufreq_get,
	.stop_cpu	= powernv_cpufreq_stop_cpu,
	.attr		= powernv_cpu_freq_attr,
};

static int init_chip_info(void)
{
	unsigned int chip[256];
	unsigned int cpu, i;
	unsigned int prev_chip_id = UINT_MAX;

	for_each_possible_cpu(cpu) {
		unsigned int id = cpu_to_chip_id(cpu);

		if (prev_chip_id != id) {
			prev_chip_id = id;
			chip[nr_chips++] = id;
		}
	}

	chips = kmalloc_array(nr_chips, sizeof(struct chip), GFP_KERNEL);
	if (!chips)
		return -ENOMEM;

	for (i = 0; i < nr_chips; i++) {
		chips[i].id = chip[i];
		chips[i].throttled = false;
		cpumask_copy(&chips[i].mask, cpumask_of_node(chip[i]));
		INIT_WORK(&chips[i].throttle, powernv_cpufreq_work_fn);
		chips[i].restore = false;
	}

	return 0;
}

static int __init powernv_cpufreq_init(void)
{
	int rc = 0;

	/* Don't probe on pseries (guest) platforms */
	if (!firmware_has_feature(FW_FEATURE_OPAL))
		return -ENODEV;

	/* Discover pstates from device tree and init */
	rc = init_powernv_pstates();
	if (rc) {
		pr_info("powernv-cpufreq disabled. System does not support PState control\n");
		return rc;
	}

	/* Populate chip info */
	rc = init_chip_info();
	if (rc)
		return rc;

	register_reboot_notifier(&powernv_cpufreq_reboot_nb);
	opal_message_notifier_register(OPAL_MSG_OCC, &powernv_cpufreq_opal_nb);
	return cpufreq_register_driver(&powernv_cpufreq_driver);
}
module_init(powernv_cpufreq_init);

static void __exit powernv_cpufreq_exit(void)
{
	unregister_reboot_notifier(&powernv_cpufreq_reboot_nb);
	opal_message_notifier_unregister(OPAL_MSG_OCC,
					 &powernv_cpufreq_opal_nb);
	cpufreq_unregister_driver(&powernv_cpufreq_driver);
}
module_exit(powernv_cpufreq_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vaidyanathan Srinivasan <svaidy at linux.vnet.ibm.com>");
                                                                                                                                                         /*
 * cpufreq driver for the cell processor
 *
 * (C) Copyright IBM Deutschland Entwicklung GmbH 2005-2007
 *
 * Author: Christian Krafft <krafft@de.ibm.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/cpufreq.h>
#include <linux/module.h>
#include <linux/of_platform.h>

#include <asm/machdep.h>
#include <asm/prom.h>
#include <asm/cell-regs.h>

#include "ppc_cbe_cpufreq.h"

/* the CBE supports an 8 step frequency scaling */
static struct cpufreq_frequency_table cbe_freqs[] = {
	{0, 1,	0},
	{0, 2,	0},
	{0, 3,	0},
	{0, 4,	0},
	{0, 5,	0},
	{0, 6,	0},
	{0, 8,	0},
	{0, 10,	0},
	{0, 0,	CPUFREQ_TABLE_END},
};

/*
 * hardware specific functions
 */

static int set_pmode(unsigned int cpu, unsigned int slow_mode)
{
	int rc;

	if (cbe_cpufreq_has_pmi)
		rc = cbe_cpufreq_set_pmode_pmi(cpu, slow_mode);
	else
		rc = cbe_cpufreq_set_pmode(cpu, slow_mode);

	pr_debug("register contains slow mode %d\n", cbe_cpufreq_get_pmode(cpu));

	return rc;
}

/*
 * cpufreq functions
 */

static int cbe_cpufreq_cpu_init(struct cpufreq_policy *policy)
{
	struct cpufreq_frequency_table *pos;
	const u32 *max_freqp;
	u32 max_freq;
	int cur_pmode;
	struct device_node *cpu;

	cpu = of_get_cpu_node(policy->cpu, NULL);

	if (!cpu)
		return -ENODEV;

	pr_debug("init cpufreq on CPU %d\n", policy->cpu);

	/*
	 * Let's check we can actually get to the CELL regs
	 */
	if (!cbe_get_cpu_pmd_regs(policy->cpu) ||
	    !cbe_get_cpu_mic_tm_regs(policy->cpu)) {
		pr_info("invalid CBE regs pointers for cpufreq\n");
		of_node_put(cpu);
		return -EINVAL;
	}

	max_freqp = of_get_property(cpu, "clock-frequency", NULL);

	of_node_put(cpu);

	if (!max_freqp)
		return -EINVAL;

	/* we need the freq in kHz */
	max_freq = *max_freqp / 1000;

	pr_debug("max clock-frequency is at %u kHz\n", max_freq);
	pr_debug("initializing frequency table\n");

	/* initialize frequency table */
	cpufreq_for_each_entry(pos, cbe_freqs) {
		pos->frequency = max_freq / pos->driver_data;
		pr_debug("%d: %d\n", (int)(pos - cbe_freqs), pos->frequency);
	}

	/* if DEBUG is enabled set_pmode() measures the latency
	 * of a transition */
	policy->cpuinfo.transition_latency = 25000;

	cur_pmode = cbe_cpufreq_get_pmode(policy->cpu);
	pr_debug("current pmode is at %d\n",cur_pmode);

	policy->cur = cbe_freqs[cur_pmode].frequency;

#ifdef CONFIG_SMP
	cpumask_copy(policy->cpus, cpu_sibling_mask(policy->cpu));
#endif

	/* this ensures that policy->cpuinfo_min
	 * and policy->cpuinfo_max are set correctly */
	return cpufreq_table_validate_and_show(policy, cbe_freqs);
}

static int cbe_cpufreq_target(struct cpufreq_policy *policy,
			      unsigned int cbe_pmode_new)
{
	pr_debug("setting frequency for cpu %d to %d kHz, " \
		 "1/%d of max frequency\n",
		 policy->cpu,
		 cbe_freqs[cbe_pmode_new].frequency,
		 cbe_freqs[cbe_pmode_new].driver_data);

	return set_pmode(policy->cpu, cbe_pmode_new);
}

static struct cpufreq_driver cbe_cpufreq_driver = {
	.verify		= cpufreq_generic_frequency_table_verify,
	.target_index	= cbe_cpufreq_target,
	.init		= cbe_cpufreq_cpu_init,
	.name		= "cbe-cpufreq",
	.flags		= CPUFREQ_CONST_LOOPS,
};

/*
 * module init and destoy
 */

static int __init cbe_cpufreq_init(void)
{
	if (!machine_is(cell))
		return -ENODEV;

	return cpufreq_register_driver(&cbe_cpufreq_driver);
}

static void __exit cbe_cpufreq_exit(void)
{
	cpufreq_unregister_driver(&cbe_cpufreq_driver);
}

module_init(cbe_cpufreq_init);
module_exit(cbe_cpufreq_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Christian Krafft <krafft@de.ibm.com>");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            /*
 * ppc_cbe_cpufreq.h
 *
 * This file contains the definitions used by the cbe_cpufreq driver.
 *
 * (C) Copyright IBM Deutschland Entwicklung GmbH 2005-2007
 *
 * Author: Christian Krafft <krafft@de.ibm.com>
 *
 */

#include <linux/cpufreq.h>
#include <linux/types.h>

int cbe_cpufreq_set_pmode(int cpu, unsigned int pmode);
int cbe_cpufreq_get_pmode(int cpu);

int cbe_cpufreq_set_pmode_pmi(int cpu, unsigned int pmode);

#if defined(CONFIG_CPU_FREQ_CBE_PMI) || defined(CONFIG_CPU_FREQ_CBE_PMI_MODULE)
extern bool cbe_cpufreq_has_pmi;
#else
#define cbe_cpufreq_has_pmi (0)
#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        /*
 * pervasive backend for the cbe_cpufreq driver
 *
 * This driver makes use of the pervasive unit to
 * engage the desired frequency.
 *
 * (C) Copyright IBM Deutschland Entwicklung GmbH 2005-2007
 *
 * Author: Christian Krafft <krafft@de.ibm.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <asm/machdep.h>
#include <asm/hw_irq.h>
#include <asm/cell-regs.h>

#include "ppc_cbe_cpufreq.h"

/* to write to MIC register */
static u64 MIC_Slow_Fast_Timer_table[] = {
	[0 ... 7] = 0x007fc00000000000ull,
};

/* more values for the MIC */
static u64 MIC_Slow_Next_Timer_table[] = {
	0x0000240000000000ull,
	0x0000268000000000ull,
	0x000029C000000000ull,
	0x00002D0000000000ull,
	0x0000300000000000ull,
	0x0000334000000000ull,
	0x000039C000000000ull,
	0x00003FC000000000ull,
};


int cbe_cpufreq_set_pmode(int cpu, unsigned int pmode)
{
	struct cbe_pmd_regs __iomem *pmd_regs;
	struct cbe_mic_tm_regs __iomem *mic_tm_regs;
	unsigned long flags;
	u64 value;
#ifdef DEBUG
	long time;
#endif

	local_irq_save(flags);

	mic_tm_regs = cbe_get_cpu_mic_tm_regs(cpu);
	pmd_regs = cbe_get_cpu_pmd_regs(cpu);

#ifdef DEBUG
	time = jiffies;
#endif

	out_be64(&mic_tm_regs->slow_fast_timer_0, MIC_Slow_Fast_Timer_table[pmode]);
	out_be64(&mic_tm_regs->slow_fast_timer_1, MIC_Slow_Fast_Timer_table[pmode]);

	out_be64(&mic_tm_regs->slow_next_timer_0, MIC_Slow_Next_Timer_table[pmode]);
	out_be64(&mic_tm_regs->slow_next_timer_1, MIC_Slow_Next_Timer_table[pmode]);

	value = in_be64(&pmd_regs->pmcr);
	/* set bits to zero */
	value &= 0xFFFFFFFFFFFFFFF8ull;
	/* set bits to next pmode */
	value |= pmode;

	out_be64(&pmd_regs->pmcr, value);

#ifdef DEBUG
	/* wait until new pmode appears in status register */
	value = in_be64(&pmd_regs->pmsr) & 0x07;
	while (value != pmode) {
		cpu_relax();
		value = in_be64(&pmd_regs->pmsr) & 0x07;
	}

	time = jiffies  - time;
	time = jiffies_to_msecs(time);
	pr_debug("had to wait %lu ms for a transition using " \
		 "pervasive unit\n", time);
#endif
	local_irq_restore(flags);

	return 0;
}


int cbe_cpufreq_get_pmode(int cpu)
{
	int ret;
	struct cbe_pmd_regs __iomem *pmd_regs;

	pmd_regs = cbe_get_cpu_pmd_regs(cpu);
	ret = in_be64(&pmd_regs->pmsr) & 0x07;

	return ret;
}

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                /*
 * pmi backend for the cbe_cpufreq driver
 *
 * (C) Copyright IBM Deutschland Entwicklung GmbH 2005-2007
 *
 * Author: Christian Krafft <krafft@de.ibm.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <linux/module.h>
#include <linux/of_platform.h>

#include <asm/processor.h>
#include <asm/prom.h>
#include <asm/pmi.h>
#include <asm/cell-regs.h>

#ifdef DEBUG
#include <asm/time.h>
#endif

#include "ppc_cbe_cpufreq.h"

static u8 pmi_slow_mode_limit[MAX_CBE];

bool cbe_cpufreq_has_pmi = false;
EXPORT_SYMBOL_GPL(cbe_cpufreq_has_pmi);

/*
 * hardware specific functions
 */

int cbe_cpufreq_set_pmode_pmi(int cpu, unsigned int pmode)
{
	int ret;
	pmi_message_t pmi_msg;
#ifdef DEBUG
	long time;
#endif
	pmi_msg.type = PMI_TYPE_FREQ_CHANGE;
	pmi_msg.data1 =	cbe_cpu_to_node(cpu);
	pmi_msg.data2 = pmode;

#ifdef DEBUG
	time = jiffies;
#endif
	pmi_send_message(pmi_msg);

#ifdef DEBUG
	time = jiffies  - time;
	time = jiffies_to_msecs(time);
	pr_debug("had to wait %lu ms for a transition using " \
		 "PMI\n", time);
#endif
	ret = pmi_msg.data2;
	pr_debug("PMI returned slow mode %d\n", ret);

	return ret;
}
EXPORT_SYMBOL_GPL(cbe_cpufreq_set_pmode_pmi);


static void cbe_cpufreq_handle_pmi(pmi_message_t pmi_msg)
{
	u8 node, slow_mode;

	BUG_ON(pmi_msg.type != PMI_TYPE_FREQ_CHANGE);

	node = pmi_msg.data1;
	slow_mode = pmi_msg.data2;

	pmi_slow_mode_limit[node] = slow_mode;

	pr_debug("cbe_handle_pmi: node: %d max_freq: %d\n", node, slow_mode);
}

static int pmi_notifier(struct notifier_block *nb,
				       unsigned long event, void *data)
{
	struct cpufreq_policy *policy = data;
	struct cpufreq_frequency_table *cbe_freqs;
	u8 node;

	/* Should this really be called for CPUFREQ_ADJUST and CPUFREQ_NOTIFY
	 * policy events?)
	 */
	if (event == CPUFREQ_START)
		return 0;

	cbe_freqs = cpufreq_frequency_get_table(policy->cpu);
	node = cbe_cpu_to_node(policy->cpu);

	pr_debug("got notified, event=%lu, node=%u\n", event, node);

	if (pmi_slow_mode_limit[node] != 0) {
		pr_debug("limiting node %d to slow mode %d\n",
			 node, pmi_slow_mode_limit[node]);

		cpufreq_verify_within_limits(policy, 0,

			cbe_freqs[pmi_slow_mode_limit[node]].frequency);
	}

	return 0;
}

static struct notifier_block pmi_notifier_block = {
	.notifier_call = pmi_notifier,
};

static struct pmi_handler cbe_pmi_handler = {
	.type			= PMI_TYPE_FREQ_CHANGE,
	.handle_pmi_message	= cbe_cpufreq_handle_pmi,
};



static int __init cbe_cpufreq_pmi_init(void)
{
	cbe_cpufreq_has_pmi = pmi_register_handler(&cbe_pmi_handler) == 0;

	if (!cbe_cpufreq_has_pmi)
		return -ENODEV;

	cpufreq_register_notifier(&pmi_notifier_block, CPUFREQ_POLICY_NOTIFIER);

	return 0;
}

static void __exit cbe_cpufreq_pmi_exit(void)
{
	cpufreq_unregister_notifier(&pmi_notifier_block, CPUFREQ_POLICY_NOTIFIER);
	pmi_unregister_handler(&cbe_pmi_handler);
}

module_init(cbe_cpufreq_pmi_init);
module_exit(cbe_cpufreq_pmi_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Christian Krafft <krafft@de.ibm.com>");
                                                                                                                                                                                                                                                                                                                                                                                                         /*
 *  Copyright (C) 2002,2003 Intrinsyc Software
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * History:
 *   31-Jul-2002 : Initial version [FB]
 *   29-Jan-2003 : added PXA255 support [FB]
 *   20-Apr-2003 : ported to v2.5 (Dustin McIntire, Sensoria Corp.)
 *
 * Note:
 *   This driver may change the memory bus clock rate, but will not do any
 *   platform specific access timing changes... for example if you have flash
 *   memory connected to CS0, you will need to register a platform specific
 *   notifier which will adjust the memory access strobes to maintain a
 *   minimum strobe width.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/err.h>
#include <linux/regulator/consumer.h>
#include <linux/io.h>

#include <mach/pxa2xx-regs.h>
#include <mach/smemc.h>

#ifdef DEBUG
static unsigned int freq_debug;
module_param(freq_debug, uint, 0);
MODULE_PARM_DESC(freq_debug, "Set the debug messages to on=1/off=0");
#else
#define freq_debug  0
#endif

static struct regulator *vcc_core;

static unsigned int pxa27x_maxfreq;
module_param(pxa27x_maxfreq, uint, 0);
MODULE_PARM_DESC(pxa27x_maxfreq, "Set the pxa27x maxfreq in MHz"
		 "(typically 624=>pxa270, 416=>pxa271, 520=>pxa272)");

struct pxa_freqs {
	unsigned int khz;
	unsigned int membus;
	unsigned int cccr;
	unsigned int div2;
	unsigned int cclkcfg;
	int vmin;
	int vmax;
};

/* Define the refresh period in mSec for the SDRAM and the number of rows */
#define SDRAM_TREF	64	/* standard 64ms SDRAM */
static unsigned int sdram_rows;

#define CCLKCFG_TURBO		0x1
#define CCLKCFG_FCS		0x2
#define CCLKCFG_HALFTURBO	0x4
#define CCLKCFG_FASTBUS		0x8
#define MDREFR_DB2_MASK		(MDREFR_K2DB2 | MDREFR_K1DB2)
#define MDREFR_DRI_MASK		0xFFF

#define MDCNFG_DRAC2(mdcnfg) (((mdcnfg) >> 21) & 0x3)
#define MDCNFG_DRAC0(mdcnfg) (((mdcnfg) >> 5) & 0x3)

/*
 * PXA255 definitions
 */
/* Use the run mode frequencies for the CPUFREQ_POLICY_PERFORMANCE policy */
#define CCLKCFG			CCLKCFG_TURBO | CCLKCFG_FCS

static const struct pxa_freqs pxa255_run_freqs[] =
{
	/* CPU   MEMBUS  CCCR  DIV2 CCLKCFG	           run  turbo PXbus SDRAM */
	{ 99500,  99500, 0x121, 1,  CCLKCFG, -1, -1},	/*  99,   99,   50,   50  */
	{132700, 132700, 0x123, 1,  CCLKCFG, -1, -1},	/* 133,  133,   66,   66  */
	{199100,  99500, 0x141, 0,  CCLKCFG, -1, -1},	/* 199,  199,   99,   99  */
	{265400, 132700, 0x143, 1,  CCLKCFG, -1, -1},	/* 265,  265,  133,   66  */
	{331800, 165900, 0x145, 1,  CCLKCFG, -1, -1},	/* 331,  331,  166,   83  */
	{398100,  99500, 0x161, 0,  CCLKCFG, -1, -1},	/* 398,  398,  196,   99  */
};

/* Use the turbo mode frequencies for the CPUFREQ_POLICY_POWERSAVE policy */
static const struct pxa_freqs pxa255_turbo_freqs[] =
{
	/* CPU   MEMBUS  CCCR  DIV2 CCLKCFG	   run  turbo PXbus SDRAM */
	{ 99500, 99500,  0x121, 1,  CCLKCFG, -1, -1},	/*  99,   99,   50,   50  */
	{199100, 99500,  0x221, 0,  CCLKCFG, -1, -1},	/*  99,  199,   50,   99  */
	{298500, 99500,  0x321, 0,  CCLKCFG, -1, -1},	/*  99,  287,   50,   99  */
	{298600, 99500,  0x1c1, 0,  CCLKCFG, -1, -1},	/* 199,  287,   99,   99  */
	{398100, 99500,  0x241, 0,  CCLKCFG, -1, -1},	/* 199,  398,   99,   99  */
};

#define NUM_PXA25x_RUN_FREQS ARRAY_SIZE(pxa255_run_freqs)
#define NUM_PXA25x_TURBO_FREQS ARRAY_SIZE(pxa255_turbo_freqs)

static struct cpufreq_frequency_table
	pxa255_run_freq_table[NUM_PXA25x_RUN_FREQS+1];
static struct cpufreq_frequency_table
	pxa255_turbo_freq_table[NUM_PXA25x_TURBO_FREQS+1];

static unsigned int pxa255_turbo_table;
module_param(pxa255_turbo_table, uint, 0);
MODULE_PARM_DESC(pxa255_turbo_table, "Selects the frequency table (0 = run table, !0 = turbo table)");

/*
 * PXA270 definitions
 *
 * For the PXA27x:
 * Control variables are A, L, 2N for CCCR; B, HT, T for CLKCFG.
 *
 * A = 0 => memory controller clock from table 3-7,
 * A = 1 => memory controller clock = system bus clock
 * Run mode frequency	= 13 MHz * L
 * Turbo mode frequency = 13 MHz * L * N
 * System bus frequency = 13 MHz * L / (B + 1)
 *
 * In CCCR:
 * A = 1
 * L = 16	  oscillator to run mode ratio
 * 2N = 6	  2 * (turbo mode to run mode ratio)
 *
 * In CCLKCFG:
 * B = 1	  Fast bus mode
 * HT = 0	  Half-Turbo mode
 * T = 1	  Turbo mode
 *
 * For now, just support some of the combinations in table 3-7 of
 * PXA27x Processor Family Developer's Manual to simplify frequency
 * change sequences.
 */
#define PXA27x_CCCR(A, L, N2) (A << 25 | N2 << 7 | L)
#define CCLKCFG2(B, HT, T) \
  (CCLKCFG_FCS | \
   ((B)  ? CCLKCFG_FASTBUS : 0) | \
   ((HT) ? CCLKCFG_HALFTURBO : 0) | \
   ((T)  ? CCLKCFG_TURBO : 0))

static struct pxa_freqs pxa27x_freqs[] = {
	{104000, 104000, PXA27x_CCCR(1,	 8, 2), 0, CCLKCFG2(1, 0, 1),  900000, 1705000 },
	{156000, 104000, PXA27x_CCCR(1,	 8, 3), 0, CCLKCFG2(1, 0, 1), 1000000, 1705000 },
	{208000, 208000, PXA27x_CCCR(0, 16, 2), 1, CCLKCFG2(0, 0, 1), 1180000, 1705000 },
	{312000, 208000, PXA27x_CCCR(1, 16, 3), 1, CCLKCFG2(1, 0, 1), 1250000, 1705000 },
	{416000, 208000, PXA27x_CCCR(1, 16, 4), 1, CCLKCFG2(1, 0, 1), 1350000, 1705000 },
	{520000, 208000, PXA27x_CCCR(1, 16, 5), 1, CCLKCFG2(1, 0, 1), 1450000, 1705000 },
	{624000, 208000, PXA27x_CCCR(1, 16, 6), 1, CCLKCFG2(1, 0, 1), 1550000, 1705000 }
};

#define NUM_PXA27x_FREQS ARRAY_SIZE(pxa27x_freqs)
static struct cpufreq_frequency_table
	pxa27x_freq_table[NUM_PXA27x_FREQS+1];

extern unsigned get_clk_frequency_khz(int info);

#ifdef CONFIG_REGULATOR

static int pxa_cpufreq_change_voltage(const struct pxa_freqs *pxa_freq)
{
	int ret = 0;
	int vmin, vmax;

	if (!cpu_is_pxa27x())
		return 0;

	vmin = pxa_freq->vmin;
	vmax = pxa_freq->vmax;
	if ((vmin == -1) || (vmax == -1))
		return 0;

	ret = regulator_set_voltage(vcc_core, vmin, vmax);
	if (ret)
		pr_err("cpufreq: Failed to set vcc_core in [%dmV..%dmV]\n",
		       vmin, vmax);
	return ret;
}

static void pxa_cpufreq_init_voltages(void)
{
	vcc_core = regulator_get(NULL, "vcc_core");
	if (IS_ERR(vcc_core)) {
		pr_info("cpufreq: Didn't find vcc_core regulator\n");
		vcc_core = NULL;
	} else {
		pr_info("cpufreq: Found vcc_core regulator\n");
	}
}
#else
static int pxa_cpufreq_change_voltage(const struct pxa_freqs *pxa_freq)
{
	return 0;
}

static void pxa_cpufreq_init_voltages(void) { }
#endif

static void find_freq_tables(struct cpufreq_frequency_table **freq_table,
			     const struct pxa_freqs **pxa_freqs)
{
	if (cpu_is_pxa25x()) {
		if (!pxa255_turbo_table) {
			*pxa_freqs = pxa255_run_freqs;
			*freq_table = pxa255_run_freq_table;
		} else {
			*pxa_freqs = pxa255_turbo_freqs;
			*freq_table = pxa255_turbo_freq_table;
		}
	} else if (cpu_is_pxa27x()) {
		*pxa_freqs = pxa27x_freqs;
		*freq_table = pxa27x_freq_table;
	} else {
		BUG();
	}
}

static void pxa27x_guess_max_freq(void)
{
	if (!pxa27x_maxfreq) {
		pxa27x_maxfreq = 416000;
		printk(KERN_INFO "PXA CPU 27x max frequency not defined "
		       "(pxa27x_maxfreq), assuming pxa271 with %dkHz maxfreq\n",
		       pxa27x_maxfreq);
	} else {
		pxa27x_maxfreq *= 1000;
	}
}

static void init_sdram_rows(void)
{
	uint32_t mdcnfg = __raw_readl(MDCNFG);
	unsigned int drac2 = 0, drac0 = 0;

	if (mdcnfg & (MDCNFG_DE2 | MDCNFG_DE3))
		drac2 = MDCNFG_DRAC2(mdcnfg);

	if (mdcnfg & (MDCNFG_DE0 | MDCNFG_DE1))
		drac0 = MDCNFG_DRAC0(mdcnfg);

	sdram_rows = 1 << (11 + max(drac0, drac2));
}

static u32 mdrefr_dri(unsigned int freq)
{
	u32 interval = freq * SDRAM_TREF / sdram_rows;

	return (interval - (cpu_is_pxa27x() ? 31 : 0)) / 32;
}

static unsigned int pxa_cpufreq_get(unsigned int cpu)
{
	return get_clk_frequency_khz(0);
}

static int pxa_set_target(struct cpufreq_policy *policy, unsigned int idx)
{
	struct cpufreq_frequency_table *pxa_freqs_table;
	const struct pxa_freqs *pxa_freq_settings;
	unsigned long flags;
	unsigned int new_freq_cpu, new_freq_mem;
	unsigned int unused, preset_mdrefr, postset_mdrefr, cclkcfg;
	int ret = 0;

	/* Get the current policy */
	find_freq_tables(&pxa_freqs_table, &pxa_freq_settings);

	new_freq_cpu = pxa_freq_settings[idx].khz;
	new_freq_mem = pxa_freq_settings[idx].membus;

	if (freq_debug)
		pr_debug("Changing CPU frequency to %d Mhz, (SDRAM %d Mhz)\n",
			 new_freq_cpu / 1000, (pxa_freq_settings[idx].div2) ?
			 (new_freq_mem / 2000) : (new_freq_mem / 1000));

	if (vcc_core && new_freq_cpu > policy->cur) {
		ret = pxa_cpufreq_change_voltage(&pxa_freq_settings[idx]);
		if (ret)
			return ret;
	}

	/* Calculate the next MDREFR.  If we're slowing down the SDRAM clock
	 * we need to preset the smaller DRI before the change.	 If we're
	 * speeding up we need to set the larger DRI value after the change.
	 */
	preset_mdrefr = postset_mdrefr = __raw_readl(MDREFR);
	if ((preset_mdrefr & MDREFR_DRI_MASK) > mdrefr_dri(new_freq_mem)) {
		preset_mdrefr = (preset_mdrefr & ~MDREFR_DRI_MASK);
		preset_mdrefr |= mdrefr_dri(new_freq_mem);
	}
	postset_mdrefr =
		(postset_mdrefr & ~MDREFR_DRI_MASK) | mdrefr_dri(new_freq_mem);

	/* If we're dividing the memory clock by two for the SDRAM clock, this
	 * must be set prior to the change.  Clearing the divide must be done
	 * after the change.
	 */
	if (pxa_freq_settings[idx].div2) {
		preset_mdrefr  |= MDREFR_DB2_MASK;
		postset_mdrefr |= MDREFR_DB2_MASK;
	} else {
		postset_mdrefr &= ~MDREFR_DB2_MASK;
	}

	local_irq_save(flags);

	/* Set new the CCCR and prepare CCLKCFG */
	CCCR = pxa_freq_settings[idx].cccr;
	cclkcfg = pxa_freq_settings[idx].cclkcfg;

	asm volatile("							\n\
		ldr	r4, [%1]		/* load MDREFR */	\n\
		b	2f						\n\
		.align	5						\n\
1:									\n\
		str	%3, [%1]		/* preset the MDREFR */	\n\
		mcr	p14, 0, %2, c6, c0, 0	/* set CCLKCFG[FCS] */	\n\
		str	%4, [%1]		/* postset the MDREFR */ \n\
									\n\
		b	3f						\n\
2:		b	1b						\n\
3:		nop							\n\
	  "
		     : "=&r" (unused)
		     : "r" (MDREFR), "r" (cclkcfg),
		       "r" (preset_mdrefr), "r" (postset_mdrefr)
		     : "r4", "r5");
	local_irq_restore(flags);

	/*
	 * Even if voltage setting fails, we don't report it, as the frequency
	 * change succeeded. The voltage reduction is not a critical failure,
	 * only power savings will suffer from this.
	 *
	 * Note: if the voltage change fails, and a return value is returned, a
	 * bug is triggered (seems a deadlock). Should anybody find out where,
	 * the "return 0" should become a "return ret".
	 */
	if (vcc_core && new_freq_cpu < policy->cur)
		ret = pxa_cpufreq_change_voltage(&pxa_freq_settings[idx]);

	return 0;
}

static int pxa_cpufreq_init(struct cpufreq_policy *policy)
{
	int i;
	unsigned int freq;
	struct cpufreq_frequency_table *pxa255_freq_table;
	const struct pxa_freqs *pxa255_freqs;

	/* try to guess pxa27x cpu */
	if (cpu_is_pxa27x())
		pxa27x_guess_max_freq();

	pxa_cpufreq_init_voltages();

	init_sdram_rows();

	/* set default policy and cpuinfo */
	policy->cpuinfo.transition_latency = 1000; /* FIXME: 1 ms, assumed */

	/* Generate pxa25x the run cpufreq_frequency_table struct */
	for (i = 0; i < NUM_PXA25x_RUN_FREQS; i++) {
		pxa255_run_freq_table[i].frequency = pxa255_run_freqs[i].khz;
		pxa255_run_freq_table[i].driver_data = i;
	}
	pxa255_run_freq_table[i].frequency = CPUFREQ_TABLE_END;

	/* Generate pxa25x the turbo cpufreq_frequency_table struct */
	for (i = 0; i < NUM_PXA25x_TURBO_FREQS; i++) {
		pxa255_turbo_freq_table[i].frequency =
			pxa255_turbo_freqs[i].khz;
		pxa255_turbo_freq_table[i].driver_data = i;
	}
	pxa255_turbo_freq_table[i].frequency = CPUFREQ_TABLE_END;

	pxa255_turbo_table = !!pxa255_turbo_table;

	/* Generate the pxa27x cpufreq_frequency_table struct */
	for (i = 0; i < NUM_PXA27x_FREQS; i++) {
		freq = pxa27x_freqs[i].khz;
		if (freq > pxa27x_maxfreq)
			break;
		pxa27x_freq_table[i].frequency = freq;
		pxa27x_freq_table[i].driver_data = i;
	}
	pxa27x_freq_table[i].driver_data = i;
	pxa27x_freq_table[i].frequency = CPUFREQ_TABLE_END;

	/*
	 * Set the policy's minimum and maximum frequencies from the tables
	 * just constructed.  This sets cpuinfo.mxx_freq, min and max.
	 */
	if (cpu_is_pxa25x()) {
		find_freq_tables(&pxa255_freq_table, &pxa255_freqs);
		pr_info("PXA255 cpufreq using %s frequency table\n",
			pxa255_turbo_table ? "turbo" : "run");

		cpufreq_table_validate_and_show(policy, pxa255_freq_table);
	}
	else if (cpu_is_pxa27x()) {
		cpufreq_table_validate_and_show(policy, pxa27x_freq_table);
	}

	printk(KERN_INFO "PXA CPU frequency change support initialized\n");

	return 0;
}

static struct cpufreq_driver pxa_cpufreq_driver = {
	.flags	= CPUFREQ_NEED_INITIAL_FREQ_CHECK,
	.verify	= cpufreq_generic_frequency_table_verify,
	.target_index = pxa_set_target,
	.init	= pxa_cpufreq_init,
	.get	= pxa_cpufreq_get,
	.name	= "PXA2xx",
};

static int __init pxa_cpu_init(void)
{
	int ret = -ENODEV;
	if (cpu_is_pxa25x() || cpu_is_pxa27x())
		ret = cpufreq_register_driver(&pxa_cpufreq_driver);
	return ret;
}

static void __exit pxa_cpu_exit(void)
{
	cpufreq_unregister_driver(&pxa_cpufreq_driver);
}


MODULE_AUTHOR("Intrinsyc Software Inc.");
MODULE_DESCRIPTION("CPU frequency changing driver for the PXA architecture");
MODULE_LICENSE("GPL");
module_init(pxa_cpu_init);
module_exit(pxa_cpu_exit);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  /*
 * Copyright (C) 2008 Marvell International Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/slab.h>
#include <linux/io.h>

#include <mach/generic.h>
#include <mach/pxa3xx-regs.h>

#define HSS_104M	(0)
#define HSS_156M	(1)
#define HSS_208M	(2)
#define HSS_312M	(3)

#define SMCFS_78M	(0)
#define SMCFS_104M	(2)
#define SMCFS_208M	(5)

#define SFLFS_104M	(0)
#define SFLFS_156M	(1)
#define SFLFS_208M	(2)
#define SFLFS_312M	(3)

#define XSPCLK_156M	(0)
#define XSPCLK_NONE	(3)

#define DMCFS_26M	(0)
#define DMCFS_260M	(3)

struct pxa3xx_freq_info {
	unsigned int cpufreq_mhz;
	unsigned int core_xl : 5;
	unsigned int core_xn : 3;
	unsigned int hss : 2;
	unsigned int dmcfs : 2;
	unsigned int smcfs : 3;
	unsigned int sflfs : 2;
	unsigned int df_clkdiv : 3;

	int	vcc_core;	/* in mV */
	int	vcc_sram;	/* in mV */
};

#define OP(cpufreq, _xl, _xn, _hss, _dmc, _smc, _sfl, _dfi, vcore, vsram) \
{									\
	.cpufreq_mhz	= cpufreq,					\
	.core_xl	= _xl,						\
	.core_xn	= _xn,						\
	.hss		= HSS_##_hss##M,				\
	.dmcfs		= DMCFS_##_dmc##M,				\
	.smcfs		= SMCFS_##_smc##M,				\
	.sflfs		= SFLFS_##_sfl##M,				\
	.df_clkdiv	= _dfi,						\
	.vcc_core	= vcore,					\
	.vcc_sram	= vsram,					\
}

static struct pxa3xx_freq_info pxa300_freqs[] = {
	/*  CPU XL XN  HSS DMEM SMEM SRAM DFI VCC_CORE VCC_SRAM */
	OP(104,  8, 1, 104, 260,  78, 104, 3, 1000, 1100), /* 104MHz */
	OP(208, 16, 1, 104, 260, 104, 156, 2, 1000, 1100), /* 208MHz */
	OP(416, 16, 2, 156, 260, 104, 208, 2, 1100, 1200), /* 416MHz */
	OP(624, 24, 2, 208, 260, 208, 312, 3, 1375, 1400), /* 624MHz */
};

static struct pxa3xx_freq_info pxa320_freqs[] = {
	/*  CPU XL XN  HSS DMEM SMEM SRAM DFI VCC_CORE VCC_SRAM */
	OP(104,  8, 1, 104, 260,  78, 104, 3, 1000, 1100), /* 104MHz */
	OP(208, 16, 1, 104, 260, 104, 156, 2, 1000, 1100), /* 208MHz */
	OP(416, 16, 2, 156, 260, 104, 208, 2, 1100, 1200), /* 416MHz */
	OP(624, 24, 2, 208, 260, 208, 312, 3, 1375, 1400), /* 624MHz */
	OP(806, 31, 2, 208, 260, 208, 312, 3, 1400, 1400), /* 806MHz */
};

static unsigned int pxa3xx_freqs_num;
static struct pxa3xx_freq_info *pxa3xx_freqs;
static struct cpufreq_frequency_table *pxa3xx_freqs_table;

static int setup_freqs_table(struct cpufreq_policy *policy,
			     struct pxa3xx_freq_info *freqs, int num)
{
	struct cpufreq_frequency_table *table;
	int i;

	table = kzalloc((num + 1) * sizeof(*table), GFP_KERNEL);
	if (table == NULL)
		return -ENOMEM;

	for (i = 0; i < num; i++) {
		table[i].driver_data = i;
		table[i].frequency = freqs[i].cpufreq_mhz * 1000;
	}
	table[num].driver_data = i;
	table[num].frequency = CPUFREQ_TABLE_END;

	pxa3xx_freqs = freqs;
	pxa3xx_freqs_num = num;
	pxa3xx_freqs_table = table;

	return cpufreq_table_validate_and_show(policy, table);
}

static void __update_core_freq(struct pxa3xx_freq_info *info)
{
	uint32_t mask = ACCR_XN_MASK | ACCR_XL_MASK;
	uint32_t accr = ACCR;
	uint32_t xclkcfg;

	accr &= ~(ACCR_XN_MASK | ACCR_XL_MASK | ACCR_XSPCLK_MASK);
	accr |= ACCR_XN(info->core_xn) | ACCR_XL(info->core_xl);

	/* No clock until core PLL is re-locked */
	accr |= ACCR_XSPCLK(XSPCLK_NONE);

	xclkcfg = (info->core_xn == 2) ? 0x3 : 0x2;	/* turbo bit */

	ACCR = accr;
	__asm__("mcr p14, 0, %0, c6, c0, 0\n" : : "r"(xclkcfg));

	while ((ACSR & mask) != (accr & mask))
		cpu_relax();
}

static void __update_bus_freq(struct pxa3xx_freq_info *info)
{
	uint32_t mask;
	uint32_t accr = ACCR;

	mask = ACCR_SMCFS_MASK | ACCR_SFLFS_MASK | ACCR_HSS_MASK |
		ACCR_DMCFS_MASK;

	accr &= ~mask;
	accr |= ACCR_SMCFS(info->smcfs) | ACCR_SFLFS(info->sflfs) |
		ACCR_HSS(info->hss) | ACCR_DMCFS(info->dmcfs);

	ACCR = accr;

	while ((ACSR & mask) != (accr & mask))
		cpu_relax();
}

static unsigned int pxa3xx_cpufreq_get(unsigned int cpu)
{
	return pxa3xx_get_clk_frequency_khz(0);
}

static int pxa3xx_cpufreq_set(struct cpufreq_policy *policy, unsigned int index)
{
	struct pxa3xx_freq_info *next;
	unsigned long flags;

	if (policy->cpu != 0)
		return -EINVAL;

	next = &pxa3xx_freqs[index];

	local_irq_save(flags);
	__update_core_freq(next);
	__update_bus_freq(next);
	local_irq_restore(flags);

	return 0;
}

static int pxa3xx_cpufreq_init(struct cpufreq_policy *policy)
{
	int ret = -EINVAL;

	/* set default policy and cpuinfo */
	policy->min = policy->cpuinfo.min_freq = 104000;
	policy->max = policy->cpuinfo.max_freq =
		(cpu_is_pxa320()) ? 806000 : 624000;
	policy->cpuinfo.transition_latency = 1000; /* FIXME: 1 ms, assumed */

	if (cpu_is_pxa300() || cpu_is_pxa310())
		ret = setup_freqs_table(policy, pxa300_freqs,
					ARRAY_SIZE(pxa300_freqs));

	if (cpu_is_pxa320())
		ret = setup_freqs_table(policy, pxa320_freqs,
					ARRAY_SIZE(pxa320_freqs));

	if (ret) {
		pr_err("failed to setup frequency table\n");
		return ret;
	}

	pr_info("CPUFREQ support for PXA3xx initialized\n");
	return 0;
}

static struct cpufreq_driver pxa3xx_cpufreq_driver = {
	.flags		= CPUFREQ_NEED_INITIAL_FREQ_CHECK,
	.verify		= cpufreq_generic_frequency_table_verify,
	.target_index	= pxa3xx_cpufreq_set,
	.init		= pxa3xx_cpufreq_init,
	.get		= pxa3xx_cpufreq_get,
	.name		= "pxa3xx-cpufreq",
};

static int __init cpufreq_init(void)
{
	if (cpu_is_pxa3xx())
		return cpufreq_register_driver(&pxa3xx_cpufreq_driver);

	return 0;
}
module_init(cpufreq_init);

static void __exit cpufreq_exit(void)
{
	cpufreq_unregister_driver(&pxa3xx_cpufreq_driver);
}
module_exit(cpufreq_exit);

MODULE_DESCRIPTION("CPU frequency scaling driver for PXA3xx");
MODULE_LICENSE("GPL");
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     