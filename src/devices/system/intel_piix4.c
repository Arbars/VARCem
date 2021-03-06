/*
 * VARCem	Virtual ARchaeological Computer EMulator.
 *		An emulator of (mostly) x86-based PC systems and devices,
 *		using the ISA,EISA,VLB,MCA  and PCI system buses, roughly
 *		spanning the era between 1981 and 1995.
 *
 *		This file is part of the VARCem Project.
 *
 *		Preliminary emulation of the Intel PIIX4 Xcelerator.
 *
 *		PRD format :
 *		    word 0 - base address
 *		    word 1 - bits 1-15 = byte count, bit 31 = end of transfer
 *
 * Version:	@(#)intel_piix4.c	1.0.3	2018/05/06
 *
 * Author:	Miran Grca, <mgrca8@gmail.com>
 *
 *		Copyright 2016-2018 Miran Grca.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free  Software  Foundation; either  version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is  distributed in the hope that it will be useful, but
 * WITHOUT   ANY  WARRANTY;  without  even   the  implied  warranty  of
 * MERCHANTABILITY  or FITNESS  FOR A PARTICULAR  PURPOSE. See  the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the:
 *
 *   Free Software Foundation, Inc.
 *   59 Temple Place - Suite 330
 *   Boston, MA 02111-1307
 *   USA.
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>
#include "../../emu.h"
#include "../../io.h"
#include "../../mem.h"
#include "../../device.h"
#include "../input/keyboard.h"
#include "../disk/hdc.h"
#include "../disk/hdc_ide.h"
#include "dma.h"
#include "pci.h"
#include "intel_piix.h"


static uint8_t card_piix4[256], card_piix4_ide[256];


static void piix4_write(int func, int addr, uint8_t val, void *priv)
{
	uint16_t old_base = (card_piix4_ide[0x20] & 0xf0) | (card_piix4_ide[0x21] << 8);
        if (func > 1)
           return;
        
        if (func == 1) /*IDE*/
        {
		/* pclog("PIIX IDE write: %02X %02X\n", addr, val); */

                switch (addr)
                {
                        case 0x04:
                        card_piix4_ide[0x04] = (val & 5) | 2;
                        break;
                        case 0x07:
                        card_piix4_ide[0x07] = val & 0x3e;
                        break;
                        case 0x0d:
                        card_piix4_ide[0x0d] = val;
                        break;
                        
                        case 0x20:
                        card_piix4_ide[0x20] = (val & ~0x0f) | 1;
                        break;
                        case 0x21:
                        card_piix4_ide[0x21] = val;
                        break;
                        
                        case 0x40:
                        card_piix4_ide[0x40] = val;
                        break;
                        case 0x41:
                        if ((val ^ card_piix4_ide[0x41]) & 0x80)
                        {
                                ide_pri_disable();
                                if (val & 0x80)
                                        ide_pri_enable();
                        }
                        card_piix4_ide[0x41] = val;
                        break;
                        case 0x42:
                        card_piix4_ide[0x42] = val;
                        break;
                        case 0x43:
                        if ((val ^ card_piix4_ide[0x43]) & 0x80)
                        {
                                ide_sec_disable();
                                if (val & 0x80)
                                   ide_sec_enable();                                  
                        }
                        card_piix4_ide[0x43] = val;
                        break;
			case 0x44:
			card_piix4_ide[0x44] = val;
			break;
			case 0x48:
			card_piix4_ide[0x44] = val;
			break;
			case 0x4A:
			card_piix4_ide[0x44] = val;
			break;
			case 0x4B:
			card_piix4_ide[0x44] = val;
			break;
                }
                if (addr == 4 || (addr & ~3) == 0x20) /*Bus master base address*/                
                {
                        uint16_t base = (card_piix4_ide[0x20] & 0xf0) | (card_piix4_ide[0x21] << 8);
                        io_removehandler(old_base, 0x10, piix_bus_master_read, NULL, NULL, piix_bus_master_write, NULL, NULL,  NULL);
                        if (card_piix4_ide[0x04] & 1)
			{
				io_sethandler(base, 0x10, piix_bus_master_read, NULL, NULL, piix_bus_master_write, NULL, NULL,  NULL);
			}
                }
        }
        else
        {
		/* pclog("PIIX writing value %02X to register %02X\n", val, addr); */
                if ((addr >= 0x0f) && (addr < 0x4c))
                        return;

                switch (addr)
                {
                        case 0x00: case 0x01: case 0x02: case 0x03:
                        case 0x08: case 0x09: case 0x0a: case 0x0b:
                        case 0x0e:
                        return;
                        
                        case 0x60:
			/* pclog("Set IRQ routing: INT A -> %02X\n", val); */
                        if (val & 0x80)
                                pci_set_irq_routing(PCI_INTA, PCI_IRQ_DISABLED);
                        else
                                pci_set_irq_routing(PCI_INTA, val & 0xf);
                        break;
                        case 0x61:
			/* pclog("Set IRQ routing: INT B -> %02X\n", val); */
                        if (val & 0x80)
                                pci_set_irq_routing(PCI_INTB, PCI_IRQ_DISABLED);
                        else
                                pci_set_irq_routing(PCI_INTB, val & 0xf);
                        break;
                        case 0x62:
			/* pclog("Set IRQ routing: INT C -> %02X\n", val); */
                        if (val & 0x80)
                                pci_set_irq_routing(PCI_INTC, PCI_IRQ_DISABLED);
                        else
                                pci_set_irq_routing(PCI_INTC, val & 0xf);
                        break;
                        case 0x63:
			/* pclog("Set IRQ routing: INT D -> %02X\n", val); */
                        if (val & 0x80)
                                pci_set_irq_routing(PCI_INTD, PCI_IRQ_DISABLED);
                        else
                                pci_set_irq_routing(PCI_INTD, val & 0xf);
                        break;
                }
		if (addr == 0x4C)
		{
			if (!((val ^ card_piix4[addr]) & 0x80))
			{
		                card_piix4[addr] = val;
				return;
			}

	                card_piix4[addr] = val;
			if (val & 0x80)
			{
				dma_alias_remove();
			}
			else
			{
				dma_alias_set();
			}
		}
		else if (addr == 0x4E)
		{
			keyboard_at_set_mouse_scan((val & 0x10) ? 1 : 0);
	                card_piix4[addr] = val;
		}
		else
	                card_piix4[addr] = val;
        }
}

