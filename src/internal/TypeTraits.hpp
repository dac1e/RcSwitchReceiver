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

#ifndef RCSWITCH_RECEIVER_TYPETRAITS_HPP_
#define RCSWITCH_RECEIVER_TYPETRAITS_HPP_

#include <stddef.h>
#include <stdint.h>

namespace RcSwitch {

/**
 * Some compilers (e.g. for ATmega328P) don't support type_traits. So a local
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

} // namespace RcSwitch

#endif /* RCSWITCH_RECEIVER_TYPETRAITS_HPP_ */
