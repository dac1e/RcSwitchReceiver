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

#pragma once

#ifndef RCSWITCH_RECEIVER_INTERNAL_FORMATTED_PRINT_HPP_
#define RCSWITCH_RECEIVER_INTERNAL_FORMATTED_PRINT_HPP_

#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace RcSwitch {

static constexpr size_t NUMTOA_BUFFER_SIZE = sizeof(size_t)*8+1;

/**
 * Prints an unsigned integer with a particular width. If the the number less
 * has decimal digits than width, the number is prepended with spaces.
 */
void sprintNumAsDecimal(char *string, const unsigned int value, const size_t width);

/**
 * Returns the number of decimal digits of an unsigned integer.
 */
size_t decimalDigits(unsigned int value);

/**
 * Returns a base - rounded and divided 32 bit integer.
 */
uint32_t scaleUint32(const uint32_t value, const unsigned int  base);

template<typename T>
void printNumWithSeparator(T& stream, const unsigned int value, const size_t width, const char* separator) {
	char buffer[NUMTOA_BUFFER_SIZE];
	sprintNumAsDecimal(buffer, value, width);
	stream.print(buffer);
	if(separator && strlen(separator)) {
		stream.print(separator);
	} else {
		// Print a space as default separator.
		stream.print(' ');
	}
}

template<typename T>
void printNumWithUnitAndSeparator(T& stream, const unsigned int value, const size_t width, const char* unit, const char* separator) {
	char buffer[NUMTOA_BUFFER_SIZE];
	sprintNumAsDecimal(buffer, value, width);
	stream.print(buffer);
	if(separator && strlen(separator)) {
		stream.print(separator);
	}
	stream.print(unit);
	if(separator && strlen(separator)) {
		stream.print(separator);
	} else {
		// Print a space as default separator.
		stream.print(' ');
	}
}

template<typename T>
inline void printUsecWithSeparator(T& stream, const unsigned int value, const size_t width, const char* separator) {
	printNumWithUnitAndSeparator(stream, value, width, "usec", separator);
}

template<typename T>
void printStringWithSeparator(T& stream, const char* string, const char* separator) {
	stream.print(string);
	if(separator && strlen(separator)) {
		stream.print(separator);
	} else {
		// Print a space as default separator.
		stream.print(' ');
	}
}

template<typename T>
inline void printPercentWithSeparator(T& stream, const unsigned int value, const size_t width, const char* separator) {
	printNumWithSeparator(stream, value, width, nullptr);
	printStringWithSeparator(stream, "%", separator);
}

template<typename T>
inline void printRatioAsPercentWithSeparator(T& stream, const uint32_t nominator, const uint32_t denominator
		, const size_t width, const char* separator) {

	// decimal places width:
	//   1 -> width is 1
	//  10 -> width is 2
	// 100 -> width is 3
	constexpr size_t DECIMAL_PLACES_DIV = 10;
	constexpr uint32_t SCALE = 1000;

	const uint32_t scaledRatio = (SCALE * (100 * nominator)) / denominator;
	const uint32_t roundedScaledRation = scaledRatio + DECIMAL_PLACES_DIV / 2;
	const uint32_t preDecimalPlaces = roundedScaledRation / SCALE;
	uint32_t decimalPlaces = (roundedScaledRation % SCALE) / DECIMAL_PLACES_DIV;

	printNumWithSeparator(stream, preDecimalPlaces, width, ".");
	stream.print(decimalPlaces);
	stream.print('%');
	stream.print(separator);
}

}

#endif /* RCSWITCH_RECEIVER_INTERNAL_FORMATTED_PRINT_HPP_ */
