pu_pte); /* PTE to map per-CPU area */
DEFINE_PER_CPU(u64, ia64_mca_pal_pte);	    /* PTE to map PAL code */
DEFINE_PER_CPU(u64, ia64_mca_pal_base);    /* vaddr PAL code granule */
DEFINE_PER_CPU(u64, ia64_mca_tr_reload);   /* Flag for TR reload */

unsigned long __per_cpu_mca[NR_CPUS];

/* In mca_asm.S */
extern void			ia64_os_init_dispatch_monarch (void);
extern void			ia64_os_init_dispatch_slave (void);

static int monarch_cpu = -1;

static ia64_mc_info_t		ia64_mc_info;

#define MAX_CPE_POLL_INTERVAL (15*60*HZ) /* 15 minutes */
#define MIN_CPE_POLL_INTERVAL (2*60*HZ)  /* 2 minutes */
#define CMC_POLL_INTERVAL     (1*60*HZ)  /* 1 minute */
#define CPE_HISTORY_LENGTH    5
#define CMC_HISTORY_LENGTH    5

#ifdef CONFIG_ACPI
static struct timer_list cpe_poll_timer;
#endif
static struct timer_list cmc_poll_timer;
/*
 * This variable tells whether we are currently in polling mode.
 * Start with this in the wrong state so we won't play w/ timers
 * before the system is ready.
 */
static int cmc_polling_enabled = 1;

/*
 * Clearing this variable prevents CPE polling from getting activated
 * in mca_late_init.  Use it if your system doesn't provide a CPEI,
 * but encounters problems retrieving CPE logs.  This should only be
 * necessary for debugging.
 */
static int cpe_poll_enabled = 1;

extern void salinfo_log_wakeup(int type, u8 *buffer, u64 size, int irqsafe);

static int mca_init __initdata;

/*
 * limited & delayed printing support for MCA/INIT handler
 */

#define mprintk(fmt...) ia64_mca_printk(fmt)

#define MLOGBUF_SIZE (512+256*NR_CPUS)
#define MLOGBUF_MSGMAX 256
static char mlogbuf[MLOGBUF_SIZE];
static DEFINE_SPINLOCK(mlogbuf_wlock);	/* mca context only */
static DEFINE_SPINLOCK(mlogbuf_rlock);	/* normal context only */
static unsigned long mlogbuf_start;
static unsigned long mlogbuf_end;
static unsigned int mlogbuf_finished = 0;
static unsigned long mlogbuf_timestamp = 0;

static int loglevel_save = -1;
#define BREAK_LOGLEVEL(__console_loglevel)		\
	oops_in_progress = 1;				\
	if (loglevel_save < 0)				\
		loglevel_save = __console_loglevel;	\
	__console_loglevel = 15;

#define RESTORE_LOGLEVEL(__console_loglevel)		\
	if (loglevel_save >= 0) {			\
		__console_loglevel = loglevel_save;	\
		loglevel_save = -1;			\
	}						\
	mlogbuf_finished = 0;				\
	oops_in_progress = 0;

/*
 * Push messages into buffer, print them later if not urgent.
 */
void ia64_mca_printk(const char *fmt, ...)
{
	va_list args;
	int printed_len;
	char temp_buf[MLOGBUF_MSGMAX];
	char *p;

	va_start(args, fmt);
	printed_len = vscnprintf(temp_buf, sizeof(temp_buf), fmt, args);
	va_end(args);

	/* Copy the output into mlogbuf */
	if (oops_in_progress) {
		/* mlogbuf was abandoned, use printk directly instead. */
		printk("%s", temp_buf);
	} else {
		spin_lock(&mlogbuf_wlock);
		for (p = temp_buf; *p; p++) {
			unsigned long next = (mlogbuf_end + 1) % MLOGBUF_SIZE;
			if (next != mlogbuf_start) {
				mlogbuf[mlogbuf_end] = *p;
				mlogbuf_end = next;
			} else {
				/* buffer full */
				break;
			}
		}
		mlogbuf[mlogbuf_end] = '\0';
		spin_unlock(&mlogbuf_wlock);
	}
}
EXPORT_SYMBOL(ia64_mca_printk);

