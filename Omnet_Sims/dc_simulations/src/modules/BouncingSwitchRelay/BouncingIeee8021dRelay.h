// Copyright (C) 2013 OpenSim Ltd.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//
// Author: Benjamin Martin Seregi

#ifndef BOUNCINGIEEE8021DRELAY_H
#define BOUNCINGIEEE8021DRELAY_H

#include "inet/common/INETDefs.h"
#include "inet/common/LayeredProtocolBase.h"
#include "inet/common/lifecycle/ModuleOperations.h"
#include "inet/common/packet/Packet.h"
#include "inet/networklayer/common/InterfaceTable.h"
#include "inet/linklayer/configurator/Ieee8021dInterfaceData.h"
#include "inet/linklayer/ethernet/EtherFrame_m.h"
#include "../LSSwitch/LSMACTable/LSIMacAddressTable.h"
#include "../Augmented_Mac/AugmentedEtherMac.h"
#include "../V2/V2PIFO.h"
#include "../pFabric/pFabric.h"
#include "../V2/buffer/V2PacketBuffer.h"
#include "../V2/V2PIFOBoltQueue.h"
#include "unordered_map"
#include "unordered_set"

using namespace inet;

//
// This module forward frames (~EtherFrame) based on their destination MAC addresses to appropriate ports.
// See the NED definition for details.
//
class BouncingIeee8021dRelay : public LayeredProtocolBase
{
  public:
    BouncingIeee8021dRelay();
    virtual ~BouncingIeee8021dRelay();

    /**
     * Register single MAC address that this switch supports.
     */

    void registerAddress(MacAddress mac);

    /**
     * Register range of MAC addresses that this switch supports.
     */
    void registerAddresses(MacAddress startMac, MacAddress endMac);

    // incremental deployment: public
    int deployed_with_deflection = -1;
    bool can_deflect = false;
    bool incremental_deployment = false;
    bool deflection_graph_partitioned = false;
    cModule *switch_module;
    void read_inc_deflection_properties(std::string incremental_deployment_identifier,
            std::string input_file_name);

  protected:
    MacAddress bridgeAddress;
    IInterfaceTable *ifTable = nullptr;
    LSIMacAddressTable *macTable = nullptr;
    InterfaceEntry *ie = nullptr;
    bool isStpAware = false;
    std::list<int> port_idx_connected_to_switch_neioghbors;
    int random_power_factor;
    int random_power_bounce_factor;
    // memory for power of N bouncing and forwarding
    bool use_memory;
    int random_power_memory_size;
    int random_power_bounce_memory_size;
    /*
     * For forwarding we keep a prefix-based record.
     * Same memory for those prefixes with the same ECMP group
     * We use two unordered maps for this
     * 1) Maps groups (prefixes) to memory lists for that prefix : group_id --> list of previous choices
     * 2) maps IP addresses to groups based on their optional ports: IP_address --> group_id
     * 3) maps group_id_strings to groups: Group string --> group_id
     */
    std::unordered_map<unsigned int, std::list<int>> prefix_to_memory_map;
    std::unordered_map<uint64, unsigned int> mac_addr_to_prefix_map;
    std::unordered_map<std::string, unsigned int> group_to_prefix_map;
    unsigned int prefix_id_counter = 0;
    virtual unsigned int update_memory_hash_maps(MacAddress address, std::list<int> interface_ids);

    /*
     * Vertigo deflects packets to neighboring switches, so for any arbitrary packet
     * with any prefix, the memory is the same
     * so we just need a list that is the size of memory for deflection
     */
    std::list<int> deflection_memory;

    bool use_ecmp, use_power_of_n_lb;
    std::hash<std::string> header_hash;
    //Naive Deflection
    bool bounce_naively;
    int naive_deflection_idx;   // This indicates the index (ranging from 0 - number of neighboring switches) for packet deflection
    //DIBS
    bool bounce_randomly, filter_out_full_ports, approximate_random_deflection;
    //Vertigo
    bool bounce_on_same_path;
    //V2
    bool bounce_randomly_v2;
    bool use_v2_pifo; // V2PIFO is used <--> this should be true
    bool drop_bounced_in_relay; // if this is true, bounced packet get dropped in relay if the chosen queue is full. In other words, you don't do the last step of v2 which is pushing it and dropping the worst packet
    bool send_header_of_dropped_packet_to_receiver; // NDP
    // dctcp
    bool dctcp_mark_deflected_packets_only;
    // bolt
    bool use_bolt, use_bolt_queue, use_bolt_with_vertigo_queue, use_pifo_bolt_reaction;
    bool ignore_cc_thresh_for_deflected_packets;
    uint16 src_enabled_packet_type;
    //pFabric
    bool use_pfabric;
    //vertigo prio queue
    bool use_vertigo_prio_queue;
    //PABO
    bool bounce_probabilistically;
    double utilization_thresh;
    double bounce_probability_lambda;
    // selective network feedback
    bool apply_selective_net_reaction;
    int selective_net_reaction_type;
    double sel_reaction_alpha;

    typedef std::pair<MacAddress, MacAddress> MacAddressPair;

