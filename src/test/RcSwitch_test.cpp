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

#include "RcSwitch_test.hpp"
#include "../ProtocolDefinition.hpp"

#if ENABLE_RCSWITCH_TEST

#include <limits.h>
#include <assert.h>

namespace RcSwitch {

/** Call RcSwitch::RcSwitch_test::theTest.run() to execute tests. */
RcSwitch_test RcSwitch_test::theTest;

static const RxProtocolTable <
	//               #, clk,  %, syA,  syB,  d0A,d0B,  d1A,d1B, inverseLevel
	makeTimingSpec<  1, 350, 20,   1,   31,    1,  3,    3,  1, false>, // (PT2262)
	makeTimingSpec<  2, 650, 20,   1,   10,    1,  3,    3,  1, false>, // ()
	makeTimingSpec<  3, 100, 20,  30,   71,    4, 11,    9,  6, false>, // ()
	makeTimingSpec<  4, 380, 20,   1,    6,    1,  3,    3,  1, false>, // ()
	makeTimingSpec<  5, 500, 20,   6,   14,    1,  2,    2,  1, false>, // ()
	makeTimingSpec<  6, 450, 20,   1,   23,    1,  2,    2,  1, true>, 	// (HT6P20B)
	makeTimingSpec<  7, 150, 20,   2,   62,    1,  6,    6,  1, false>, // (HS2303-PT)
	makeTimingSpec<  8, 200, 20,   3,  130,    7, 16,    3, 16, false>, // (Conrad RS-200)
	makeTimingSpec<  9, 365, 20,   1,   18,    3,  1,    1,  3, true>, 	// (1ByOne Doorbell)
	makeTimingSpec< 10, 270, 20,   1,   36,    1,  2,    2,  1, true>, 	// (HT12E)
	makeTimingSpec< 11, 320, 20,   1,   36,    1,  2,    2,  1, true>  	// (SM5212)
> rxProtocolTable;

/** Message repeat is required for the message packet end detection */
constexpr size_t MIN_MSG_PACKET_REPEATS = 1;

static const TxDataBit validMessagePacket_A[] = {
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_1},
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_1},
		{DATA_BIT::LOGICAL_1},

		// delimiter
		{DATA_BIT::UNKNOWN},
};

static_assert( sizeof(validMessagePacket_A) / sizeof(validMessagePacket_A[0]) > MIN_MSG_PACKET_BITS,
		"Insufficient data bits for a valid message packet.");

static const TxDataBit validMessagePacket_B[] = {
		{DATA_BIT::LOGICAL_1},
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_1},
		{DATA_BIT::LOGICAL_1},
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_0},

		// delimiter
		{DATA_BIT::UNKNOWN},
};

static_assert( (sizeof(validMessagePacket_B) - 1 /* one less for the delimiter */) /
			sizeof(validMessagePacket_B[0]) >= MIN_MSG_PACKET_BITS,
		"Error: Insufficient data bits for a valid message packet.");

static const TxDataBit invalidMessagePacket_tooLessMessagePackteBits[] = {
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_1},

		// delimiter
		{DATA_BIT::UNKNOWN},
};

static_assert( (sizeof(invalidMessagePacket_tooLessMessagePackteBits) - 1 /* one less for the delimiter */)/
			sizeof(invalidMessagePacket_tooLessMessagePackteBits[0]) < MIN_MSG_PACKET_BITS,
		"Error: Sufficient data bits has made the invalid message packet valid.");


static const TxDataBit invalidMessagePacket_firstPulseTooShort[] = {
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_1},
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_0, 0.3, 1.0 /* Send a too short first pulse */},
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_1},
		{DATA_BIT::LOGICAL_1},

		// delimiter
		{DATA_BIT::UNKNOWN},
};

static const TxDataBit invalidMessagePacket_firstPulseTooLong[] = {
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_1},
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_0, 1.3, 1.0 /* Send a too long first pulse */},
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_1},
		{DATA_BIT::LOGICAL_1},

		// delimiter
		{DATA_BIT::UNKNOWN},
};

static const TxDataBit invalidMessagePacket_secondPulseTooShort[] = {
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_1},
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_0, 1.0, 0.3 /* Send a too short second pulse */},
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_1},
		{DATA_BIT::LOGICAL_1},

		// delimiter
		{DATA_BIT::UNKNOWN},
};

static const TxDataBit invalidMessagePacket_secondPulseTooLong[] = {
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_1},
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_0, 1.0, 1.3 /* Send a too long second pulse */},
		{DATA_BIT::LOGICAL_0},
		{DATA_BIT::LOGICAL_1},
		{DATA_BIT::LOGICAL_1},

		// delimiter
		{DATA_BIT::UNKNOWN},
};

