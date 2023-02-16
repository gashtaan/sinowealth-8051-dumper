"""
	Helper script to decrypt Keil C51 *.?pt files
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
"""

import sys
import string

def parse_key(s):
	try:
		return int(s.rstrip('ghijklmnopqrstuvwxyzGHIJKLMNOPQRSTUVWXYZ'), 16)
	except:
		return 0

with open(sys.argv[1], 'rb') as file:
	data = file.read()
	file.close()

	if data[0:17] == b'[Version]\r\n3.00\r\n':
		data = data[17:]

		key1 = parse_key(sys.argv[1][-6:-4])
		key2 = parse_key(sys.argv[1][-8:-6])

		output = bytearray()
		for c in data:
			if c < key1:
				c = c + 256
			output.append((c - key1) ^ key2)

		if output[:10] == b'[ChipName]' or output[:9] == b'[Version]':
			with open(sys.argv[1], 'wb') as file:
				file.write(output)
				file.close()