/*
 * Print buffered messages.
 *  NOTE: call this after returning normal context. (ex. from salinfod)
 */
void ia64_mlogbuf_dump(void)
{
	char temp_buf[MLOGBUF_MSGMAX];
	char *p;
	unsigned long index;
	unsigned long flags;
	unsigned int printed_len;

	/* Get output from mlogbuf */
	while (mlogbuf_start != mlogbuf_end) {
		temp_buf[0] = '\0';
		p = temp_buf;
		printed_len = 0;

		spin_lock_irqsave(&mlogbuf_rlock, flags);

		index = mlogbuf_start;
		while (index != mlogbuf_end) {
			*p = mlogbuf[index];
			index = (index + 1) % MLOGBUF_SIZE;
			if (!*p)
				break;
			p++;
			if (++printed_len >= MLOGBUF_MSGMAX - 1)
				break;
		}
		*p = '\0';
		if (temp_buf[0])
			printk("%s", temp_buf);
		mlogbuf_start = index;

		mlogbuf_timestamp = 0;
		spin_unlock_irqrestore(&mlogbuf_rlock, flags);
	}
}
EXPORT_SYMBOL(ia64_mlogbuf_dump);

/*
 * Call this if system is going to down or if immediate flushing messages to
 * console is required. (ex. recovery was failed, crash dump is going to be
 * invoked, long-wait rendezvous etc.)
 *  NOTE: this should be called from monarch.
 */
static void ia64_mlogbuf_finish(int wait)
{
	BREAK_LOGLEVEL(console_loglevel);

	spin_lock_init(&mlogbuf_rlock);
	ia64_mlogbuf_dump();
	printk(KERN_EMERG "mlogbuf_finish: printing switched to urgent mode, "
		"MCA/INIT might be dodgy or fail.\n");

	if (!wait)
		return;

	/* wait for console */
	printk("Delaying for 5 seconds...\n");
	udelay(5*1000000);

	mlogbuf_finished = 1;
}

/*
 * Print buffered messages from INIT context.
 */
static void ia64_mlogbuf_dump_from_init(void)
{
	if (mlogbuf_finished)
		return;

	if (mlogbuf_timestamp &&
			time_before(jiffies, mlogbuf_timestamp + 30 * HZ)) {
		printk(KERN_ERR "INIT: mlogbuf_dump is interrupted by INIT "
			" and the system seems to be messed up.\n");
		ia64_mlogbuf_finish(0);
		return;
	}

	if (!spin_trylock(&mlogbuf_rlock)) {
		printk(KERN_ERR "INIT: mlogbuf_dump is interrupted by INIT. "
			"Generated messages other than stack dump will be "
			"buffered to mlogbuf and will be printed later.\n");
		printk(KERN_ERR "INIT: If messages would not printed after "
			"this INIT, wait 30sec and assert INIT again.\n");
		if (!mlogbuf_timestamp)
			mlogbuf_timestamp = jiffies;
		return;
	}
	spin_unlock(&mlogbuf_rlock);
	ia64_mlogbuf_dump();
}

static void inline
ia64_mca_spin(const char *func)
{
	if (monarch_cpu == smp_processor_id())
		ia64_mlogbuf_finish(0);
	mprintk(KERN_EMERG "%s: spinning here, not returning to SAL\n", func);
	while (1)
		cpu_relax();
}
/*
 * IA64_MCA log support
 */
#define IA64_MAX_LOGS		2	/* Double-buffering for nested MCAs */
#define IA64_MAX_LOG_TYPES      4   /* MCA, INIT, CMC, CPE */

typedef struct ia64_state_log_s
{
	spinlock_t	isl_lock;
	int		isl_index;
	unsigned long	isl_count;
	ia64_err_rec_t  *isl_log[IA64_MAX_LOGS]; /* need space to store header + error log */
} ia64_state_log_t;