template<unsigned int protocolNumber> class PulseLength {};

template<> struct PulseLength<1> {
	static constexpr uint32_t synchShortPulseLength =  1 * 350;
	static constexpr uint32_t synchLongPulseLength  = 31 * 350;
	static constexpr uint32_t dataShortPulseLength  =  1 * 350;
	static constexpr uint32_t dataLongPulseLength   =  3 * 350;
	/**
	 * The target level of the edge of the pulse.
	 */
	static constexpr uint32_t firstPulseEndLevel  =     0;
};

void RcSwitch_test::sendDataPulse(uint32_t &usec, Receiver &receiver, const uint32_t firstPulse
		, const uint32_t secondPulse, const uint32_t firstPulseEndLevel) {

	const uint32_t secondPulseEndLevel = not firstPulseEndLevel;

	usec += firstPulse;
	receiver.handleInterrupt(firstPulseEndLevel, usec);

	usec += secondPulse;
	receiver.handleInterrupt(secondPulseEndLevel, usec);
}

template<unsigned int protocolNumber> struct Protocol {
	static void sendLogical0(uint32_t &usec, Receiver &receiver,
			const double& firstPulseDurationFactor, const double& secondPulseDurationFactor) {

		const uint32_t firstPulseEndLevel  = PulseLength<protocolNumber>::firstPulseEndLevel;

		const uint32_t shortPulseLength = PulseLength<protocolNumber>::dataShortPulseLength;
		const uint32_t longPulseLength  = PulseLength<protocolNumber>::dataLongPulseLength;

		RcSwitch_test::sendDataPulse(usec, receiver,
				shortPulseLength * firstPulseDurationFactor,
				longPulseLength  * secondPulseDurationFactor,
				firstPulseEndLevel);
	}

	static void sendLogical1(uint32_t &usec, Receiver &receiver,
			const double& firstPulseDurationFactor, const double& secondPulseDurationFactor) {

		const uint32_t firstPulseEndLevel  = PulseLength<protocolNumber>::firstPulseEndLevel;

		const uint32_t shortPulseLength = PulseLength<protocolNumber>::dataShortPulseLength;
		const uint32_t longPulseLength  = PulseLength<protocolNumber>::dataLongPulseLength;

		RcSwitch_test::sendDataPulse(usec, receiver,
				longPulseLength  * firstPulseDurationFactor,
				shortPulseLength * secondPulseDurationFactor,
				firstPulseEndLevel);
	}

	static void sendSynchPulses(uint32_t& usec, Receiver& receiver) {
		const uint32_t firstPulseEndLevel  = PulseLength<protocolNumber>::firstPulseEndLevel;
		const uint32_t secondPulseEndLevel = not firstPulseEndLevel;

		usec += PulseLength<protocolNumber>::synchShortPulseLength;
		RcSwitch_test::handleInterrupt(receiver, firstPulseEndLevel, usec);

		usec += PulseLength<protocolNumber>::synchLongPulseLength;
		RcSwitch_test::handleInterrupt(receiver, secondPulseEndLevel, usec);
	}

	static void sendDataBit(uint32_t &usec, Receiver &receiver, const TxDataBit* const dataBit) {
		switch(dataBit->mDataBit) {
			case DATA_BIT::LOGICAL_0:
				sendLogical0(usec, receiver,
						dataBit->mFirstPulseDurationFactor, dataBit->mSecondPulseDurationFactor);
				break;
			case DATA_BIT::LOGICAL_1:
				sendLogical1(usec, receiver,
						dataBit->mFirstPulseDurationFactor, dataBit->mSecondPulseDurationFactor);
				break;
			case DATA_BIT::UNKNOWN:
				break;
		}
	}
};

void RcSwitch_test::sendMessagePacket(uint32_t &usec, Receiver &receiver
		, const TxDataBit* const dataBits, const size_t count) const {

	for(size_t i = 0; i < count; i++) {
		Protocol<1>::sendSynchPulses(usec, receiver);

		if(!receiver.available()) {
			for(size_t j = 0; dataBits[j].mDataBit != DATA_BIT::UNKNOWN; j++) {
				Protocol<1>::sendDataBit(usec, receiver, &dataBits[j]);
			}
		}
	}
}

void RcSwitch_test::faultyMessagePacketTest(uint32_t& usec, Receiver &receiver,
		const TxDataBit* const faultyMessagePacket) const {
	{
		// Send faulty message
		sendMessagePacket(usec, receiver, faultyMessagePacket, 1);

		assert(receiver.state() == Receiver::SYNC_STATE);

		assert(receiver.mProtocolCandidates.size() == 0);
	}
	{
		// Send valid message
		sendMessagePacket(usec, receiver, validMessagePacket_A, MIN_MSG_PACKET_REPEATS + 1);
		assert(receiver.available());
		const uint32_t receivedValue = receiver.receivedValue();
		/* binary: 010011 */
		assert(receivedValue == 0x13 /* binary: 010011 */);
	}
}

