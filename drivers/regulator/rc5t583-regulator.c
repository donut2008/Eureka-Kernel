n 1 */
#define MC_SETTINGS		0x40
  #define IS_MIRRORED(mc)		((mc) & (1 << 16))
  #define IS_ECC_ENABLED(mc)		((mc) & (1 << 5))
  #define IS_RETRY_ENABLED(mc)		((mc) & (1 << 31))
  #define IS_SCRBALGO_ENHANCED(mc)	((mc) & (1 << 8))

#define MC_SETTINGS_A		0x58
  #define IS_SINGLE_MODE(mca)		((mca) & (1 << 14))

#define TOLM			0x6C

#define MIR0			0x80
#define MIR1			0x84
#define MIR2			0x88

/*
 * Note: Other Intel EDAC drivers use AMBPRESENT to identify if the available
 * memory. From datasheet item 7.3.1 (FB-DIMM technology & organization), it
 * seems that we cannot use this information directly for the same usage.
 * Each memory slot may have up to 2 AMB interfaces, one for income and another
 * for outcome interface to the next slot.
 * For now, the driver just stores the AMB present registers, but rely only at
 * the MTR info to detect memory.
 * Datasheet is also not clear about how to map each AMBPRESENT registers to
 * one of the 4 available channels.
 */
#define AMBPRESENT_0	0x64
#define AMBPRESENT_1	0x66

static const u16 mtr_regs[MAX_SLOTS] = {
	0x80, 0x84, 0x88, 0x8c,
	0x82, 0x86, 0x8a, 0x8e
};

/*
 * Defines to extract the vaious fields from the
 *	MTRx - Memory Technology Registers
 */
#define MTR_DIMMS_PRESENT(mtr)		((mtr) & (1 << 8))
#define MTR_DIMMS_ETHROTTLE(mtr)	((mtr) & (1 << 7))
#define MTR_DRAM_WIDTH(mtr)		(((mtr) & (1 << 6)) ? 8 : 4)
#define MTR_DRAM_BANKS(mtr)		(((mtr) & (1 << 5)) ? 8 : 4)
#define MTR_DIMM_RANKS(mtr)		(((mtr) & (1 << 4)) ? 1 : 0)
#define MTR_DIMM_ROWS(mtr)		(((mtr) >> 2) & 0x3)
#define MTR_DRAM_BANKS_ADDR_BITS	2
#define MTR_DIMM_ROWS_ADDR_BITS(mtr)	(MTR_DIMM_ROWS(mtr) + 13)
#define MTR_DIMM_COLS(mtr)		((mtr) & 0x3)
#define MTR_DIMM_COLS_ADDR_BITS(mtr)	(MTR_DIMM_COLS(mtr) + 10)

/************************************************
 * i7300 Register definitions for error detection
 ************************************************/

/*
 * Device 16.1: FBD Error Registers
 */
#define FERR_FAT_FBD	0x98
static const char *ferr_fat_fbd_name[] = {
	[22] = "Non-Redundant Fast Reset Timeout",
	[2]  = ">Tmid Thermal event with intelligent throttling disabled",
	[1]  = "Memory or FBD configuration CRC read error",
	[0]  = "Memory Write error on non-redundant retry or "
	       "FBD configuration Write error on retry",
};
#define GET_FBD_FAT_IDX(fbderr)	(((fbderr) >> 28) & 3)
#define FERR_FAT_FBD_ERR_MASK ((1 << 0) | (1 << 1) | (1 << 2) | (1 << 22))

#define FERR_NF_FBD	0xa0
static const char *ferr_nf_fbd_name[] = {
	[24] = "DIMM-Spare Copy Completed",
	[23] = "DIMM-Spare Copy Initiated",
	[22] = "Redundant Fast Reset Timeout",
	[21] = "Memory Write error on redundant retry",
	[18] = "SPD protocol Error",
	[17] = "FBD Northbound parity error on FBD Sync Status",
	[16] = "Correctable Patrol Data ECC",
	[15] = "Correctable Resilver- or Spare-Copy Data ECC",
	[14] = "Correctable Mirrored Demand Data ECC",
	[13] = "Correctable Non-Mirrored Demand Data ECC",
	[11] = "Memory or FBD configuration CRC read error",
	[10] = "FBD Configuration Write error on first attempt",
	[9]  = "Memory Write error on first attempt",
	[8]  = "Non-Aliased Uncorrectable Patrol Data ECC",
	[7]  = "Non-Aliased Uncorrectable Resilver- or Spare-Copy Data ECC",
	[6]  = "Non-Aliased Uncorrectable Mirrored Demand Data ECC",
	[5]  = "Non-Aliased Uncorrectable Non-Mirrored Demand Data ECC",
	[4]  = "Aliased Uncorrectable Patrol Data ECC",
	[3]  = "Aliased Uncorrectable Resilver- or Spare-Copy Data ECC",
	[2]  = "Aliased Uncorrectable Mirrored Demand Data ECC",
	[1]  = "Aliased Uncorrectable Non-Mirrored Demand Data ECC",
	[0]  = "Uncorrectable Data ECC on Replay",
};
#define GET_FBD_NF_IDX(fbderr)	(((fbderr) >> 28) & 3)
#define FERR_NF_FBD_ERR_MASK ((1 << 24) | (1 << 23) | (1 << 22) | (1 << 21) |\
			      (1 << 18) | (1 << 17) | (1 << 16) | (1 << 15) |\
			      (1 << 14) | (1 << 13) | (1 << 11) | (1 << 10) |\
			      (1 << 9)  | (1 << 8)  | (1 << 7)  | (1 << 6)  |\
			      (1 << 5)  | (1 << 4)  | (1 << 3)  | (1 << 2)  |\
			      (1 << 1)  | (1 << 0))

