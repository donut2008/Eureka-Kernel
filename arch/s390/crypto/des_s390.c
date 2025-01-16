/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (c) 2003-2005 Silicon Graphics, Inc. All rights reserved.
 */

#ifndef __ASM_IA64_SN_TIOCE_H__
#define __ASM_IA64_SN_TIOCE_H__

/* CE ASIC part & mfgr information  */
#define TIOCE_PART_NUM			0xCE00
#define TIOCE_SRC_ID			0x01
#define TIOCE_REV_A			0x1

/* CE Virtual PPB Vendor/Device IDs */
#define CE_VIRT_PPB_VENDOR_ID		0x10a9
#define CE_VIRT_PPB_DEVICE_ID		0x4002

/* CE Host Bridge Vendor/Device IDs */
#define CE_HOST_BRIDGE_VENDOR_ID	0x10a9
#define CE_HOST_BRIDGE_DEVICE_ID	0x4001


#define TIOCE_NUM_M40_ATES		4096
#define TIOCE_NUM_M3240_ATES		2048
#define TIOCE_NUM_PORTS			2

/*
 * Register layout for TIOCE.  MMR offsets are shown at the far right of the
 * structure definition.
 */
typedef volatile struct tioce {
	/*
	 * ADMIN : Administration Registers
	 */
	u64	ce_adm_id;				/* 0x000000 */
	u64	ce_pad_000008;				/* 0x000008 */
	u64	ce_adm_dyn_credit_status;		/* 0x000010 */
	u64	ce_adm_last_credit_status;		/* 0x000018 */
	u64	ce_adm_credit_limit;			/* 0x000020 */
	u64	ce_adm_force_credit;			/* 0x000028 */
	u64	ce_adm_control;				/* 0x000030 */
	u64	ce_adm_mmr_chn_timeout;			/* 0x000038 */
	u64	ce_adm_ssp_ure_timeout;			/* 0x000040 */
	u64	ce_adm_ssp_dre_timeout;			/* 0x000048 */
	u64	ce_adm_ssp_debug_sel;			/* 0x000050 */
	u64	ce_adm_int_status;			/* 0x000058 */
	u64	ce_adm_int_status_alias;		/* 0x000060 */
	u64	ce_adm_int_mask;			/* 0x000068 */
	u64	ce_adm_int_pending;			/* 0x000070 */
	u64	ce_adm_force_int;			/* 0x000078 */
	u64	ce_adm_ure_ups_buf_barrier_flush;	/* 0x000080 */
	u64	ce_adm_int_dest[15];	    /* 0x000088 -- 0x0000F8 */
	u64	ce_adm_error_summary;			/* 0x000100 */
	u64	ce_adm_error_summary_alias;		/* 0x000108 */
	u64	ce_adm_error_mask;			/* 0x000110 */
	u64	ce_adm_first_error;			/* 0x000118 */
	u64	ce_adm_error_overflow;			/* 0x000120 */
	u64	ce_adm_error_overflow_alias;		/* 0x000128 */
	u64	ce_pad_000130[2];	    /* 0x000130 -- 0x000138 */
	u64	ce_adm_tnum_error;			/* 0x000140 */
	u64	ce_adm_mmr_err_detail;			/* 0x000148 */
	u64	ce_adm_msg_sram_perr_detail;		/* 0x000150 */
	u64	ce_adm_bap_sram_perr_detail;		/* 0x000158 */
	u64	ce_adm_ce_sram_perr_detail;		/* 0x000160 */
	u64	ce_adm_ce_credit_oflow_detail;		/* 0x000168 */
	u64	ce_adm_tx_link_idle_max_timer;		/* 0x000170 */
	u64	ce_adm_pcie_debug_sel;			/* 0x000178 */
	u64	ce_pad_000180[16];	    /* 0x000180 -- 0x0001F8 */

	u64	ce_adm_pcie_debug_sel_top;		/* 0x000200 */
	u64	ce_adm_pcie_debug_lat_sel_lo_top;	/* 0x000208 */
	u64	ce_adm_pcie_debug_lat_sel_hi_top;	/* 0x000210 */
	u64	ce_adm_pcie_debug_trig_sel_top;		/* 0x000218 */
	u64	ce_adm_pcie_debug_trig_lat_sel_lo_top;	/* 0x000220 */
	u64	ce_adm_pcie_debug_trig_lat_sel_hi_top;	/* 0x000228 */
	u64	ce_adm_pcie_trig_compare_top;		/* 0x000230 */
	u64	ce_adm_pcie_trig_compare_en_top;	/* 0x000238 */
	u64	ce_adm_ssp_debug_sel_top;		/* 0x000240 */
	u64	ce_adm_ssp_debug_lat_sel_lo_top;	/* 0x000248 */
	u64	ce_adm_ssp_debug_lat_sel_hi_top;	/* 0x000250 */
	u64	ce_adm_ssp_debug_trig_sel_top;		/* 0x000258 */
	u64	ce_adm_ssp_debug_trig_lat_sel_lo_top;	/* 0x000260 */
	u64	ce_adm_ssp_debug_trig_lat_sel_hi_top;	/* 0x000268 */
	u64	ce_adm_ssp_trig_compare_top;		/* 0x000270 */
	u64	ce_adm_ssp_trig_compare_en_top;		/* 0x000278 */
	u64	ce_pad_000280[48];	    /* 0x000280 -- 0x0003F8 */

	u64	ce_adm_bap_ctrl;			/* 0x000400 */
	u64	ce_pad_000408[127];	    /* 0x000408 -- 0x0007F8 */

	u64	ce_msg_buf_data63_0[35];    /* 0x000800 -- 0x000918 */
	u64	ce_pad_000920[29];	    /* 0x000920 -- 0x0009F8 */

	u64	ce_msg_buf_data127_64[35];  /* 0x000A00 -- 0x000B18 */
	u64	ce_pad_000B20[29];	    /* 0x000B20 -- 0x000BF8 */

	u64	ce_msg_buf_parity[35];	    /* 0x000C00 -- 0x000D18 */
	u64	ce_pad_000D20[29];	    /* 0x000D20 -- 0x000DF8 */

	u64	ce_pad_000E00[576];	    /* 0x000E00 -- 0x001FF8 */

	/*
	 * LSI : LSI's PCI Express Link Registers (Link#1 and Link#2)
	 * Link#1 MMRs at start at 0x002000, Link#2 MMRs at 0x003000
	 * NOTE: the comment offsets at far right: let 'z' = {2 or 3}
	 */
	#define ce_lsi(link_num)	ce_lsi[link_num-1]
	struct ce_lsi_reg {
		u64	ce_lsi_lpu_id;			/* 0x00z000 */
		u64	ce_lsi_rst;			/* 0x00z008 */
		u64	ce_lsi_dbg_stat;		/* 0x00z010 */
		u64	ce_lsi_dbg_cfg;			/* 0x00z018 */
		u64	ce_lsi_ltssm_ctrl;		/* 0x00z020 */
		u64	ce_lsi_lk_stat;			/* 0x00z028 */
		u64	ce_pad_00z030[2];   /* 0x00z030 -- 0x00z038 */
		u64	ce_lsi_int_and_stat;		/* 0x00z040 */
		u64	ce_lsi_int_mask;		/* 0x00z048 */
		u64	ce_pad_00z050[22];  /* 0x00z050 -- 0x00z0F8 */
		u64	ce_lsi_lk_perf_cnt_sel;		/* 0x00z100 */
		u64	ce_pad_00z108;			/* 0x00z108 */
		u64	ce_lsi_lk_perf_cnt_ctrl;	/* 0x00z110 */
		u64	ce_pad_00z118;			/* 0x00z118 */
		u64	ce_lsi_lk_perf_cnt1;		/* 0x00z120 */
		u64	ce_lsi_lk_perf_cnt1_test;	/* 0x00z128 */
		u64	ce_lsi_lk_perf_cnt2;		/* 0x00z130 */
		u64	ce_lsi_lk_perf_cnt2_test;	/* 0x00z138 */
		u64	ce_pad_00z140[24];  /* 0x00z140 -- 0x00z1F8 */
		u64	ce_lsi_lk_lyr_cfg;		/* 0x00z200 */
		u64	ce_lsi_lk_lyr_status;		/* 0x00z208 */
		u64	ce_lsi_lk_lyr_int_stat;		/* 0x00z210 */
		u64	ce_lsi_lk_ly_int_stat_test;	/* 0x00z218 */
		u64	ce_lsi_lk_ly_int_stat_mask;	/* 0x00z220 */
		u64	ce_pad_00z228[3];   /* 0x00z228 -- 0x00z238 */
		u64	ce_lsi_fc_upd_ctl;		/* 0x00z240 */
		u64	ce_pad_00z248[3];   /* 0x00z248 -- 0x00z258 */
		u64	ce_lsi_flw_ctl_upd_to_timer;	/* 0x00z260 */
		u64	ce_lsi_flw_ctl_upd_timer0;	/* 0x00z268 */
		u64	ce_lsi_flw_ctl_upd_timer1;	/* 0x00z270 */
		u64	ce_pad_00z278[49];  /* 0x00z278 -- 0x00z3F8 */
		u64	ce_lsi_freq_nak_lat_thrsh;	/* 0x00z400 */
		u64	ce_lsi_ack_nak_lat_tmr;		/* 0x00z408 */
		u64	ce_lsi_rply_tmr_thr;		/* 0x00z410 */
		u64	ce_lsi_rply_tmr;		/* 0x00z418 */
		u64	ce_lsi_rply_num_stat;		/* 0x00z420 */
		u64	ce_lsi_rty_buf_max_addr;	/* 0x00z428 */
		u64	ce_lsi_rty_fifo_ptr;		/* 0x00z430 */
		u64	ce_lsi_rty_fifo_rd_wr_ptr;	/* 0x00z438 */
		u64	ce_lsi_rty_fifo_cred;		/* 0x00z440 */
		u64	ce_lsi_seq_cnt;			/* 0x00z448 */
		u64	ce_lsi_ack_sent_seq_num;	/* 0x00z450 */
		u64	ce_lsi_seq_cnt_fifo_max_addr;	/* 0x00z458 */
		u64	ce_lsi_seq_cnt_fifo_ptr;	/* 0x00z460 */
		u64	ce_lsi_seq_cnt_rd_wr_ptr;	/* 0x00z468 */
		u64	ce_lsi_tx_lk_ts_ctl;		/* 0x00z470 */
		u64	ce_pad_00z478;			/* 0x00z478 */
		u64	ce_lsi_mem_addr_ctl;		/* 0x00z480 */
		u64	ce_lsi_mem_d_ld0;		/* 0x00z488 */
		u64	ce_lsi_mem_d_ld1;		/* 0x00z490 */
		u64	ce_lsi_mem_d_ld2;		/* 0x00z498 */
		u64	ce_lsi_mem_d_ld3;		/* 0x00z4A0 */
		u64	ce_lsi_mem_d_ld4;		/* 0x00z4A8 */
		u64	ce_pad_00z4B0[2];   /* 0x00z4B0 -- 0x00z4B8 */
		u64	ce_lsi_rty_d_cnt;		/* 0x00z4C0 */
		u64	ce_lsi_seq_buf_cnt;		/* 0x00z4C8 */
		u64	ce_lsi_seq_buf_bt_d;		/* 0x00z4D0 */
		u64	ce_pad_00z4D8;			/* 0x00z4D8 */
		u64	ce_lsi_ack_lat_thr;		/* 0x00z4E0 */
		u64	ce_pad_00z4E8[3];   /* 0x00z4E8 -- 0x00z4F8 */
		u64	ce_lsi_nxt_rcv_seq_1_cntr;	/* 0x00z500 */
		u64	ce_lsi_unsp_dllp_rcvd;		/* 0x00z508 */
		u64	ce_lsi_rcv_lk_ts_ctl;		/* 0x00z510 */
		u64	ce_pad_00z518[29];  /* 0x00z518 -- 0x00z5F8 */
		u64	ce_lsi_phy_lyr_cfg;		/* 0x00z600 */
		u64	ce_pad_00z608;			/* 0x00z608 */
		u64	ce_lsi_phy_lyr_int_stat;	/* 0x00z610 */
		u64	ce_lsi_phy_lyr_int_stat_test;	/* 0x00z618 */
		u64	ce_lsi_phy_lyr_int_mask;	/* 0x00z620 */
		u64	ce_pad_00z628[11];  /* 0x00z628 -- 0x00z678 */
		u64	ce_lsi_rcv_phy_cfg;		/* 0x00z680 */
		u64	ce_lsi_rcv_phy_stat1;		/* 0x00z688 */
		u64	ce_lsi_rcv_phy_stat2;		/* 0x00z690 */
		u64	ce_lsi_rcv_phy_stat3;		/* 0x00z698 */
		u64	ce_lsi_rcv_phy_int_stat;	/* 0x00z6A0 */
		u64	ce_lsi_rcv_phy_int_stat_test;	/* 0x00z6A8 */
		u64	ce_lsi_rcv_phy_int_mask;	/* 0x00z6B0 */
		u64	ce_pad_00z6B8[9];   /* 0x00z6B8 -- 0x00z6F8 */
		u64	ce_lsi_tx_phy_cfg;		/* 0x00z700 */
		u64	ce_lsi_tx_phy_stat;		/* 0x00z708 */
		u64	ce_lsi_tx_phy_int_stat;		/* 0x00z710 */
		u64	ce_lsi_tx_phy_int_stat_test;	/* 0x00z718 */
		u64	ce_lsi_tx_phy_int_mask;		/* 0x00z720 */
		u64	ce_lsi_tx_phy_stat2;		/* 0x00z728 */
		u64	ce_pad_00z730[10];  /* 0x00z730 -- 0x00z77F */
		u64	ce_lsi_ltssm_cfg1;		/* 0x00z780 */
		u64	ce_lsi_ltssm_cfg2;		/* 0x00z788 */
		u64	ce_lsi_ltssm_cfg3;		/* 0x00z790 */
		u64	ce_lsi_ltssm_cfg4;		/* 0x00z798 */
		u64	ce_lsi_ltssm_cfg5;		/* 0x00z7A0 */
		u64	ce_lsi_ltssm_stat1;		/* 0x00z7A8 */
		u64	ce_lsi_ltssm_stat2;		/* 0x00z7B0 */
		u64	ce_lsi_ltssm_int_stat;		/* 0x00z7B8 */
		u64	ce_lsi_ltssm_int_stat_test;	/* 0x00z7C0 */
		u64	ce_lsi_ltssm_int_mask;		/* 0x00z7C8 */
		u64	ce_lsi_ltssm_stat_wr_en;	/* 0x00z7D0 */
		u64	ce_pad_00z7D8[5];   /* 0x00z7D8 -- 0x00z7F8 */
		u64	ce_lsi_gb_cfg1;			/* 0x00z800 */
		u64	ce_lsi_gb_cfg2;			/* 0x00z808 */
		u64	ce_lsi_gb_cfg3;			/* 0x00z810 */
		u64	ce_lsi_gb_cfg4;			/* 0x00z818 */
		u64	ce_lsi_gb_stat;			/* 0x00z820 */
		u64	ce_lsi_gb_int_stat;		/* 0x00z828 */
		u64	ce_lsi_gb_int_stat_test;	/* 0x00z830 */
		u64	ce_lsi_gb_int_mask;		/* 0x00z838 */
		u64	ce_lsi_gb_pwr_dn1;		/* 0x00z840 */
		u64	ce_lsi_gb_pwr_dn2;		/* 0x00z848 */
		u64	ce_pad_00z850[246]; /* 0x00z850 -- 0x00zFF8 */
	} ce_lsi[2];

	u64	ce_pad_004000[10];	    /* 0x004000 -- 0x004048 */

	/*
	 * CRM: Coretalk Receive Module Registers
	 */
	u64	ce_crm_debug_mux;			/* 0x004050 */
	u64	ce_pad_004058;				/* 0x004058 */
	u64	ce_crm_ssp_err_cmd_wrd;			/* 0x004060 */
	u64	ce_crm_ssp_err_addr;			/* 0x004068 */
	u64	ce_crm_ssp_err_syn;			/* 0x004070 */

	u64	ce_pad_004078[499];	    /* 0x004078 -- 0x005008 */

	/*
         * CXM: Coretalk Xmit Module Registers
         */
	u64	ce_cxm_dyn_credit_status;		/* 0x005010 */
	u64	ce_cxm_last_credit_status;		/* 0x005018 */
	u64	ce_cxm_credit_limit;			/* 0x005020 */
	u64	ce_cxm_force_credit;			/* 0x005028 */
	u64	ce_cxm_disable_bypass;			/* 0x005030 */
	u64	ce_pad_005038[3];	    /* 0x005038 -- 0x005048 */
	u64	ce_cxm_debug_mux;			/* 0x005050 */

        u64        ce_pad_005058[501];         /* 0x005058 -- 0x005FF8 */

	/*
	 * DTL: Downstream Transaction Layer Regs (Link#1 and Link#2)
	 * DTL: Link#1 MMRs at start at 0x006000, Link#2 MMRs at 0x008000
	 * DTL: the comment offsets at far right: let 'y' = {6 or 8}
	 *
	 * UTL: Downstream Transaction Layer Regs (Link#1 and Link#2)
	 * UTL: Link#1 MMRs at start at 0x007000, Link#2 MMRs at 0x009000
	 * UTL: the comment offsets at far right: let 'z' = {7 or 9}
	 */
	#define ce_dtl(link_num)	ce_dtl_utl[link_num-1]
	#define ce_utl(link_num)	ce_dtl_utl[link_num-1]
	struct ce_dtl_utl_reg {
		/* DTL */
		u64	ce_dtl_dtdr_credit_limit;	/* 0x00y000 */
		u64	ce_dtl_dtdr_credit_force;	/* 0x00y008 */
		u64	ce_dtl_dyn_credit_status;	/* 0x00y010 */
		u64	ce_dtl_dtl_last_credit_stat;	/* 0x00y018 */
		u64	ce_dtl_dtl_ctrl;		/* 0x00y020 */
		u64	ce_pad_00y028[5];   /* 0x00y028 -- 0x00y048 */
		u64	ce_dtl_debug_sel;		/* 0x00y050 */
		u64	ce_pad_00y058[501]; /* 0x00y058 -- 0x00yFF8 */

		/* UTL */
		u64	ce_utl_utl_ctrl;		/* 0x00z000 */
		u64	ce_utl_debug_sel;		/* 0x00z008 */
		u64	ce_pad_00z010[510]; /* 0x00z010 -- 0x00zFF8 */
	} ce_dtl_utl[2];

	u64	ce_pad_00A000[514];	    /* 0x00A000 -- 0x00B008 */

	/*
	 * URE: Upstream Request Engine
         */
	u64	ce_ure_dyn_credit_status;		/* 0x00B010 */
	u64	ce_ure_last_credit_status;		/* 0x00B018 */
	u64	ce_ure_credit_limit;			/* 0x00B020 */
	u64	ce_pad_00B028;				/* 0x00B028 */
	u64	ce_ure_control;				/* 0x00B030 */
	u64	ce_ure_status;				/* 0x00B038 */
	u64	ce_pad_00B040[2];	    /* 0x00B040 -- 0x00B048 */
	u64	ce_ure_debug_sel;			/* 0x00B050 */
	u64	ce_ure_pcie_debug_sel;			/* 0x00B058 */
	u64	ce_ure_ssp_err_cmd_wrd;			/* 0x00B060 */
	u64	ce_ure_ssp_err_addr;			/* 0x00B068 */
	u64	ce_ure_page_map;			/* 0x00B070 */
	u64	ce_ure_dir_map[TIOCE_NUM_PORTS];	/* 0x00B078 */
	u64	ce_ure_pipe_sel1;			/* 0x00B088 */
	u64	ce_ure_pipe_mask1;			/* 0x00B090 */
	u64	ce_ure_pipe_sel2;			/* 0x00B098 */
	u64	ce_ure_pipe_mask2;			/* 0x00B0A0 */
	u64	ce_ure_pcie1_credits_sent;		/* 0x00B0A8 */
	u64	ce_ure_pcie1_credits_used;		/* 0x00B0B0 */
	u64	ce_ure_pcie1_credit_limit;		/* 0x00B0B8 */
	u64	ce_ure_pcie2_credits_sent;		/* 0x00B0C0 */
	u64	ce_ure_pcie2_credits_used;		/* 0x00B0C8 */
	u64	ce_ure_pcie2_credit_limit;		/* 0x00B0D0 */
	u64	ce_ure_pcie_force_credit;		/* 0x00B0D8 */
	u64	ce_ure_rd_tnum_val;			/* 0x00B0E0 */
	u64	ce_ure_rd_tnum_rsp_rcvd;		/* 0x00B0E8 */
	u64	ce_ure_rd_tnum_esent_timer;		/* 0x00B0F0 */
	u64	ce_ure_rd_tnum_error;			/* 0x00B0F8 */
	u64	ce_ure_rd_tnum_first_cl;		/* 0x00B100 */
	u64	ce_ure_rd_tnum_link_buf;		/* 0x00B108 */
	u64	ce_ure_wr_tnum_val;			/* 0x00B110 */
	u64	ce_ure_sram_err_addr0;			/* 0x00B118 */
	u64	ce_ure_sram_err_addr1;			/* 0x00B120 */
	u64	ce_ure_sram_err_addr2;			/* 0x00B128 */
	u64	ce_ure_sram_rd_addr0;			/* 0x00B130 */
	u64	ce_ure_sram_rd_addr1;			/* 0x00B138 */
	u64	ce_ure_sram_rd_addr2;			/* 0x00B140 */
	u64	ce_ure_sram_wr_addr0;			/* 0x00B148 */
	u64	ce_ure_sram_wr_addr1;			/* 0x00B150 */
	u64	ce_ure_sram_wr_addr2;			/* 0x00B158 */
	u64	ce_ure_buf_flush10;			/* 0x00B160 */
	u64	ce_ure_buf_flush11;			/* 0x00B168 */
	u64	ce_ure_buf_flush12;			/* 0x00B170 */
	u64	ce_ure_buf_flush13;			/* 0x00B178 */
	u64	ce_ure_buf_flush20;			/* 0x00B180 */
	u64	ce_ure_buf_flush21;			/* 0x00B188 */
	u64	ce_ure_buf_flush22;			/* 0x00B190 */
	u64	ce_ure_buf_flush23;			/* 0x00B198 */
	u64	ce_ure_pcie_control1;			/* 0x00B1A0 */
	u64	ce_ure_pcie_control2;			/* 0x00B1A8 */

	u64	ce_pad_00B1B0[458];	    /* 0x00B1B0 -- 0x00BFF8 */

	/* Upstream Data Buffer, Port1 */
	struct ce_ure_maint_ups_dat1_data {
		u64	data63_0[512];	    /* 0x00C000 -- 0x00CFF8 */
		u64	data127_64[512];    /* 0x00D000 -- 0x00DFF8 */
		u64	parity[512];	    /* 0x00E000 -- 0x00EFF8 */
	} ce_ure_maint_ups_dat1;

	/* Upstream Header Buffer, Port1 */
	struct ce_ure_maint_ups_hdr1_data {
		u64	data63_0[512];	    /* 0x00F000 -- 0x00FFF8 */
		u64	data127_64[512];    /* 0x010000 -- 0x010FF8 */
		u64	parity[512];	    /* 0x011000 -- 0x011FF8 */
	} ce_ure_maint_ups_hdr1;

	/* Upstream Data Buffer, Port2 */
	struct ce_ure_maint_ups_dat2_data {
		u64	data63_0[512];	    /* 0x012000 -- 0x012FF8 */
		u64	data127_64[512];    /* 0x013000 -- 0x013FF8 */
		u64	parity[512];	    /* 0x014000 -- 0x014FF8 */
	} ce_ure_maint_ups_dat2;

	/* Upstream Header Buffer, Port2 */
	struct ce_ure_maint_ups_hdr2_data {
		u64	data63_0[512];	    /* 0x015000 -- 0x015FF8 */
		u64	data127_64[512];    /* 0x016000 -- 0x016FF8 */
		u64	parity[512];	    /* 0x017000 -- 0x017FF8 */
	} ce_ure_maint_ups_hdr2;

	/* Downstream Data Buffer */
	struct ce_ure_maint_dns_dat_data {
		u64	data63_0[512];	    /* 0x018000 -- 0x018FF8 */
		u64	data127_64[512];    /* 0x019000 -- 0x019FF8 */
		u64	parity[512];	    /* 0x01A000 -- 0x01AFF8 */
	} ce_ure_maint_dns_dat;

	/* Downstream Header Buffer */
	struct	ce_ure_maint_dns_hdr_data {
		u64	data31_0[64];	    /* 0x01B000 -- 0x01B1F8 */
		u64	data95_32[64];	    /* 0x01B200 -- 0x01B3F8 */
		u64	parity[64];	    /* 0x01B400 -- 0x01B5F8 */
	} ce_ure_maint_dns_hdr;

	/* RCI Buffer Data */
	struct	ce_ure_maint_rci_data {
		u64	data41_0[64];	    /* 0x01B600 -- 0x01B7F8 */
		u64	data69_42[64];	    /* 0x01B800 -- 0x01B9F8 */
	} ce_ure_maint_rci;

	/* Response Queue */
	u64	ce_ure_maint_rspq[64];	    /* 0x01BA00 -- 0x01BBF8 */

	u64	ce_pad_01C000[4224];	    /* 0x01BC00 -- 0x023FF8 */

	/* Admin Build-a-Packet Buffer */
	struct	ce_adm_maint_bap_buf_data {
		u64	data63_0[258];	    /* 0x024000 -- 0x024808 */
		u64	data127_64[258];    /* 0x024810 -- 0x025018 */
		u64	parity[258];	    /* 0x025020 -- 0x025828 */
	} ce_adm_maint_bap_buf;

	u64	ce_pad_025830[5370];	    /* 0x025830 -- 0x02FFF8 */

	/* URE: 40bit PMU ATE Buffer */		    /* 0x030000 -- 0x037FF8 */
	u64	ce_ure_ate40[TIOCE_NUM_M40_ATES];

	/* URE: 32/40bit PMU ATE Buffer */	    /* 0x038000 -- 0x03BFF8 */
	u64	ce_ure_ate3240[TIOCE_NUM_M3240_ATES];

	u64	ce_pad_03C000[2050];	    /* 0x03C000 -- 0x040008 */

	/*
	 * DRE: Down Stream Request Engine
         */
	u64	ce_dre_dyn_credit_status1;		/* 0x040010 */
	u64	ce_dre_dyn_credit_status2;		/* 0x040018 */
	u64	ce_dre_last_credit_status1;		/* 0x040020 */
	u64	ce_dre_last_credit_status2;		/* 0x040028 */
	u64	ce_dre_credit_limit1;			/* 0x040030 */
	u64	ce_dre_credit_limit2;			/* 0x040038 */
	u64	ce_dre_force_credit1;			/* 0x040040 */
	u64	ce_dre_force_credit2;			/* 0x040048 */
	u64	ce_dre_debug_mux1;			/* 0x040050 */
	u64	ce_dre_debug_mux2;			/* 0x040058 */
	u64	ce_dre_ssp_err_cmd_wrd;			/* 0x040060 */
	u64	ce_dre_ssp_err_addr;			/* 0x040068 */
	u64	ce_dre_comp_err_cmd_wrd;		/* 0x040070 */
	u64	ce_dre_comp_err_addr;			/* 0x040078 */
	u64	ce_dre_req_status;			/* 0x040080 */
	u64	ce_dre_config1;				/* 0x040088 */
	u64	ce_dre_config2;				/* 0x040090 */
	u64	ce_dre_config_req_status;		/* 0x040098 */
	u64	ce_pad_0400A0[12];	    /* 0x0400A0 -- 0x0400F8 */
	u64	ce_dre_dyn_fifo;			/* 0x040100 */
	u64	ce_pad_040108[3];	    /* 0x040108 -- 0x040118 */
	u64	ce_dre_last_fifo;			/* 0x040120 */

	u64	ce_pad_040128[27];	    /* 0x040128 -- 0x0401F8 */

	/* DRE Downstream Head Queue */
	struct	ce_dre_maint_ds_head_queue {
		u64	data63_0[32];	    /* 0x040200 -- 0x0402F8 */
		u64