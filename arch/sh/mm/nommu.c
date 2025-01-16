  bus_id          : 1,
		    bus_address     : 1,
		    bus_data        : 1,
		    bus_cmd         : 1,
		    requestor_id    : 1,
		    responder_id    : 1,
		    target_id       : 1,
		    oem_data        : 1,
		    reserved        : 54;
	} valid;
	u64 err_status;
	u16 err_type;
	u16 bus_id;
	u32 reserved;
	u64 bus_address;
	u64 bus_data;
	u64 bus_cmd;
	u64 requestor_id;
	u64 responder_id;
	u64 target_id;
	u8 oem_data[1];			/* Variable length data */
} sal_log_pci_bus_err_info_t;

typedef struct sal_log_smbios_dev_err_info {
	sal_log_section_hdr_t header;
	struct {
		u64 event_type      : 1,
		    length          : 1,
		    time_stamp      : 1,
		    data            : 1,
		    reserved1       : 60;
	} valid;
	u8 event_type;
	u8 length;
	u8 time_stamp[6];
	u8 data[1];			/* data of variable length, length == slsmb_length */
} sal_log_smbios_dev_err_info_t;

typedef struct sal_log_pci_comp_err_info {
	sal_log_section_hdr_t header;
	struct {
		u64 err_status      : 1,
		    comp_info       : 1,
		    num_mem_regs    : 1,
		    num_io_regs     : 1,
		    reg_data_pairs  : 1,
		    oem_data        : 1,
		    reserved        : 58;
	} valid;
	u64 err_status;
	struct {
		u16 vendor_id;
		u16 device_id;
		u8 class_code[3];
		u8 func_num;
		u8 dev_num;
		u8 bus_num;
		u8 seg_num;
		u8 reserved[5];
	} comp_info;
	u32 num_mem_regs;
	u32 num_io_regs;
	u64 reg_data_pairs[1];
	/*
	 * array of address/data register pairs is num_mem_regs + num_io_regs elements
	 * long.  Each array element consists of a u64 address followed by a u64 data
	 * value.  The oem_data array immediately follows the reg_data_pairs array
	 */
	u8 oem_data[1];			/* Variable length data */
} sal_log_pci_