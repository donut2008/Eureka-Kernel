tus Register */
#define IOAT_CHANSTS_OFFSET(ver)		((ver) < IOAT_VER_2_0 \
						? IOAT1_CHANSTS_OFFSET : IOAT2_CHANSTS_OFFSET)
#define IOAT1_CHANSTS_OFFSET_LOW	0x04
#define IOAT2_CHANSTS_OFFSET_LOW	0x08
#define IOAT_CHANSTS_OFFSET_LOW(ver)		((ver) < IOAT_VER_2_0 \
						? IOAT1_CHANSTS_OFFSET_LOW : IOAT2_CHANSTS_OFFSET_LOW)
#define IOAT1_CHANSTS_OFFSET_HIGH	0x08
#define IOAT2_CHANSTS_OFFSET_HIGH	0x0C
#define IOAT_CHANSTS_OFFSET_HIGH(ver)		((ver) < IOAT_VER_2_0 \
						? IOAT1_CHANSTS_OFFSET_HIGH : IOAT2_CHANSTS_OFFSET_HIGH)
#define IOAT_CHANSTS_COMPLETED_DESCRIPTOR_ADDR	(~0x3fULL)
#define IOAT_CHANSTS_SOFT_ERR			0x10ULL
#define IOAT_CHANSTS_UNAFFILIATED_ERR		0x8ULL
#define IOAT_CHANSTS_STATUS	0x7ULL
#define IOAT_CHANSTS_ACTIVE	0x0
#define IOAT_CHANSTS_DONE	0x1
#define IOAT_CHANSTS_SUSPENDED	0x2
#define IOAT_CHANSTS_HALTED	0x3



#define IOAT_CHAN_DMACOUNT_OFFSET	0x06    /* 16-bit DMA Count register */

#define IOAT_DCACTRL_OFFSET         0x30   /* 32 bit Direct Cache Access Control Register */
#define IOAT_DCACTRL_CMPL_WRITE_ENABLE 0x10000
#define IOAT_DCACTRL_TARGET_CPU_MASK   0xFFFF /* APIC ID */

/* CB DCA Memory Space Registers */
#define IOAT_DCAOFFSET_OFFSET       0x14
/* CB_BAR + IOAT_DCAOFFSET value */
#define IOAT_DCA_VER_OFFSET         0x00
#define IOAT_DCA_VER_MAJOR_MASK     0xF0
#define IOAT_DCA_VER_MINOR_MASK     0x0F

#define IOAT_DCA_COMP_OFFSET        0x02
#define IOAT_DCA_COMP_V1            0x1

#define IOAT_FSB_CAPABILITY_OFFSET  0x04
#define IOAT_FSB_CAPABILITY_PREFETCH    0x1

#define IOAT_PCI_CAPABILITY_OFFSET  0x06
#define IOAT_PCI_CAPABILITY_MEMWR   0x1

#define IOAT_FSB_CAP_ENABLE_OFFSET  0x08
#define IOAT_FSB_CAP_ENABLE_PREFETCH    0x1

#define IOAT_PCI_CAP_ENABLE_OFFSET  0x0A
#define IOAT_PCI_CAP_ENABLE_MEMWR   0x1

#define IOAT_APICID_TAG_MAP_OFFSET  0x0C
#define IOAT_APICID_TAG_MAP_TAG0    0x0000000F
#define IOAT_APICID_TAG_MAP_TAG0_SHIFT 0
#define IOAT_APICID_TAG_MAP_TAG1    0x000000F0
#define IOAT_APICID_TAG_MAP_TAG1_SHIFT 4
#define IOAT_APICID_TAG_MAP_TAG2    0x00000F00
#define IOAT_APICID_TAG_MAP_TAG2_SHIFT 8
#define IOAT_APICID_TAG_MAP_TAG3    0x0000F000
#define IOAT_APICID_TAG_MAP_TAG3_SHIFT 12
#define IOAT_APICID_TAG_MAP_TAG4    0x000F0000
#define IOAT_APICID_TAG_MAP_TAG4_SHIFT 16
#define IOAT_APICID_TAG_CB2_VALID   0x8080808080