    static simsignal_t feedBackPacketDroppedSignal;
    static simsignal_t feedBackPacketDroppedPortSignal;
    static simsignal_t feedBackPacketGeneratedSignal;
    static simsignal_t feedBackPacketGeneratedReqIDSignal;
    static simsignal_t bounceLimitPassedSignal;
    static simsignal_t burstyPacketReceivedSignal;
    unsigned long long light_in_relay_packet_drop_counter = 0;


    struct Comp
    {
        bool operator() (const MacAddressPair& first, const MacAddressPair& second) const
        {
            return (first.first < second.first && first.second < second.first);
        }
    };

    bool in_range(const std::set<MacAddressPair, Comp>& ranges, MacAddress value)
    {
        return ranges.find(MacAddressPair(value, value)) != ranges.end();
    }


    std::set<MacAddressPair, Comp> registeredMacAddresses;

    // statistics: see finish() for details.
    int numReceivedNetworkFrames = 0;
    int numDroppedFrames = 0;
    int numReceivedBPDUsFromSTP = 0;
    int numDeliveredBDPUsToSTP = 0;
    int numDispatchedNonBPDUFrames = 0;
    int numDispatchedBDPUFrames = 0;
    bool learn_mac_addresses;

  protected:
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }

    /**
     * Updates address table (if the port is in learning state)
     * with source address, determines output port
     * and sends out (or broadcasts) frame on ports
     * (if the ports are in forwarding state).
     * Includes calls to updateTableWithAddress() and getPortForAddress().
     *
     */
    void handleAndDispatchFrame(Packet *packet);

    void handleUpperPacket(Packet *packet) override;
    void handleLowerPacket(Packet *packet) override;

    void dispatch(Packet *packet, InterfaceEntry *ie);
    void learn(MacAddress srcAddr, int arrivalInterfaceId);
    void broadcast(Packet *packet, int arrivalInterfaceId);

    void sendUp(Packet *packet);

    //@{ For ECMP
    void chooseDispatchType(Packet *packet, InterfaceEntry *ie);
    unsigned int Compute_CRC16_Simple(std::list<int> bytes, int bytes_size);
    std::list<int> getInfoFlowByteArray(std::string srcAddr, std::string destAddr, int srcPort, int destPort);
    //@}

    //@{ For lifecycle
    virtual void start();
    virtual void stop();
    virtual void handleStartOperation(LifecycleOperation *operation) override { start(); }
    virtual void handleStopOperation(LifecycleOperation *operation) override { stop(); }
    virtual void handleCrashOperation(LifecycleOperation *operation) override { stop(); }
    virtual bool isUpperMessage(cMessage *message) override { return message->arrivedOn("upperLayerIn"); }
    virtual bool isLowerMessage(cMessage *message) override { return message->arrivedOn("ifIn"); }

    virtual bool isInitializeStage(int stage) override { return stage == INITSTAGE_LINK_LAYER; }
    virtual bool isModuleStartStage(int stage) override { return stage == ModuleStartOperation::STAGE_LINK_LAYER; }
    virtual bool isModuleStopStage(int stage) override { return stage == ModuleStopOperation::STAGE_LINK_LAYER; }
    //@}

    /*
     * Gets port data from the InterfaceTable
     */
    Ieee8021dInterfaceData *getPortInterfaceData(unsigned int portNum);

    bool isForwardingInterface(InterfaceEntry *ie);

    /*
     * Returns the first non-loopback interface.
     */
    virtual InterfaceEntry *chooseInterface();
    virtual void finish() override;

    InterfaceEntry* find_interface_to_bounce_naively();
    InterfaceEntry* find_interface_to_bounce_randomly(Packet *packet);
    InterfaceEntry* find_interface_to_bounce_on_the_same_path(Packet *packet, InterfaceEntry *original_output_if);
    InterfaceEntry* find_interface_to_fw_randomly_power_of_n(Packet *packet, bool consider_servers);
    void find_interface_to_bounce_randomly_v2(Packet *packet, bool consider_servers, InterfaceEntry *ie2);
    double get_port_utilization(int port, Packet *packet);
    InterfaceEntry* find_a_port_for_packet_towards_source(Packet *packet);
    InterfaceEntry* find_interface_to_bounce_probabilistically(Packet *packet, InterfaceEntry *original_output_if);
    void apply_early_deflection(Packet *packet, bool consider_servers, InterfaceEntry *ie2);
    // dctcp
    void dctcp_mark_ecn_for_deflected_packets(Packet *packet, bool has_phy_header=true);
    // bolt
    void generate_and_send_bolt_src_packet(Packet *packet, int queue_occupancy_pkt_num, long link_util, int extraction_port_interface_id);
    void bolt_pifo_evaluate_if_src_packet_should_be_generated(Packet *packet, InterfaceEntry *, bool ignore_cc_thresh);
    void bolt_evaluate_if_src_packet_should_be_generated(Packet *packet, InterfaceEntry *, bool ignore_cc_thresh, bool has_phy_header=true, int extraction_port_interface_id=-1);
    void mark_packet_deflection_tag(Packet *packet, bool has_phy_header=true);
    bool bolt_is_packet_src(Packet *packet);
};


#endif // ifndef __INET_BouncingIEEE8021DRELAY_H

