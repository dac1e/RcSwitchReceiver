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

namespace RcSwitch {

/**
 * A template function structure providing initial values for
 * the particular types. Will be specialized for the types where
 * initial value is needed. */
template<typename ELEMENT_TYPE> struct INITIAL_VALUE;

/** Specialize INITIAL_VALUE for size_t */
template<> struct INITIAL_VALUE<size_t> {
	static constexpr size_t value = static_cast<size_t>(-1);
};

/** Forward declaration */
class RxTimingSpec;

struct rxTimingSpecTable {
	const RxTimingSpec* start;
	size_t size;
};

void sprintUint(char *string, const size_t value, const size_t width);
size_t digitCount(size_t value);

}

#endif /* RCSWITCH_RECEIVER_INTERNAL_COMMON_HPP_ */
