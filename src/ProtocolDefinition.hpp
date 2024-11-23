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

#ifndef _PROTOCOLDEFINITION_HPP_
#define _PROTOCOLDEFINITION_HPP_

#include <sys/types.h>
#include <stdint.h>

/*
 * The protocol is a stream of pulse pairs with different duration and pulse levels.
 * In the context of this documentation, the first pulse will be referenced as
 * "pulse A" and the second one as "pulse B".
 *
 *   Normal level protocols start with a high level:
 *          ___________________
 *	   XXXX|                   |____________________|XXXX
 *
 *   Inverse level protocols start with a low level:
 *                              ____________________
 *	   XXXX|___________________|                    |XXXX
 *
 *	       ^                   ^                    ^
 *         | pulse A duration  | pulse B duration   |
 *
 *
 *  In the synchronization phase there is a short pulse followed by a very long pulse:
 *     Normal level protocols:
 *          ____
 *     XXXX|    |_____________________________________________________________|XXXX
 *
 *     Inverse level protocols:
 *               _____________________________________________________________
 *     XXXX|____|                                                             |XXXX
 *
 *
 *  In the data phase there is
 *   a short pulse followed by a long pulse for a logical 0 data bit:
 *     Normal level protocols:
 *           __
 *     XXXXX|  |________|XXXX
 *
 *     Inverse level protocols:
 *             ________
 *     XXXX|__|        |XXXX
 *
 *   a long pulse followed by a short pulse for a logical 1 data bit:
 *     Normal level protocols:
 *          ________
 *     XXXX|        |__|XXXX
 *
 *     Inverse level protocols:
 *                   __
 *     XXXX|________|  |XXXX
 *
 *
 * Pulse durations sent out by a real world transmitter can vary. Hence the
 * specification contains upper and lower boundaries for a pulse to be
 * recognized as a valid synchronization pulse respectively data pulse.
 *
 * Synch. pulses and data pulses are typically a multiple of a protocol
 * specific clock cycle.
 *
 * There is a decision to be made, when a the reception of data bits shall be stopped,
 * because they constitute a completed message packet. Here is assumed, that the
 * transmitter transmits the same message packets multiple times in a row. The
 * completion of a message packet is noted, when new synch pulses from a subsequent
 * transmission appear.
 */

/**
 * makeTimingSpec
 * Calculate the pulse timings specification from a given protocol specification at compile time.
 * To be used in combination with RxProtocolTable.
 */
template<
	/** A protocol specification is given by the following parameters: */
	size_t protocolNumber,					/* A unique integer identifier for this protocol. */
	size_t clock,							/* The clock rate in microseconds.  */
	size_t percentTolerance,				/* The tolerance for a pulse length to be recognized as a valid. */
	size_t synchA,  size_t synchB,			/* Number of clocks for the synchronization pulse pair. */
	size_t data0_A, size_t data0_B,			/* Number of clocks for a logical 0 bit data pulse pair. */
	size_t data1_A, size_t data1_B,			/* Number of clocks for a logical 1 bit data pulse pair. */
	bool inverseLevel = false>				/* Flag whether pulse levels are normal or inverse. */
struct makeTimingSpec;

/**
 * RxProtocolTable provides an array of timing specifications from given protocol specifications.
 * The array gets sorted at compile time to keep the interrupt handler quick. Sort criteria are
 * the inverseLevel flag and the lowerBound of the synch A pulse.
 *
 * Usage example:
 *
 * static const RxProtocolTable <
 *  //                   #, clk,  %, syA,  syB,  d0A,d0B,  d1A,d1B , inverseLevel
 *  	makeTimingSpec<  1, 350, 20,   1,   31,    1,  3,    3,  1>, 		// ()
 *  	makeTimingSpec<  2, 650, 20,   1,   10,    1,  3,    3,  1>, 		// ()
 *  	makeTimingSpec<  3, 100, 20,  30,   71,    4, 11,    9,  6>, 		// ()
 *  	makeTimingSpec<  4, 380, 20,   1,    6,    1,  3,    3,  1>, 		// ()
 *  	makeTimingSpec<  5, 500, 20,   6,   14,    1,  2,    2,  1>, 		// ()
 *  	makeTimingSpec<  6, 450, 20,   1,   23,    1,  2,    2,  1, true>, 	// (HT6P20B)
 *  	makeTimingSpec<  7, 150, 20,   2,   62,    1,  6,    6,  1>, 		// (HS2303-PT)
 *  	makeTimingSpec<  8, 200, 20,   3,  130,    7, 16,    3, 16>, 		// (Conrad RS-200)
 *  	makeTimingSpec<  9, 365, 20,   1,   18,    3,  1,    1,  3, true>, 	// (1ByOne Doorbell)
 *  	makeTimingSpec< 10, 270, 20,   1,   36,    1,  2,    2,  1, true>, 	// (HT12E)
 *  	makeTimingSpec< 11, 320, 20,   1,   36,    1,  2,    2,  1, true>  	// (SM5212)
 *  > rxProtocolTable;
 */
template<typename ...TimingSpecs> struct RxProtocolTable;

#include "internal/ProtocolTimingSpec.inc"

#endif /* SRC_INTERNAL_PROTOCOLDEFINITION_HPP_ */
