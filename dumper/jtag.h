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

	void reset();
	void switchMode(uint8_t mode);
	void ping() const;
	bool check() const;

	void readFlash(uint8_t* buffer, uint16_t address, bool customBlock);

private:
	void sendMode(uint8_t value) const;
	void sendData8(uint8_t value) const;
	uint8_t receiveData8() const;

	void pulseClock() const;
	void pulseClocks(uint8_t count) const;

	uint8_t m_mode = 0;
};
