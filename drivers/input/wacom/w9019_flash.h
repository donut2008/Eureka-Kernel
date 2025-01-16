prom_data) << 16) +
				nes_read16_eeprom(nesdev->regs, eeprom_offset);
		nes_debug(NES_DBG_HW, "rx_threshold = 0x%08X\n", nesadapter->rx_threshold);

		eeprom_offset += 2;
		eeprom_data = nes_read16_eeprom(nesdev->regs, eeprom_offset);
		eeprom_offset += 2;
		nesadapter->tcp_timer_core_clk_divisor = (((u32)eeprom_data) << 16) +
				nes_read16_eeprom(nesdev->regs, eeprom_offset);
		nes_debug(NES_DBG_HW, "tcp_timer_core_clk_divisor = 0x%08X\n",
				nesadapter->tcp_timer_core_clk_divisor);

		eeprom_offset += 2;
		eeprom_data = nes_read16_eeprom(nesdev->regs, eeprom_offset);
		eeprom_offset += 2;
		nesadapter->iwarp_config = (((u32)eeprom_data) << 16) +
				nes_read16_eeprom(nesdev->regs, eeprom_offset);
		nes_debug(NES_DBG_HW, "iwarp_config = 0x%08X\n", nesadapter->iwarp_config);

		eeprom_offset += 2;
		eeprom_data = nes_read16_eeprom(nesdev->regs, eeprom_offset);
		eeprom_offset += 2;
		nesadapter->cm_config = (((u32)eeprom_data) << 16) +
				nes_read16_eeprom(nesdev->regs, eeprom_offset);
		nes_debug(NES_DBG_HW, "cm_config = 0x%08X\n", nesadapter->cm_config);

		eeprom_offset += 2;
		eeprom_data = nes_read16_eeprom(nesdev->regs, eeprom_offset);
		eeprom_offset += 2;
		nesadapter->sws_timer_config = (((u32)eeprom_data) << 16) +
				nes_read16_eeprom(nesdev->regs, eeprom_offset);
		nes_debug(NES_DBG_HW, "sws_timer_config = 0x%08X\n", nesadapter->sws_timer_config);

		eeprom_offset += 2;
		eeprom_data = nes_read16_eeprom(nesdev->regs, eeprom_offset);
		eeprom_offset += 2;
		nesadapter->tcp_config1 = (((u32)eeprom_data) << 16) +
				nes_read16_eeprom(nesdev->regs, eeprom_offset);
		nes_debug(NES_DBG_HW, "tcp_config1 = 0x%08X\n", nesadapter->tcp_config1);

		eeprom_offset += 2;
		eeprom_data = nes_read16_eeprom(nesdev->regs, eeprom_offset);
		eeprom_offset += 2;
		nesadapter->wqm_wat = (((u32)eeprom_data) << 16) +
				nes_read16_eeprom(nesdev->regs, eeprom_offset);
		nes_debug(NES_DBG_HW, "wqm_wat = 0x%08X\n", nesadapter->wqm_wat);

		eeprom_offset += 2;
		eeprom_data = nes_read16_eeprom(nesdev->regs, eeprom_offset);
		eeprom_offset += 2;
		nesadapter->core_clock = (((u32)eeprom_data) << 16) +
				nes_read16_eeprom(nesdev->regs, eeprom_offset);
		nes_debug(NES_DBG_HW, "core_clock = 0x%08X\n", nesadapter->core_clock);

		if ((sw_section_ver) && (nesadapter->hw_rev != NE020_REV)) {
			eeprom_offset += 2;
			eeprom_data = nes_read16_eeprom(nesdev->regs, eeprom_offset);
			nesadapter->phy_index[0] = (eeprom_data & 0xff00)>>8;
			nesadapter->phy_index[1] = eeprom_data & 0x00ff;
			eeprom_offset += 2;
			eeprom_data = nes_read16_eeprom(nesdev->regs, eeprom_offset);
			nesadapter->phy_index[2] = (eeprom_data & 0xff00)>>8;
			nesadapter->phy_index[3] = eeprom_data & 0x00ff;
		} else {
			nesadapter->phy_index[0] = 4;
			nesadapter->phy_index[1] = 5;
			nesadapter->phy_index[2] = 6;
			nesadapter->phy_index[3] = 7;
		}
		nes_debug(NES_DBG_HW, "Phy address map = 0 > %u,  1 > %u, 2 > %u, 3 > %u\n",
			   nesadapter->phy_index[0],nesadapter->phy_index[1],
			   nesadapter->phy_index[2],nesadapter->phy_index[3]);
	}

	return 0;
}


/**
 * nes_read16_eeprom
 */
static u16 nes_read16_eeprom(void __iomem *addr, u16 offset)
{
	writel(NES_EEPROM_READ_REQUEST + (offset >> 1),
			(void __iomem *)addr + NES_EEPROM_COMMAND);

	do {
	} while (readl((void __iomem *)addr + NES_EEPROM_COMMAND) &
			NES_EEPROM_READ_REQUEST);

	return readw((void __iomem *)addr + NES_EEPROM_DATA);
}


/**
 * nes_write_1G_phy_reg
 */
void nes_write_1G_phy_reg(struct nes_device *nesdev, u8 phy_reg, u8 phy_addr, u16 data)
{
	u32 u32temp;
	u32 counter;

	nes_write_indexed(nesdev, NES_IDX_MAC_MDIO_CONTROL,
			0x50020000 | data | ((u32)phy_reg << 18) | ((u32)phy_addr << 23));
	for (counter = 0; counter < 100 ; counter++) {
		udelay(30);
		u32temp = nes_read_indexed(nesdev, NES_IDX_MAC_INT_STATUS);
		if (u32temp & 1) {
			/* nes_debug(NES_DBG_PHY, "Phy interrupt status = 0x%X.\n", u32temp); */
			nes_write_indexed(nesdev, NES_IDX_MAC_INT_STATUS, 1);
			break;
		}
	}
	if (!(u32temp & 1))
		nes_debug(NES_DBG_PHY, "Phy is not responding. interrupt status = 0x%X.\n",
				u32temp);
}


/**
 * nes_read_1G_phy_reg
 * This routine only issues the read, the data must be read
 * separately.
 */
void nes_read_1G_phy_reg(struct nes_device *nesdev, u8 phy_reg, u8 phy_addr, u16 *data)
{
	u32 u32temp;
	u32 counter;

	/* nes_debug(NES_DBG_PHY, "phy addr = %d, mac_index = %d\n",
			phy_addr, nesdev->mac_index); */

	