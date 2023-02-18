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

#pragma once

#include <stdbool.h>
#include <stdint.h>

#define clrBit(b) (PORTD &= ~_BV(b))
#define setBit(b) (PORTD |= _BV(b))
#define getBit(b) (PIND & _BV(b))

class JTAG
{
public:
	JTAG();

	static const uint8_t TDO = 2;	// D2
	static const uint8_t TMS = 3;	// D3
	static const uint8_t TDI = 4;	// D4
	static const uint8_t TCK = 5;	// D5

	void connect();
	bool check();
	void ping() const;

	void readFlash(uint8_t* buffer, uint32_t address, bool customBlock);

private:
	enum class Mode
	{
		ERROR = 0,
		READY = 1,
		ICP = 150,
		JTAG = 165
	};

	enum ICPCommand : uint8_t
	{
		ICP_SET_IB_OFFSET_L = 0x40,
		ICP_SET_IB_OFFSET_H = 0x41,
		ICP_GET_IB_OFFSET = 0x43,
		ICP_READ_FLASH = 0x44,
		ICP_PING = 0x49,
		ICP_READ_CUSTOM_BLOCK = 0x4A,
		ICP_SET_XPAGE = 0x4C,
	};

	void reset();
	void switchMode(Mode mode);

	void sendMode(Mode mode) const;
	void sendData8(uint8_t value) const;
	uint8_t receiveData8() const;

	void pulseClock() const;
	void pulseClocks(uint8_t count) const;

	Mode m_mode = Mode::ERROR;
};
