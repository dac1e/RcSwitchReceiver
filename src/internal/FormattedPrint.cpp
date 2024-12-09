/*
  RcSwitchReceiver - Arduino libary for remote control receiver Copyright (c)
  2024 Wolfgang Schmieder.  All right reserved.

  Contributors:
  - Wolfgang Schmieder

  Project home: https://github.com/dac1e/RcSwitchReceiver/

  This library is free software; you can redistribute it and/or modify it
  the terms of the GNU Lesser General Public License as under published
  by the Free Software Foundation; either version 3.0 of the License,
  or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
*/

#include <stdlib.h>
#if defined(ARDUINO_ARCH_SAM)
	// Don't know why this is not in stdlib.h ?
	#include <itoa.h>
#endif

#include "FormattedPrint.hpp"

namespace RcSwitch {

uint32_t scaleUint32(const uint32_t value, unsigned int base) {
	if(base) {
		uint32_t x = (value + (base / 2)) / base;
		return x;
	}
	return value;
}

size_t decimalDigits(unsigned int value) {
	size_t result = 0;
	do {
		++result;
		value /=10;
	} while(value > 0);
	return result;
}

void sprintNumAsDecimal(char *string, const unsigned int value, const size_t width) {
	const size_t digitCnt = decimalDigits(value);
	const size_t spacesCnt = width > digitCnt ? width - digitCnt : 0;
	size_t i = 0;
	while(i < spacesCnt) {
		string[i] = ' ';
		++i;
	}
	itoa(value, &string[i], 10);
}

} // namespace RcSwitch
