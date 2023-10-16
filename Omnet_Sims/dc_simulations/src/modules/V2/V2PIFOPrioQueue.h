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

#ifndef __INET_V2PIFOPRIOQUEUE_H
#define __INET_V2PIFOPRIOQUEUE_H

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

class INET_API V2PIFOPrioQueue : public PacketQueue
{

  protected:
    static simsignal_t packetDropSeqSignal;
    static simsignal_t packetDropRetCountSignal;
    static simsignal_t packetDropTotalPayloadLenSignal;
    unsigned long long light_in_queue_packet_drop_count = 0;
    virtual void initialize(int stage) override;
    unsigned long calculate_priority(unsigned long seq, unsigned long ret_count);
    unsigned long extract_priority(Packet *packet, bool is_packet_being_dropped=false);
    virtual int map_priority_to_queue_idx(unsigned long priority);
    virtual int map_priority_to_queue_idx_sppifo(unsigned long priority);
    virtual int update_sppifo_tags(int queue_idx, unsigned long priority);
    std::list<std::list<Packet*>> prio_queues;
    std::list<unsigned int> prio_queues_bit_length;

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
    unsigned long priority_of_last_packet_inserted_in_queue = 0;

    int num_queues;

    int priority_mapping_scheme;
    int per_queue_packetCapacity = -1;
    b per_queue_dataCapacity = b(-1);

    // SP-PIFO
    std::list<unsigned long> sppifo_queue_bounds;

    // AIFO
    double aifo_k;
    int quantile_wind_size;
    std::list<unsigned long> quantile_list;
    int aifo_count, aifo_sample_count;

    // per flow prioritization
    bool just_prioritize_bursty_flows = false;

    // RED
    double red_wq = 0.0;
    double red_minth = NaN;
    double red_maxth = NaN;
    double red_maxp = NaN;
    double red_pkrate = NaN;
    double red_count = NaN;
    double red_avg = 0.0;
    simtime_t red_q_time = 0;

    // WRED
    bool deploy_wred;
    int rank_grouping_cut_off;
    double hp_red_wq = 0.0;
    double hp_red_minth = NaN;
    double hp_red_maxth = NaN;
    double hp_red_maxp = NaN;
    double hp_red_pkrate = NaN;
    double hp_red_count = NaN;
    double hp_red_avg = 0.0;

  public:
    virtual ~V2PIFOPrioQueue();
    virtual void pushPacket(Packet *packet, cGate *gate) override;
    virtual Packet *popPacket(cGate *gate) override;
    virtual void removePacket(Packet *packet) override;
    virtual bool is_queue_full(b packet_length, long on_the_way_packet_num = 0, b on_the_way_packet_length = b(0)) override;
    virtual bool is_over_v2_threshold_full(b packet_length, Packet* packet, long on_the_way_packet_num = 0, b on_the_way_packet_length = b(0)) override;
    virtual bool is_over_v2_threshold_full_aifo(b packet_length, Packet* packet, long on_the_way_packet_num = 0, b on_the_way_packet_length = b(0));
    virtual bool is_over_v2_threshold_full_sppifo(b packet_length, Packet* packet, long on_the_way_packet_num = 0, b on_the_way_packet_length = b(0));
    virtual bool is_over_v2_threshold_full_red(b packet_length, Packet* packet, long on_the_way_packet_num = 0, b on_the_way_packet_length = b(0));
    virtual int wred_get_traffic_class(Packet* packet);
    virtual int red_get_queue_length();
    virtual long get_queue_occupancy(long on_the_way_packet_num = 0, b on_the_way_packet_length = b(0)) override;
    virtual bool isFullPrioQueue(int queue_idx, int packet_len, Packet* packet);
    virtual bool is_packet_tag_larger_than_last_packet(Packet* packet) override;
    double calc_quantile(Packet* packet);
    void update_quantile_list(Packet* packet);


};

#endif // ifndef __V2PIFOPRIOQUEUE_H

