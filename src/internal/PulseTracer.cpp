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

#include "PulseTracer.hpp"
#include <Arduino.h>

namespace RcSwitch {

template<> void TraceRecord::dump(typeof(Serial)& serial, const char* separator, const size_t i, const size_t indexWidth) const {
	serial.print("[");
	printNumWithSeparator(serial, i, indexWidth, "]");
	printStringWithSeparator(serial, "", separator);

	// print pulse type (LOW, HIGH)
	printStringWithSeparator(serial, pulseTypeToString(getPulse()), separator);
	printStringWithSeparator(serial, "for", separator);
	printUsecWithSeparator(serial, getPulse().getDuration(), 5, separator);

	printStringWithSeparator(serial, "CPU interrupt load =", separator);
	printUsecWithSeparator(serial, getInterruptDuration(), 3, separator);

	printRatioAsPercentWithSeparator(serial, getInterruptDuration()
			, getPulse().getDuration(), 2, separator);
	serial.println();
}

} /* namespace RcSwitch */
