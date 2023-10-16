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

#ifndef __INET_V2PIFOBoltQueue_H
#define __INET_V2PIFOBoltQueue_H

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
#include "inet/transportlayer/tcp_common/TcpHeader_m.h"

using namespace inet;
using namespace queueing;

class INET_API V2PIFOBoltQueue : public PacketQueue
{

  protected:
    static simsignal_t packetDropSeqSignal;
    static simsignal_t packetDropRetCountSignal;
    static simsignal_t packetDropTotalPayloadLenSignal;
    unsigned long long light_in_queue_packet_drop_count = 0;
    virtual void initialize(int stage) override;
    unsigned long calculate_priority(unsigned long seq, unsigned long ret_count);
    unsigned long extract_priority(Packet *packet, bool is_packet_being_dropped=false);
    std::list<Packet*> data_packet_queue;
    std::list<Packet*> signal_packet_queue;


    bool bounce_randomly_v2;

    double denominator_for_retrasnmissions;

    double all_packets_queueing_time_sum = 0;
    double mice_packets_queueing_time_sum = 0;
    unsigned int num_all_packets = 0;
    unsigned int num_mice_packets = 0;

    int deflection_threshold;
//    unsigned long priority_of_last_packet_inserted_in_queue = 0;

    int relative_priority_calculation_type;
    int relative_priority_distribution_type;

    // AIFO
    // percentile-based relative prio calculation
    int quantile_wind_size;
    std::list<unsigned long> quantile_list;
    int aifo_count, aifo_sample_count;

    // dist-based relative prio calculation
    unsigned long sum_ranks_in_queue = 0;

    // Deterministic
    double k;

  public:
    b CCThresh = b(-1);
    int mtu = -1;
    double bw = -1;

    int sm_token = 0;
    simtime_t last_sm_time = 0;
    simtime_t last_sm_time_backup = 0;
    int pru_token = 0;

  public:
    virtual ~V2PIFOBoltQueue();
    virtual void pushPacket(Packet *packet, cGate *gate) override;
    virtual Packet *popPacket(cGate *gate) override;
    virtual void removePacket(Packet *packet) override;
    virtual bool is_queue_full(b packet_length, long on_the_way_packet_num = 0, b on_the_way_packet_length = b(0)) override;
    virtual bool is_over_v2_threshold_full(b packet_length, Packet* packet, long on_the_way_packet_num = 0, b on_the_way_packet_length = b(0)) override;
    virtual long get_queue_occupancy(long on_the_way_packet_num = 0, b on_the_way_packet_length = b(0)) override;
    virtual bool isFullPrioQueue(int queue_idx, int packet_len, Packet* packet);
    virtual bool is_packet_tag_larger_than_last_packet(Packet* packet) override;
    double calc_quantile(Packet* packet);
    double calculate_relative_priority_dist_expon(Packet* packet);
    double calculate_relative_priority_dist(Packet* packet);
    double calculate_relative_priority(Packet* packet);
    void update_quantile_list(Packet* packet);
    void set_BW_bps(double bandwidth) override;
    void calculate_suply_token(int packet_size_bits) override;


};

#endif // ifndef __V2PIFOBoltQueue_H

