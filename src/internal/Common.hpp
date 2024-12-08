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

#ifndef RCSWITCH_RECEIVER_INTERNAL_COMMON_HPP_
#define RCSWITCH_RECEIVER_INTERNAL_COMMON_HPP_

#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace RcSwitch {

/** Forward declaration */
class RxTimingSpec;

struct rxTimingSpecTable {
	const RxTimingSpec* start;
	size_t size;
};

/**
 * Some compilers (e.g. for Atmega128) don't support type_traits. So a local
 * implementation of this feature is required.
 */
template<size_t BIT_COUNT> struct INT_TRAITS_IMPL;

template<> struct INT_TRAITS_IMPL<7> {
	static constexpr int8_t MIN = INT8_MIN;
	static constexpr int8_t MAX = INT8_MAX;
};

template<> struct INT_TRAITS_IMPL<8> {
	static constexpr uint8_t MIN = 0;
	static constexpr uint8_t MAX = UINT8_MAX;
};

template<> struct INT_TRAITS_IMPL<15> {
	static constexpr int16_t MIN = INT16_MIN;
	static constexpr int16_t MAX = INT16_MAX;
};

template<> struct INT_TRAITS_IMPL<16> {
	static constexpr uint16_t MIN = 0;
	static constexpr uint16_t MAX = UINT16_MAX;
};

template<> struct INT_TRAITS_IMPL<31> {
	static constexpr int32_t MIN = INT32_MIN;
	static constexpr int32_t MAX = INT32_MAX;
};

template<> struct INT_TRAITS_IMPL<32> {
	static constexpr uint32_t MIN = 0;
	static constexpr uint32_t MAX = UINT32_MAX;
};

template<typename T> struct INT_TRAITS {
	static constexpr size_t WIDTH = T(-1) < T(0) ? 8*sizeof(T)-1 : 8*sizeof(T);
	static constexpr T MIN = INT_TRAITS_IMPL<WIDTH>::MIN;
	static constexpr T MAX = INT_TRAITS_IMPL<WIDTH>::MAX;
};

static constexpr size_t NUMTOA_BUFFER_SIZE = sizeof(size_t)*8+1;

/**
 * Called for systems where size_t is 32 bit (e.g. SAM)
 */
void sprintNum(char *string, const size_t value, const size_t width);


size_t digitCount(size_t value);
uint32_t scale(uint32_t value, uint16_t base);

template<typename T>
void printNumWithSeparator(T& stream, const size_t value, const size_t width, const char* separator) {
	char buffer[NUMTOA_BUFFER_SIZE];
	sprintNum(buffer, value, width);
	stream.print(buffer);
	if(separator && strlen(separator)) {
		stream.print(separator);
	} else {
		// Print a space as default separator.
		stream.print(' ');
	}
}

template<typename T>
void printNumWithUnitAndSeparator(T& stream, const size_t value, const size_t width, const char* unit, const char* separator) {
	char buffer[NUMTOA_BUFFER_SIZE];
	sprintNum(buffer, value, width);
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
inline void printUsecWithSeparator(T& stream, const size_t value, const size_t width, const char* separator) {
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
inline void printPercentWithSeparator(T& stream, const size_t value, const size_t width, const char* separator) {
	printNumWithSeparator(stream, value, width, nullptr);
	printStringWithSeparator(stream, "%", separator);
}

template<typename T>
inline void printRatioAsPercentWithSeparator(T& stream, const uint32_t nominator, const uint32_t denominator
		, const size_t width, const char* separator) {

	constexpr uint32_t SCALE = 100;
	const uint32_t preDecimalPlaces = (100 * SCALE * nominator / denominator) / SCALE;
	const uint32_t decimalPlaces    = (100 * SCALE * nominator / denominator) % SCALE;

	printUintWithSeparator(stream, preDecimalPlaces, width, ".");
	printPercentWithSeparator(stream, decimalPlaces, width, separator);
}

}

#endif /* RCSWITCH_RECEIVER_INTERNAL_COMMON_HPP_ */
