DRAM_2__WR_DQS__SHIFT 0x9
#define MC_SEQ_DRAM_2__PLL_EST_MASK 0x400
#define MC_SEQ_DRAM_2__PLL_EST__SHIFT 0xa
#define MC_SEQ_DRAM_2__PLL_CLR_MASK 0x800
#define MC_SEQ_DRAM_2__PLL_CLR__SHIFT 0xb
#define MC_SEQ_DRAM_2__DLL_EST_MASK 0x1000
#define MC_SEQ_DRAM_2__DLL_EST__SHIFT 0xc
#define MC_SEQ_DRAM_2__BNK_MRS_MASK 0x2000
#define MC_SEQ_DRAM_2__BNK_MRS__SHIFT 0xd
#define MC_SEQ_DRAM_2__DBI_OVR_MASK 0x4000
#define MC_SEQ_DRAM_2__DBI_OVR__SHIFT 0xe
#define MC_SEQ_DRAM_2__TRI_CLK_MASK 0x8000
#define MC_SEQ_DRAM_2__TRI_CLK__SHIFT 0xf
#define MC_SEQ_DRAM_2__PLL_CNT_MASK 0xff0000
#define MC_SEQ_DRAM_2__PLL_CNT__SHIFT 0x10
#define MC_SEQ_DRAM_2__PCH_BNK_MASK 0x1000000
#define MC_SEQ_DRAM_2__PCH_BNK__SHIFT 0x18
#define MC_SEQ_DRAM_2__ADBI_DF1_MASK 0x2000000
#define MC_SEQ_DRAM_2__ADBI_DF1__SHIFT 0x19
#define MC_SEQ_DRAM_2__ADBI_ACT_MASK 0x4000000
#define MC_SEQ_DRAM_2__ADBI_ACT__SHIFT 0x1a
#define MC_SEQ_DRAM_2__DBI_DF1_MASK 0x8000000
#define MC_SEQ_DRAM_2__DBI_DF1__SHIFT 0x1b
#define MC_SEQ_DRAM_2__DBI_ACT_MASK 0x10000000
#define MC_SEQ_DRAM_2__DBI_ACT__SHIFT 0x1c
#define MC_SEQ_DRAM_2__DBI_EDC_DF1_MASK 0x20000000
#define MC_SEQ_DRAM_2__DBI_EDC_DF1__SHIFT 0x1d
#define MC_SEQ_DRAM_2__TESTCHIP_EN_MASK 0x40000000
#define MC_SEQ_DRAM_2__TESTCHIP_EN__SHIFT 0x1e
#define MC_SEQ_DRAM_2__CS_BY16_MASK 0x80000000
#define MC_SEQ_DRAM_2__CS_BY16__SHIFT 0x1f
#define MC_SEQ_RAS_TIMING__TRCDW_MASK 0x1f
#define MC_SEQ_RAS_TIMING__TRCDW__SHIFT 0x0
#define MC_SEQ_RAS_TIMING__TRCDWA_MASK 0x3e0
#define MC_SEQ_RAS_TIMING__TRCDWA__SHIFT 0x5
#define MC_SEQ_RAS_TIMING__TRCDR_MASK 0x7c00
#define MC_SEQ_RAS_TIMING__TRCDR__SHIFT 0xa
#define MC_SEQ_RAS_TIMING__TRCDRA_MASK 0xf8000
#define MC_SEQ_RAS_TIMING__TRCDRA__SHIFT 0xf
#define MC_SEQ_RAS_TIMING__TRRD_MASK 0xf00000
#define MC_SEQ_RAS_TIMING__TRRD__SHIFT 0x14
#define MC_SEQ_RAS_TIMING__TRC_MASK 0x7f000000
#define MC_SEQ_RAS_TIMING__TRC__SHIFT 0x18
#define MC_SEQ_CAS_TIMING__TNOPW_MASK 0x3
#define MC_SEQ_CAS_TIMING__TNOPW__SHIFT 0x0
#define MC_SEQ_CAS_TIMING__TNOPR_MASK 0xc
#define MC_SEQ_CAS_TIMING__TNOPR__SHIFT 0x2
#define MC_SEQ_CAS_TIMING__TR2W_MASK 0x1f0
#define MC_SEQ_CAS_TIMING__TR2W__SHIFT 0x4
#define MC_SEQ_CAS_TIMING__TCCDL_MASK 0xe00
#define MC_SEQ_CAS_TIMING__TCCDL__SHIFT 0x9
#define MC_SEQ_CAS_TIMING__TR2R_MASK 0xf000
#define MC_SEQ_CAS_TIMING__TR2R__SHIFT 0xc
#define MC_SEQ_CAS_TIMING__TW2R_MASK 0x1f0000
#define MC_SEQ_CAS_TIMING__TW2R__SHIFT 0x10
#define MC_SEQ_CAS_TIMING__TCL_MASK 0x1f000000
#define MC_SEQ_CAS_TIMING__TCL__SHIFT 0x18
#define MC_SEQ_MISC_TIMING__TRP_WRA_MASK 0x3f
#define MC_SEQ_MISC_TIMING__TRP_WRA__SHIFT 0x0
#define MC_SEQ_MISC_TIMING__TRP_RDA_MASK 0x3f00
#define MC_SEQ_MISC_TIMING__TRP_RDA__SHIFT 0x8
#define MC_SEQ_MISC_TIMING__TRP_MASK 0xf8000
#define MC_SEQ_MISC_TIMING__TRP__SHIFT 0xf
#define MC_SEQ_MISC_TIMING__TRFC_MASK 0x1ff00000
#define MC_SEQ_MISC_TIMING__TRFC__SHIFT 0x14
#define MC_SEQ_MISC_TIMING2__PA2RDATA_MASK 0x7
#define MC_SEQ_MISC_TIMING2__PA2RDATA__SHIFT 0x0
#define MC_SEQ_MISC_TIMING2__PA2WDATA_MASK 0x70
#define MC_SEQ_MISC_TIMING2__PA2WDATA__SHIFT 0x4
#define MC_SEQ_MISC_TIMING2__FAW_MASK 0x1f00
#define MC_SEQ_MISC_TIMING2__FAW__SHIFT 0x8
#define MC_SEQ_MISC_TIMING2__TREDC_MASK 0xe000
#define MC_SEQ_MISC_TIMING2__TREDC__SHIFT 0xd
#define MC_SEQ_MISC_TIMING2__TWEDC_MASK 0x1f0000
#define MC_SEQ_MISC_TIMING2__TWEDC__SHIFT 0x10
#define MC_SEQ_MISC_TIMING2__T32AW_MASK 0x1e00000
#define MC_SEQ_MISC_TIMING2__T32AW__SHIFT 0x15
#define MC_SEQ_MISC_TIMING2__TWDATATR_MASK 0xf0000000
#define MC_SEQ_MISC_TIMING2__TWDATATR__SHIFT 0x1c
#define MC_SEQ_PMG_TIMING__TCKSRE_MASK 0x7
#define MC_SEQ_PMG_TIMING__TCKSRE__SHIFT 0x0
#define MC_SEQ_PMG_TIMING__TCKSRX_MASK 0x70
#define MC_SEQ_PMG_TIMING__TCKSRX__SHIFT 0x4
#define MC_SEQ_PMG_TIMING__TCKE_PULSE_MASK 0xf00
#define MC_SEQ_PMG_TIMING__TCKE_PULSE__SHIFT 0x8
#define MC_SEQ_PMG_TIMING__TCKE_MASK 0x3f000
#define MC_SEQ_PMG_TIMING__TCKE__SHIFT 0xc
#define MC_SEQ_PMG_TIMING__SEQ_IDLE_MASK 0x1c0000
#define MC_SEQ_PMG_TIMING__SEQ_IDLE__SHIFT 0x12
#define MC_SEQ_PMG_TIMING__TCKE_PULSE_MSB_MASK 0x800000
#define MC_SEQ_PMG_TIMING__TCKE_PULSE_MSB__SHIFT 0x17
#define MC_SEQ_PMG_TIMING__SEQ_IDLE_SS_MASK 0xff000000
#define MC_SEQ_PMG_TIMING__SEQ_IDLE_SS__SHIFT 0x18
#define MC_SEQ_RD_CTL_D0__RCV_DLY_MASK 0x7
#define MC_SEQ_RD_CTL_D0__RCV_DLY__SHIFT 0x0
#define MC_SEQ_RD_CTL_D0__RCV_EXT_MASK 0xf8
#define MC_SEQ_RD_CTL_D0__RCV_EXT__SHIFT 0x3
#define MC_SEQ_RD_CTL_D0__RST_SEL_MASK 0x300
#define MC_SEQ_RD_CTL_D0__RST_SEL__SHIFT 0x8
#define MC_SEQ_RD_CTL_D0__RXDPWRON_DLY_MASK 0xc00
#define MC_SEQ_RD_CTL_D0__RXDPWRON_DLY__SHIFT 0xa
#define MC_SEQ_RD_CTL_D0__RST_HLD_MASK 0xf000
#define MC_SEQ_RD_CTL_D0__RST_HLD__SHIFT 0xc
#define MC_SEQ_RD_CTL_D0__STR_PRE_MASK 0x10000
#define MC_SEQ_RD_CTL_D0__STR_PRE__SHIFT 0x10
#define MC_SEQ_RD_CTL_D0__STR_PST_MASK 0x20000
#define MC_SEQ_RD_CTL_D0__STR_PST__SHIFT 0x11
#define MC_SEQ_RD_CTL_D0__RBS_DLY_MASK 0x1f00000
#define MC_SEQ_RD_CTL_D0__RBS_DLY__SHIFT 0x14
#define MC_SEQ_RD_CTL_D0__RBS_WEDC_DLY_MASK 0x3e000000
#define MC_SEQ_RD_CTL_D0__RBS_WEDC_DLY__SHIFT 0x19
#define MC_SEQ_RD_CTL_D1__RCV_DLY_MASK 0x7
#define MC_SEQ_RD_CTL_D1__RCV_DLY__SHIFT 0x0
#define MC_SEQ_RD_CTL_D1__RCV_EXT_MASK 0xf8
#define MC_SEQ_RD_CTL_D1__RCV_EXT__SHIFT 0x3
#define MC_SEQ_RD_CTL_D1__RST_SEL_MASK 0x300
#define MC_SEQ_RD_CTL_D1__RST_SEL__SHIFT 0x8
#define MC_SEQ_RD_CTL_D1__RXDPWRON_DLY_MASK 0xc00
#define MC_SEQ_RD_CTL_D1__RXDPWRON_DLY__SHIFT 0xa
#define MC_SEQ_RD_CTL_D1__RST_HLD_MASK 0xf000
#define MC_SEQ_RD_CTL_D1__RST_HLD__SHIFT 0xc
#define MC_SEQ_RD_CTL_D1__STR_PRE_MASK 0x10000
#define MC_SEQ_RD_CTL_D1__STR_PRE__SHIFT 0x10
#define MC_SEQ_RD_CTL_D1__STR_PST_MASK 0x20000
#define MC_SEQ_RD_CTL_D1__STR_PST__SHIFT 0x11
#define MC_SEQ_RD_CTL_D1__RBS_DLY_MASK 0x1f00000
#define MC_SEQ_RD_CTL_D1__RBS_DLY__SHIFT 0x14
#define MC_SEQ_RD_CTL_D1__RBS_WEDC_DLY_MASK 0x3e000000
#define MC_SEQ_RD_CTL_D1__RBS_WEDC_DLY__SHIFT 0x19
#define MC_SEQ_WR_CTL_D0__DAT_DLY_MASK 0xf
#define MC_SEQ_WR_CTL_D0__DAT_DLY__SHIFT 0x0
#define MC_SEQ_WR_CTL_D0__DQS_DLY_MASK 0xf0
#define MC_SEQ_WR_CTL_D0__DQS_DLY__SHIFT 0x4
#define MC_SEQ_WR_CTL_D0__DQS_XTR_MASK 0x100
#define MC_SEQ_WR_CTL_D0__DQS_XTR__SHIFT 0x8
#define MC_SEQ_WR_CTL_D0__DAT_2Y_DLY_MASK 0x200
#define MC_SEQ_WR_CTL_D0__DAT_2Y_DLY__SHIFT 0x9
#define MC_SEQ_WR_CTL_D0__ADR_2Y_DLY_MASK 0x400
#define MC_SEQ_WR_CTL_D0__ADR_2Y_DLY__SHIFT 0xa
#define MC_SEQ_WR_CTL_D0__CMD_2Y_DLY_MASK 0x800
#define MC_SEQ_WR_CTL_D0__CMD_2Y_DLY__SHIFT 0xb
#define MC_SEQ_WR_CTL_D0__OEN_DLY_MASK 0xf000
#define MC_SEQ_WR_CTL_D0__OEN_DLY__SHIFT 0xc
#define MC_SEQ_WR_CTL_D0__OEN_EXT_MASK 0xf0000
#define MC_SEQ_WR_CTL_D0__OEN_EXT__SHIFT 0x10
#define MC_SEQ_WR_CTL_D0__OEN_SEL_MASK 0x300000
#define MC_SEQ_WR_CTL_D0__OEN_SEL__SHIFT 0x14
#define MC_SEQ_WR_CTL_D0__ODT_DLY_MASK 0xf000000
#define MC_SEQ_WR_CTL_D0__ODT_DLY__SHIFT 0x18
#define MC_SEQ_WR_CTL_D0__ODT_EXT_MASK 0x10000000
#define MC_SEQ_WR_CTL_D0__ODT_EXT__SHIFT 0x1c
#define MC_SEQ_WR_CTL_D0__ADR_DLY_MASK 0x20000000
#define MC_SEQ_WR_CTL_D0__ADR_DLY__SHIFT 0x1d
#define MC_SEQ_WR_CTL_D0__CMD_DLY_MASK 0x40000000
#define MC_SEQ_WR_CTL_D0__CMD_DLY__SHIFT 0x1e
#define MC_SEQ_WR_CTL_D1__DAT_DLY_MASK 0xf
#define MC_SEQ_WR_CTL_D1__DAT_DLY__SHIFT 0x0
#define MC_SEQ_WR_CTL_D1__DQS_DLY_MASK 0xf0
#define MC_SEQ_WR_CTL_D1__DQS_DLY__SHIFT 0x4
#define MC_SEQ_WR_CTL_D1__DQS_XTR_MASK 0x100
#define MC_SEQ_WR_CTL_D1__DQS_XTR__SHIFT 0x8
#define MC_SEQ_WR_CTL_D1__DAT_2Y_DLY_MASK 0x200
#define MC_SEQ_WR_CTL_D1__DAT_2Y_DLY__SHIFT 0x9
#define MC_SEQ_WR_CTL_D1__ADR_2Y_DLY_MASK 0x400
#define MC_SEQ_WR_CTL_D1__ADR_2Y_DLY__SHIFT 0xa
#define MC_SEQ_WR_CTL_D1__CMD_2Y_DLY_MASK 0x800
#define MC_SEQ_WR_CTL_D1__CMD_2Y_DLY__SHIFT 0xb
#define MC_SEQ_WR_CTL_D1__OEN_DLY_MASK 0xf000
#define MC_SEQ_WR_CTL_D1__OEN_DLY__SHIFT 0xc
#define MC_SEQ_WR_CTL_D1__OEN_EXT_MASK 0xf0000
#define MC_SEQ_WR_CTL_D1__OEN_EXT__SHIFT 0x10
#define MC_SEQ_WR_CTL_D1__OEN_SEL_MASK 0x300000
#define MC_SEQ_WR_CTL_D1__OEN_SEL__SHIFT 0x14
#define MC_SEQ_WR_CTL_D1__ODT_DLY_MASK 0xf000000
#define MC_SEQ_WR_CTL_D1__ODT_DLY__SHIFT 0x18
#define MC_SEQ_WR_CTL_D1__ODT_EXT_MASK 0x10000000
#define MC_SEQ_WR_CTL_D1__ODT_EXT__SHIFT 0x1c
#define MC_SEQ_WR_CTL_D1__ADR_DLY_MASK 0x20000000
#define MC_SEQ_WR_CTL_D1__ADR_DLY__SHIFT 0x1d
#define MC_SEQ_WR_CTL_D1__CMD_DLY_MASK 0x40000000
#define MC_SEQ_WR_CTL_D1__CMD_DLY__SHIFT 0x1e
#define MC_SEQ_WR_CTL_2__DAT_DLY_H_D0_MASK 0x1
#define MC_SEQ_WR_CTL_2__DAT_DLY_H_D0__SHIFT 0x0
#define MC_SEQ_WR_CTL_2__DQS_DLY_H_D0_MASK 0x2
#define MC_SEQ_WR_CTL_2__DQS_DLY_H_D0__SHIFT 0x1
#define MC_SEQ_WR_CTL_2__OEN_DLY_H_D0_MASK 0x4
#define MC_SEQ_WR_CTL_2__OEN_DLY_H_D0__SHIFT 0x2
#define MC_SEQ_WR_CTL_2__DAT_DLY_H_D1_MASK 0x8
#define MC_SEQ_WR_CTL_2__DAT_DLY_H_D1__SHIFT 0x3
#define MC_SEQ_WR_CTL_2__DQS_DLY_H_D1_MASK 0x10
#define MC_SEQ_WR_CTL_2__DQS_DLY_H_D1__SHIFT 0x4
#define MC_SEQ_WR_CTL_2__OEN_DLY_H_D1_MASK 0x20
#define MC_SEQ_WR_CTL_2__OEN_DLY_H_D1__SHIFT 0x5
#define MC_SEQ_WR_CTL_2__WCDR_EN_MASK 0x40
#define MC_SEQ_WR_CTL_2__WCDR_EN__SHIFT 0x6
#define MC_SEQ_CMD__ADR_MASK 0xffff
#define MC_SEQ_CMD__ADR__SHIFT 0x0
#define MC_SEQ_CMD__MOP_MASK 0xf0000
#define MC_SEQ_CMD__MOP__SHIFT 0x10
#define MC_SEQ_CMD__END_MASK 0x100000
#define MC_SEQ_CMD__END__SHIFT 0x14
#define MC_SEQ_CMD__CSB_MASK 0x600000
#define MC_SEQ_CMD__CSB__SHIFT 0x15
#define MC_SEQ_CMD__CHAN0_MASK 0x1000000
#define MC_SEQ_CMD__CHAN0__SHIFT 0x18
#define MC_SEQ_CMD__CHAN1_MASK 0x2000000
#define MC_SEQ_CMD__CHAN1__SHIFT 0x19
#define MC_SEQ_CMD__ADR_MSB1_MASK 0x10000000
#define MC_SEQ_CMD__ADR_MSB1__SHIFT 0x1c
#define MC_SEQ_CMD__ADR_MSB0_MASK 0x20000000
#define MC_SEQ_CMD__ADR_MSB0__SHIFT 0x1d
#define MC_PMG_CMD_EMRS__ADR_MASK 0xffff
#define MC_PMG_CMD_EMRS__ADR__SHIFT 0x0
#define MC_PMG_CMD_EMRS__MOP_MASK 0x70000
#define MC_PMG_CMD_EMRS__MOP__SHIFT 0x10
#define MC_PMG_CMD_EMRS__BNK_MSB_MASK 0x80000
#define MC_PMG_CMD_EMRS__BNK_MSB__SHIFT 0x13
#define MC_PMG_CMD_EMRS__END_MASK 0x100000
#define MC_PMG_CMD_EMRS__END__SHIFT 0x14
#define MC_PMG_CMD_EMRS__CSB_MASK 0x600000
#define MC_PMG_CMD_EMRS__CSB__SHIFT 0x15
#define MC_PMG_CMD_EMRS__ADR_MSB1_MASK 0x10000000
#define MC_PMG_CMD_EMRS__ADR_MSB1__SHIFT 0x1c
#define MC_PMG_CMD_EMRS__ADR_MSB0_MASK 0x20000000
#define MC_PMG_CMD_EMRS__ADR_MSB0__SHIFT 0x1d
#define MC_PMG_CMD_MRS__ADR_MASK 0xffff
#define MC_PMG_CMD_MRS__ADR__SHIFT 0x0
#define MC_PMG_CMD_MRS__MOP_MASK 0x70000
#define MC_PMG_CMD_MRS__MOP__SHIFT 0x10
#define MC_PMG_CMD_MRS__BNK_MSB_MASK 0x80000
#define MC_PMG_CMD_MRS__BNK_MSB__SHIFT 0x13
#define MC_PMG_CMD_MRS__END_MASK 0x100000
#define MC_PMG_CMD_MRS__END__SHIFT 0x14
#define MC_PMG_CMD_MRS__CSB_MASK 0x600000
#define MC_PMG_CMD_MRS__CSB__SHIFT 0x15
#define MC_PMG_CMD_MRS__ADR_MSB1_MASK 0x10000000
#define MC_PMG_CMD_MRS__ADR_MSB1__SHIFT 0x1c
#define MC_PMG_CMD_MRS__ADR_MSB0_MASK 0x20000000
#define MC_PMG_CMD_MRS__ADR_MSB0__SHIFT 0x1d
#define MC_PMG_CMD_MRS1__ADR_MASK 0xffff
#define MC_PMG_CMD_MRS1__ADR__SHIFT 0x0
#define MC_PMG_CMD_MRS1__MOP_MASK 0x70000
#define MC_PMG_CMD_MRS1__MOP__SHIFT 0x10
#define MC_PMG_CMD_MRS1__BNK_MSB_MASK 0x80000
#define MC_PMG_CMD_MRS1__BNK_MSB__SHIFT 0x13
#define MC_PMG_CMD_MRS1__END_MASK 0x100000
#define MC_PMG_CMD_MRS1__END__SHIFT 0x14
#define MC_PMG_CMD_MRS1__CSB_MASK 0x600000
#define MC_PMG_CMD_MRS1__CSB__SHIFT 0x15
#define MC_PMG_CMD_MRS1__ADR_MSB1_MASK 0x10000000
#define MC_PMG_CMD_MRS1__ADR_MSB1__SHIFT 0x1c
#define MC_PMG_CMD_MRS1__ADR_MSB0_MASK 0x20000000
#define MC_PMG_CMD_MRS1__ADR_MSB0__SHIFT 0x1d
#define MC_PMG_CMD_MRS2__ADR_MASK 0xffff
#define MC_PMG_CMD_MRS2__ADR__SHIFT 0x0
#define MC_PMG_CMD_MRS2__MOP_MASK 0x70000
#define MC_PMG_CMD_MRS2__MOP__SHIFT 0x10
#define MC_PMG_CMD_MRS2__BNK_MSB_MASK 0x80000
#define MC_PMG_CMD_MRS2__BNK_MSB__SHIFT 0x13
#define MC_PMG_CMD_MRS2__END_MASK 0x100000
#define MC_PMG_CMD_MRS2__END__SHIFT 0x14
#define MC_PMG_CMD_MRS2__CSB_MASK 0x600000
#define MC_PMG_CMD_MRS2__CSB__SHIFT 0x15
#define MC_PMG_CMD_MRS2__ADR_MSB1_MASK 0x10000000
#define MC_PMG_CMD_MRS2__ADR_MSB1__SHIFT 0x1c
#define MC_PMG_CMD_MRS2__ADR_MSB0_MASK 0x20000000
#define MC_PMG_CMD_MRS2__ADR_MSB0__SHIFT 0x1d
#define MC_PMG_CFG__SYC_CLK_MASK 0x1
#define MC_PMG_CFG__SYC_CLK__SHIFT 0x0
#define MC_PMG_CFG__RST_MRS_MASK 0x2
#define MC_PMG_CFG__RST_MRS__SHIFT 0x1
#define MC_PMG_CFG__RST_EMRS_MASK 0x4
#define MC_PMG_CFG__RST_EMRS__SHIFT 0x2
#define MC_PMG_CFG__TRI_MIO_MASK 0x8
#define MC_PMG_CFG__TRI_MIO__SHIFT 0x3
#define MC_PMG_CFG__XSR_TMR_MASK 0xf0
#define MC_PMG_CFG__XSR_TMR__SHIFT 0x4
#define MC_PMG_CFG__RST_MRS1_MASK 0x100
#define MC_PMG_CFG__RST_MRS1__SHIFT 0x8
#define MC_PMG_CFG__RST_MRS2_MASK 0x200
#define MC_PMG_CFG__RST_MRS2__SHIFT 0x9
#define MC_PMG_CFG__DPM_WAKE_MASK 0x400
#define MC_PMG_CFG__DPM_WAKE__SHIFT 0xa
#define MC_PMG_CFG__RFS_SRX_MASK 0x1000
#define MC_PMG_CFG__RFS_SRX__SHIFT 0xc
#define MC_PMG_CFG__PREA_SRX_MASK 0x2000
#define MC_PMG_CFG__PREA_SRX__SHIFT 0xd
#define MC_PMG_CFG__MRS_WAIT_CNT_MASK 0xf0000
#define MC_PMG_CFG__MRS_WAIT_CNT__SHIFT 0x10
#define MC_PMG_CFG__WRITE_DURING_DLOCK_MASK 0x100000
#define MC_PMG_CFG__WRITE_DURING_DLOCK__SHIFT 0x14
#define MC_PMG_CFG__YCLK_ON_MASK 0x200000
#define MC_PMG_CFG__YCLK_ON__SHIFT 0x15
#define MC_PMG_CFG__EARLY_ACK_ACPI_MASK 0x400000
#define MC_PMG_CFG__EARLY_ACK_ACPI__SHIFT 0x16
#define MC_PMG_CFG__RXPDNB_MASK 0x2000000
#define MC_PMG_CFG__RXPDNB__SHIFT 0x19
#define MC_PMG_CFG__ZQCL_SEND_MASK 0xc000000
#define MC_PMG_CFG__ZQCL_SEND__SHIFT 0x1a
#define MC_PMG_AUTO_CMD__ADR_MASK 0x1ffff
#define MC_PMG_AUTO_CMD__ADR__SHIFT 0x0
#define MC_PMG_AUTO_CMD__ADR_MSB1_MASK 0x10000000
#define MC_PMG_AUTO_CMD__ADR_MSB1__SHIFT 0x1c
#define MC_PMG_AUTO_CMD__ADR_MSB0_MASK 0x20000000
#define MC_PMG_AUTO_CMD__ADR_MSB0__SHIFT 0x1d
#define MC_PMG_AUTO_CFG__SYC_CLK_MASK 0x1
#define MC_PMG_AUTO_CFG__SYC_CLK__SHIFT 0x0
#define MC_PMG_AUTO_CFG__RST_MRS_MASK 0x2
#define MC_PMG_AUTO_CFG__RST_MRS__SHIFT 0x1
#define MC_PMG_AUTO_CFG__TRI_MIO_MASK 0x4
#define MC_PMG_AUTO_CFG__TRI_MIO__SHIFT 0x2
#define MC_PMG_AUTO_CFG__XSR_TMR_MASK 0xf0
#define MC_PMG_AUTO_CFG__XSR_TMR__SHIFT 0x4
#define MC_PMG_AUTO_CFG__SS_ALWAYS_SLF_MASK 0x100
#define MC_PMG_AUTO_CFG__SS_ALWAYS_SLF__SHIFT 0x8
#define MC_PMG_AUTO_CFG__SS_S_SLF_MASK 0x200
#define MC_PMG_AUTO_CFG__SS_S_SLF__SHIFT 0x9
#define MC_PMG_AUTO_CFG__SCDS_MODE_MASK 0x400
#define MC_PMG_AUTO_CFG__SCDS_MODE__SHIFT 0xa
#define MC_PMG_AUTO_CFG__EXIT_ALLOW_STOP_MASK 0x800
#define MC_PMG_AUTO_CFG__EXIT_ALLOW_STOP__SHIFT 0xb
#define MC_PMG_AUTO_CFG__RFS_SRX_MASK 0x1000
#define MC_PMG_AUTO_CFG__RFS_SRX__SHIFT 0xc
#define MC_PMG_AUTO_CFG__PREA_SRX_MASK 0x2000
#define MC_PMG_AUTO_CFG__PREA_SRX__SHIFT 0xd
#define MC_PMG_AUTO_CFG__STUTTER_EN_MASK 0x4000
#define MC_PMG_AUTO_CFG__STUTTER_EN__SHIFT 0xe
#define MC_PMG_AUTO_CFG__SELFREFR_COMMIT_0_MASK 0x8000
#define MC_PMG_AUTO_CFG__SELFREFR_COMMIT_0__SHIFT 0xf
#define MC_PMG_AUTO_CFG__MRS_WAIT_CNT_MASK 0xf0000
#define MC_PMG_AUTO_CFG__MRS_WAIT_CNT__SHIFT 0x10
#define MC_PMG_AUTO_CFG__WRITE_DURING_DLOCK_MASK 0x100000
#define MC_PMG_AUTO_CFG__WRITE_DURING_DLOCK__SHIFT 0x14
#define MC_PMG_AUTO_CFG__YCLK_ON_MASK 0x200000
#define MC_PMG_AUTO_CFG__YCLK_ON__SHIFT 0x15
#define MC_PMG_AUTO_CFG__RXPDNB_MASK 0x400000
#define MC_PMG_AUTO_CFG__RXPDNB__SHIFT 0x16
#define MC_PMG_AUTO_CFG__SELFREFR_COMMIT_1_MASK 0x800000
#define MC_PMG_AUTO_CFG__SELFREFR_COMMIT_1__SHIFT 0x17
#define MC_PMG_AUTO_CFG__DLL_CNT_MASK 0xff000000
#define MC_PMG_AUTO_CFG__DLL_CNT__SHIFT 0x18
#define MC_IMP_CNTL__MEM_IO_UPDATE_RATE_MASK 0x1f
#define MC_IMP_CNTL__MEM_IO_UPDATE_RATE__SHIFT 0x0
#define MC_IMP_CNTL__CAL_VREF_SEL_MASK 0x20
#define MC_IMP_CNTL__CAL_VREF_SEL__SHIFT 0x5
#define MC_IMP_CNTL__CAL_VREFMODE_MASK 0x40
#define MC_IMP_CNTL__CAL_VREFMODE__SHIFT 0x6
#define MC_IMP_CNTL__TIMEOUT_ERR_MASK 0x100