static ia64_state_log_t ia64_state_log[IA64_MAX_LOG_TYPES];

#define IA64_LOG_ALLOCATE(it, size) \
	{ia64_state_log[it].isl_log[IA64_LOG_CURR_INDEX(it)] = \
		(ia64_err_rec_t *)alloc_bootmem(size); \
	ia64_state_log[it].isl_log[IA64_LOG_NEXT_INDEX(it)] = \
		(ia64_err_rec_t *)alloc_bootmem(size);}
#define IA64_LOG_LOCK_INIT(it) spin_lock_init(&ia64_state_log[it].isl_lock)
#define IA64_LOG_LOCK(it)      spin_lock_irqsave(&ia64_state_log[it].isl_lock, s)
#define IA64_LOG_UNLOCK(it)    spin_unlock_irqrestore(&ia64_state_log[it].isl_lock,s)
#define IA64_LOG_NEXT_INDEX(it)    ia64_state_log[it].isl_index
#define IA64_LOG_CURR_INDEX(it)    1 - ia64_state_log[it].isl_index
#define IA64_LOG_INDEX_INC(it) \
    {ia64_state_log[it].isl_index = 1 - ia64_state_log[it].isl_index; \
    ia64_state_log[it].isl_count++;}
#define IA64_LOG_INDEX_DEC(it) \
    ia64_state_log[it].isl_index = 1 - ia64_state_log[it].isl_index
#define IA64_LOG_NEXT_BUFFER(it)   (void *)((ia64_state_log[it].isl_log[IA64_LOG_NEXT_INDEX(it)]))
#define IA64_LOG_CURR_BUFFER(it)   (void *)((ia64_state_log[it].isl_log[IA64_LOG_CURR_INDEX(it)]))
#define IA64_LOG_COUNT(it)         ia64_state_log[it].isl_count

/*
 * ia64_log_init
 *	Reset the OS ia64 log buffer
 * Inputs   :   info_type   (SAL_INFO_TYPE_{MCA,INIT,CMC,CPE})
 * Outputs	:	None
 */
static void __init
ia64_log_init(int sal_info_type)
{
	u64	max_size = 0;

	IA64_LOG_NEXT_INDEX(sal_info_type) = 0;
	IA64_LOG_LOCK_INIT(sal_info_type);

	// SAL will tell us the maximum size of any error record of this type
	max_size = ia64_sal_get_state_info_size(sal_info_type);
	if (!max_size)
		/* alloc_bootmem() doesn't like zero-sized allocations! */
		return;

	// set up OS data structures to hold error info
	IA64_LOG_ALLOCATE(sal_info_type, max_size);
	memset(IA64_LOG_CURR_BUFFER(sal_info_type), 0, max_size);
	memset(IA64_LOG_NEXT_BUFFER(sal_info_type), 0, max_size);
}

/*
 * ia64_log_get
 *
 *	Get the current MCA log from SAL and copy it into the OS log buffer.
 *
 *  Inputs  :   info_type   (SAL_INFO_TYPE_{MCA,INIT,CMC,CPE})
 *              irq_safe    whether you can use printk at this point
 *  Outputs :   size        (total record length)
 *              *buffer     (ptr to error record)
 *
 */
static u64
ia64_log_get(int sal_info_type, u8 **buffer, int irq_safe)
{
	sal_log_record_header_t     *log_buffer;
	u64                         total_len = 0;
	unsigned long               s;

	IA64_LOG_LOCK(sal_info_type);

	/* Get the process state information */
	log_buffer = IA64_LOG_NEXT_BUFFER(sal_info_type);

	total_len = ia64_sal_get_state_info(sal_info_type, (u64 *)log_buffer);

	if (total_len) {
		IA64_LOG_INDEX_INC(sal_info_type);
		IA64_LOG_UNLOCK(sal_info_type);
		if (irq_safe) {
			IA64_MCA_DEBUG("%s: SAL error record type %d retrieved. Record length = %ld\n",
				       __func__, sal_info_type, total_len);
		}
		*buffer = (u8 *) log_buffer;
		return total_len;
	} else {
		IA64_LOG_UNLOCK(sal_info_type);
		return 0;
	}
}