static uint8_t piix4_read(int func, int addr, void *priv)
{
        if (func > 1)
           return 0xff;

        if (func == 1) /*IDE*/
        {
		if (addr == 4)
		{
			return (card_piix4_ide[addr] & 5);
		}
		else if (addr == 5)
		{
			return 0;
		}
		else if (addr == 6)
		{
			return 0x80;
		}
		else if (addr == 7)
		{
			return card_piix4_ide[addr] & 0x3E;
		}
		else if (addr == 0xD)
		{
			return card_piix4_ide[addr] & 0xF0;
		}
		else if (addr == 0x20)
		{
			return (card_piix4_ide[addr] & 0xF0) | 1;
		}
		else if (addr == 0x22)
		{
			return 0;
		}
		else if (addr == 0x23)
		{
			return 0;
		}
		else if (addr == 0x41)
		{
			return card_piix4_ide[addr] & 0xF3;
		}
		else if (addr == 0x43)
		{
			return card_piix4_ide[addr] & 0xF3;
		}
		else if (addr == 0x48)
		{
			return card_piix4_ide[addr] & 0x0F;
		}
		else if (addr == 0x4A)
		{
			return card_piix4_ide[addr] & 0x33;
		}
		else if (addr == 0x4B)
		{
			return card_piix4_ide[addr] & 0x33;
		}
		else
		{
                	return card_piix4_ide[addr];
		}
        }
        else
	{
		if ((addr & 0xFC) == 0x60)
		{
			return card_piix4[addr] & 0x8F;
		}
		if (addr == 4)
		{
			return (card_piix4[addr] & 0x08) | 7;
		}
		else if (addr == 5)
		{
			return card_piix4[addr] & 0x01;
		}
		else if (addr == 6)
		{
			return 0x80;
		}
		else if (addr == 7)
		{
			return (card_piix4[addr] & 0x78) | 0x02;
		}
		else if (addr == 0x4E)
		{
			return (card_piix4[addr] & 0xEF) | keyboard_at_get_mouse_scan();
		}
		else if (addr == 0x4F)
		{
			return card_piix4[addr] & 0x06;
		}
		else if (addr == 0x69)
		{
			return card_piix4[addr] & 0xFE;
		}
		else if (addr == 0x6A)
		{
			return card_piix4[addr] & 0x80;
		}
		else if (addr == 0x6B)
		{
			return card_piix4[addr] & 0x80;
		}
		else if (addr == 0x76)
		{
			return (card_piix4[addr] & 0x87) | 0x08;
		}
		else if (addr == 0x77)
		{
			return (card_piix4[addr] & 0x87) | 0x08;
		}
		else if (addr == 0x80)
		{
			return card_piix4[addr] & 0x7F;
		}
		else if (addr == 0x82)
		{
			return card_piix4[addr] & 0x0F;
		}
		else if (addr == 0x91)
		{
			return card_piix4[addr] & 0xFC;
		}
		else if (addr == 0x92)
		{
			return card_piix4[addr] & 0xC0;
		}
		else if (addr == 0x94)
		{
			return card_piix4[addr] & 0xC0;
		}
		else if (addr == 0xB0)
		{
			return card_piix4[addr] & 0x7F;
		}
		else if (addr == 0xB1)
		{
			return card_piix4[addr] & 0xDF;
		}
		else if (addr == 0xB3)
		{
			return card_piix4[addr] & 0xFD;
		}
		else if (addr == 0xCB)
		{
			return card_piix4[addr] & 0x3D;
		}
		else
			return card_piix4[addr];
	}

	return 0;
}

