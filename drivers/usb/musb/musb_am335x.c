CTRL_4
ucTravisLVDSVolAdjust             When ucLVDSMisc[5]=1,it means platform SBIOS want to overwrite TravisLVDSVoltage. Then VBIOS will use ucTravisLVDSVolAdjust
                                  value to program Travis register LVDS_CTRL_4
ucLVDSPwrOnSeqDIGONtoDE_in4Ms:
                                  LVDS power up sequence time in unit of 4ms, time delay from DIGON signal active to data enable signal active( DE ).
                                  =0 mean use VBIOS default which is 8 ( 32ms ). The LVDS power up sequence is as following: DIGON->DE->VARY_BL->BLON.
                                  This parameter is used by VBIOS only. VBIOS will patch LVDS_InfoTable.
ucLVDSPwrOnDEtoVARY_BL_in4Ms:
                                  LVDS power up sequence time in unit of 4ms., time delay from DE( data enable ) active to Vary Brightness enable signal active( VARY_BL ).
                                  =0 mean use VBIOS defau