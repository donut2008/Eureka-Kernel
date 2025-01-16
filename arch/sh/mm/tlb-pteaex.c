B_ECODE_* (in shubio.h). The hardware uses
 * an error code of 0 (IIO_ICRB_ECODE_DERR), but we want zero
 * to mean BTE_SUCCESS, so add one (BTEFAIL_OFFSET) to the error
 * codes to give the following error codes.
 */
#define BTEFAIL_OFFSET	1

typedef enum {
	BTE_SUCCESS,		/* 0 is success */
	BTEFAIL_DIR,		/* Directory error due to IIO access*/
	BTEFAIL_POISON,		/* poison error on IO access (write to poison page) */
	BTEFAIL_WERR,		/* Write error (ie WINV to a Read only line) */
	BTEFAIL_ACCESS,		/* access error (protection violation) */
	BTEFAIL_PWERR,		/* Partial Write Error */
	BTEFAIL_PRERR,		/* Partial Read Error */
	BTEFAIL_TOUT,		/* CRB Time out */
	BTEFAIL_XTERR,		/* Incoming xtalk pkt had error bit */
	BTEFAIL_NOTAVAIL,	/* BTE not available */
} bte_result_t;

#define BTEFAIL_SH2_RESP_SHORT	0x1	/* bit 000001 */
#define BTEFAIL_SH2_RESP_LONG	0x2	/* bit 000010 */
#define BTEFAIL_SH2_RESP_DSP	0x4	/* bit 000100 */
#define BTEFAIL_SH2_RESP_ACCESS	0x8	/* bit 001000 */
#define BTEFAIL_SH2_CRB_TO	0x10	/* bit 010000 */
#define BTEFAIL_SH2_NACK_LIMIT	0x20	/* bit 100000 */
#define BTEFAIL_SH2_ALL		0x3F	/* bit 111111 */

#define	BTE_ERR_BITS	0x3FUL
#define	BTE_ERR_SHIFT	36
#define BTE_ERR_MASK	(BTE_ERR_BITS << BTE_ERR_SHIFT)

#define BTE_ERROR_RETRY(value)						\
	(is_shub2() ? (value != BTEFAIL_SH2_CRB_TO)			\
		: (value != BTEFAIL_TOUT))

/*
 * On shub1 BTE_ERR_MASK will always be false, so no need for is_shub2()
 */
#define BTE_SHUB2_ERROR(_status)					\
	((_status & BTE_ERR_MASK) 					\
	   ? (((_status >> BTE_ERR_SHIFT) & BTE_ERR_BITS) | IBLS_ERROR) \
	   : _status)

#define BTE_GET_ERROR_STATUS(_status)					\
	(BTE_SHUB2_ERROR(_status) & ~IBLS_ERROR)

#define BTE_VALID_SH2_ERROR(value)					\
	((value >= BTEFAIL_SH2_RESP_SHORT) && (value <= BTEFAIL_SH2_ALL))

/*
 * Structure defining a bte.  An instance of this
 * structure is created in the nodepda for each
 * bte on that node (as defined by BTES_PER_NODE)
 * This structure contains everything necessary
 * to work with a BTE.
 */
struct bteinfo_s {
	volatile u64 notify ____cacheline_aligned;
	u64 *bte_base_addr ____cacheline_aligned;
	u64 *bte_source_addr;
	u64 *bte_destination_addr;
	u64 *bte_control_addr;
	u64 *bte_notify_addr;
	spinlock_t spinlock;
	cnodeid_t bte_cnode;	/* cnode                            */
	int bte_error_count;	/* Number of errors encountered     */
	int bte_num;		/* 0 --> BTE0, 1 --> BTE1           */
	int cleanup_active;	/* Interface is locked for cleanup  */
	volatile bte_result_t bh_error;	/* error while processing   */
	volatile u64 *most_rcnt_na;
	struct bteinfo_s *btes_to_try[MAX_BTES_PER_NODE];
};


/*
 * Function prototypes (functions defined in bte.c, used elsewhere)
 */
extern bte_result_t bte_copy(u64, u64, u64, u64, void *);
extern bte_result_t bte_unaligned_copy(u6