void RcSwitch_test::tooShortMessagePacketTest(uint32_t& usec, Receiver &receiver) const {
	{
		// Send too short message

		sendMessagePacket(usec, receiver, invalidMessagePacket_tooLessMessagePackteBits, 1);
		assert(receiver.state() == Receiver::DATA_STATE);
		assert(receiver.mProtocolCandidates.size() > 0);

		// Send valid message
		sendMessagePacket(usec, receiver, validMessagePacket_A, MIN_MSG_PACKET_REPEATS + 1);
		assert(receiver.available());
		const uint32_t receivedValue = receiver.receivedValue();
		/* binary: 010011 */
		assert(receivedValue == 0x13 /* binary: 010011 */);
	}
}

void RcSwitch_test::testFaultyDataRx() const {
	Receiver receiver;
	receiver.setRxTimingSpecTable(rxProtocolTable.toTimingSpecTable());
	uint32_t usec = 0;

	usec += 100; // start hi pulse 100 usec duration.
	receiver.handleInterrupt(not PulseLength<1>::firstPulseEndLevel, usec);

	faultyMessagePacketTest(usec, receiver, invalidMessagePacket_firstPulseTooShort);
	receiver.reset();
	faultyMessagePacketTest(usec, receiver, invalidMessagePacket_firstPulseTooLong);
	receiver.reset();
	faultyMessagePacketTest(usec, receiver, invalidMessagePacket_secondPulseTooShort);
	receiver.reset();
	faultyMessagePacketTest(usec, receiver, invalidMessagePacket_secondPulseTooLong);
	receiver.reset();
	tooShortMessagePacketTest(usec, receiver);
	receiver.reset();
}

void RcSwitch_test::testDataRx() const {
	Receiver receiver;
	receiver.setRxTimingSpecTable(rxProtocolTable.toTimingSpecTable());
	uint32_t usec = 0;

	usec += 100; // start hi pulse 100 usec duration.
	receiver.handleInterrupt(not PulseLength<1>::firstPulseEndLevel, usec);

	{ // Send valid message
		sendMessagePacket(usec, receiver, validMessagePacket_A, MIN_MSG_PACKET_REPEATS + 1);
		assert(receiver.available());

		const uint32_t receivedValue = receiver.receivedValue();
		assert(receivedValue == 0x13 /* binary: 010011 */); // Confirm received value.
	}

	{ // Send a different valid message, without resetting old received value.
		sendMessagePacket(usec, receiver, validMessagePacket_B, MIN_MSG_PACKET_REPEATS + 1);
		assert(receiver.available());

		const uint32_t receivedValue = receiver.receivedValue();
		assert(receivedValue == 0x13 /* binary: 010011 */); // Confirm old received value.
	}

	{ // Send a different valid message, with resetting the old received before.
		receiver.reset();
		sendMessagePacket(usec, receiver, validMessagePacket_B, MIN_MSG_PACKET_REPEATS + 1);
		assert(receiver.available());

		const uint32_t receivedValue = receiver.receivedValue();
		assert(receivedValue == 0x2C /* binary: 101100 */); // Confirm new received value.
	}
}

void RcSwitch_test::testSynchRx() const {
	Receiver receiver;
	receiver.setRxTimingSpecTable(rxProtocolTable.toTimingSpecTable());
	uint32_t usec = 0;

	usec += 100;  // lo pulse  100 usec duration.
	receiver.handleInterrupt(1, usec);

	usec += 300;  // hi pulse 300 usec duration.
	receiver.handleInterrupt(0, usec);

	usec += 2736; // lo pulse 2736 usec duration, too long for any protocol.
	receiver.handleInterrupt(1, usec);
	assert(receiver.state() == Receiver::SYNC_STATE);

	usec += 300;  // hi pulse 300 usec duration.
	receiver.handleInterrupt(0, usec);

	usec += 9300; // lo pulse 9300 usec duration, match protocol #1 and #7.
	receiver.handleInterrupt(1, usec);
	assert(receiver.state() == Receiver::DATA_STATE);
}

