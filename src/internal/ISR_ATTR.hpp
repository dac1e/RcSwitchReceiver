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

#ifndef RCSWITCH_RECEIVER_INTERNAL_ISR_ATTR_HPP_
#define RCSWITCH_RECEIVER_INTERNAL_ISR_ATTR_HPP_

#if defined (ESP32)
#include <esp_attr.h>
#endif

#if not defined(TEXT_ISR_ATTR)
	#if defined(ESP8266)
		// interrupt handler and related code must be in RAM on ESP8266,
		// according to issue #46.
		#define TEXT_ISR_ATTR ICACHE_RAM_ATTR
	#elif defined(ESP32)
		#define TEXT_ISR_ATTR IRAM_ATTR
	#else
		#define TEXT_ISR_ATTR
	#endif
#endif // not defined(TEXT_ISR_ATTR)

#if not defined(DATA_ISR_ATTR)
	#if defined(ESP8266)
		// interrupt handler and related code must be in RAM on ESP8266,
		// according to issue #46.
		#define DATA_ISR_ATTR
	#elif defined(ESP32)
		#define DATA_ISR_ATTR DRAM_ATTR
	#else
		#define DATA_ISR_ATTR
	#endif
#endif // not defined(DATA_ISR_ATTR)

#define TEXT_ISR_ATTR_0 TEXT_ISR_ATTR // attibute for handleInterrupt()
#define TEXT_ISR_ATTR_1 TEXT_ISR_ATTR // attibute functions called by handleInterrupt()
#define TEXT_ISR_ATTR_2 TEXT_ISR_ATTR // attibute functions called by functions called by handleInterrupt()

#endif /* RCSWITCH_RECEIVER_INTERNAL_ISR_ATTR_HPP_ */