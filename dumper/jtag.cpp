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
		// reset JTAG state
		for (uint8_t n = 0; n < 35; ++n)
			nextState(1);

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

		pingICP();
	}
	else if (m_mode == Mode::JTAG)
	{
		// reset JTAG state
		for (uint8_t n = 0; n < 8; ++n)
			nextState(1);

		sendInstruction(2);
		sendData<4>(4);

		sendInstruction(3);
		sendData<23>(0x403000);
		_delay_us(50);
		sendData<23>(0x402000);
		sendData<23>(0x400000);

		// most likely breakpoints initialization
		// SH68F881W works without it, but maybe for other chips it's mandatory
		{
			sendData<23>(0x630000);
			sendData<23>(0x670000);
			sendData<23>(0x6B0000);
			sendData<23>(0x6F0000);
			sendData<23>(0x730000);
			sendData<23>(0x770000);
			sendData<23>(0x7B0000);
			sendData<23>(0x7F0000);
		}

		sendInstruction(2);
		sendData<4>(1);

		sendInstruction(12);
	}
}

bool JTAG::checkJTAG()
{
	uint16_t id = getID();
	return (id != 0x0000 && id != 0xFFFF);
}

bool JTAG::checkICP()
{
	switchMode(Mode::ICP);

	sendICPData(ICP_SET_IB_OFFSET_L);
	sendICPData(0x69);
	sendICPData(ICP_SET_IB_OFFSET_H);
	sendICPData(0xFF);

	sendICPData(ICP_GET_IB_OFFSET);
	auto b = receiveICPData();
	(void)receiveICPData();

	return (b == 0x69);
}

void JTAG::pingICP() const
{
	if (m_mode != Mode::ICP)
		return;

	sendICPData(ICP_PING);
	sendICPData(0xFF);
}

uint16_t JTAG::getID()
{
	switchMode(Mode::JTAG);

	sendInstruction(JTAG_GET_ID);
	return receiveData<16, uint16_t>();
}

void JTAG::readFlash(uint8_t* buffer, uint32_t address, bool customBlock)
{
	reset();
	switchMode(Mode::ICP);

#if CHIP_TYPE != 1
	sendICPData(0x46);
	sendICPData(0xFE);
	sendICPData(0xFF);
#endif

	sendICPData(ICP_SET_IB_OFFSET_L);
	sendICPData(address & 0x000000FF);
	sendICPData(ICP_SET_IB_OFFSET_H);
	sendICPData((address & 0x0000FF00) >> 8);
#if CHIP_TYPE == 4 || CHIP_TYPE == 7
	sendData8(ICP_SET_XPAGE);
	sendData8((address & 0x00FF0000) >> 16);
#endif

	sendICPData(customBlock ? ICP_READ_CUSTOM_BLOCK : ICP_READ_FLASH);

	for (uint8_t n = 0; n < 16; ++n)
		buffer[n] = receiveICPData();
}

void JTAG::sendMode(Mode mode)
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

void JTAG::sendICPData(uint8_t value)
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

uint8_t JTAG::receiveICPData()
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

bool JTAG::nextState(bool tms)
{
	if (tms)
		setBit(TMS);
	else
		clrBit(TMS);

	setBit(TCK);
	_delay_us(2);

	bool b = getBit(TDO);

	clrBit(TCK);
	_delay_us(2);

	return b;
}

bool JTAG::nextState(bool tms, bool out)
{
	if (out)
		setBit(TDI);
	else
		clrBit(TDI);

	return nextState(tms);
}

void JTAG::sendInstruction(uint8_t value)
{
	nextState(0); // Idle
	nextState(1); // Select-DR
	nextState(1); // Select-IR
	nextState(0); // Capture-IR
	nextState(0); // Shift-IR
	sendBits<4, uint8_t>(value);
	nextState(1); // Update-IR
	nextState(0); // Idle
}

void JTAG::pulseClock()
{
	_delay_us(1);
	setBit(TCK);
	_delay_us(1);
	clrBit(TCK);
}

void JTAG::pulseClocks(uint8_t count)
{
	while (count-- > 0)
		pulseClock();
}
