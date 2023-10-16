//
// Copyright (C) 2004 Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#ifndef __INET_BOLT_H
#define __INET_BOLT_H

#include "inet/common/INETDefs.h"
#include "inet/transportlayer/tcp/flavours/SwiftFamily.h"


namespace inet {
namespace tcp {

/**
 * State variables for Bolt.
 */
typedef SwiftFamilyStateVariables BoltStateVariables;

/**
 * Implements TCP Reno.
 */
class INET_API Bolt : public SwiftFamily
{
  protected:

    static simsignal_t fcwndSignal;
    static simsignal_t ecwndSignal;
    static simsignal_t pacingDelaySignal;
    static simsignal_t pacingTimerExpiredSignal;
    static simsignal_t pacingTimerSetSignal;

    BoltStateVariables *& state;    // alias to TCLAlgorithm's 'state'

    // for siwft
    simtime_t t_last_decrease = 0;       // fabric last time decrease happened
    double pacing_delay = 0;

    // for bolt
    simtime_t last_dec_time = 0;

    unsigned int retransmit_cnt = 0;
    simtime_t most_recent_rtt = 0;
    simtime_t target_delay;

    /** Create and return a BoltStateVariables object. */
    virtual TcpStateVariables *createStateVariables() override
    {
        return new BoltStateVariables();
    }

  public:
    /** Ctor */
    Bolt();
    virtual void calculate_pacing_delay_after_cwnd_change(simtime_t now, bool cwnd_decreased);
    virtual void initialize() override;
    virtual void processRexmitTimer(TcpEventCode& event) override;
    virtual void restart_pacing_timer(int timer_type = -1);
    virtual void processPacingTimer(TcpEventCode& event, int timer_type) override;
    virtual void receivedSrc(Packet *packet, const Ptr<const TcpHeader>& tcpseg) override;
    virtual bool is_bolt() override;
    virtual void receivedDataAck(uint32 firstSeqAcked) override;
    virtual void rttMeasurementCompleteUsingTS(simtime_t echoedTS) override;
    virtual void receivedDuplicateAck() override;
    virtual bool sendData(bool sendCommandInvoked) override;
};

} // namespace tcp
} // namespace inet

#endif // ifndef __INET_SWIFT_H



/*
 * What should be implemented for Swift:
 * ACKs should be prioritized in switches
 * ACK Packets should piggyback the delays to the sender (This adds 4 bytes overhead to the packet headers)
 */
