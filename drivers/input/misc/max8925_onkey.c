om_offset  + 6);
		nes_debug(NES_DBG_HW, "Software section version number = 0x%04X\n",
				sw_section_ver);

		eeprom_data = nes_read16_eeprom(nesdev->regs, eeprom_offset + 2);
		nes_debug(NES_DBG_HW, "EEPROM Offset %u (next section)  = 0x%04X\n",
				eeprom_offset + 2, eeprom_data);
		next_section_address = eeprom_offset + (((eeprom_data & 0x00ff) << 3) <<
				((eeprom_data & 0x0100) >> 8));
		eeprom_data = nes_read16_eeprom(nesdev->regs, next_section_address + 4);
		if (eeprom_data != 0x414d) {
			nes_debug(NES_DBG_HW, "EEPROM Changed offset should be 0x414d but was 0x%04X\n",
					eeprom_data);
			goto no_fw_rev;
		}
		eeprom_offset = next_section_address;

		eeprom_data = nes_read16_eeprom(nesdev->regs, eeprom_offset + 2);
		nes_debug(NES_DBG_HW, "EEPROM Offset %u (next section)  = 0x%04X\n",
				eeprom_offset + 2, eeprom_data);
		next_section_address = eeprom_offset + (((eeprom_data & 0x00ff) << 3) <<
				((eeprom_data & 0x0100) >> 8));
		eeprom_data = nes_read16_eeprom(nesdev->regs, next_section_address + 4);
		if (eeprom_data != 0x4f52) {
			nes_debug(NES_DBG_HW, "EEPROM Changed offset should be 0x4f52 but was 0x%04X\n",
					eeprom_data);
			goto no_fw_rev;
		}
		eeprom_offset = next_section_address;

		eeprom_data = nes_read16_eeprom(nesdev->regs, eeprom_offset + 2);
		nes_debug(NES_DBG_HW, "EEPROM Offset %u (next section)  = 0x%04X\n",
				eeprom_offset + 2, eeprom_data);
		next_section_address = eeprom_offset + ((eeprom_data & 0x00ff) << 3);
		eeprom_data = nes_read16_eeprom(nesdev->regs, next_section_address + 4);
		if (eeprom_data != 0x5746) {
			nes_debug(NES_DBG_HW, "EEPROM Changed offset should be 0x5746 but was 0x%04X\n",
					eeprom_data);
			goto no_fw_rev;
		}
		eeprom_offset = next_section_address;

		eeprom_data = nes_read16_eeprom(nesdev->regs, eeprom_offset + 2);
		nes_debug(NES_DBG_HW, "EEPROM Offset %u (next section)  = 0x%04X\n",
				eeprom_offset + 2, eeprom_data);
		next_section_address = eeprom_offset + ((eeprom_data & 0x00ff) << 3);
		eeprom_data = nes_read16_eeprom(nesdev->regs, next_section_address + 4);
		if (eeprom_data != 0x5753) {
			nes_debug(NES_DBG_HW, "EEPROM Changed offset should be 0x5753 but was 0x%04X\n",
					eeprom_data);
			goto no_fw_rev;
		}
		eeprom_offset = next_section_address;

		eeprom_data = nes_read16_eeprom(nesdev->regs, eeprom_offset + 2);
		nes_debug(NES_DBG_HW, "EEPROM Offset %u (next section)  = 0x%04X\n",
				eeprom_offset + 2, eeprom_data);
		next_section_address = eeprom_offset + ((eeprom_data & 0x00ff) << 3);
		eeprom_data = nes_read16_eeprom(nesdev->regs, next_section_address + 4);
		if (eeprom_data != 0x414d) {
			nes_debug(NES_DBG_HW, "EEPROM Changed offset should be 0x414d but was 0x%04X\n",
					eeprom_data);
			goto no_fw_rev;
		}
		eeprom_offset = next_section_address;

		eeprom_data = nes_read16_eeprom(nesdev->regs, eeprom_offset + 2);
		nes_debug(NES_DBG_HW, "EEPROM Offset %u (next section)  = 0x%04X\n",
				eeprom_offset + 2, eeprom_data);
		next_section_address = eeprom_offset + ((eeprom_data & 0x00ff) << 3);
		eeprom_data = nes_read16_eeprom(nesdev->regs, next_section_address + 4);
		if (eeprom_data != 0x464e) {
			nes_debug(NES_DBG_HW, "EEPROM Changed offset should be 0x464e but was 0x%04X\n",
					eeprom_data);
			goto no_fw_rev;
		}
		eeprom_data = nes_read16_eeprom(nesdev->regs, next_section_address + 8);
		printk(PFX "Firmware version %u.%u\n", (u8)(eeprom_data>>8), (u8)eeprom_data);
		major_ver = (u8)(eeprom_data >> 8);
		minor_ver = (u8)(eeprom_data);

		if (nes_drv_opt & NES_DRV_OPT_DISABLE_VIRT_WQ) {
			nes_debug(NES_DBG_HW, "Virtual WQs have been disabled\n");
		} else if (((major_ver == 2) && (minor_ver > 21)) || ((major_ver > 2) && (major_ver != 255))) {
			nesadapter->virtwq = 1;
		}
		if (((major_ver == 3) && (minor_ver >= 16)) || (major_ver > 3))
			nesadapter->send_term_ok = 1;

		if (nes_drv_opt & NES_DRV_OPT_ENABLE_PAU) {
			if (!nes_set_pau(nesdev))
				nesadapter->allow_unaligned_fpdus = 1;
		}

		nesadapter->firmware_version = (((u32)(u8)(eeprom_data>>8))  <<  16) +
				(u32)((u8)eeprom_data);

		eeprom_data = nes_read16_eeprom(nesdev->regs, next_section_address + 10);
		printk(PFX "EEPROM version %u.%u\n", (u8)(eeprom_data>>8), (u8)eeprom_data);
		nesadapter->eeprom_version = (((u32)(u8)(eeprom_data>>8)) << 16) +
				(u32)((u8)eeprom_data);

no_fw_rev:
		/* eeprom is valid */
		eeprom_offset = nesadapter->software_eeprom_offset;
		eeprom_offset += 8;
		nesadapter->netdev_max = (u8)nes_read16_eeprom(nesdev->regs, eeprom_offset);
		eeprom_offset += 2;
		mac_addr_high = nes_read16_eeprom(nesdev->regs, eeprom_offset);
		eeprom_offset += 2;
		mac_addr_low = (u32)nes_read16_eeprom(nesdev->regs, eeprom_offset);
		eeprom_offset += 2;
		mac_addr_low <<= 16;
		mac_addr_low += (u32)nes_read16_eeprom(nesdev->regs, eeprom_offset);