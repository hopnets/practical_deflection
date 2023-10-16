/*
 * Copyright (C) 2003 Andras Varga; CTIE, Monash University, Australia
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __INET_AUGMENTED_ETHERMAC_H
#define __INET_AUGMENTED_ETHERMAC_H

#include "inet/common/INETDefs.h"
#include "inet/common/packet/Packet.h"
#include "inet/linklayer/ethernet/EtherMacBase.h"
#include "../V2/V2PIFO.h"
#include "../pFabric/pFabric.h"
#include "../V2/buffer/V2PacketBuffer.h"
#include "../V2/V2PIFOPrioQueue.h"
#include "../V2/V2PIFOCanaryQueue.h"
#include "../V2/V2PIFOBoltQueue.h"


namespace inet {
    class EthernetJamSignal;
    class EtherPauseFrame;
}

using namespace inet;

/**
 * Ethernet MAC module which supports both half-duplex (CSMA/CD) and full-duplex
 * operation. (See also EtherMacFullDuplex which has a considerably smaller
 * code with all the CSMA/CD complexity removed.)
 *
 * See NED file for more details.
 */
class INET_API AugmentedEtherMac : public EtherMacBase
{
  public:
    AugmentedEtherMac() {}
    virtual ~AugmentedEtherMac();
    bool is_queue_full(b packet_length, std::string queue_path="");
    void add_on_the_way_packet(b packet_length, bool is_v2_dropped_packet_header = false);
    bool is_queue_over_v2_threshold(b packet_length, std::string queue_path, Packet* packet);
    long get_queue_occupancy(std::string queue_path = "");
    bool is_packet_tag_larger_than_last_packet(std::string queue_path, Packet* packet);
    virtual double get_link_util() override;

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void initializeFlags() override;
    virtual void initializeStatistics() override;
    virtual void handleMessageWhenUp(cMessage *msg) override;
    virtual void finish() override;

  protected:
    // states
    int numConcurrentTransmissions = 0;    // number of colliding frames -- we must receive this many jams (caches endRxTimeList.size())
    int backoffs = 0;    // value of backoff for exponential back-off algorithm
    long currentSendPkTreeID = -1;
    int v2pifo_queue_type;

    // other variables
    EthernetSignal *frameBeingReceived = nullptr;
    cMessage *endRxMsg = nullptr;
    cMessage *endBackoffMsg = nullptr;
    cMessage *endJammingMsg = nullptr;

    // list of receptions during reconnect state; an additional special entry (with packetTreeId=-1)
    // stores the end time of the reconnect state
    struct PkIdRxTime
    {
        long packetTreeId;    // >=0: tree ID of packet being received; -1: this is a special entry that stores the end time of the reconnect state
        simtime_t endTime;    // end of reception
        PkIdRxTime(long id, simtime_t time) { packetTreeId = id; endTime = time; }
    };
    typedef std::list<PkIdRxTime> EndRxTimeList;
    EndRxTimeList endRxTimeList;    // list of incoming packets, ordered by endTime

    // statistics
    simtime_t totalCollisionTime;    // total duration of collisions on channel
    simtime_t totalSuccessfulRxTxTime;    // total duration of successful transmissions on channel
    simtime_t channelBusySince;    // needed for computing totalCollisionTime/totalSuccessfulRxTxTime
    unsigned long numCollisions = 0;    // collisions (NOT number of collided frames!) sensed
    unsigned long numBackoffs = 0;    // number of retransmissions
    int framesSentInBurst = 0;    // Number of frames send out in current frame burst
    B bytesSentInBurst = B(0);    // Number of bytes transmitted in current frame burst

    static simsignal_t collisionSignal;
    static simsignal_t backoffSlotsGeneratedSignal;

    //V2
    bool use_v2_pifo;
    bool use_pfabric;
    bool use_vertigo_prio_queue;
    bool send_header_of_dropped_packet_to_receiver;
    std::string switch_name;

    //bolt
    bool use_bolt_queue, use_bolt_with_vertigo_queue, use_bolt;
    bool bw_set = false;

  public:
    long on_the_way_packet_num = 0;
    b on_the_way_packet_length = b(0);
    V2PacketBuffer* buffer;

  protected:
    // event handlers
    virtual void handleSelfMessage(cMessage *msg) override;
    virtual void handleEndIFGPeriod();
    virtual void handleEndPausePeriod();
    virtual void handleEndTxPeriod();
    virtual void handleEndRxPeriod();
    virtual void handleEndBackoffPeriod();
    virtual void handleEndJammingPeriod();
    virtual void handleRetransmission();

    // helpers
    virtual void readChannelParameters(bool errorWhenAsymmetric) override;
    virtual void handleUpperPacket(Packet *msg) override;
    virtual void processJamSignalFromNetwork(EthernetSignal *msg);
    virtual void processMsgFromNetwork(EthernetSignal *msg);
    virtual void scheduleEndIFGPeriod();
    virtual void fillIFGIfInBurst();
    virtual void scheduleEndTxPeriod(B sentFrameByteLength);
    virtual void scheduleEndRxPeriod(EthernetSignal *);
    virtual void scheduleEndPausePeriod(int pauseUnits);
    virtual void beginSendFrames();
    virtual void sendJamSignal();
    virtual void startFrameTransmission();
    virtual void frameReceptionComplete();
    virtual void processReceivedDataFrame(Packet *frame);
    virtual void processReceivedJam(EthernetJamSignal *jam);
    virtual void processReceivedControlFrame(Packet *packet);
    virtual void processConnectDisconnect() override;
    virtual void addReception(simtime_t endRxTime);
    virtual void addReceptionInReconnectState(long id, simtime_t endRxTime);
    virtual void processDetectedCollision();

    B calculateMinFrameLength();
    B calculatePaddedFrameLength(Packet *frame);

    void check_on_the_way_variables();
    virtual void printState();
};

#endif // ifndef __INET_AUGMENTED_ETHERMAC_H