#define IOAT_DCA_GREQID_OFFSET      0x10
#define IOAT_DCA_GREQID_SIZE        0x04
#define IOAT_DCA_GREQID_MASK        0xFFFF
#define IOAT_DCA_GREQID_IGNOREFUN   0x10000000
#define IOAT_DCA_GREQID_VALID       0x20000000
#define IOAT_DCA_GREQID_LASTID      0x80000000

#define IOAT3_CSI_CAPABILITY_OFFSET 0x08
#define IOAT3_CSI_CAPABILITY_PREFETCH    0x1

#define IOAT3_PCI_CAPABILITY_OFFSET 0x0A
#define IOAT3_PCI_CAPABILITY_MEMWR  0x1

#define IOAT3_CSI_CONTROL_OFFSET    0x0C
#define IOAT3_CSI_CONTROL_PREFETCH  0x1

#define IOAT3_PCI_CONTROL_OFFSET    0x0E
#define IOAT3_PCI_CONTROL_MEMWR     0x1

#define IOAT3_APICID_TAG_MAP_OFFSET 0x10
#define IOAT3_APICID_TAG_MAP_OFFSET_LOW  0x10
#define IOAT3_APICID_TAG_MAP_OFFSET_HIGH 0x14

#define IOAT3_DCA_GREQID_OFFSET     0x02

#define IOAT1_CHAINADDR_OFFSET		0x0C	/* 64-bit Descriptor Chain Address Register */
#define IOAT2_CHAINADDR_OFFSET		0x10	/* 64-bit Descriptor Chain Address Register */
#define IOAT_CHAINADDR_OFFSET(ver)		((ver) < IOAT_VER_2_0 \
						? IOAT1_CHAINADDR_OFFSET : IOAT2_CHAINADDR_OFFSET)
#define IOAT1_CHAINADDR_OFFSET_LOW	0x0C
#define IOAT2_CHAINADDR_OFFSET_LOW	0x10
#define IOAT_CHAINADDR_OFFSET_LOW(ver)		((ver) < IOAT_VER_2_0 \
						? IOAT1_CHAINADDR_OFFSET_LOW : IOAT2_CHAINADDR_OFFSET_LOW)
#define IOAT1_CHAINADDR_OFFSET_HIGH	0x10
#define IOAT2_CHAINADDR_OFFSET_HIGH	0x14
#define IOAT_CHAINADDR_OFFSET_HIGH(ver)		((ver) < IOAT_VER_2_0 \
						? IOAT1_CHAINADDR_OFFSET_HIGH : IOAT2_CHAINADDR_OFFSET_HIGH)

#define IOAT1_CHANCMD_OFFSET		0x14	/*  8-bit DMA Channel Command Register */
#define IOAT2_CHANCMD_OFFSET		0x04	/*  8-bit DMA Channel Command Register */
#define IOAT_CHANCMD_OFFSET(ver)		((ver) < IOAT_VER_2_0 \
						? IOAT1_CHANCMD_OFFSET : IOAT2_CHANCMD_OFFSET)
#define IOAT_CHANCMD_RESET			0x20
#define IOAT_CHANCMD_RESUME			0x10
#define IOAT_CHANCMD_ABORT			0x08
#define IOAT_CHANCMD_SUSPEND			0x04
#define IOAT_CHANCMD_APPEND			0x02
#define IOAT_CHANCMD_START			0x01

#define IOAT_CHANCMP_OFFSET			0x18	/* 64-bit Channel Completion Address Register */
#define IOAT_CHANCMP_OFFSET_LOW			0x18
#define IOAT_CHANCMP_OFFSET_HIGH		0x1C

#define IOAT_CDAR_OFFSET			0x20	/* 64-bit Current Descriptor Add