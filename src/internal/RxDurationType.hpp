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

#ifndef RCSWITCH_RECEIVER_INTERNAL_DURATIONTYPE_HPP_
#define RCSWITCH_RECEIVER_INTERNAL_DURATIONTYPE_HPP_

#include <stdint.h>

namespace RcSwitch {

#if defined RCSWITCH_RECEIVER_DURATION_TYPE
  /**
   * set RCSWITCH_RECEIVER_DURATION_TYPE to uint32_t if larger pulse
   * durations than from a typical remote control are expected.
   */
  using duration_t = RCSWITCH_RECEIVER_DURATION_TYPE;
#else
  /**
   * unsigned int will be 16 bit on 8 and 16 bit systems and 32 bit
   * on 32 bit systems.
   * Typical pulse durations from remote controls are in the
   * range of 200us .. 15000us. Smaller and longer pulses might
   * come form HF noise. Hence 16 bit width is sufficient.
   */
  using duration_t = unsigned int;
#endif
}

#endif /* RCSWITCH_RECEIVER_INTERNAL_DURATIONTYPE_HPP_ */