void RcSwitch_test::testProtocolCandidates() const {
	Receiver receiver;
	receiver.setRxTimingSpecTable(rxProtocolTable.toTimingSpecTable());

	Pulse pulse_0 = {				// Hi level pulse too short
			239, PULSE_LEVEL::HI
	};

	Pulse pulse_1 = {				// Lo level pulse within protocol #1 and #7
			10850, PULSE_LEVEL::LO
	};

	receiver.collectProtocolCandidates(pulse_0, pulse_1);
	assert(receiver.mProtocolCandidates.size() == 0); 			// No matching protocol

	pulse_0.setDuration(280);
	receiver.collectProtocolCandidates(pulse_0, pulse_1);
	assert(receiver.mProtocolCandidates.size() == 2); 					// Match protocol #1 and #7

	for(size_t i = 0; i < receiver.mProtocolCandidates.size(); i++) {  	// Check protocol candidates
		static const unsigned int expectedProtocols[] = {7,1};
		assert(receiver.getProtcolNumber(i) == expectedProtocols[i]);
	}

	receiver.mProtocolCandidates.reset();						// Remove protocol candidates
	pulse_1.setDuration(7439);							// Lo level pulse too short
	receiver.collectProtocolCandidates(pulse_0, pulse_1);
	assert(receiver.mProtocolCandidates.size() == 0); 			// No matching protocol

	pulse_0.setDuration(360);							// Hi level pulse matches protocols #1 and #4, Lo pulse still too short.
	receiver.collectProtocolCandidates(pulse_0, pulse_1);
	assert(receiver.mProtocolCandidates.size() == 0); 			// No matching protocol

	pulse_1.setDuration(2735);							// Lo level pulse matches protocol #4
	receiver.collectProtocolCandidates(pulse_0, pulse_1);
	assert(receiver.mProtocolCandidates.size() == 1); 			// No matching protocol
	for(size_t i = 0; i < receiver.mProtocolCandidates.size(); i++) {  // Check protocol candidates
		static const unsigned int expectedProtocols[] = {4};
		assert(receiver.getProtcolNumber(i) == expectedProtocols[i]);
	}
}

void RcSwitch_test::testStackBuffer() const {
	constexpr int start = -2;
	constexpr int end = 3;

	StackBuffer <int, end - start> blockingStack;

	int e = start;
	for(; e < end; e++) { 										// fill stack elements with -2 .. 3.
		assert(blockingStack.size() == static_cast<size_t>(e-start));
		blockingStack.push(e);
		assert(blockingStack[e-start] == e);
		assert(blockingStack.overflowCount() == 0);
	}

	assert(blockingStack.size() == end-start); 					// stack should be full.
	assert(blockingStack.push(e) == false); 					// element should be dropped.
	assert(blockingStack.size() == blockingStack.capacity); 	// stack should still be full.
	assert(blockingStack.overflowCount() == 1); 				// overflow should be raised.

	blockingStack.remove(2); 									// remove the middle element.
	assert(blockingStack.size() == blockingStack.capacity-1); 	// expect one element less.

	for(size_t i = 0; i < blockingStack.size(); i++) {      	// check the remaining elements.
		static const int expected[] = {-2,-1,1,2};
		assert(blockingStack[i] == expected[i]);
	}

	assert(blockingStack.overflowCount() == 1); 					// overflow should still be raised.
	assert(blockingStack.size() == blockingStack.capacity - 1);
	blockingStack.push(e);   										// push should be successful.
	assert(blockingStack.size() == blockingStack.capacity);
	blockingStack.reset(); 											// remove all elements.
	assert(blockingStack.size() == 0); 								// size should be zero.
	assert(blockingStack.overflowCount() == 0); 					// overflow should be reset.
}

void RcSwitch_test::testRingBuffer() const {
	constexpr int start = -2;
	constexpr int end = 3;

	RingBuffer<int, end - start> overwritingStack;

	int e = start;
	for(; e < end; e++) { 											// fill stack elements with -2 .. 3.
		assert(overwritingStack.size() == static_cast<size_t>(e-start));
		overwritingStack.push(e);
		assert(overwritingStack.at(e-start) == e);
	}

	assert(overwritingStack.size() == end-start); 					// stack should be full.
	overwritingStack.push(e++); 															  // element should overwrite the oldest one.
	assert(overwritingStack.size() == overwritingStack.capacity); 	// stack should still be full.

	for(size_t i = 0; i < overwritingStack.size(); i++) {      		// check the elements.
		static const int expected[] = {-1,0,1,2,3};
		assert(overwritingStack.at(i) == expected[i]);
	}

	overwritingStack.push(e++); 									// element should overwrite the oldest one.
	assert(overwritingStack.size() == overwritingStack.capacity); 	// stack should still be full.

	for(size_t i = 0; i < overwritingStack.size(); i++) {      		// check the elements.
		static const int expected[] = {0,1,2,3,4};
		assert(overwritingStack.at(i) == expected[i]);
	}
}

} /* namespace RcSwitch */

#endif // ENABLE_RCSWITCH_TEST
