S with SAL */
enum {
	SAL_VECTOR_OS_MCA	  = 0,
	SAL_VECTOR_OS_INIT	  = 1,
	SAL_VECTOR_OS_BOOT_RENDEZ = 2
};

/* Encodings for mca_opt parameter sent to SAL_MC_SET_PARAMS */
#define	SAL_MC_PARAM_RZ_ALWAYS		0x1
#define	SAL_MC_PARAM_BINIT_ESCALATE	0x10

/*
 * Definition of the SAL Error Log from the SAL spec
 */

/* SAL Error Record Section GUID Definitions */
#define SAL_PROC_DEV_ERR_SECT_GUID  \
    EFI_GUID(0xe429faf1, 0x3cb7, 0x11d4, 0xbc, 0xa7, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81)
#define SAL_PLAT_MEM_DEV_ERR_SECT_GUID  \
    EFI_GUID(0xe429faf2, 0x3cb7, 0x11d4, 0xbc, 0xa7, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81)
#define SAL_PLAT_SEL_DEV_ERR_SECT_GUID  \
    EFI_GUID(0xe429faf3, 0x3cb7, 0x11d4, 0xbc, 0xa7, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81)
#define SAL_PLAT_PCI_BUS_ERR_SECT_GUID  \
    EFI_GUID(0xe429faf4, 0x3cb7, 0x11d4, 0xbc, 0xa7, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81)
#define SAL_PLAT_SMBIOS_DEV_ERR_SECT_GUID  \
    EFI_GUI