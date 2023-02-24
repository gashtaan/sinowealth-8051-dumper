/*
   https://github.com/gashtaan/sinowealth-8051-dumper

   Copyright (C) 2023, Michal Kovacik

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 3, as
   published by the Free Software Foundation.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "jtag.h"
#include "serial.h"
#include "config.h"

uint8_t buffer[16] = {};

int main()
{
	serialInit();
	serialWrite("\r\nSinoWealth 8051-based MCU flash dumper\r\n");

	JTAG jtag;
	jtag.connect();

	if (jtag.checkICP())
	{
		serialWrite("Connection established\r\n");

		serialWrite("\r\nJTAG ID: ");
		uint16_t id = jtag.getID();
		serialWriteHex(id >> 8);
		serialWriteHex(id & 0xFF);
		serialWrite("\r\n");

#if CHIP_PRODUCT_BLOCK == 1
		serialWrite("\r\nDumping part number:\r\n");

		uint16_t custom_block_address = 0;
		switch (CHIP_CUSTOM_BLOCK)
		{
			case 2:
				custom_block_address = 0x0A00;
				break;
			case 3:
				custom_block_address = 0x1200;
				break;
			case 4:
				custom_block_address = 0x2200;
				break;
		}

		jtag.readFlashICP(buffer, sizeof(buffer), custom_block_address, true);
		for (uint8_t n = 0; n < 5; ++n)
			serialWriteHex(buffer[n + 9]);
		serialWrite("\r\n");
#endif

		serialWrite("\r\nDumping flash memory:\r\n");

		for (uint32_t a = 0; a < CHIP_FLASH_SIZE; a += sizeof(buffer))
		{
			jtag.readFlashICP(buffer, sizeof(buffer), a, false);
			for (auto n : buffer)
				serialWriteHex(n);
			serialWrite("\r\n");
		}

		serialWrite("\r\nDone!\r\n");
	}
	else
	{
		serialWrite("Connection failed\r\n");
	}

	jtag.disconnect();
}
