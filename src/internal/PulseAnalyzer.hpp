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

#ifndef RCSWITCH_RECEIVER_INTERNAL_PULSE_ANALYZER_HPP_
#define RCSWITCH_RECEIVER_INTERNAL_PULSE_ANALYZER_HPP_

#include <stddef.h>

#include "Pulse.hpp"
#include "ISR_ATTR.hpp"
#include "Container.hpp"

namespace RcSwitch {

static constexpr size_t MAX_PULSE_CATEGORIES = 6;

class PulseAnalyzer : public StackBuffer<PulseCategorie, MAX_PULSE_CATEGORIES> {
	using baseClass = StackBuffer<PulseCategorie, MAX_PULSE_CATEGORIES>;
	const size_t mPercentTolerance;

	bool isOfCategorie(const PulseCategorie &categorie, const Pulse &pulse) const;
	size_t findCategorie(const Pulse &pulse) const;

public:
	PulseAnalyzer(size_t percentTolerance = 10);
	bool addPulse(const Pulse &pulse);
};

} /* namespace RcSwitch */

#endif /* RCSWITCH_RECEIVER_INTERNAL_PULSE_ANALYZER_HPP_ */
