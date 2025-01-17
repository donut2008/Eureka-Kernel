
	struct kobject kobj;
};

/* To get from the instance's wq to the beginning of the ctl structure */
#define to_edac_mem_ctl_work(w) \
		container_of(w, struct mem_ctl_info, work)

#define to_edac_device_ctl_work(w) \
		container_of(w,struct edac_device_ctl_info,work)

/*
 * The alloc() and free() functions for the 'edac_device' control info
 * structure. A MC driver will allocate one of these for each edac_device
 * it is going to control/register with the EDAC CORE.
 */
extern struct edac_device_ctl_info *edac_device_alloc_ctl_info(
		unsigned sizeof_private,
		char *edac_device_name, unsigned nr_instances,
		char *edac_block_name, unsigned nr_blocks,
		unsigned offset_value,
		struct edac_dev_sysfs_block_attribute *block_attributes,
		unsigned nr_attribs,
		int device_index);

/* The offset value can be:
 *	-1 indicating no offset value
 *	0 for zero-based block numbers
 *	1 for 1-based block number
 *	other for other-based block number
 */
#define	BLOCK_OFFSET_VALUE_OFF	((unsigned) -1)

extern void edac_device_free_ctl_info(struct edac_device_ctl_info *ctl_info);

#ifdef CONFIG_PCI

struct edac_pci_counter {
	atomic_t pe_count;
	atomic_t npe_count;
};

/*
 * Abstract edac_pci control info structure
 *
 */
struct edac_pci_ctl_info {
	/* for global list of edac_pci_ctl_info structs */
	struct list_head link;

	int pci_idx;

	struct bus_type *edac_subsys;	/* pointer to subsystem */

	/* the internal state of this controller instance */
	int op_state;
	/* work struct for this instance */
	struct delayed_work work;

	/* pointer to edac polling checking routine:
	 *      If NOT NULL: points to polling check routine
	 *      If NULL: Then assumes INTERRUPT operation, where
	 *              MC driver will receive events
	 */
	void (*edac_check) (struct edac_pci_ctl_info * edac_dev);

	struct device *dev;	/* pointer to device structure */

	const char *mod_name;	/* module name */
	const char *ctl_name;	/* edac controller  name */
	const char *dev_name;	/* pci/platform/etc... name */

	void *pvt_info;		/* pointer to 'private driver' info */

	unsigned long start_time;	/* edac_pci load start time (jiffies) */

	struct completion complete;

	/* sysfs top name under 'edac' directory
	 * and instance name:
	 *      cpu/cpu0/...
	 *      cpu/cpu1/...
	 *      cpu/cpu2/...
	 *      ...
	 */
	char name[EDAC_DEVICE_NAME_LEN + 1];

	/* Event counters for the this whole EDAC Device */
	struct edac_pci_counter counters;

	/* edac sysfs device control for the 'name'
	 * device this structure controls
	 */
	struct kobject kobj;
	struct completion kobj_complete;
};

#define to_edac_pci_ctl_work(w) \
		container_of(w, struct edac_pci_ctl_info,work)

/* write all or some bits in a byte-register*/
static inline void pci_write_bits8(struct pci_dev *pdev, int offset, u8 value,
				   u8 mask)
{
	if (mask != 0xff) {
		u8 buf;

		pci_read_config_byte(pdev, offset, &buf);
		value &= mask;
		buf &= ~mask;
		value |= buf;
	}

	pci_write_config_byte(pdev, offset, value);
}

/* write all or some bits in a word-register*/
static inline void pci_write_bits16(struct pci_dev *pdev, int offset,
				    u16 value, u16 mask)
{
	if (mask != 0xffff) {
		u16 buf;

		pci_read_config_word(pdev, offset, &buf);
		value &= mask;
		buf &= ~mask;
		value |= buf;
	}

	pci_write_config_word(pdev, offset, value);
}

/*
 * pci_write_bits32
 *
 * edac local routine to do pci_write_config_dword, but adds
 * a mask parameter. If mask is all ones, ignore the mask.
 * Otherwise utilize the mask to isolate specified bits
 *
 * write all or some bits in a dword-register
 */
static inline void pci_write_bits32(struct pci_dev *pdev, int offset,
				    u32 value, u32 mask)
{
	if (mask != 0xffffffff) {
		u32 buf;

		pci_read_config_dword(pdev, offset, &buf);
		value &= mask;
		buf &= ~mask;
		value |= buf;
	}

	pci_write_config_dword(pdev, offset, value);
}

#endif				/* CONFIG_PCI */

struct mem_ctl_info *edac_mc_alloc(unsigned mc_num,
				   unsigned n_layers,
				   struct edac_mc_layer *layers,
				   unsigned sz_pvt);
extern int edac_mc_add_mc_with_groups(struct mem_ctl_info *mci,
				      const struct attribute_group **groups);
#define edac_mc_add_mc(mci)	edac_mc_add_mc_with_groups(mci, NULL)
extern void edac_mc_free(struct mem_ctl_info *mci);
extern struct mem_ctl_info *edac_mc_find(int idx);
extern struct mem_ctl_info *find_mci_by_dev(struct device *dev);
extern struct mem_ctl_info *edac_mc_del_mc(struct device *dev);
extern int edac_mc_find_csrow_by_page(struct mem_ctl_info *mci,
				      unsigned long page);

void edac_raw_mc_handle_error(const enum hw_event_mc_err_type type,
			      struct mem_ctl_info *mci,
			      struct edac_raw_error_desc *e);

void edac_mc_handle_error(const enum hw_event_mc_err_type type,
			  struct mem_ctl_info *mci,
			  const u16 error_count,
			  const unsigned long page_frame_number,
			  const unsigned long offset_in_page,
			  const unsigned long syndrome,
			  const int top_layer,
			  const int mid_layer,
			  const int low_layer,
			  const char *msg,
			  const char *other_detail);

/*
 * edac_device APIs
 */
extern int edac_device_add_device(struct edac_device_ctl_info *edac_dev);
extern struct edac_device_ctl_info *edac_device_del_device(struct device *dev);
extern void edac_device_handle_ue(struct edac_device_ctl_info *edac_dev,
				int inst_nr, int block_nr, const char *msg);
extern void edac_device_handle_ce(struct edac_device_ctl_info *edac_dev,
				int inst_nr, int block_nr, const char *msg);
extern int edac_device_alloc_index(void);
extern const char *edac_layer_name[];

/*
 * edac_pci APIs
 */
extern struct edac_pci_ctl_info *edac_pci_alloc_ctl_info(unsigned int sz_pvt,
				const char *edac_pci_name);

extern void edac_pci_free_ctl_info(struct edac_pci_ctl_info *pci);

extern void edac_pci_reset_delay_period(struct edac_pci_ctl_info *pci,
				unsigned long value);

extern int edac_pci_alloc_index(void);
extern int edac_pci_add_device(struct edac_pci_ctl_info *pci, int edac_idx);
extern struct edac_pci_ctl_info *edac_pci_del_device(struct device *dev);

extern struct edac_pci_ctl_info *edac_pci_create_generic_ctl(
				struct device *dev,
				const char *mod_name);

extern void edac_pci_release_generic_ctl(struct edac_pci_ctl_info *pci);
extern int edac_pci_create_sysfs(struct edac_pci_ctl_info *pci);
extern void edac_pci_remove_sysfs(struct edac_pci_ctl_info *pci);

/*
 * edac misc APIs
 */
extern char *edac_op_state_to_string(int op_state);

#endif				/* _EDAC_CORE_H_ */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             