he SAL system table is followed by a variable number of variable
 * length descriptors.  The structure of these descriptors follows
 * below.
 * The defininition follows SAL specs from July 2000
 */
struct ia64_sal_systab {
	u8 signature[4];	/* should be "SST_" */
	u32 size;		/* size of this table in bytes */
	u8 sal_rev_minor;
	u8 sal_rev_major;
	u16 entry_count;	/* # of entries in variable portion */
	u8 checksum;
	u8 reserved1[7];
	u8 sal_a_rev_minor;
	u8 sal_a_rev_major;
	u8 sal_b_rev_minor;
	u8 sal_b_rev_major;
	/* oem_id & product_id: terminating NUL is missing if string is exactly 32 bytes long. */
	u8 oem_id[32];
	u8 product_id[32];	/* ASCII product id  */
	u8 reserved2[8];
};

enum sal_systab_entry_type {
	SAL_DESC_ENTRY_POINT = 0,
	SAL_DESC_MEMORY = 1,
	SAL_DESC_PLATFORM_FEATURE = 2,
	SAL_DESC_TR = 3,
	SAL_DESC_PTC = 4,
	SAL_DESC_AP_WAKEUP = 5
};

/*
 * Entry type:	Size:
 *	0	48
 *	1	32
 *	2	16
 *	3	32
 *	4	16
 *	5	16
 */
#define SAL_DESC_SIZE(type)	"\060\040\020\040\020\020"[(unsigned) type]

typedef struct ia64_sal_desc_entry_point {
	u8 type;
	u8 reserved1[7];
	u64 pal_proc;
	u64 sal_proc;
	u64 gp;
	u8 reserved2[16];
}ia64_sal_desc_entry_point_t;

typedef struct ia64_sal_desc_memory {
	u8 type;
	u8 used_by_sal;	/* needs to be mapped for SAL? */
	u8 mem_attr;		/* current memory attribute setting */
	u8 access_rights;	/* access rights set up by SAL */
	u8 mem_attr_mask;	/* mask of supported memory attributes */
	u8 reserved1;
	u8 mem_type;		/* memory type */
	u8 mem_usage;		/* memory usage */
	u64 addr;		/* physical address of memory */
	u32 length;	/* length (multiple of 4KB pages) */
	u32 reserved2;
	u8 oem_reserved[8];
} ia64_sal_desc_memory_t;

typedef struct ia64_sal_desc_platform_feature {
	u8 type;
	u8 feature_mask;
	u8 reserved1[14];
} ia64_sal_desc_platform_feature_t;

typedef struct ia64_sal_desc_tr {
	u8 type;
	u8 tr_type;		/* 0 == instruction, 1 == data */
	u8 regnum;		/* translation register number */
	u8 reserved1[5];
	u64 addr;		/* virtual address of area covered */
	u64 page_size;		/* encoded page size */
	u8 reserved2[8];
} ia64_sal_desc_tr_t;

typedef struct ia64_sal_desc_ptc {
	u8 type;
	u8 reserved1[3];
	u32 num_domains;	/* # of coherence domains */
	u64 domain_info;	/* physical address of domain info table */
} ia64_sal_desc_ptc_t;

typedef struct ia64_sal_ptc_domain_info {
	u64 proc_count;		/* number of processors in domain */
	u64 proc_list;		/* physical address of LID array */
} ia64_sal_ptc_domain_info_t;

typedef struct ia64_sal_ptc_domain_proc_entry {
	u64 id  : 8;		/* id of processor */
	u64 eid : 8;		/* eid of processor */
} ia64_sal_ptc_domain_proc_entry_t;


#define IA64_SAL_AP_EXTERNAL_INT 0

typedef struct ia64_sal_desc_ap_wakeup {
	u8 type;
	u8 mechanism;		/* 0 == external interrupt */
	u8 reserved1[6];
	u64 vector;		/* interrupt vector in range 0x10-0xff */
} ia64_sal_desc_ap_wakeup_t ;

extern ia64_sal_handler ia64_sal;
extern struct ia64_sal_desc_ptc *ia64_ptc_domain_info;

extern unsigned short sal_revision;	/* supported SAL spec revision */
extern unsigned short sal_version;	/* SAL version; OEM dependent */
#define SAL_VERSION_CODE(major, minor) ((bin2bcd(major) << 8) | bin2bcd(minor))

extern const char *ia64_sal_strerror (long status);
extern void ia64_sal_init (struct ia64_sal_systab *sal_systab);

/* SAL information type encodings */
enum {
	SAL_INFO_TYPE_MCA  = 0,		/* Machine check abort information */
        SAL_INFO_TYPE_INIT = 1,		/* Init information */
        SAL_INFO_TYPE_CMC  = 2,		/* Corrected machine check information */
        SAL_INFO_TYPE_CPE  = 3		/* Corrected platform error information */
};

/* Encodings for machine check parameter types */
enum {
	SAL_MC_PARAM_RENDEZ_INT    = 1,	/* Rendezvous interrupt */
	SAL_MC_PARAM_RENDEZ_WAKEUP = 2,	/* Wakeup */
	SAL_MC_PARAM_CPE_INT	   = 3	/* Corrected Platform Error Int */
};

/* Encodings for rendezvous mechanisms */
enum {
	SAL_MC_PARAM_MECH