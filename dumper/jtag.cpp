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

#include <avr/io.h>
#include <util/delay.h>

#include "config.h"
#include "jtag.h"

JTAG::JTAG()
{
	DDRD &= ~_BV(TDO);
	DDRD |= _BV(TDI);
	DDRD |= _BV(TMS);
	DDRD |= _BV(TCK);

	clrBit(TDO);
	setBit(TCK);
	setBit(TDI);
	setBit(TMS);

	_delay_us(500);

	clrBit(TCK);
	_delay_us(1);
	setBit(TCK);
	_delay_us(50);

	for (uint8_t n = 0; n < 165; ++n)
	{
		clrBit(TMS);
		_delay_us(2);
		setBit(TMS);
		_delay_us(2);
	}

	for (uint8_t n = 0; n < 105; ++n)
	{
		clrBit(TDI);
		_delay_us(2);
		setBit(TDI);
		_delay_us(2);
	}

	for (uint8_t n = 0; n < 90; ++n)
	{
		clrBit(TCK);
		_delay_us(2);
		setBit(TCK);
		_delay_us(2);
	}

	for (uint16_t n = 0; n < 25600; ++n)
	{
		clrBit(TMS);
		_delay_us(2);
		setBit(TMS);
		_delay_us(2);
	}

	_delay_us(8);

	clrBit(TMS);
	clrBit(TCK);
	_delay_us(2);

	sendMode(150);

	setBit(TCK);
	_delay_us(2);

	for (uint16_t n = 0; n < 25600; ++n)
	{
		clrBit(TCK);
		_delay_us(2);
		setBit(TCK);
		_delay_us(2);
	}

	setBit(TMS);
	_delay_us(5);
	clrBit(TMS);
	_delay_us(5);

	m_mode = 1;
}

void JTAG::reset()
{
	if (m_mode == 0)
		return;

	if (m_mode == 165)
	{
		setBit(TMS);

		for (uint8_t n = 0; n < 35; ++n)
		{
			setBit(TCK);
			_delay_us(2);
			clrBit(TCK);
			_delay_us(2);
		}
		setBit(TCK);

		clrBit(TMS);
	}
	else
	{
		setBit(TCK);

		setBit(TMS);
		_delay_us(2);
		clrBit(TMS);
		_delay_us(2);
	}

	m_mode = 1;
}

void JTAG::switchMode(uint8_t mode)
{
	if (m_mode == mode)
		return;

	if (m_mode != 1)
		reset();

	m_mode = mode;

	clrBit(TCK);
	_delay_us(2);

	sendMode(m_mode);

	if (m_mode == 150)
	{
		_delay_us(800);
		setBit(TCK);
		_delay_us(2);

		ping();
	}
	else if (m_mode == 165)
	{
		setBit(TMS);
		pulseClocks(6);
		clrBit(TMS);
		pulseClocks(2);

		// TODO...
	}
}

bool JTAG::check() const
{
	clrBit(TCK);

	sendData8(0x40);
	sendData8(0x69);
	sendData8(0x41);
	sendData8(0xFF);
	sendData8(0xFF);

	sendData8(0x43);
	auto b = receiveData8();
	receiveData8();

	return (b == 0x69);
}

void JTAG::ping() const
{
	sendData8(0x49);
	sendData8(0xFF);
}

void JTAG::readFlash(uint8_t* buffer, uint16_t address, bool customBlock)
{
	reset();
	switchMode(150);

#if CHIP_TYPE > 1
	sendData8(0x46);
	sendData8(0xFE);
	sendData8(0xFF);
#endif

	sendData8(0x40);
	sendData8(address & 0x00FF);
	sendData8(0x41);
	sendData8((address & 0xFF00) >> 8);

	sendData8(customBlock ? 0x4A : 0x44);

	for (uint8_t n = 0; n < 16; ++n)
		buffer[n] = receiveData8();
}

void JTAG::sendMode(uint8_t value) const
{
	for (uint8_t m = 0x80; m; m >>= 1)
	{
		if (value & m)
			setBit(TDI);
		else
			clrBit(TDI);

		setBit(TCK);
		_delay_us(2);
		clrBit(TCK);
		_delay_us(2);
	}

	setBit(TCK);
	_delay_us(2);
	clrBit(TCK);
	_delay_us(2);

	setBit(TCK);
	_delay_us(2);
	clrBit(TCK);
	_delay_us(2);
}

void JTAG::sendData8(uint8_t value) const
{
	for (uint8_t m = 0x80; m; m >>= 1)
	{
		if (value & m)
			setBit(TDI);
		else
			clrBit(TDI);

		pulseClock();
	}

	pulseClock();

	clrBit(TDI);
}

uint8_t JTAG::receiveData8() const
{
	uint8_t value = 0;
	for (uint8_t m = 1; m; m <<= 1)
	{
		pulseClock();

		if (getBit(TDO))
			value |= m;
	}

	pulseClock();

	return value;
}

void JTAG::pulseClock() const
{
	_delay_us(1);
	setBit(TCK);
	_delay_us(1);
	clrBit(TCK);
}

void JTAG::pulseClocks(uint8_t count) const
{
	while (count-- > 0)
		pulseClock();
}
