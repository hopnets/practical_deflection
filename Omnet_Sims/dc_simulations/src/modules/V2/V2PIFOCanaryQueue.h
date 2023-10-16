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

#ifndef __INET_V2PIFOCanaryQueue_H
#define __INET_V2PIFOCanaryQueue_H

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

class INET_API V2PIFOCanaryQueue : public PacketQueue
{

  protected:
    static simsignal_t packetDropSeqSignal;
    static simsignal_t packetDropRetCountSignal;
    static simsignal_t packetDropTotalPayloadLenSignal;
    unsigned long long light_in_queue_packet_drop_count = 0;
    virtual void initialize(int stage) override;
    unsigned long calculate_priority(unsigned long seq, unsigned long ret_count);
    unsigned long extract_priority(Packet *packet, bool is_packet_being_dropped=false);
    std::list<Packet*> prio_queue;
//    unsigned int prio_queue_bit_length;

    // dctcp support
    int dctcp_thresh;
    bool mark_packets_in_enqueue;

    bool bounce_randomly_v2;

    double denominator_for_retrasnmissions;

    double all_packets_queueing_time_sum = 0;
    double mice_packets_queueing_time_sum = 0;
    unsigned int num_all_packets = 0;
    unsigned int num_mice_packets = 0;

    int deflection_threshold;
//    unsigned long priority_of_last_packet_inserted_in_queue = 0;

    int algorithm_type;
    int relative_priority_calculation_type;
    int relative_priority_distribution_type;
//    int per_queue_packetCapacity = -1;
//    b per_queue_dataCapacity = b(-1);

//    // SP-PIFO
//    std::list<unsigned long> sppifo_queue_bounds;

    // AIFO
    // percentile-based relative prio calculation
    int quantile_wind_size;
    std::list<unsigned long> quantile_list;
    int aifo_count, aifo_sample_count;

    // dist-based relative prio calculation
    unsigned long sum_ranks_in_queue = 0;

    // Probabilistic Canary
    double minth = NaN;

    // Deterministic
    double k;

    // periodic queue occupancy update
    bool update_qlen_periodically;
    cMessage *updateQueueOccupancyMsg = nullptr;
    double qlen_update_period;

    // placeholder for when we add buffer to this :-D
    bool using_buffer = false;

    // per flow prioritization
    bool just_prioritize_bursty_flows = false;

  public:
    int periodic_qlen_num_packets;
    b periodic_qlen_bytes;

  public:
    virtual ~V2PIFOCanaryQueue();
    virtual void pushPacket(Packet *packet, cGate *gate) override;
    virtual Packet *popPacket(cGate *gate) override;
    virtual void removePacket(Packet *packet) override;
    virtual bool is_queue_full(b packet_length, long on_the_way_packet_num = 0, b on_the_way_packet_length = b(0)) override;
    virtual bool is_over_v2_threshold_full(b packet_length, Packet* packet, long on_the_way_packet_num = 0, b on_the_way_packet_length = b(0)) override;
    virtual bool is_over_v2_threshold_full_deterministic(b packet_length, Packet* packet, long on_the_way_packet_num = 0, b on_the_way_packet_length = b(0));
    virtual bool is_over_v2_threshold_full_probabilistic(b packet_length, Packet* packet, long on_the_way_packet_num = 0, b on_the_way_packet_length = b(0));
    virtual long get_queue_occupancy(long on_the_way_packet_num = 0, b on_the_way_packet_length = b(0)) override;
    virtual bool isFullPrioQueue(int queue_idx, int packet_len, Packet* packet);
    virtual bool is_packet_tag_larger_than_last_packet(Packet* packet) override;
    virtual void handleMessage(cMessage *message) override;
    double calc_quantile(Packet* packet);
    double calculate_relative_priority_dist_expon(Packet* packet);
    double calculate_relative_priority_dist(Packet* packet);
    double calculate_relative_priority(Packet* packet);
    void update_quantile_list(Packet* packet);


};

#endif // ifndef __V2PIFOCanaryQueue_H

