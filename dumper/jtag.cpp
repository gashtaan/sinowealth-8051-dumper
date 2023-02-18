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

	clrBit(TCK);
	clrBit(TDI);
	clrBit(TMS);
}

void JTAG::connect()
{
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

	sendMode(Mode::ICP);

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

	m_mode = Mode::READY;
}

void JTAG::reset()
{
	if (m_mode == Mode::ERROR)
		return;

	if (m_mode == Mode::JTAG)
	{
		setBit(TMS);

		// reset JTAG state
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

	m_mode = Mode::READY;
}

void JTAG::switchMode(Mode mode)
{
	if (m_mode == mode)
		return;

	if (m_mode != Mode::READY)
		reset();

	m_mode = mode;

	clrBit(TCK);
	_delay_us(2);

	sendMode(m_mode);

	if (m_mode == Mode::ICP)
	{
		_delay_us(800);
		setBit(TCK);
		_delay_us(2);

		ping();
	}
	else if (m_mode == Mode::JTAG)
	{
		setBit(TMS);
		pulseClocks(6);
		clrBit(TMS);
		pulseClocks(2);

		// TODO...
	}
}

bool JTAG::check()
{
	switchMode(Mode::ICP);

	sendData8(ICP_SET_IB_OFFSET_L);
	sendData8(0x69);
	sendData8(ICP_SET_IB_OFFSET_H);
	sendData8(0xFF);

	sendData8(ICP_GET_IB_OFFSET);
	auto b = receiveData8();
	(void)receiveData8();

	return (b == 0x69);
}

void JTAG::ping() const
{
	if (m_mode != Mode::ICP)
		return;

	sendData8(ICP_PING);
	sendData8(0xFF);
}

void JTAG::readFlash(uint8_t* buffer, uint32_t address, bool customBlock)
{
	reset();
	switchMode(Mode::ICP);

#if CHIP_TYPE != 1
	sendData8(0x46);
	sendData8(0xFE);
	sendData8(0xFF);
#endif

	sendData8(ICP_SET_IB_OFFSET_L);
	sendData8(address & 0x000000FF);
	sendData8(ICP_SET_IB_OFFSET_H);
	sendData8((address & 0x0000FF00) >> 8);
#if CHIP_TYPE == 4 || CHIP_TYPE == 7
	sendData8(ICP_SET_XPAGE);
	sendData8((address & 0x00FF0000) >> 16);
#endif

	sendData8(customBlock ? ICP_READ_CUSTOM_BLOCK : ICP_READ_FLASH);

	for (uint8_t n = 0; n < 16; ++n)
		buffer[n] = receiveData8();
}

void JTAG::sendMode(Mode mode) const
{
	for (uint8_t m = 0x80; m; m >>= 1)
	{
		if (uint8_t(mode) & m)
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
