//
// Copyright (C) OpenSim Ltd.
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
// along with this program; if not, see http://www.gnu.org/licenses/.
//

#ifndef __INET_PFABRIC_H
#define __INET_PFABRIC_H

#include "inet/queueing/queue/PacketQueue.h"
#include "inet/queueing/compat/cpacketqueue.h"
#include "inet/queueing/contract/IPacketBuffer.h"
#include "inet/queueing/contract/IActivePacketSink.h"
#include "inet/queueing/contract/IPacketComparatorFunction.h"
#include "inet/queueing/contract/IPacketDropperFunction.h"
#include "inet/queueing/contract/IActivePacketSource.h"
#include "inet/networklayer/ipv4/Ipv4Header_m.h"
#include "inet/linklayer/ethernet/EtherFrame_m.h"
#include "inet/queueing/marker/EcnMarker.h"

using namespace inet;
using namespace queueing;

class INET_API pFabric : public PacketQueue
{

  protected:
    static simsignal_t packetDropSeqSignal;
    static simsignal_t packetDropRetCountSignal;
    static simsignal_t packetDropTotalPayloadLenSignal;
    unsigned long long light_in_queue_packet_drop_count = 0;
    virtual void initialize(int stage) override;
    unsigned long calculate_priority(unsigned long seq, unsigned long ret_count);
    unsigned long extract_priority(Packet *packet, bool is_packet_being_dropped=false);
    unsigned long extract_hash_of_flow(Packet *packet);
    std::map<unsigned long, std::list<Packet*>> fifo_per_flow_queue;
    std::multimap<unsigned long, Packet*> priority_sorted_packet_queue;
    std::hash<std::string> flow_hash;

    // dctcp support
    int dctcp_thresh;

    bool bounce_randomly_v2;

    double denominator_for_retrasnmissions;

    double all_packets_queueing_time_sum = 0;
    double mice_packets_queueing_time_sum = 0;
    unsigned int num_all_packets = 0;
    unsigned int num_mice_packets = 0;


  public:
    virtual ~pFabric();
    virtual void pushPacket(Packet *packet, cGate *gate) override;
    virtual Packet *popPacket(cGate *gate) override;
    virtual void removePacket(Packet *packet) override;
    virtual int getNumPacketsToEject(b packet_length, long seq, long ret_count,
            long on_the_way_packet_num, b on_the_way_packet_length) override;
    virtual std::list<Packet*> eject_and_push(int num_packets_to_eject) override;
    virtual bool is_queue_full(b packet_length, long on_the_way_packet_num = 0, b on_the_way_packet_length = b(0)) override;
    virtual long get_queue_occupancy(long on_the_way_packet_num = 0, b on_the_way_packet_length = b(0)) override;


};

#endif // ifndef __INET_V2PIFO_H

