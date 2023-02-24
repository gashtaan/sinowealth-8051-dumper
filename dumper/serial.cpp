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

#include "serial.h"

void serialInit()
{
	static const unsigned long baud_rate = 115200;

	UCSR0A = _BV(U2X0);
	UCSR0B = _BV(RXEN0) | _BV(TXEN0);
	UCSR0C = _BV(UCSZ00) | _BV(UCSZ01);
	UBRR0L = uint8_t((F_CPU + baud_rate * 4L) / (baud_rate * 8L) - 1);
}

void serialWrite(char c)
{
	while (!(UCSR0A & _BV(UDRE0)));
	UDR0 = c;
}

void serialWrite(const char* str)
{
	while (*str)
		serialWrite(*str++);
}

void serialWriteHex(uint8_t byte)
{
	static const char hexs[] = "0123456789ABCDEF";
	serialWrite(hexs[(byte & 0xF0) >> 4]);
	serialWrite(hexs[(byte & 0x0F)]);
}

bool serialWait()
{
	if (UCSR0A & _BV(RXC0) && (UDR0 == '\r' || UDR0 == '\n'))
		return true;

	return false;
}
