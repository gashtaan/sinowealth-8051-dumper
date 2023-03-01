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

constexpr uint8_t reverseBits(uint8_t b)
{
	return (b & 0x80 ? 0x01 : 0) | (b & 0x40 ? 0x02 : 0) | (b & 0x20 ? 0x04 : 0) | (b & 0x10 ? 0x08 : 0) | (b & 0x08 ? 0x10 : 0) | (b & 0x04 ? 0x20 : 0) | (b & 0x02 ? 0x40 : 0) | (b & 0x01 ? 0x80 : 0);
}

class JTAG
{
public:
	JTAG();

	static const uint8_t TDO = 2;	// D2
	static const uint8_t TMS = 3;	// D3
	static const uint8_t TDI = 4;	// D4
	static const uint8_t TCK = 5;	// D5

	void connect();
	void disconnect();

	bool checkJTAG();
	bool checkICP();

	void pingICP() const;

	uint16_t getID();

	typedef bool (JTAG::*readFlashMethod)(uint8_t* buffer, uint8_t bufferSize, uint32_t address, bool customBlock);
	bool readFlashICP(uint8_t* buffer, uint8_t bufferSize, uint32_t address, bool customBlock);
	bool readFlashJTAG(uint8_t* buffer, uint8_t bufferSize, uint32_t address, bool customBlock);

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

	enum JTAGInstruction : uint8_t
	{
		JTAG_IDCODE = 14,
	};

	void reset();
	void switchMode(Mode mode);
	void startMode() const;

	static void sendICPData(uint8_t value);
	static uint8_t receiveICPData();

	static bool nextState(bool tms);
	static bool nextState(bool tms, bool out);

	template <uint8_t N, typename T>
	static void sendBits(T value)
	{
		uint8_t n = N;
		while (n-- > 0)
		{
			nextState(n == 0, value & 1);
			value >>= 1;
		}

		clrBit(TDI);
	}

	template <uint8_t N, typename T>
	static T receiveBits()
	{
		T value = 0;
		uint8_t n = N;
		while (n-- > 0)
		{
			value <<= 1;
			value |= nextState(n == 0);
		}
		return value;
	}

	static void sendInstruction(uint8_t value)
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

	template <uint8_t N, typename T>
	static void sendData(T value)
	{
		nextState(1); // Select-DR
		nextState(0); // Capture-DR
		nextState(0); // Shift-DR
		sendBits<N, T>(value);
		nextState(1); // Update-DR
		nextState(0); // Idle
		nextState(0); // Idle? Needed, don't know why
	}

	template <uint8_t N, typename T>
	static T receiveData()
	{
		nextState(1); // Select-DR
		nextState(0); // Capture-DR
		nextState(0); // Shift-DR
		T value = receiveBits<N, T>();
		nextState(1); // Update-DR
		nextState(0); // Idle
		return value;
	}

	static void pulseClock();
	static void pulseClocks(uint8_t count);

	Mode m_mode = Mode::ERROR;
};