void piix4_reset(void)
{
        memset(card_piix4, 0, 256);
        memset(card_piix4_ide, 0, 256);

        card_piix4[0x00] = 0x86; card_piix4[0x01] = 0x80; /*Intel*/
        card_piix4[0x02] = 0x10; card_piix4[0x03] = 0x71; /*82371AB (PIIX4)*/
        card_piix4[0x04] = 0x07; card_piix4[0x05] = 0x00;
        card_piix4[0x06] = 0x80; card_piix4[0x07] = 0x02;
        card_piix4[0x08] = 0x00; /*A0 stepping*/
        card_piix4[0x09] = 0x00; card_piix4[0x0a] = 0x01; card_piix4[0x0b] = 0x06;
        card_piix4[0x0e] = 0x80; /*Multi-function device*/
        card_piix4[0x4c] = 0x4d;
        card_piix4[0x4e] = 0x03;
        card_piix4[0x60] = card_piix4[0x61] = card_piix4[0x62] = card_piix4[0x63] = 0x80;
	card_piix4[0x64] = 0x10;
        card_piix4[0x69] = 0x02;
        card_piix4[0x76] = card_piix4[0x77] = 0x04;
	card_piix4[0xcb] = 0x21;

        card_piix4_ide[0x00] = 0x86; card_piix4_ide[0x01] = 0x80; /*Intel*/
        card_piix4_ide[0x02] = 0x11; card_piix4_ide[0x03] = 0x71; /*82371AB (PIIX)*/
        card_piix4_ide[0x04] = 0x07; card_piix4_ide[0x05] = 0x00;
        card_piix4_ide[0x06] = 0x80; card_piix4_ide[0x07] = 0x02;
        card_piix4_ide[0x08] = 0x00;
        card_piix4_ide[0x09] = 0x80; card_piix4_ide[0x0a] = 0x01; card_piix4_ide[0x0b] = 0x01;
        card_piix4_ide[0x0d] = 0x00;
        card_piix4_ide[0x0e] = 0x00;
        card_piix4_ide[0x20] = 0x01; card_piix4_ide[0x21] = card_piix4_ide[0x22] = card_piix4_ide[0x23] = 0x00; /*Bus master interface base address*/
        card_piix4_ide[0x40] = card_piix4_ide[0x42] = 0x00;
	card_piix4_ide[0x41] = card_piix4_ide[0x43] = 0x80;
}

void piix4_init(int card)
{
	device_add(&ide_pci_2ch_device);

        pci_add_card(card, piix4_read, piix4_write, NULL);

	piix4_reset();
        
        ide_set_bus_master(piix_bus_master_dma_read, piix_bus_master_dma_write, piix_bus_master_set_irq);

	port_92_reset();

	port_92_add();

	dma_alias_set();

	pci_reset_handler.pci_set_reset = piix4_reset;
}