#define EMASK_FBD	0xa8
#define EMASK_FBD_ERR_MASK ((1 << 27) | (1 << 26) | (1 << 25) | (1 << 24) |\
			    (1 << 22) | (1 << 21) | (1 << 20) | (1 << 19) |\
			    (1 << 18) | (1 << 17) | (1 << 16) | (1 << 14) |\
			    (1 << 13) | (1 << 12) | (1 << 11) | (1 << 10) |\
			    (1 << 9)  | (1 << 8)  | (1 << 7)  | (1 << 6)  |\
			    (1 << 5)  | (1 << 4)  | (1 << 3)  | (1 << 2)  |\
			    (1 << 1)  | (1 << 0))

/*
 * Device 16.2: Global Error Registers
 */

#define FERR_GLOBAL_HI	0x48
static const char *ferr_global_hi_name[] = {
	[3] = "FSB 3 Fatal Error",
	[2] = "FSB 2 Fatal Error",
	[1] = "FSB 1 Fatal Error",
	[0] = "FSB 0 Fatal Error",
};
#define ferr_global_hi_is_fatal(errno)	1

#define FERR_GLOBAL_LO	0x40
static const char *ferr_global_lo_name[] = {
	[31] = "Internal MCH Fatal Error",
	[30] = "Intel QuickData Technology Device Fatal Error",
	[29] = "FSB1 Fatal Error",
	[28] = "FSB0 Fatal Error",
	[27] = "FBD Channel 3 Fatal Error",
	[26] = "FBD Channel 2 Fatal Error",
	[25] = "FBD Channel 1 Fatal Error",
	[24] = "FBD Channel 0 Fatal Error",
	[23] = "PCI Express Device 7Fatal Error",
	[22] = "PCI Express Device 6 Fatal Error",
	[21] = "PCI Express Device 5 Fatal Error",
	[20] = "PCI Express Device 4 Fatal Error",
	[19] = "PCI Express Device 3 Fatal Error",
	[18] = "PCI Express Device 2 Fatal Error",
	[17] = "PCI Express Device 1 Fatal Error",
	[16] = "ESI Fatal Error",
	[15] = "Internal MCH Non-Fatal Error",
	[14] = "Intel QuickData Technology Device Non Fatal Error",
	[13] = "FSB1 Non-Fatal Error",
	[12] = "FSB 0 Non-Fatal Error",
	[11] = "FBD Channel 3 Non-Fatal Error",
	[10] = "FBD Channel 2 Non-Fatal Error",
	[9]  = "FBD Channel 1 Non-Fatal Error",
	[8]  = "FBD Channel 0 Non-Fatal Error",
	[7]  = "PCI Express Device 7 Non-Fatal Error",
	[6]  = "PCI Express Device 6 Non-Fatal Error",
	[5]  = "PCI Express Device 5 Non-Fatal Error",
	[4]  = "PCI Express Device 4 Non-Fatal Error",
	[3]  = "PCI Express Device 3 Non-Fatal Error",
	[2]  = "PCI Express Device 2 Non-Fatal Error",
	[1]  = "PCI Express Device 1 Non-Fatal Error",
	[0]  = "ESI Non-Fatal Error",
};
#define ferr_global_lo_is_fatal(errno)	((errno < 16) ? 0 : 1)

#define NRECMEMA	0xbe
  #define NRECMEMA_BANK(v)	(((v) >> 12) & 7)
  #define NRECMEMA_RANK(v)	(((v) >> 8) & 15)

#define NRECMEMB	0xc0
  #define NRECMEMB_IS_WR(v)	((v) & (1 << 31))
  #define NRECMEMB_CAS(v)	(((v) >> 16) & 0x1fff)
  #define NRECMEMB_RAS(v)	((v) & 0xffff)

#define