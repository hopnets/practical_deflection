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

#ifndef __INET_V2PIFO_H
#define __INET_V2PIFO_H

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
#include "buffer/V2PacketBuffer.h"

using namespace inet;
using namespace queueing;

class INET_API V2PIFO : public PacketQueue
{

  protected:
    static simsignal_t packetDropSeqSignal;
    static simsignal_t packetDropRetCountSignal;
    static simsignal_t packetDropTotalPayloadLenSignal;
    static simsignal_t packetRankSignal;
    unsigned long long light_in_queue_packet_drop_count = 0;
    virtual void initialize(int stage) override;
    unsigned long calculate_priority(unsigned long seq, unsigned long ret_count);
    unsigned long extract_priority(Packet *packet, bool is_packet_being_dropped=false, bool record_rank=false);
    virtual void check_correctness();
    std::map<unsigned long, std::list<Packet*>> sorted_queued_packet_hash_table; // sorted based on priority
    int dropper_type;
    int scheduler_type;

    // incremental deployment
    bool incremental_deployment = false;
//    std::string incremental_deployment_file_name;
    bool use_bolt_queue, use_bolt_with_vertigo_queue, use_bolt;
    bool using_buffer = false;
    bool just_prioritize_bursty_flows = false;

    // dctcp support
    int dctcp_thresh;
    bool mark_packets_in_enqueue;
    bool dctcp_mark_deflected_packets_only;

    bool bounce_randomly_v2;

    double denominator_for_retrasnmissions;

    double all_packets_queueing_time_sum = 0;
    double mice_packets_queueing_time_sum = 0;
    unsigned int num_all_packets = 0;
    unsigned int num_mice_packets = 0;

    // periodic queue occupancy update
    cMessage *updateQueueOccupancyMsg = nullptr;
    double qlen_update_period;

  public:
    b CCThresh = b(-1);
    int mtu = -1;
    double bw = -1;

    bool update_qlen_periodically;
    int periodic_qlen_num_packets;
    b periodic_qlen_bytes;

    // selective reaction
    bool apply_selective_net_reaction;
    b bolt_CCThresh_max_selective_reaction = b(-1);
    int sel_reaction_quantile_wind_size;
    std::list<unsigned long> sel_reaction_quantile_list;
    int sel_reaction_count, sel_reaction_sample_count;

    int sm_token = 0;
    simtime_t last_sm_time = 0;
    simtime_t last_sm_time_backup = 0;
    int pru_token = 0;
    V2PacketBuffer* buffer;


  public:
    virtual ~V2PIFO();
    virtual void pushPacket(Packet *packet, cGate *gate) override;
    virtual Packet *popPacket(cGate *gate) override;
    virtual void removePacket(Packet *packet) override;
    virtual int getNumPacketsToEject(b packet_length, long seq, long ret_count,
            long on_the_way_packet_num, b on_the_way_packet_length) override;
    virtual std::list<Packet*> eject_and_push(int num_packets_to_eject) override;
    virtual bool is_queue_full(b packet_length, long on_the_way_packet_num = 0, b on_the_way_packet_length = b(0)) override;
    virtual long get_queue_occupancy(long on_the_way_packet_num = 0, b on_the_way_packet_length = b(0)) override;
    void read_inc_deflection_properties(std::string incremental_deployment_identifier,
            std::string input_file_name);
    void set_BW_bps(double bandwidth) override;
    void calculate_suply_token(int packet_size_bits) override;
    virtual void handleMessage(cMessage *message) override;
    virtual bool isOverloaded() override;
    virtual void sel_reaction_update_quantile_list(Packet* packet);
    virtual double sel_reaction_calc_quantile(Packet* packet);
    virtual std::list<Packet*> get_list_of_packets_to_react(Packet *new_packet);

    // incremental deployment: public
    int deployed_with_deflection = -1;


};

#endif // ifndef __INET_V2PIFO_H