/*
 *  ia64_mca_log_sal_error_record
 *
 *  This function retrieves a specified error record type from SAL
 *  and wakes up any processes waiting for error records.
 *
 *  Inputs  :   sal_info_type   (Type of error record MCA/CMC/CPE)
 *              FIXME: remove MCA and irq_safe.
 */
static void
ia64_mca_log_sal_error_record(int sal_info_type)
{
	u8 *buffer;
	sal_log_record_header_t *rh;
	u64 size;
	int irq_safe = sal_info_type != SAL_INFO_TYPE_MCA;
#ifdef IA64_MCA_DEBUG_INFO
	static const char * const rec_name[] = { "MCA", "INIT", "CMC", "CPE" };
#endif

	size = ia64_log_get(sal_info_type, &buffer, irq_safe);
	if (!size)
		return;

	salinfo_log_wakeup(sal_info_type, buffer, size, irq_safe);

	if (irq_safe)
		IA64_MCA_DEBUG("CPU %d: SAL log contains %s error record\n",
			smp_processor_id(),
			sal_info_type < ARRAY_SIZE(rec_name) ? rec_name[sal_info_type] : "UNKNOWN");

	/* Clear logs from corrected errors in case there's no user-level logger */
	rh = (sal_log_record_header_t *)buffer;
	if (rh->severity == sal_log_severity_corrected)
		ia64_sal_clear_state_info(sal_info_type);
}

/*
 * search_mca_table
 *  See if the MCA surfaced in an instruction range
 *  that has been tagged as recoverable.
 *
 *  Inputs
 *	first	First address range to check
 *	last	Last address range to check
 *	ip	Instruction pointer, address we are looking for
 *
 * Return value:
 *      1 on Success (in the table)/ 0 on Failure (not in the  table)
 */
int
search_mca_table (const struct mca_table_entry *first,
                const struct mca_table_entry *last,
                unsigned long ip)
{
        const struct mca_table_entry *curr;
        u64 curr_start, curr_end;

        curr = first;
        while (curr <= last) {
                curr_start = (u64) &curr->start_addr + curr->start_addr;
                curr_end = (u64) &curr->end_addr + curr->end_addr;

                if ((ip >= curr_start) && (ip <= curr_end)) {
                        return 1;
                }
                curr++;
        }
        return 0;
}

/* Given an address, look for it in the mca tables. */
int mca_recover_range(unsigned long addr)
{
	extern struct mca_table_entry __start___mca_table[];
	extern struct mca_table_entry __stop___mca_table[];

	return search_mca_table(__start___mca_table, __stop___mca_table-1, addr);
}
EXPORT_SYMBOL_GPL(mca_recover_range);

#ifdef CONFIG_ACPI

int cpe_vector = -1;
int ia64_cpe_irq = -1;

static irqreturn_t
ia64_mca_cpe_int_handler (int cpe_irq, void *arg)
{
	static unsigned long	cpe_history[CPE_HISTORY_LENGTH];
	static int		index;
	static DEFINE_SPINLOCK(cpe_history_lock);

	IA64_MCA_DEBUG("%s: received interrupt vector = %#x on CPU %d\n",
		       __func__, cpe_irq, smp_processor_id());

	/* SAL spec states this should run w/ interrupts enabled */
	local_irq_enable();

	spin_lock(&cpe_history_lock);
	if (!cpe_poll_enabled && cpe_vector >= 0) {

		int i, count = 1; /* we know 1 happened now */
		unsigned long now = jiffies;

		for (i = 0; i < CPE_HISTORY_LENGTH; i++) {
			if (now - cpe_history[i] <= HZ)
				count++;
		}

		IA64_MCA_DEBUG(KERN_INFO "CPE threshold %d/%