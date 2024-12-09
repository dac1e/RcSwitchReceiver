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

namespace RcSwitch {

TraceRecord::TraceRecord(const Pulse &pulse,
		const duration_t usecInterruptDuration) :
		mUsecInteruptDuration(usecInterruptDuration), mPulseLevel(
				pulse.getLevel() == PULSE_LEVEL::LO ? 0 : 1), mPulseDuration(
				pulse.getDuration()) {
}

TraceRecord::TraceRecord() :
		mUsecInteruptDuration(0), mPulseLevel(0), mPulseDuration(0) {
}

void TraceRecord::set(duration_t pulseDuration, PULSE_LEVEL pulseLevel,
		const duration_t usecInterruptDuration) {
	mUsecInteruptDuration = usecInterruptDuration;
	mPulseLevel = pulseLevel == PULSE_LEVEL::LO ? 0 : 1;
	mPulseDuration = pulseDuration;
}

} // namespace RcSwitch
