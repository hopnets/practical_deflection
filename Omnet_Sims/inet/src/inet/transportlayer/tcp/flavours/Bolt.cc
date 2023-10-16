//
// Copyright (C) 2004-2005 Andras Varga
// Copyright (C) 2009 Thomas Reschka
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

#include <algorithm>    // min,max

#include "inet/transportlayer/tcp/Tcp.h"
#include "inet/transportlayer/tcp/flavours/Bolt.h"
#include <math.h>

#define MIN_CWND_PACKET_NUMBER 0.01    // In bolt, this is 15 bytes which is almost 0.01 packets
#define MAX_CWND_PACKET_NUMBER 43      // todo: borrowed from TCP. around 65536 bytes
#define RETX_RESET_THRESHOLD 100    //todo: I don't know what the value actually is

#define ENDPOINT 0
#define FABRIC 1

//PACING TYPES
#define RETRANSMISSION 0
#define NORMAL 1

#define M_MIN_CWND  15 // bytes

namespace inet {
namespace tcp {

Register_Class(Bolt);

simsignal_t Bolt::pacingTimerExpiredSignal = cComponent::registerSignal("pacingTimerExpired");
simsignal_t Bolt::pacingTimerSetSignal = cComponent::registerSignal("pacingTimerSet");
simsignal_t Bolt::pacingDelaySignal = cComponent::registerSignal("pacingDelay");
simsignal_t Bolt::fcwndSignal = cComponent::registerSignal("fcwnd");
simsignal_t Bolt::ecwndSignal = cComponent::registerSignal("ecwnd");

Bolt::Bolt() : SwiftFamily(),
    state((BoltStateVariables *&)TcpAlgorithm::state)
{
}

void Bolt::initialize()
{
    TcpBaseAlg::initialize();
    pacingTimer->addPar("pacing_type") = -1;
    double cwnd_new_packet_num;

    if (state->use_custom_IW) {
        cwnd_new_packet_num = state->custom_IW_mult;
    } else {
        cwnd_new_packet_num = 1;
    }

    t_last_decrease = 0;

    // turn cwnd back to byte
    state->snd_cwnd = cwnd_new_packet_num * state->snd_mss;


    EV << "Bolt initialize: cwnd in bytes is " << state->snd_cwnd << endl;
}

void Bolt::calculate_pacing_delay_after_cwnd_change(simtime_t now, bool cwnd_decreased) {
    EV << "Bolt::set_final_cwnd_values called." << endl;

    if (cwnd_decreased) {
        t_last_decrease = now;
        EV << "last_decrease changed to " << now << endl;
    }

    if (state->snd_cwnd < state->snd_mss) {
        double cwnd_new_packet_num = state->snd_cwnd * 1.0 / state->snd_mss;
        pacing_delay = (most_recent_rtt.dbl()) / cwnd_new_packet_num;
    }
    else
        pacing_delay = 0;

    EV << "Pacing delay: " << pacing_delay << ", and cwnd: " << state->snd_cwnd << endl;

    if (state->snd_cwnd < state->snd_mss && pacing_delay == 0) {
        state->snd_cwnd = state->snd_mss;
        EV << "No rtt is yet recorded! Setting cwnd to " << state->snd_cwnd << endl;

    }

    uint32 max_cwnd_bytes = MAX_CWND_PACKET_NUMBER * state->snd_mss;
    state->snd_cwnd = std::max((uint32)M_MIN_CWND, std::min(state->snd_cwnd, max_cwnd_bytes));

    EV << "cwnd in bytes is " << state->snd_cwnd << endl;

    conn->emit(pacingDelaySignal, pacing_delay);
    conn->emit(cwndSignal, state->snd_cwnd);
}

void Bolt::processPacingTimer(TcpEventCode& event, int timer_type) {
    EV << "Bolt::processPacingTimer called." << endl;
    conn->emit(pacingTimerExpiredSignal, timer_type);
    if (timer_type == RETRANSMISSION) {
        EV << "Timer expired is retransmission" << endl;
        state->afterRto = true;
        conn->retransmitOneSegment(true);
    } else if (timer_type == NORMAL) {
        EV << "Timer expired is pacing" << endl;
        sendData(false);
    } else {
        throw cRuntimeError("Pacing timer expired, unknown timer type!");
    }
}

void Bolt::processRexmitTimer(TcpEventCode& event)
{
    EV << "Bolt::processRexmitTimer called." << endl;
    SwiftFamily::processRexmitTimer(event);

    if (event == TCP_E_ABORT)
        return;

    bool cwnd_decreased = false;
    simtime_t now = simTime();
    bool can_decrease;
    if (t_last_decrease <= 0)
        can_decrease = true;
    else
        can_decrease = ((now - t_last_decrease) >= most_recent_rtt);
    EV << "last_decrease is " << t_last_decrease << ", most_recent_rtt is " << most_recent_rtt
            << ", can_decrease: " << can_decrease << endl;

    retransmit_cnt++;
    if (retransmit_cnt >= RETX_RESET_THRESHOLD) {
        cwnd_decreased = true;
        EV << "retransmit_cnt >= RETX_RESET_THRESHOLD, reseting the e-/f-cwnds to MIN_CWND_PACKET_NUMBER" << endl;
        state->snd_cwnd = MIN_CWND_PACKET_NUMBER * state->snd_mss;
    } else {
        if (can_decrease) {
            cwnd_decreased = true;
            EV << "retransmit_cnt < RETX_RESET_THRESHOLD" << endl;
            state->snd_cwnd *= (1.0 - state->max_mdf);
        }
    }

    calculate_pacing_delay_after_cwnd_change(now, cwnd_decreased);


    // From here it's coppied from TCP

    // if pacing_delay < 1, we should pace retransmission
    if (pacing_delay > 0) {
        restart_pacing_timer(RETRANSMISSION);
        return;
    }

    state->afterRto = true;
    conn->retransmitOneSegment(true);
}

void Bolt::restart_pacing_timer(int timer_type) {
    EV << "Bolt::restart_pacing_timer called." << endl;
    if (pacingTimer->isScheduled())
        cancelEvent(pacingTimer);
    if (pacing_delay > 0) {
        simtime_t target_time = simTime() + pacing_delay;
        conn->emit(pacingTimerSetSignal, target_time);
        EV << "Setting the pacing timer to " << target_time << endl;
        pacingTimer->par("pacing_type") = timer_type;
        conn->scheduleAt(target_time, pacingTimer);
    }
}

bool Bolt::sendData(bool sendCommandInvoked)
{
    EV << "Bolt::sendData called." << endl;
    //
    // Nagle's algorithm: when a TCP connection has outstanding data that has not
    // yet been acknowledged, small segments cannot be sent until the outstanding
    // data is acknowledged. (In this case, small amounts of data are collected
    // by TCP and sent in a single segment.)
    //
    // FIXME there's also something like this: can still send if
    // "b) a segment that can be sent is at least half the size of
    // the largest window ever advertised by the receiver"

    bool fullSegmentsOnly = sendCommandInvoked && state->nagle_enabled && state->snd_una != state->snd_max;

    if (fullSegmentsOnly)
        EV_INFO << "Nagle is enabled and there's unacked data: only full segments will be sent\n";

    // RFC 2581, pages 7 and 8: "When TCP has not received a segment for
    // more than one retransmission timeout, cwnd is reduced to the value
    // of the restart window (RW) before transmission begins.
    // For the purposes of this standard, we define RW = IW.
    // (...)
    // Using the last time a segment was received to determine whether or
    // not to decrease cwnd fails to deflate cwnd in the common case of
    // persistent HTTP connections [HTH98].
    // (...)
    // Therefore, a TCP SHOULD set cwnd to no more than RW before beginning
    // transmission if the TCP has not sent data in an interval exceeding
    // the retransmission timeout."
    if (!conn->isSendQueueEmpty()) {    // do we have any data to send?
        if ((simTime() - state->time_last_data_sent) > state->rexmit_timeout) {
            // RFC 5681, page 11: "For the purposes of this standard, we define RW = min(IW,cwnd)."
            if (state->increased_IW_enabled)
                state->snd_cwnd = std::min(std::min(4 * state->snd_mss, std::max(2 * state->snd_mss, (uint32)4380)), state->snd_cwnd);
            else if (state->use_custom_IW) {
                state->snd_cwnd = state->custom_IW_mult * state->snd_mss;
                EV << "SEPEHR: Using custom IW with mult: " << state->custom_IW_mult <<
                        ", so the snd_cwnd is " << state->snd_cwnd << endl;
            }
            else
                state->snd_cwnd = state->snd_mss;

            EV << "Restarting idle connection, CWND is set to " << state->snd_cwnd << "\n";

        }
    }

    //
    // Send window is effectively the minimum of the congestion window (cwnd)
    // and the advertised window (snd_wnd).
    //

    // this might have been triggered after pacing
    if (pacing_delay > 0) {
        EV << "sendData is triggered after pacing delay" << endl;
        return conn->sendData(fullSegmentsOnly, state->snd_mss);
    }

    return conn->sendData(fullSegmentsOnly, state->snd_cwnd);
}

bool Bolt::is_bolt() {
    return true;
}

void Bolt::receivedSrc(Packet *packet, const Ptr<const TcpHeader>& tcpseg) {
    EV << "src packet received " << packet->str() << endl;
    EV << "old_cwnd: " << state->snd_cwnd << endl;

    simtime_t now = simTime();

    if (state->snd_cwnd <= state->snd_mss) {
        state->snd_cwnd *= (1.0 - state->max_mdf);
    } else {

        auto packet_dup = packet->dup();
        b position = packet_dup->getFrontOffset();
        auto tcp_header = packet->peekAtFront<tcp::TcpHeader>();

        double rtt_src = (now - tcp_header->getBolt_tx_time()).dbl();
        if (rtt_src < 0)
            throw cRuntimeError("rtt_src < 0");
        EV << "rtt_src is " << rtt_src << endl;
        double flow_rate;
        if (most_recent_rtt != 0) {
            flow_rate = (state->snd_cwnd * 8.0) / most_recent_rtt.dbl();
            EV << "most_recent_rtt: " << most_recent_rtt.dbl() << endl;
        } else {
            flow_rate = state->server_link_rate;
    //        throw cRuntimeError("most_recent_rtt is zero");
        }
        if (flow_rate < 0)
            throw cRuntimeError("flow_rate < 0");
        if ((state->snd_nxt - state->iss != 0) && flow_rate == 0)
            throw cRuntimeError("(state->snd_nxt - state->iss != 0) && flow_rate == 0");
        EV << "flow_rate is " << flow_rate << endl;
        EV << "link utilization: " << tcp_header->getLink_util() << endl;
        double reaction_factor = flow_rate / tcp_header->getLink_util();
        EV << "reaction_factor is " << reaction_factor << endl;
        EV << "queue occupancy packet num: " << tcp_header->getQueue_occupancy_packet_num() << endl;
        double target_q = tcp_header->getQueue_occupancy_packet_num() * 1.0 / reaction_factor;
        EV << "target_q is " << target_q << endl;
        EV << "last_dec_time: " << last_dec_time << endl;
        if (rtt_src / target_q <= (now - last_dec_time).dbl()) {
            EV << "reducing cwnd..." << endl;
            state->snd_cwnd -= state->snd_mss;
            last_dec_time = now;
        }
        delete packet_dup;
    }

    calculate_pacing_delay_after_cwnd_change(now, true);
}

void Bolt::receivedDataAck(uint32 firstSeqAcked)
{
    EV << "Bolt::receivedDataAck called." << endl;
    SwiftFamily::receivedDataAck(firstSeqAcked);
    EV << "receivedDataAck called in Bolt. end_point_delay is: " << state->end_point_delay << " and fabric_delay is: " <<
            state->fabric_delay << endl;

    simtime_t now = simTime();
    bool cwnd_decreased = false;
    bool can_decrease;
    if (t_last_decrease <= 0)
        can_decrease = true;
    else
        can_decrease = ((now - t_last_decrease) >= most_recent_rtt);
    uint32 BytesAcked = state -> snd_una - firstSeqAcked;
    int packets_akced = ceil(((double)BytesAcked / state->snd_mss));

    EV << "last_decrease is " << t_last_decrease << ", most_recent_rtt is " << most_recent_rtt
                << ", can_decrease: " << can_decrease << endl;

    EV << "BytesAcked is " << BytesAcked << ", and packets_akced is " << packets_akced << endl;

    if (state->dupacks >= DUPTHRESH) {    // DUPTHRESH = 3
        //
        // Perform Fast Recovery.
        //
        // This part is the same for swift and bolt
        retransmit_cnt = 0;
        if (!state->FRs_disabled) {
            EV_INFO << "Swfit Fast Recovery:" << endl;
            if (can_decrease) {
                EV << "Multiplying e-/f-cwnds by 1 - state->max_mdf: " << (1 - state->max_mdf) << endl;
                state->snd_cwnd *= (1.0 - state->max_mdf);
                cwnd_decreased = true;
            }
            conn->tcpMain->num_fast_fast_recoveries++;
        } else
            EV << "Fast Recovery disabled" << endl;
    }
    else {
        retransmit_cnt = 0;
        EV << "Old cwnd: " << state->snd_cwnd << endl;
        auto tcp_header = state->last_ack_packet_rcvd->peekAtFront<tcp::TcpHeader>();
        if (tcp_header->getBolt_inc()) {
            EV << "Inc is on..." << endl;
            state->snd_cwnd += state->snd_mss;
        }

        if (tcp_header->getAckNo() > state->seq_no_at_last_AI) {
            EV << "Increasing cwnd based on AI" << endl;
            state->snd_cwnd += (state->ai * state->snd_mss);
            state->seq_no_at_last_AI = state->snd_nxt;
        }
    }

    calculate_pacing_delay_after_cwnd_change(now, cwnd_decreased);



    if (state->sack_enabled && state->lossRecovery) {
        // RFC 3517, page 7: "Once a TCP is in the loss recovery phase the following procedure MUST
        // be used for each arriving ACK:
        //
        // (A) An incoming cumulative ACK for a sequence number greater than
        // RecoveryPoint signals the end of loss recovery and the loss
        // recovery phase MUST be terminated.  Any information contained in
        // the scoreboard for sequence numbers greater than the new value of
        // HighACK SHOULD NOT be cleared when leaving the loss recovery
        // phase."
        if (seqGE(state->snd_una, state->recoveryPoint)) {
            EV_INFO << "Loss Recovery terminated.\n";
            state->lossRecovery = false;
        }
        // RFC 3517, page 7: "(B) Upon receipt of an ACK that does not cover RecoveryPoint the
        //following actions MUST be taken:
        //
        // (B.1) Use Update () to record the new SACK information conveyed
        // by the incoming ACK.
        //
        // (B.2) Use SetPipe () to re-calculate the number of octets still
        // in the network."
        else {
            // update of scoreboard (B.1) has already be done in readHeaderOptions()
            conn->setPipe();

            // RFC 3517, page 7: "(C) If cwnd - pipe >= 1 SMSS the sender SHOULD transmit one or more
            // segments as follows:"
            if (((int)state->snd_cwnd - (int)state->pipe) >= (int)state->snd_mss) // Note: Typecast needed to avoid prohibited transmissions
                conn->sendDataDuringLossRecoveryPhase(state->snd_cwnd);
        }
    }

    // RFC 3517, pages 7 and 8: "5.1 Retransmission Timeouts
    // (...)
    // If there are segments missing from the receiver's buffer following
    // processing of the retransmitted segment, the corresponding ACK will
    // contain SACK information.  In this case, a TCP sender SHOULD use this
    // SACK information when determining what data should be sent in each
    // segment of the slow start.  The exact algorithm for this selection is
    // not specified in this document (specifically NextSeg () is
    // inappropriate during slow start after an RTO).  A relatively
    // straightforward approach to "filling in" the sequence space reported
    // as missing should be a reasonable approach."

    // the following stops timer if pacing delay <= 0
    restart_pacing_timer(NORMAL);
    if (pacing_delay > 0) {
        return;
    }

    sendData(false);

}


void Bolt::receivedDuplicateAck()
{
    EV << "Bolt::receivedDuplicateAck called." << endl;
    simtime_t now = simTime();
    if (!state->FRs_disabled) {
        SwiftFamily::receivedDuplicateAck();

        if (state->dupacks == DUPTHRESH) {    // DUPTHRESH = 3
            EV_INFO << "Reno on dupAcks == DUPTHRESH(=3): perform Fast Retransmit, and enter Fast Recovery:";

            if (state->sack_enabled) {
                // RFC 3517, page 6: "When a TCP sender receives the duplicate ACK corresponding to
                // DupThresh ACKs, the scoreboard MUST be updated with the new SACK
                // information (via Update ()).  If no previous loss event has occurred
                // on the connection or the cumulative acknowledgment point is beyond
                // the last value of RecoveryPoint, a loss recovery phase SHOULD be
                // initiated, per the fast retransmit algorithm outlined in [RFC2581].
                // The following steps MUST be taken:
                //
                // (1) RecoveryPoint = HighData
                //
                // When the TCP sender receives a cumulative ACK for this data octet
                // the loss recovery phase is terminated."

                // RFC 3517, page 8: "If an RTO occurs during loss recovery as specified in this document,
                // RecoveryPoint MUST be set to HighData.  Further, the new value of
                // RecoveryPoint MUST be preserved and the loss recovery algorithm
                // outlined in this document MUST be terminated.  In addition, a new
                // recovery phase (as described in section 5) MUST NOT be initiated
                // until HighACK is greater than or equal to the new value of
                // RecoveryPoint."
                if (state->recoveryPoint == 0 || seqGE(state->snd_una, state->recoveryPoint)) {    // HighACK = snd_una
                    state->recoveryPoint = state->snd_max;    // HighData = snd_max
                    state->lossRecovery = true;
                    EV_DETAIL << " recoveryPoint=" << state->recoveryPoint;
                }
            }
            // RFC 2581, page 5:
            // "After the fast retransmit algorithm sends what appears to be the
            // missing segment, the "fast recovery" algorithm governs the
            // transmission of new data until a non-duplicate ACK arrives.
            // (...) the TCP sender can continue to transmit new
            // segments (although transmission must continue using a reduced cwnd)."

            // enter Fast Recovery
            bool cwnd_decreased = false;
            bool can_decrease;
            if (t_last_decrease <= 0)
                can_decrease = true;
            else
                can_decrease = ((now - t_last_decrease) >= most_recent_rtt);
            if (can_decrease) {
                cwnd_decreased = true;
                state->snd_cwnd *= (1.0 - state->max_mdf);
            }
            calculate_pacing_delay_after_cwnd_change(now, true);

            EV_DETAIL << " set cwnd=" << state->snd_cwnd << endl;

            // Fast Retransmission: retransmit missing segment without waiting
            // for the REXMIT timer to expire
            if (pacing_delay > 0)
                restart_pacing_timer(RETRANSMISSION);
            else
                conn->retransmitOneSegment(false);

            // Do not restart REXMIT timer.
            // Note: Restart of REXMIT timer on retransmission is not part of RFC 2581, however optional in RFC 3517 if sent during recovery.
            // Resetting the REXMIT timer is discussed in RFC 2582/3782 (NewReno) and RFC 2988.

            if (state->sack_enabled) {
                // RFC 3517, page 7: "(4) Run SetPipe ()
                //
                // Set a "pipe" variable  to the number of outstanding octets
                // currently "in the pipe"; this is the data which has been sent by
                // the TCP sender but for which no cumulative or selective
                // acknowledgment has been received and the data has not been
                // determined to have been dropped in the network.  It is assumed
                // that the data is still traversing the network path."
                conn->setPipe();
                // RFC 3517, page 7: "(5) In order to take advantage of potential additional available
                // cwnd, proceed to step (C) below."
                if (state->lossRecovery) {
                    // RFC 3517, page 9: "Therefore we give implementers the latitude to use the standard
                    // [RFC2988] style RTO management or, optionally, a more careful variant
                    // that re-arms the RTO timer on each retransmission that is sent during
                    // recovery MAY be used.  This provides a more conservative timer than
                    // specified in [RFC2988], and so may not always be an attractive
                    // alternative.  However, in some cases it may prevent needless
                    // retransmissions, go-back-N transmission and further reduction of the
                    // congestion window."
                    // Note: Restart of REXMIT timer on retransmission is not part of RFC 2581, however optional in RFC 3517 if sent during recovery.
                    EV_INFO << "Retransmission sent during recovery, restarting REXMIT timer.\n";
                    restartRexmitTimer();

                    // RFC 3517, page 7: "(C) If cwnd - pipe >= 1 SMSS the sender SHOULD transmit one or more
                    // segments as follows:"
                    if (((int)state->snd_cwnd - (int)state->pipe) >= (int)state->snd_mss) // Note: Typecast needed to avoid prohibited transmissions
                        conn->sendDataDuringLossRecoveryPhase(state->snd_cwnd);
                }
            }

            // try to transmit new segments (RFC 2581)
            conn->tcpMain->num_fast_retransmits++;
            // TODO: I'm not sure if this is true. Should we only do pacing for retransmission or what?

        } else {
            state->snd_cwnd += (state->ai * state->snd_mss);
            calculate_pacing_delay_after_cwnd_change(now, false);
        }
        if (pacing_delay <= 0)
            sendData(false);
    } else
        EV << "Fast Retransmit disabled!" << endl;
}

void Bolt::rttMeasurementCompleteUsingTS(simtime_t echoedTS)
{
    EV << "Bolt::rttMeasurementCompleteUsingTS called." << endl;

//    if (!state->sack_enabled)
//        throw cRuntimeError("Sack must be enabled for Bolt!");

    ASSERT(state->ts_enabled);

    simtime_t tSent = echoedTS;
    simtime_t tAcked = simTime();
    most_recent_rtt = tAcked - tSent;
    // TODO change this when we have endpoint delay
//    state->fabric_delay = most_recent_rtt - state->end_point_delay;
    state->fabric_delay = most_recent_rtt;
    if (state->fabric_delay <= 0)
        throw cRuntimeError("state->fabric_delay <= 0");
    EV << "Bolt: RTT is " << most_recent_rtt << " and end_point_delay is: " << state->end_point_delay << " so fabric_delay is " <<
            state->fabric_delay << endl;
    rttMeasurementComplete(tSent, tAcked);
}


} // namespace tcp
} // namespace inet
