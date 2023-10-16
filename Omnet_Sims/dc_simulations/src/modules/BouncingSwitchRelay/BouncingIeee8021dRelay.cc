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

#include "inet/common/IProtocolRegistrationListener.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/linklayer/common/MacAddressTag_m.h"
#include "inet/linklayer/ieee8022/Ieee8022LlcHeader_m.h"
#include "inet/networklayer/common/InterfaceEntry.h"
#include "inet/networklayer/ipv4/Ipv4Header_m.h"
#include "inet/transportlayer/tcp_common/TcpHeader_m.h"
#include "inet/linklayer/ethernet/EtherPhyFrame_m.h"
#include "inet/queueing/queue/PacketQueue.h"
#include "BouncingIeee8021dRelay.h"
#include "inet/applications/tcpapp/GenericAppMsg_m.h"
#include <sqlite3.h>

using namespace inet;

// todo: remove
int temp_seq = 10000;
int drop_cnt = 5;

Define_Module(BouncingIeee8021dRelay);

// bolt vertigo src generation options
#define BOLT_VERTIGO_ORG_PKT    0 // sending SRC packet for the packet that faces full queue
#define BOLT_VERTIGO_DEF_PKT    1 // sending SRC packet for the deflected packets
#define BOLT_VERTIGO_ORG_DEF_PKT    2 // sending SRC for both packets stated above

#define QUANTILE_SEL_REACTION 0

#define TTL 250


simsignal_t BouncingIeee8021dRelay::feedBackPacketDroppedSignal = registerSignal("feedBackPacketDropped");
simsignal_t BouncingIeee8021dRelay::feedBackPacketDroppedPortSignal = registerSignal("feedBackPacketDroppedPort");
simsignal_t BouncingIeee8021dRelay::feedBackPacketGeneratedSignal = registerSignal("feedBackPacketGenerated");
simsignal_t BouncingIeee8021dRelay::bounceLimitPassedSignal = registerSignal("bounceLimitPassed");
simsignal_t BouncingIeee8021dRelay::burstyPacketReceivedSignal = registerSignal("burstyPacketReceived");

BouncingIeee8021dRelay::BouncingIeee8021dRelay()
{
}

BouncingIeee8021dRelay::~BouncingIeee8021dRelay()
{
    recordScalar("lightInRelayPacketDropCounter", light_in_relay_packet_drop_counter);
}

static int callback_incremental_deployment(void *data, int argc, char **argv, char **azColName){
   int i;
   BouncingIeee8021dRelay *relay_object = (BouncingIeee8021dRelay*) data;

   for(i = 0; i<argc; i++){
      if (std::strcmp(argv[i], "") != 0) {
          int result = std::stoi(argv[i]);
          if (result != 1 && result != 0) {
              throw cRuntimeError("Incremental deflection is neither 0 nor 1!");
          }
          relay_object->deployed_with_deflection = result;
          return 0;
      }
   }

   throw cRuntimeError("No result found for incremental deflection identifier!");
   return -1;
}

void BouncingIeee8021dRelay::read_inc_deflection_properties(std::string incremental_deployment_identifier, std::string input_file_name)
{

    sqlite3* DB;
    char *zErrMsg = 0;
    int rc;
    std::string sql;
    int exit = 0;

    std::string column_name = incremental_deployment_identifier;
    std::string incremental_deployment_table_name = "deflection_identifiers";

    exit = sqlite3_open(input_file_name.c_str(), &DB);
    if (exit) {
        throw cRuntimeError(sqlite3_errmsg(DB));
        return;
    }
    sql = "SELECT " + column_name + " from " + incremental_deployment_table_name;
    rc = sqlite3_exec(DB, sql.c_str(), callback_incremental_deployment, (void*)this, &zErrMsg);
    if( rc != SQLITE_OK ) {
      throw cRuntimeError("SQL error: %s\n", zErrMsg);
    }
    sqlite3_close(DB);
    return;
}

void BouncingIeee8021dRelay::initialize(int stage)
{
    LayeredProtocolBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        // statistics
//        std::cout << getFullPath() << ": " << stage << ": INITSTAGE_LOCAL" << endl;
        numDispatchedBDPUFrames = numDispatchedNonBPDUFrames = numDeliveredBDPUsToSTP = 0;
        numReceivedBPDUsFromSTP = numReceivedNetworkFrames = numDroppedFrames = 0;
        isStpAware = par("hasStp");

        macTable = getModuleFromPar<LSIMacAddressTable>(par("macTableModule"), this);
        ifTable = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);

        use_ecmp = getAncestorPar("useECMP");
        use_power_of_n_lb = getAncestorPar("use_power_of_n_lb");
        random_power_factor = getAncestorPar("random_power_factor");

        if ((!use_ecmp && !use_power_of_n_lb)) {
            // TODO if you want to have the option to don't use no LB, comment out this if.
            throw cRuntimeError("No load balancing technique used. Are you sure you want this?");
        }

        if (use_ecmp && use_power_of_n_lb)
            throw cRuntimeError("More than one LB technique is chosen. WTF?");

        learn_mac_addresses = par("learn_mac_addresses");

        //NAIVE DEFLECTION
        bounce_naively = getAncestorPar("bounce_naively");
        naive_deflection_idx = getAncestorPar("naive_deflection_idx");

        //DIBS
        bounce_randomly = getAncestorPar("bounce_randomly");
        filter_out_full_ports = getAncestorPar("filter_out_full_ports");
        approximate_random_deflection = getAncestorPar("approximate_random_deflection");

        // Vertigo
        bounce_on_same_path = getAncestorPar("bounce_on_same_path");
        random_power_bounce_factor = getAncestorPar("random_power_bounce_factor");
        use_memory = getAncestorPar("use_memory");
        random_power_memory_size = getAncestorPar("random_power_memory_size");
        random_power_bounce_memory_size = getAncestorPar("random_power_bounce_memory_size");

        // Power of N bouncing
        bounce_randomly_v2 = getAncestorPar("bounce_randomly_v2");
        use_v2_pifo = getAncestorPar("use_v2_pifo");
        drop_bounced_in_relay = getAncestorPar("drop_bounced_in_relay");

        // dctcp
        dctcp_mark_deflected_packets_only = getAncestorPar("dctcp_mark_deflected_packets_only");

        // bolt
        use_bolt_queue = getAncestorPar("use_bolt_queue");
        use_bolt_with_vertigo_queue = getAncestorPar("use_bolt_with_vertigo_queue");
        if (use_bolt_queue && use_bolt_with_vertigo_queue)
            throw cRuntimeError("use_bolt_queue && use_bolt_with_vertigo_queue");
        use_bolt = use_bolt_queue || use_bolt_with_vertigo_queue;
        use_pifo_bolt_reaction = getAncestorPar("use_pifo_bolt_reaction");
        ignore_cc_thresh_for_deflected_packets = getAncestorPar("ignore_cc_thresh_for_deflected_packets");
        if (use_pifo_bolt_reaction && (!use_bolt_with_vertigo_queue || ignore_cc_thresh_for_deflected_packets))
            throw cRuntimeError("use_pifo_bolt_reaction && (!use_bolt_with_vertigo_queue || ignore_cc_thresh_for_deflected_packets)");

        // pFabric
        use_pfabric = getAncestorPar("use_pfabric");

        // Vertigo priority queue
        use_vertigo_prio_queue = getAncestorPar("use_vertigo_prio_queue");

        // PABO
        bounce_probabilistically = getAncestorPar("bounce_probabilistically");
        utilization_thresh = getAncestorPar("utilization_thresh");
        bounce_probability_lambda = getAncestorPar("bounce_probability_lambda");

        // selective network feedback
        apply_selective_net_reaction = getAncestorPar("apply_selective_net_reaction");
        std::string selective_net_reaction_type_string = getAncestorPar("selective_net_reaction_type_string");
        if (apply_selective_net_reaction) {
            if (use_pifo_bolt_reaction)
                throw cRuntimeError("apply_selective_net_reaction && use_pifo_bolt_reaction");
            if (selective_net_reaction_type_string.find("quantile") != std::string::npos) {
                selective_net_reaction_type = QUANTILE_SEL_REACTION;
            } else
                throw cRuntimeError("unknown selective feedback paradigm!");
            sel_reaction_alpha = getAncestorPar("sel_reaction_alpha");
            if (sel_reaction_alpha < 0 || sel_reaction_alpha > 1)
                throw cRuntimeError("sel_reaction_alpha < 0 || sel_reaction_alpha > 1");
        }

        if (bounce_randomly_v2 && use_bolt_with_vertigo_queue) {
            std::string src_enabled_packet_type_str = getAncestorPar("src_enabled_packet_type");
            if (src_enabled_packet_type_str.compare("ORG") == 0)
                src_enabled_packet_type = BOLT_VERTIGO_ORG_PKT;
            else if (src_enabled_packet_type_str.compare("DEF") == 0)
                src_enabled_packet_type = BOLT_VERTIGO_DEF_PKT;
            else if (src_enabled_packet_type_str.compare("ORG_DEF") == 0)
                src_enabled_packet_type = BOLT_VERTIGO_ORG_DEF_PKT;
            else
                throw cRuntimeError("We don't know for which packet we should generate SRC while working with Vertigo!");
        }

        // incremental deployment
        switch_module = getParentModule();
        incremental_deployment = getAncestorPar("incremental_deployment");
        // switches
        if (incremental_deployment) {
            std::string incremental_deployment_identifier =
                                switch_module->getName() +
                                std::to_string(switch_module->getIndex());
            std::string incremental_deployment_file_name = getAncestorPar("incremental_deployment_file_name");
            int repetition_num = atoi(getEnvir()->getConfigEx()->getVariable(CFGVAR_REPETITION));
            std::string rep_num_string = std::to_string(repetition_num);
            incremental_deployment_file_name += ("_" + rep_num_string + "_rep.db");
            read_inc_deflection_properties(incremental_deployment_identifier, incremental_deployment_file_name);
            can_deflect = (deployed_with_deflection == 1);
            deflection_graph_partitioned = getAncestorPar("deflection_graph_partitioned");
        } else {
            can_deflect = bounce_naively || bounce_randomly || bounce_on_same_path || bounce_randomly_v2 ||
                    bounce_probabilistically;
        }

        if (bounce_naively && naive_deflection_idx < 0)
            throw cRuntimeError("bounce_naively && naive_deflection_idx < 0");

        if (use_memory && (random_power_memory_size <= 0 || random_power_bounce_memory_size <= 0)) {
            throw cRuntimeError("use_memory && (random_power_memory_size <= 0 || random_power_bounce_memory_size <= 0");
        }

        if (int(bounce_naively) + int(bounce_randomly) + int(bounce_on_same_path) + int(bounce_randomly_v2)
                + int(bounce_probabilistically) > 1)
            throw cRuntimeError("Two bouncing approaches chosen. WTF?");

        if (bounce_probabilistically && learn_mac_addresses)
            throw cRuntimeError("learning mac addresses should be off when using PABO!");

        if (int(use_v2_pifo) + int(use_pfabric) + int(use_vertigo_prio_queue) > 1)
            throw cRuntimeError("Cannot set use_v2_pifo, use_pfabric, or use_vertigo_prio_queue at the same time");

        if (bounce_randomly_v2 && !use_vertigo_prio_queue && !use_v2_pifo && !use_pfabric)
            throw cRuntimeError("How are we using v2 bouncing without v2 pifo or pFabric");

        send_header_of_dropped_packet_to_receiver = getAncestorPar("send_header_of_dropped_packet_to_receiver");
        std::string switch_name = getParentModule()->getFullName();
        std::string module_path_string = switch_name + ".eth[" + std::to_string(0) + "].mac.queue";
        if (send_header_of_dropped_packet_to_receiver)
            module_path_string += ".mainQueue";
        cModule* queue_module = getModuleByPath(module_path_string.c_str());
        std::string queue_module_name = queue_module->getModuleType()->getFullName();

        bool have_v2pifo_queues = queue_module_name.find("V2PIFO") != std::string::npos;
        if (use_v2_pifo && !have_v2pifo_queues)
            throw cRuntimeError("We're planning to use v2pifo, why we don't have v2pifo queues?");
        else if (!use_vertigo_prio_queue && !use_v2_pifo && have_v2pifo_queues)
            throw cRuntimeError("We are not using v2pifo, why we still have v2pifo queues?");

        bool have_pFabric_queues = queue_module_name.find("pFabric") != std::string::npos;
        if (use_pfabric && !have_pFabric_queues)
            throw cRuntimeError("We're planning to use pFabric, why we don't have pFabric queues?");
        else if (!use_pfabric && have_pFabric_queues)
            throw cRuntimeError("We are not using pFabric, why we still have pFabric queues?");
        if (!use_pfabric && !use_v2_pifo && send_header_of_dropped_packet_to_receiver)
            throw cRuntimeError("send_header_of_dropped_packet_to_receive only works if we are using v2_pifo!");

        if (!bounce_randomly_v2 && send_header_of_dropped_packet_to_receiver)
            throw cRuntimeError("send_header_of_dropped_packet_to_receive only works if we are using v2 bouncing!");

        bool have_vertigo_prio_queues = queue_module_name.find("V2PIFOPrioQueue") != std::string::npos ||
                queue_module_name.find("V2PIFOCanaryQueue") != std::string::npos ||
                queue_module_name.find("V2PIFOBoltQueue") != std::string::npos;
        if (use_vertigo_prio_queue && !have_vertigo_prio_queues)
            throw cRuntimeError("We're planning to use V2PIFOPrioQueue, CanaryQueue, or BoltQueue, why we don't have V2PIFOPrioQueue/CanaryPrioQueue queues?");
        if (!use_vertigo_prio_queue && !use_pfabric && !use_v2_pifo && send_header_of_dropped_packet_to_receiver)
            throw cRuntimeError("send_header_of_dropped_packet_to_receive only works if we are using v2_pifo!");

        if (!bounce_randomly_v2 && send_header_of_dropped_packet_to_receiver)
            throw cRuntimeError("send_header_of_dropped_packet_to_receive only works if we are using v2 bouncing!");

        std::string v2pifo_queue_type_str = getAncestorPar("v2pifo_queue_type");

        // For incremental deployment we want this for all switches
        if (incremental_deployment || getParentModule()->getIndex() == 0) {
            std::cout << switch_module->getFullName() <<
                    ": You're setting for forwarding and bouncing is: " << endl <<
                    "use_ecmp: " << use_ecmp << endl <<
                    "use_power_of_n_lb: " << use_power_of_n_lb << endl <<
                    "bounce_naively: " << bounce_naively << endl <<
                    "bounce_randomly: " << bounce_randomly << endl <<
                    "filter_out_full_ports: " << filter_out_full_ports << endl <<
                    "approximate_random_deflection: " << approximate_random_deflection << endl <<
                    "bounce_on_same_path: " << bounce_on_same_path << endl <<
                    "bounce_randomly_v2: " << bounce_randomly_v2 << endl <<
                    "use_v2_pifo: " << use_v2_pifo << endl <<
                    "incremental_deployment: " << incremental_deployment << endl <<
                    "deflection_graph_partitioned: " << deflection_graph_partitioned << endl <<
                    "use_pFabric: " << use_pfabric << endl <<
                    "use_vertigo_prio_queue: " << use_vertigo_prio_queue << endl <<
                    "send_header_of_dropped_packet_to_receiver: " << send_header_of_dropped_packet_to_receiver << endl <<
                    "can_deflect: " << can_deflect << endl <<
                    "use_memory: " << use_memory << endl <<
                    "random_power_memory_size: " << random_power_memory_size << endl <<
                    "random_power_bounce_memory_size: " << random_power_bounce_memory_size << endl <<
                    "bounce_probabilistically: " << bounce_probabilistically << endl <<
                    "v2pifo_queue_type_str: " << v2pifo_queue_type_str << endl <<
                    "use_bolt_queue: " << use_bolt_queue << endl <<
                    "use_bolt_with_vertigo_queue: " << use_bolt_with_vertigo_queue << endl <<
                    "apply_selective_net_reaction: " << apply_selective_net_reaction << endl;
            std::cout << "**********************************************************" << endl;
        }
    }
    else if (stage == INITSTAGE_LINK_LAYER) {
//        std::cout << getFullPath() << ": " << stage << ": INITSTAGE_LINK_LAYER" << endl;
        registerService(Protocol::ethernetMac, gate("upperLayerIn"), gate("ifIn"));
        registerProtocol(Protocol::ethernetMac, gate("ifOut"), gate("upperLayerOut"));

        //TODO FIX Move it at least to STP module (like in ANSA's CDP/LLDP)
        if(isStpAware) {
            registerAddress(MacAddress::STP_MULTICAST_ADDRESS);
        }

        WATCH(bridgeAddress);
        WATCH(numReceivedNetworkFrames);
        WATCH(numDroppedFrames);
        WATCH(numReceivedBPDUsFromSTP);
        WATCH(numDeliveredBDPUsToSTP);
        WATCH(numDispatchedNonBPDUFrames);

        int bounce_naively_counter = 0;

        if (can_deflect) {
            std::string other_side_input_module_path;
            bool is_other_side_input_module_path_server;
            for (int i = 0; i < ifTable->getNumInterfaces(); i++) {
               other_side_input_module_path = getParentModule()->gate(getParentModule()->gateBaseId("ethg$o")+ i)->getPathEndGate()->getFullPath();
               is_other_side_input_module_path_server = (other_side_input_module_path.find("server") != std::string::npos);
               if (!is_other_side_input_module_path_server) {
//                   std::cout << other_side_input_module_path << " is not a host." << endl;
                   if (bounce_naively) {
                       if (bounce_naively_counter == naive_deflection_idx) {
                           port_idx_connected_to_switch_neioghbors.push_back(i);
                           break;
                       }
                       bounce_naively_counter++;
                   } else if (incremental_deployment && deflection_graph_partitioned) {
                       std::string relay_name = "";
                       int counter = 0;
                       int dots_seen = 0;
                       while (dots_seen < 2) {
                           relay_name += other_side_input_module_path[counter];
                           if (other_side_input_module_path[counter] == '.') {
                               dots_seen += 1;
                           }
                           counter += 1;
                       }
                       relay_name += "relayUnit";
                       BouncingIeee8021dRelay *relay = check_and_cast<BouncingIeee8021dRelay *>(getModuleByPath(relay_name.c_str()));
                       if (relay->can_deflect) {
                           EV << getFullPath() << ": adding " << relay_name << " to neighbor list for deflection!" << endl;
                           port_idx_connected_to_switch_neioghbors.push_back(i);
                       }
                   } else {
                       port_idx_connected_to_switch_neioghbors.push_back(i);
                   }
               }
            }
            if (port_idx_connected_to_switch_neioghbors.size() == 0)
                throw cRuntimeError("can_deflect is on but no deflection option is added. why?");
        }

        std::cout << getFullPath() << " --> port_idx_connected_to_switch_neioghbors is: [";
        for (std::list<int>::iterator i = port_idx_connected_to_switch_neioghbors.begin(); i != port_idx_connected_to_switch_neioghbors.end(); i++) {
            std::cout << (*i) << ", ";
        }
        std::cout << "]" << endl;
        std::cout << "**********************************************************" << endl;

        if (use_memory) {
            for (int i = 0; i < random_power_bounce_memory_size; i++)
                deflection_memory.push_back(-1);
        }
    }
}

void BouncingIeee8021dRelay::registerAddress(MacAddress mac)
{
    registerAddresses(mac, mac);
}

void BouncingIeee8021dRelay::registerAddresses(MacAddress startMac, MacAddress endMac)
{
    registeredMacAddresses.insert(MacAddressPair(startMac, endMac));
}

void BouncingIeee8021dRelay::handleLowerPacket(Packet *packet)
{
    // messages from network
    numReceivedNetworkFrames++;
    std::string switch_name = this->getParentModule()->getFullName();

    EV_INFO << "Received " << packet << " from network." << endl;
    delete packet->removeTagIfPresent<DispatchProtocolReq>();
    handleAndDispatchFrame(packet);
}

void BouncingIeee8021dRelay::handleUpperPacket(Packet *packet)
{
    const auto& frame = packet->peekAtFront<EthernetMacHeader>();

    InterfaceReq* interfaceReq = packet->findTag<InterfaceReq>();
    int interfaceId =
            interfaceReq == nullptr ? -1 : interfaceReq->getInterfaceId();

    if (interfaceId != -1) {
        InterfaceEntry *ie = ifTable->getInterfaceById(interfaceId);
        chooseDispatchType(packet, ie);
    } else if (frame->getDest().isBroadcast()) {    // broadcast address
        broadcast(packet, -1);
    } else {
        std::list<int> outInterfaceId = macTable->getInterfaceIdForAddress(frame->getDest());
        // Not known -> broadcast
        if (outInterfaceId.size() == 0) {
            EV_DETAIL << "Destination address = " << frame->getDest()
                                      << " unknown, broadcasting frame " << frame
                                      << endl;

            throw cRuntimeError("2)Destination address not known. Broadcasting the frame. For DCs based on you're setting this shouldn't happen.");
            broadcast(packet, -1);
        } else {
            InterfaceEntry *ie = ifTable->getInterfaceById(interfaceId);
            chooseDispatchType(packet, ie);
        }
    }
}

bool BouncingIeee8021dRelay::isForwardingInterface(InterfaceEntry *ie)
{
    if (isStpAware) {
        if (!ie->getProtocolData<Ieee8021dInterfaceData>())
            throw cRuntimeError("Ieee8021dInterfaceData not found for interface %s", ie->getFullName());
        return ie->getProtocolData<Ieee8021dInterfaceData>()->isForwarding();
    }
    return true;
}

void BouncingIeee8021dRelay::broadcast(Packet *packet, int arrivalInterfaceId)
{
    if (!learn_mac_addresses) {
        throw cRuntimeError("Even though a learning is off, a packet is "
                "being broadcasted. If global ARP is set. This can actually"
                "mean that the tables are not created correctly and some "
                "packets are being broadcasted!");
    }
    EV_DETAIL << "Broadcast frame " << packet << endl;

    auto oldPacketProtocolTag = packet->removeTag<PacketProtocolTag>();
    packet->clearTags();
    auto newPacketProtocolTag = packet->addTag<PacketProtocolTag>();
    *newPacketProtocolTag = *oldPacketProtocolTag;
    delete oldPacketProtocolTag;
    packet->trim();

    int numPorts = ifTable->getNumInterfaces();
    EV_DETAIL << "SEPEHR: number of ports are: " << numPorts << endl;
    EV_DETAIL << "SEPEHR: arrival ID is: " << arrivalInterfaceId << endl;
    EV_DETAIL << "SEPEHR: arrival ID index is: " << arrivalInterfaceId - 100 << endl;


    std::string other_side_input_module_path = getParentModule()->gate(getParentModule()->gateBaseId("ethg$o")+ arrivalInterfaceId - 100)->getPathEndGate()->getFullPath();
    bool is_other_side_input_module_path_spine = (other_side_input_module_path.find("spine") != std::string::npos);


    for (int i = 0; i < numPorts; i++) {
        InterfaceEntry *ie = ifTable->getInterface(i);

        if (ie->isLoopback() || !ie->isBroadcast())
            continue;
        std::string other_side_output_module_path = getParentModule()->gate(getParentModule()->gateBaseId("ethg$o")+ i)->getPathEndGate()->getFullPath();
        bool is_other_side_output_module_path_spine = (other_side_output_module_path.find("spine") != std::string::npos);
        if (is_other_side_input_module_path_spine && is_other_side_output_module_path_spine) {
            EV_DETAIL << "SEPEHR: Came from upper layer and should not go to the upper layer" << endl;
            continue;
        }
//        bool is_other_side_output_module_path_client = (other_side_output_module_path.find("client") != std::string::npos);
//        if (is_other_side_output_module_path_client) {
//            std::string protocol = packet->getName();
//            if (protocol.find("arpREQ") != std::string::npos) {
//                EV << "arpREQ going to client. No!" << endl;
//                continue;
//            }
//        }
        if (ie->getInterfaceId() != arrivalInterfaceId && isForwardingInterface(ie)) {
            chooseDispatchType(packet->dup(), ie);
        }
    }
    delete packet;
}

namespace {
bool isBpdu(Packet *packet, const Ptr<const EthernetMacHeader>& hdr)
{
    if (isIeee8023Header(*hdr)) {
        const auto& llc = packet->peekDataAt<Ieee8022LlcHeader>(hdr->getChunkLength());
        return (llc->getSsap() == 0x42 && llc->getDsap() == 0x42 && llc->getControl() == 3);
    }
    else
        return false;
}
}

const uint64_t string_to_mac(std::string const& s) {
    unsigned char a[6];
    int last = -1;
    int rc = sscanf(s.c_str(), "%hhx-%hhx-%hhx-%hhx-%hhx-%hhx%n",
                    a + 0, a + 1, a + 2, a + 3, a + 4, a + 5,
                    &last);
    if(rc != 6 || s.size() != last)
        throw std::runtime_error("invalid mac address format " + s);
    return
        uint64_t(a[0]) << 40 |
        uint64_t(a[1]) << 32 | (
            // 32-bit instructions take fewer bytes on x86, so use them as much as possible.
            uint32_t(a[2]) << 24 |
            uint32_t(a[3]) << 16 |
            uint32_t(a[4]) << 8 |
            uint32_t(a[5])
        );
}

void BouncingIeee8021dRelay::handleAndDispatchFrame(Packet *packet)
{

    b packet_position = packet->getFrontOffset();
    packet->setFrontIteratorPosition(b(0));
    auto& phy_header = packet->removeAtFront<EthernetPhyHeader>();
    const auto& frame2 = packet->removeAtFront<EthernetMacHeader>();
    int hop_count = frame2->getHop_count();
    hop_count++;
    EV << "SEPEHR: packet hop count is " << hop_count << endl;
    frame2->setHop_count(hop_count);
    packet->insertAtFront(frame2);
    packet->insertAtFront(phy_header);
    packet->setFrontIteratorPosition(packet_position);
    const auto& frame = packet->peekAtFront<EthernetMacHeader>();

    if (frame->getIs_bursty()) {
        emit(burstyPacketReceivedSignal, string_to_mac(frame->getDest().str()));
    }

    int arrivalInterfaceId = packet->getTag<InterfaceInd>()->getInterfaceId();
    InterfaceEntry *arrivalInterface = ifTable->getInterfaceById(arrivalInterfaceId);
    Ieee8021dInterfaceData *arrivalPortData = arrivalInterface->findProtocolData<Ieee8021dInterfaceData>();
    if (isStpAware && arrivalPortData == nullptr)
        throw cRuntimeError("Ieee8021dInterfaceData not found for interface %s", arrivalInterface->getFullName());
    if (learn_mac_addresses && !(frame->isFB() || frame->getAllow_same_input_output())) {
        learn(frame->getSrc(), arrivalInterfaceId);
    }

    //TODO revise next "if"s: 2nd drops all packets for me if not forwarding port; 3rd sends up when dest==STP_MULTICAST_ADDRESS; etc.
    // reordering, merge 1st and 3rd, ...

    // BPDU Handling
    if (isStpAware
            && (frame->getDest() == MacAddress::STP_MULTICAST_ADDRESS || frame->getDest() == bridgeAddress)
            && arrivalPortData->getRole() != Ieee8021dInterfaceData::DISABLED
            && isBpdu(packet, frame)) {
        EV_DETAIL << "Deliver BPDU to the STP/RSTP module" << endl;
        sendUp(packet);    // deliver to the STP/RSTP module
    }
    else if (isStpAware && !arrivalPortData->isForwarding()) {
        EV_INFO << "The arrival port is not forwarding! Discarding it!" << endl;
        numDroppedFrames++;
        delete packet;
    }
    else if (in_range(registeredMacAddresses, frame->getDest())) {
        // destination MAC address is registered, send it up
        sendUp(packet);
    }
    else if (frame->getDest().isBroadcast()) {    // broadcast address
        broadcast(packet, arrivalInterfaceId);
    }
    else {
        std::list<int> outputInterfaceId = macTable->getInterfaceIdForAddress(frame->getDest());
        // Not known -> broadcast
        if (outputInterfaceId.size() == 0) {
            EV_DETAIL << "Destination address = " << frame->getDest() << " unknown, broadcasting frame " << frame << endl;
            throw cRuntimeError("1)Destination address not known. Broadcasting the frame. For DCs based on you're setting this shouldn't happen.");
            broadcast(packet, arrivalInterfaceId);
        }
        else {
            //for (std::list<int>::iterator it=outputInterfaceId.begin(); it != outputInterfaceId.end(); ++it){
            //NOTE: HERE I USED the first path
            InterfaceEntry *outputInterface = ifTable->getInterfaceById(*outputInterfaceId.begin());
            if (isForwardingInterface(outputInterface))
                chooseDispatchType(packet, outputInterface);
            else {
                EV_INFO << "Output interface " << *outputInterface->getFullName() << " is not forwarding. Discarding!" << endl;
                numDroppedFrames++;
                delete packet;
            }
        }
    }
}

unsigned int BouncingIeee8021dRelay::Compute_CRC16_Simple(std::list<int> bytes, int bytes_size){
    const unsigned int generator = 0x1021;
    unsigned int crc = 0;
    EV << "SEPEHR: The generator is: " << generator << endl;

    for (std::list<int>::iterator it=bytes.begin(); it != bytes.end(); ++it){
        int byte = *it;
        crc ^= ((unsigned int)(byte << 8));
        for (int j = 0; j < 8; j++) {
            if ((crc & 0x8000) != 0) {
                crc = ((unsigned int)((crc << 1) ^ generator));
            }
            else {
                crc <<= 1;
            }
        }
    }

    return crc;
}

std::list<int> BouncingIeee8021dRelay::getInfoFlowByteArray(std::string srcAddr, std::string destAddr, int srcPort, int destPort) {
    std::list<int> bytes;
    std::string token;
    std::stringstream src(srcAddr);
    std::stringstream dest(srcAddr);
    std::string item;
    while (std::getline(src, item, '.'))
    {
       bytes.push_back(std::stoi(item));
    }
    while (std::getline(dest, item, '.'))
    {
       bytes.push_back(std::stoi(item));
    }

    bytes.push_back(srcPort);
    bytes.push_back(destPort);

    return bytes;
}

void BouncingIeee8021dRelay::chooseDispatchType(Packet *packet, InterfaceEntry *ie){
    const auto& frame = packet->peekAtFront<EthernetMacHeader>();
    std::list<int> destInterfaceIds = macTable->getInterfaceIdForAddress(frame->getDest());
//    macTable->printState();
    int portNum = destInterfaceIds.size();
    Chunk::enableImplicitChunkSerialization = true;
    std::string protocol = packet->getName();
    bool is_packet_arp_or_broadcast = (protocol.find("arp") != std::string::npos) || (frame->getDest().isBroadcast());
//    std::cout << "***************************************************" << endl;
    if (!is_packet_arp_or_broadcast){
        EV << "SEPEHR: Should reduce packet's ttl." << endl;
        b packetPosition = packet->getFrontOffset();
        packet->setFrontIteratorPosition(b(0));
        auto phyHeader = packet->removeAtFront<EthernetPhyHeader>();
        auto ethHeader = packet->removeAtFront<EthernetMacHeader>();
        auto ipHeader = packet->removeAtFront<Ipv4Header>();
        short ttl = ipHeader->getTimeToLive() - 1;
        if (ttl <= 0) {
            EV << "ttl is " << ttl << ". dropping the packet!" << endl;
//            emit(bounceLimitPassedSignal, packet->getId());
            light_in_relay_packet_drop_counter++;
            delete packet;
            return;
        }
        EV << "SEPEHR: packet's old ttl is: " << ipHeader->getTimeToLive() << " and it's new ttl is: " << ttl << endl;
        ipHeader->setTimeToLive(ttl);
        packet->insertAtFront(ipHeader);
        packet->insertAtFront(ethHeader);
        packet->insertAtFront(phyHeader);
        packet->setFrontIteratorPosition(packetPosition);
    }

    EV << "SOUGOL: This is the Packet Name: " << protocol << endl;
    EV << "SEPEHR: The number of available ports for this packet is " << portNum << endl;
    EV << "SEPEHR: source mac address: " << frame->getSrc().str() << " and dest mac address: " << frame->getDest().str() << endl;
    if (!is_packet_arp_or_broadcast){
        InterfaceEntry *ie2;

        if (use_power_of_n_lb) {
            //forward the packet towards destination using power of n choices
            // Considering ports towards servers as well

            EV << "Finding random interface for packet " << packet->str() << endl;
            ie2 = find_interface_to_fw_randomly_power_of_n(packet, true);
            if (ie2 == nullptr)
                ie2 = ie;
        }
        else if (use_ecmp && portNum > 1) {
            destInterfaceIds.sort();
            b packetPosition = packet->getFrontOffset();
            packet->setFrontIteratorPosition(b(0));
            auto phyHeader = packet->removeAtFront<EthernetPhyHeader>();
            auto ethHeader = packet->removeAtFront<EthernetMacHeader>();
            auto ipHeader = packet->removeAtFront<Ipv4Header>();
            auto tcpHeader = packet->peekAtFront<tcp::TcpHeader>();
            EV << "S&S: The flow info is: ( src_ip: " << ipHeader->getSourceAddress() << ", dest_ip: " << ipHeader->getDestinationAddress() << ", src_port: " << tcpHeader->getSourcePort() << ", dest_port: " << tcpHeader->getDestinationPort() << " )" << endl;
            EV << "SEPEHR: Switch IS using ECMP for this packet!" << endl;
            std::string switch_name = getParentModule()->getFullPath();
            std::string header_info = ipHeader->getSourceAddress().str() + ipHeader->getDestinationAddress().str() +
                    std::to_string(tcpHeader->getSourcePort()) + std::to_string(tcpHeader->getDestinationPort()) + switch_name;
            unsigned long header_info_hash = header_hash(header_info);
            EV << "There are " << portNum << " ports and header hash is " << header_info_hash << endl;
            int outputPortNum;
            outputPortNum = header_info_hash % portNum;
            EV << "SEPEHR: output port number is: " << outputPortNum << endl;
            std::list<int>::iterator it = destInterfaceIds.begin();
            std::advance(it, outputPortNum);
            EV << "SEPEHR: output interface ID is: " << *it << endl;
            packet->insertAtFront(ipHeader);
            packet->insertAtFront(ethHeader);
            packet->insertAtFront(phyHeader);
            packet->setFrontIteratorPosition(packetPosition);
            ie2 = ifTable->getInterfaceById(*it);
        } else {
            ie2 = ie;
        }
        dispatch(packet, ie2);
    }
    else {
        EV << "SEPEHR: Switch IS NOT using ECMP for this packet!" << endl;
        dispatch(packet, ie);
    }
}

InterfaceEntry* BouncingIeee8021dRelay::find_interface_to_bounce_randomly(Packet *packet) {
    if (!can_deflect)
        throw cRuntimeError("find_interface_to_bounce_randomly called! can deflect is false!");
    // This is how its done in DIBS
    InterfaceEntry *ie = nullptr;
    std::list<int> port_idx_connected_to_switch_neioghbors_copy = port_idx_connected_to_switch_neioghbors;
    int port_idx_connected_to_switch_neioghbors_copy_size;
    int random_index;
    std::string module_path_string;
    std::string switch_name = getParentModule()->getFullName();
    b packet_length = b(packet->getBitLength());

    // choose randomly from the list without replacement
    port_idx_connected_to_switch_neioghbors_copy_size = port_idx_connected_to_switch_neioghbors_copy.size();
    random_index = rand() % port_idx_connected_to_switch_neioghbors_copy_size;
    std::list<int>::iterator it = port_idx_connected_to_switch_neioghbors_copy.begin();
    std::advance(it, random_index);

    int num_ports_checked = 0;

    while (can_deflect){
        if (approximate_random_deflection) {
            if (num_ports_checked >=
                    port_idx_connected_to_switch_neioghbors_copy_size)
                break;
        } else {
            // if not choose a random port that is connected to a switch with available buffer space
            port_idx_connected_to_switch_neioghbors_copy_size = port_idx_connected_to_switch_neioghbors_copy.size();
            if (port_idx_connected_to_switch_neioghbors_copy_size == 0) {
                break;
            }
        }

        // This is actual dibs that we keep picking randomly until
        // every port is full
        module_path_string = switch_name + ".eth[" + std::to_string(*it) + "].mac";

        EV << "SEPEHR: Extracting info for " << module_path_string << endl;
        AugmentedEtherMac *mac = check_and_cast<AugmentedEtherMac *>(getModuleByPath(module_path_string.c_str()));
        std::string queue_full_path = module_path_string + ".queue";
        if (send_header_of_dropped_packet_to_receiver) {
            queue_full_path = module_path_string + ".queue.mainQueue";
        }

        if (!filter_out_full_ports || !mac->is_queue_full(packet_length, queue_full_path)) {
            ie = ifTable->getInterface(*it);
            EV << "The packet is randomly bounced to id: " << ie->getInterfaceId() << endl;

            std::string other_side_input_module_path = getParentModule()->gate(getParentModule()->gateBaseId("ethg$o")+ ie->getIndex())->getPathEndGate()->getFullPath();
            bool is_other_side_input_module_path_server = other_side_input_module_path.find("server") != std::string::npos;
            if (is_other_side_input_module_path_server) {
                throw cRuntimeError("The chosen bouncing port is towards a host!");
            }

            return ie;
        }

        if (approximate_random_deflection) {
            // This is for our approximation of random deflection
            // where we choose one random number and circularly send it to
            // the first non-congested port
            num_ports_checked++;
            it++;
            if (it == port_idx_connected_to_switch_neioghbors_copy.end())
                it = port_idx_connected_to_switch_neioghbors_copy.begin();
        } else {
            port_idx_connected_to_switch_neioghbors_copy.erase(it);
            port_idx_connected_to_switch_neioghbors_copy_size = port_idx_connected_to_switch_neioghbors_copy.size();
            if (port_idx_connected_to_switch_neioghbors_copy_size == 0)
                break;
            if (port_idx_connected_to_switch_neioghbors_copy_size == port_idx_connected_to_switch_neioghbors.size())
                throw cRuntimeError("You're erasing from the base array two. Probably its because of calling by reference!");
            random_index = rand() % port_idx_connected_to_switch_neioghbors_copy_size;
            it = port_idx_connected_to_switch_neioghbors_copy.begin();
            std::advance(it, random_index);
        }
    }
    EV << "Dropping the packet!" << endl;
    return nullptr;
}

InterfaceEntry* BouncingIeee8021dRelay::find_interface_to_bounce_naively() {
    if (!can_deflect)
        throw cRuntimeError("find_interface_to_bounce_naively called! can deflect is false!");
    if (port_idx_connected_to_switch_neioghbors.size() != 1)
        throw cRuntimeError("port_idx_connected_to_switch_neioghbors.size() != 1");
    // port_idx_connected_to_switch_neioghbors only has one port which is the port
    // to which we should naively deflecting packets.
    InterfaceEntry *ie = nullptr;
    std::list<int>::iterator it = port_idx_connected_to_switch_neioghbors.begin();
    ie = ifTable->getInterface(*it);
    return ie;
}

double BouncingIeee8021dRelay::get_port_utilization(int port, Packet *packet)
{
    EV << "Extracting port utilization" << endl;
    auto frame = packet->peekAtFront<EthernetMacHeader>();
    double queue_util;
    std::string switch_name = getParentModule()->getFullName();

    std::string module_path_string;
//    AugmentedEtherMac *mac = check_and_cast<AugmentedEtherMac *>(getModuleByPath(module_path_string.c_str()));
    if (frame->getBouncedHop() == 0)
    {
        // packet has never been bounced.
        module_path_string = switch_name + ".eth[" + std::to_string(port) + "].mac.queue.dataQueue";
        EV << "Extracting utilization of " << module_path_string << " for port " << port << endl;

    }
    else
    {
        // packet has been bounced at least once
        module_path_string = switch_name + ".eth[" + std::to_string(port) + "].mac.queue.bounceBackQueue";
        EV << "Extracting utilization of " << module_path_string << " for port " << port << endl;
    }
    queueing::PacketQueue *queue = check_and_cast<queueing::PacketQueue *>(getModuleByPath(module_path_string.c_str()));
    if (queue->getMaxNumPackets() != -1) {
        // Packet capacity
        queue_util = (1.0 * queue->getNumPackets()) / queue->getMaxNumPackets();
        EV << "Capacity: " << queue->getMaxNumPackets() << ", occupancy: " << queue->getNumPackets() << ", util: " << queue_util << endl;
    } else {
        // Data capacity
        queue_util = (1.0 * queue->getTotalLength().get()) / queue->getMaxTotalLength().get();
        EV << "Capacity: " << queue->getMaxTotalLength() << ", occupancy: " << queue->getTotalLength() << ", util: " << queue_util << endl;
    }

    return queue_util;
}

InterfaceEntry* BouncingIeee8021dRelay::find_a_port_for_packet_towards_source(Packet *packet) {
    InterfaceEntry *ie = nullptr;

    const auto& frame = packet->peekAtFront<EthernetMacHeader>();
    std::list<int> srcInterfaceIds = macTable->getInterfaceIdForAddress(frame->getSrc());
    srcInterfaceIds.sort();
    int portNum = srcInterfaceIds.size();
    Chunk::enableImplicitChunkSerialization = true;
    std::string protocol = packet->getName();
    bool is_packet_arp_or_broadcast = (protocol.find("arp") != std::string::npos) || (frame->getDest().isBroadcast());
    if ((!is_packet_arp_or_broadcast) && (portNum > 1)){

        if (use_power_of_n_lb) {
            std::string module_path_string;
            std::string switch_name = getParentModule()->getFullName();
            long min_queue_occupancy = -1;
            int chosen_source_index = -1;
            std::list<int> chosen_interface_ids;
            int chosen_interface_counter = 0;

            //Try finding an available port that goes towards the source of the packet. Not consider ports going towards sources
            while(chosen_interface_counter < random_power_factor && srcInterfaceIds.size() != 0) {
                int random_idx = rand() % srcInterfaceIds.size();
                std::list<int>::iterator it = srcInterfaceIds.begin();
                std::advance(it, random_idx);
                chosen_interface_counter++;
                chosen_interface_ids.push_back(*it);
                srcInterfaceIds.erase(it);
                if (srcInterfaceIds.size() == (macTable->getInterfaceIdForAddress(frame->getSrc())).size())
                    throw cRuntimeError("You have also changed the size of base array of source_interface_ids.");
            }

            if (chosen_interface_ids.size() == 0)
                throw cRuntimeError("No path towards the source. Not right...!");

            // Apply power of n: Find least congested port and forward the packet to that port
            chosen_interface_ids.sort();
            for (std::list<int>::iterator it=chosen_interface_ids.begin(); it != chosen_interface_ids.end(); it++){
                long queue_occupancy;
                int interface_index = ifTable->getInterfaceById(*it)->getIndex();
                if(bounce_probabilistically) {
                    module_path_string = switch_name + ".eth[" + std::to_string(interface_index) + "].mac.queue.bounceBackQueue";
                    queueing::PacketQueue *queue = check_and_cast<queueing::PacketQueue *>(getModuleByPath(module_path_string.c_str()));
                    if (queue->getMaxNumPackets() != -1) {
                        // Packet capacity
                        queue_occupancy = queue->getNumPackets();
                    } else {
                        // Data capacity
                        queue_occupancy = queue->getTotalLength().get();
                    }
                } else if (bounce_on_same_path) {
                    module_path_string = switch_name + ".eth[" + std::to_string(interface_index) + "].mac";
                    AugmentedEtherMac *mac = check_and_cast<AugmentedEtherMac *>(getModuleByPath(module_path_string.c_str()));
                    std::string queue_full_path = module_path_string + ".queue";
                    if (send_header_of_dropped_packet_to_receiver) {
                        queue_full_path = module_path_string + ".queue.mainQueue";
                    }
                    queue_occupancy = mac->get_queue_occupancy(queue_full_path);
                } else
                    throw cRuntimeError("Function called with a bouncing technique that doesn't require this function!");
                EV << "considering " << module_path_string << " with occupancy: " << queue_occupancy << endl;
                if (min_queue_occupancy == -1 || queue_occupancy < min_queue_occupancy) {
                    min_queue_occupancy = queue_occupancy;
                    chosen_source_index = interface_index;
                }
            }
            ie = ifTable->getInterface(chosen_source_index);
        }

        else if (use_ecmp) {
            packet->setFrontIteratorPosition(b(0));
            packet->removeAtFront<EthernetMacHeader>();
            const auto& ipv4Header = packet->removeAtFront<Ipv4Header>();
            const auto& tcpHeader = packet->removeAtFront<tcp::TcpHeader>();
            EV << "Sepehr: The flow info is: ( src_ip: " << ipv4Header->getSourceAddress() << ", dest_ip: " << ipv4Header->getDestinationAddress() << ", src_port: " << tcpHeader->getSourcePort() << ", dest_port: " << tcpHeader->getDestinationPort() << " )" << endl;
            std::string header_info = ipv4Header->getSourceAddress().str() + ipv4Header->getDestinationAddress().str() +
                                std::to_string(tcpHeader->getSourcePort()) + std::to_string(tcpHeader->getDestinationPort());
            unsigned long header_info_hash = header_hash(header_info);
            int outputPortNum;
            outputPortNum = header_info_hash % portNum;
            std::list<int>::iterator it = srcInterfaceIds.begin();
            std::advance(it, outputPortNum);
            ie = ifTable->getInterfaceById(*it);
        }
    }
    else {
        ie = ifTable->getInterfaceById(srcInterfaceIds.front());
    }
    return ie;
}

InterfaceEntry* BouncingIeee8021dRelay::find_interface_to_bounce_probabilistically(Packet *packet, InterfaceEntry *original_output_if) {
    if (!can_deflect)
            throw cRuntimeError("find_interface_to_bounce_probabilistically! can deflect is false!");
    // This is how its done in PABO
    EV << "BouncingIeee8021dRelay::find_interface_to_bounce_probabilistically called." << endl;
    std::string switch_name = getParentModule()->getFullName();
    InterfaceEntry *ie = nullptr;
    double utilization = get_port_utilization(original_output_if->getIndex(), packet);
    Packet* packet_dup = packet->dup();
    auto frame = packet->removeAtFront<EthernetMacHeader>();
    int total_hop_num = frame->getTotalHopNum();
    int bounced_distance = frame->getBouncedDistance();
    int bounced_hop = frame->getBouncedHop();
    int max_bounced_distance = frame->getMaxBouncedDistance();

    if (bounced_distance < 0 || bounced_hop < 0 || bounced_distance < 0 || total_hop_num < 0)
        throw cRuntimeError("Something is wrong with your way of calculating bouncing variables.");

    EV << "Packet's bounce distance is: " << frame->getBouncedDistance() << endl;
    if (utilization > utilization_thresh) {
        double probability_numerator = std::exp(bounce_probability_lambda * (utilization_thresh - utilization) / (bounced_hop + 1)) - 1;
        double probability_denuminator = std::exp(bounce_probability_lambda * (utilization_thresh - 1) / (bounced_hop + 1)) - 1;
        double probability = probability_numerator / probability_denuminator;

        if (probability > 1)
            throw cRuntimeError("probability > 1? Doesn't make sense.");

        double dice = dblrand();

        EV << "switch: " << switch_name << " ,port: " << original_output_if->getInterfaceId() <<
                " passing back packet " << packet->str() << "(passBackNum= " << bounced_hop <<
                " ) ,with probability = " << probability << " , dice = " << dice << endl;
        bool should_bounce = dice <= probability;
        if (should_bounce) {
            // bounce back
            bounced_hop++;
            frame->setBouncedHop(bounced_hop);
            bounced_distance++;
            frame->setBouncedDistance(bounced_distance);

            if (max_bounced_distance < bounced_distance) {
                max_bounced_distance = bounced_distance;
                frame->setMaxBouncedDistance(max_bounced_distance);
            }

            total_hop_num++;
            frame->setTotalHopNum(total_hop_num);

            // Bounce back the packet on the same path
            int arrivalInterfaceId = packet->getTag<InterfaceInd>()->getInterfaceId();
            if (arrivalInterfaceId != original_output_if->getInterfaceId())
                ie = ifTable->getInterfaceById(arrivalInterfaceId);
            else {
                ie = find_a_port_for_packet_towards_source(packet_dup);
            }

            EV << "Bouncing back frame " << frame << " with src address " << frame->getSrc() <<
                    " at the " << frame->getBouncedDistance() << " hop " << " to port " << ie->getInterfaceId() << endl;


        } else {
            // Forward the packet normally
            if(bounced_distance >= 1) {
                bounced_distance--;
                frame->setBouncedDistance(bounced_distance);
            }
            total_hop_num++;
            frame->setTotalHopNum(total_hop_num);

            ie = original_output_if;
            EV << "1) Forwarding the packet towards its original destination." << endl;
        }

    } else {
        // Forward the packet normally
        if(bounced_distance >= 1) {
            bounced_distance--;
            frame->setBouncedDistance(bounced_distance);
        }
        total_hop_num++;
        frame->setTotalHopNum(total_hop_num);

        ie = original_output_if;
        EV << "2) Forwarding the packet towards its original destination." << endl;
    }

    packet->insertAtFront(frame);
    delete packet_dup;
    return ie;

}

InterfaceEntry* BouncingIeee8021dRelay::find_interface_to_bounce_on_the_same_path(Packet *packet, InterfaceEntry *original_output_if) {
    if (!can_deflect)
        throw cRuntimeError("find_interface_to_bounce_on_the_same_path! can deflect is false!");
    // This is the main version of Vertigo which bounces everything on the same path
    std::string module_path_string;
    std::string switch_name = getParentModule()->getFullName();
    b packet_length = b(packet->getBitLength());
    int arrivalInterfaceId = packet->getTag<InterfaceInd>()->getInterfaceId();
    InterfaceEntry *ie;
    if (arrivalInterfaceId != original_output_if->getInterfaceId())
        ie = ifTable->getInterfaceById(arrivalInterfaceId);
    else {
        Packet *packet_dup = packet->dup();
        ie = find_a_port_for_packet_towards_source(packet_dup);
        delete packet_dup;
    }
    module_path_string = switch_name + ".eth[" + std::to_string(ie->getIndex()) + "].mac";
    AugmentedEtherMac *mac = check_and_cast<AugmentedEtherMac *>(getModuleByPath(module_path_string.c_str()));
    EV << "Arrival interface id is " << arrivalInterfaceId << " and it's position is " << ie->getIndex() << endl;
    std::string queue_full_path = module_path_string + ".queue";
    if (send_header_of_dropped_packet_to_receiver) {
        queue_full_path = module_path_string + ".queue.mainQueue";
    }
    if (mac->is_queue_full(packet_length, queue_full_path)) {
        EV << "Arrival interface is full as well, dropping the packet!";
        return nullptr;
    }
    EV << "I am " << getFullPath() << endl;
    EV << "The packet is bounced on the same path to id: " << ie->getInterfaceId() << endl;

    std::string other_side_input_module_path = getParentModule()->gate(getParentModule()->gateBaseId("ethg$o")+ ie->getIndex())->getPathEndGate()->getFullPath();
    bool is_other_side_input_module_path_server = other_side_input_module_path.find("server") != std::string::npos;
    if (is_other_side_input_module_path_server) {
        EV << "The chosen bouncing port is towards a host! Reversing packet mac" << endl;
        b packetPosition = packet->getFrontOffset();
        EV << "SEPEHR: This packet is a feedback packet: " << packet << endl;
        EV << "SEPEHR: Change src and dest address before going on." << endl;
        packet->setFrontIteratorPosition(b(0));
        auto phyHeader = packet->removeAtFront<EthernetPhyHeader>();
        auto frame = packet->removeAtFront<EthernetMacHeader>();
        const MacAddress dest = frame->getDest();
        const MacAddress src = frame->getSrc();
        EV << "SEPEHR: packet's current addresses are src: " << src.str() << " and dest: " << dest.str() << endl;
        frame->setSrc(dest);
        frame->setDest(src);
        packet->insertAtFront(frame);
        auto frame2 = packet->peekAtFront<EthernetMacHeader>();
        EV << "SEPEHR: packet's updated addresses are src: " << frame2->getSrc().str() << " and dest: " << frame2->getDest().str() << endl;
        packet->insertAtFront(phyHeader);
        packet->setFrontIteratorPosition(packetPosition);
        EV << "SEPEHR: new packet is: " << packet << endl;
    }

    return ie;
}

unsigned int BouncingIeee8021dRelay::update_memory_hash_maps(MacAddress mac_address, std::list<int> interface_ids) {
    EV << "BouncingIeee8021dRelay::update_memory_hash_maps called" << endl;
    uint64 address = mac_address.getInt();
    auto address_it = mac_addr_to_prefix_map.find(address);

    if (address_it != mac_addr_to_prefix_map.end()) {
        // we have the group id for this address
        EV << "Address " << address << " is already recorded in mac_addr_to_prefix_map."
                " returning " << address_it->second << endl;
        return address_it->second;
    }

    std::string group_id_str = "";
    for (auto interface_id_it = interface_ids.begin();
            interface_id_it != interface_ids.end();
            interface_id_it++) {
        group_id_str += std::to_string(*interface_id_it);
        group_id_str += ",";
    }

    auto group_id_str_it = group_to_prefix_map.find(group_id_str);
    if (group_id_str_it != group_to_prefix_map.end()) {
        // the ecmp group was seen before just not for this address
        mac_addr_to_prefix_map.insert(
                            std::pair<uint64,
                            unsigned int>(address,
                                    group_id_str_it->second));
        EV << "Group " << group_id_str << " is already recorded in group_to_prefix_map."
                        " returning " << group_id_str_it->second << endl;
        return group_id_str_it->second;
    }

    // never seen this group for any address before
    unsigned int group_id = prefix_id_counter;
    prefix_id_counter++;
    group_to_prefix_map.insert(
                    std::pair<std::string, unsigned int>(group_id_str, group_id));
    mac_addr_to_prefix_map.insert( std::pair<uint64, unsigned int>(address,
                                        group_id));
    std::list<int> empty_memory_list;
    for (int i = 0; i < random_power_memory_size; i++)
        empty_memory_list.push_back(-1);
    prefix_to_memory_map.insert(std::pair<unsigned int,
            std::list<int>>(group_id, empty_memory_list));
    EV << "Created new group, returning " << group_id << endl;
    return group_id;
}

InterfaceEntry* BouncingIeee8021dRelay::find_interface_to_fw_randomly_power_of_n(Packet *packet, bool consider_servers) {
    // This forwards or bounces the packet randomly to ports with free space using power of two choices
    InterfaceEntry *ie;
    std::string module_path_string;
    std::string switch_name = getParentModule()->getFullName();
    b packet_length = b(packet->getBitLength());
    const auto& frame = packet->peekAtFront<EthernetMacHeader>();
    long min_queue_occupancy = -1, max_queue_occupancy = -1;
    int chosen_source_index = -1;
    std::list<int> chosen_interface_indexes;
    int chosen_interface_counter = 0;
    const MacAddress address = frame->getDest();
    EV << "Finding port for " << address << endl;
    std::list<int> interface_ids = macTable->getInterfaceIdForAddress(address);
    interface_ids.sort();
    bool have_more_than_one_fw_ports = (interface_ids.size() > 1);

    unsigned int group_id;
    // this makes sure that we do not consider a port multiple times
    // to avoid messing with 50-50 chance in tie breaking
    std::unordered_set<int> chosen_interfaces_set;
    if (use_memory && have_more_than_one_fw_ports) {
        group_id = update_memory_hash_maps(address, interface_ids);
    }

    //Try finding an available port that goes towards the source of the packet.
    // Consider ports towards sources for forwarding but not for bouncing
    while(chosen_interface_counter < random_power_factor && interface_ids.size() != 0) {
        int random_idx = rand() % interface_ids.size();
        EV << "randomly choosing one of the ports. random_idx is " << random_idx << endl;
        std::list<int>::iterator it = interface_ids.begin();
        std::advance(it, random_idx);
        int interface_idx = ifTable->getInterfaceById(*it)->getIndex();
        module_path_string = switch_name + ".eth[" + std::to_string(interface_idx) + "].mac";
        EV << "finding main ports: " << module_path_string << endl;
        AugmentedEtherMac *mac = check_and_cast<AugmentedEtherMac *>(getModuleByPath(module_path_string.c_str()));
        std::string other_side_input_module_path = getParentModule()->gate(getParentModule()->gateBaseId("ethg$o")+ interface_idx)->getPathEndGate()->getFullPath();
        bool is_other_side_input_module_path_server = other_side_input_module_path.find("server") != std::string::npos;

        if (consider_servers || !is_other_side_input_module_path_server) {
            // I count the chosen ports but only add those with free
            // ports because those that are full do not matter.
            chosen_interface_counter++;
            std::string queue_full_path = module_path_string + ".queue";
            if (send_header_of_dropped_packet_to_receiver) {
                queue_full_path = module_path_string + ".queue.mainQueue";
            }
            if (use_vertigo_prio_queue || use_pfabric || use_v2_pifo || !mac->is_queue_full(packet_length, queue_full_path)) {
                chosen_interface_indexes.push_back(interface_idx);
                chosen_interfaces_set.insert(interface_idx);
                EV << "Adding port: " << module_path_string << endl;
            }
        }
        interface_ids.erase(it);
        if (interface_ids.size() == (macTable->getInterfaceIdForAddress(address)).size())
            throw cRuntimeError("You have also changed the size of base array of source_interface_ids.");
    }

    // add the memory: the last element in the memory is the latest used
    std::unordered_map<unsigned int, std::list<int>>::iterator memory_map_it;
    if (use_memory && have_more_than_one_fw_ports) {
        EV << "Adding memory" << endl;
        int memory_counter = 0;
        memory_map_it = prefix_to_memory_map.find(group_id);
        auto memory_it = memory_map_it->second.end();
        while (memory_counter < random_power_memory_size) {
            if (memory_it == memory_map_it->second.begin()) {
                throw cRuntimeError("How is -- memory_it == memory_list.begin() -- possible?");
            }
            memory_it--;
            int interface_idx = (*memory_it);
            EV << "interface_idx is " << interface_idx << endl;
            memory_counter++;
            if (interface_idx >= 0) {
                // a memory is saved
                module_path_string = switch_name + ".eth[" + std::to_string(interface_idx) + "].mac";
                EV << "finding main ports: " << module_path_string << endl;
                AugmentedEtherMac *mac = check_and_cast<AugmentedEtherMac *>(getModuleByPath(module_path_string.c_str()));
                std::string other_side_input_module_path = getParentModule()->gate(getParentModule()->gateBaseId("ethg$o")+ interface_idx)->getPathEndGate()->getFullPath();
                bool is_other_side_input_module_path_server = other_side_input_module_path.find("server") != std::string::npos;

                if (consider_servers || !is_other_side_input_module_path_server) {
                    // I count the chosen ports but only add those with free
                    // ports because those that are full do not matter.
                    std::string queue_full_path = module_path_string + ".queue";
                    if (send_header_of_dropped_packet_to_receiver) {
                        queue_full_path = module_path_string + ".queue.mainQueue";
                    }
                    if (use_vertigo_prio_queue || use_pfabric || use_v2_pifo || !mac->is_queue_full(packet_length, queue_full_path)) {
                        if (chosen_interfaces_set.find(interface_idx) == chosen_interfaces_set.end()) {
                            chosen_interface_indexes.push_back(interface_idx);
                            chosen_interfaces_set.insert(interface_idx);
                            EV << "Adding memory port: " << module_path_string << endl;
                        }
                    }
                }
            }
        }
    }


    if (chosen_interface_indexes.size() == 0) {
        EV << "No available ports was found to forward the packet normally" << endl;
        return nullptr;
    }

    // Apply power of n: Find least congested port and forward the packet to that port
    chosen_interface_indexes.sort();
    for (std::list<int>::iterator it=chosen_interface_indexes.begin(); it != chosen_interface_indexes.end(); it++){
        module_path_string = switch_name + ".eth[" + std::to_string(*it) + "].mac";
        AugmentedEtherMac *mac = check_and_cast<AugmentedEtherMac *>(getModuleByPath(module_path_string.c_str()));
        std::string queue_full_path = module_path_string + ".queue";
        if (send_header_of_dropped_packet_to_receiver) {
            queue_full_path = module_path_string + ".queue.mainQueue";
        }
        long queue_occupancy = mac->get_queue_occupancy(queue_full_path);
        EV << "considering " << module_path_string << " with occupancy: " << queue_occupancy << endl;
        if (min_queue_occupancy == -1 || queue_occupancy < min_queue_occupancy) {
            min_queue_occupancy = queue_occupancy;
            chosen_source_index = *it;
        } else if (queue_occupancy == min_queue_occupancy) {
            // two equally full buffers, break the tie randomly
            double dice = dblrand();
            EV << "Two ports with occupancy of " << min_queue_occupancy << ". Breaking tie: dice is " << dice << endl;
            if (dice >= 0.5) {
                // 50% update the chosen source idx
                chosen_source_index = *it;
            }
        }
    }

    EV << "Chosen port: " << chosen_source_index << endl;


    if (use_memory && have_more_than_one_fw_ports) {
        // if the chosen source index is already in the list we don't need to update the memory
        bool found = false;
        // front is written last, so pop_front would return -1 if there is space in memory
        int last_element_in_mem = memory_map_it->second.front();
        std::list<int>::iterator max_occupancy_port_in_memory_idx;
        for (auto list_it = memory_map_it->second.begin();
                list_it != memory_map_it->second.end();
                list_it++) {
            if ((*list_it) == chosen_source_index) {
                EV << "chosen_source_index is already in the memory!" << endl;
                found = true;
                break;
            }
            if (last_element_in_mem != -1) {
                // if max_occupancy_port_in_memory_idx has -1 in it which is the initial value in
                // memory, then we have enough space in memory for a new record and do not need
                // to remove one, make sure to always push_back
                // entering this if means that all the initial -1s in memory are over-written
                module_path_string = switch_name + ".eth[" + std::to_string(*list_it) + "].mac";
                AugmentedEtherMac *mac = check_and_cast<AugmentedEtherMac *>(getModuleByPath(module_path_string.c_str()));
                std::string queue_full_path = module_path_string + ".queue";
                if (send_header_of_dropped_packet_to_receiver) {
                    queue_full_path = module_path_string + ".queue.mainQueue";
                }
                long queue_occupancy = mac->get_queue_occupancy(queue_full_path);
                if (max_queue_occupancy == -1 || queue_occupancy > max_queue_occupancy) {
                    max_queue_occupancy = queue_occupancy;
                    max_occupancy_port_in_memory_idx = list_it;
                } else if (queue_occupancy == max_queue_occupancy) {
                    // two equally full buffers, break the tie randomly
                    double dice = dblrand();
                    EV << "Two ports in memory with occupancy of " << max_queue_occupancy << ". Breaking tie: dice is " << dice << endl;
                    if (dice >= 0.5) {
                        // 50% update the chosen source idx
                        max_occupancy_port_in_memory_idx = list_it;
                    }
                }
                EV << "max_occupancy_port_in_memory_idx: " << (*max_occupancy_port_in_memory_idx) << endl;
            }
        }

        if (!found) {
            if (last_element_in_mem == -1) {
                // enough space in memory (it still has -1)
                EV << "Enough space in memory... no need to remove any record!" << endl;
                memory_map_it->second.pop_front();
                memory_map_it->second.push_back(chosen_source_index);
            } else {
                // should replace
                module_path_string = switch_name + ".eth[" + std::to_string(chosen_source_index) + "].mac";
                AugmentedEtherMac *mac = check_and_cast<AugmentedEtherMac *>(getModuleByPath(module_path_string.c_str()));
                std::string queue_full_path = module_path_string + ".queue";
                if (send_header_of_dropped_packet_to_receiver) {
                    queue_full_path = module_path_string + ".queue.mainQueue";
                }
                long queue_occupancy = mac->get_queue_occupancy(queue_full_path);
                if (queue_occupancy < max_queue_occupancy) {
                    // replace it
                    EV << "Replacing " << (*max_occupancy_port_in_memory_idx) << " with " << chosen_source_index << endl;
                    auto memory_replace_it = memory_map_it->second.erase(max_occupancy_port_in_memory_idx);
                    memory_map_it->second.push_back(chosen_source_index);
                }
            }
        }
        if (memory_map_it->second.size() != random_power_memory_size)
            throw cRuntimeError("memory_list.size() != random_power_memory_size");
    }

    module_path_string = switch_name + ".eth[" + std::to_string(chosen_source_index) + "].mac";
    EV << "Chosen port: " << module_path_string << endl;
    AugmentedEtherMac *mac = check_and_cast<AugmentedEtherMac *>(getModuleByPath(module_path_string.c_str()));
    ie = ifTable->getInterface(chosen_source_index);

    if (!consider_servers) {
        std::string other_side_input_module_path = getParentModule()->gate(getParentModule()->gateBaseId("ethg$o")+ ie->getIndex())->getPathEndGate()->getFullPath();
        bool is_other_side_input_module_path_server = other_side_input_module_path.find("server") != std::string::npos;
        if (is_other_side_input_module_path_server) {
            throw cRuntimeError("The chosen bouncing port is towards a host!");
        }
    }

    return ie;
}

void BouncingIeee8021dRelay::apply_early_deflection(Packet *packet, bool consider_servers, InterfaceEntry *ie2) {

    // bounce the packet

    EV << "BouncingIeee8021dRelay::apply_early_deflection called." << endl;
//    std::cout << simTime() << ": applying early deflection!" << endl;

    std::string module_path_string;
    std::string switch_name = getParentModule()->getFullName();
    module_path_string = switch_name + ".eth[" + std::to_string(ie2->getIndex()) + "].mac";

    // This forwards or bounces the packet randomly to ports using power of two choices
    std::string queue_full_path;
    InterfaceEntry *ie;
    EV << "Bouncing the ejected packet " << packet->str() << endl;
    b packet_length = b(packet->getBitLength());
    const auto& frame = packet->peekAtFront<EthernetMacHeader>();
    long min_queue_occupancy = -1;
    int chosen_source_index = -1;
    std::list<int> chosen_interface_indexes;
    int chosen_interface_counter = 0;

    // we choose two random ports and drop if both of them are full
    chosen_interface_counter = 0;
    std::list<int> port_idx_connected_to_switch_neioghbors_copy = port_idx_connected_to_switch_neioghbors;

    while(chosen_interface_counter < random_power_bounce_factor && port_idx_connected_to_switch_neioghbors_copy.size() != 0) {
        int random_idx = rand() % port_idx_connected_to_switch_neioghbors_copy.size();
        EV << "randomly choosing a port for bounce towards neighbor switches. random_idx is " << random_idx << endl;
        std::list<int>::iterator it = port_idx_connected_to_switch_neioghbors_copy.begin();
        std::advance(it, random_idx);
        module_path_string = switch_name + ".eth[" + std::to_string(*it) + "].mac";
        EV << "finding additional ports: " << module_path_string << endl;
        AugmentedEtherMac *mac = check_and_cast<AugmentedEtherMac *>(getModuleByPath(module_path_string.c_str()));
        queue_full_path = module_path_string + ".queue";
        if (!mac->is_queue_over_v2_threshold(packet_length, queue_full_path, packet) ||
                !mac->is_packet_tag_larger_than_last_packet(queue_full_path, packet)) {
            // We don't want to add the port only if we are using v2_pifo and we are not planning
            // to use NDP, accordingly, we leave the packet to get dropped in the queue
            chosen_interface_indexes.push_back(*it);
            EV << "Adding port: " << module_path_string << endl;
        }

        chosen_interface_counter++;
        port_idx_connected_to_switch_neioghbors_copy.erase(it);
        if (port_idx_connected_to_switch_neioghbors.size() == port_idx_connected_to_switch_neioghbors_copy.size())
            throw cRuntimeError("2)You have also changed the size of base array of port_idx_connected_to_switch_neioghbors.");
    }

    if (chosen_interface_indexes.size() == 0) {
        // Cannot bounce this packet, drop it and get to the next packet
        EV << "No available ports was found to bounce the packet, drop the bounced packet." << endl;
//            emit(feedBackPacketDroppedSignal, int(frame->getIs_bursty()));
//            emit(feedBackPacketDroppedPortSignal, ie2->getIndex());
        light_in_relay_packet_drop_counter++;
        delete packet;
        return;
    }

    // Apply power of n: Find least congested port and forward the packet to that port
    chosen_interface_indexes.sort();
    for (std::list<int>::iterator it=chosen_interface_indexes.begin(); it != chosen_interface_indexes.end(); it++){
        module_path_string = switch_name + ".eth[" + std::to_string(*it) + "].mac";
        AugmentedEtherMac *mac = check_and_cast<AugmentedEtherMac *>(getModuleByPath(module_path_string.c_str()));
        std::string queue_full_path = module_path_string + ".queue";
        if (send_header_of_dropped_packet_to_receiver) {
            queue_full_path = module_path_string + ".queue.mainQueue";
        }
        long queue_occupancy = mac->get_queue_occupancy(queue_full_path);
        EV << "considering " << module_path_string << " with occupancy: " << queue_occupancy << endl;
        if (min_queue_occupancy == -1 || queue_occupancy < min_queue_occupancy) {
            min_queue_occupancy = queue_occupancy;
            chosen_source_index = *it;
        } else if (queue_occupancy == min_queue_occupancy) {
            // two equally full buffers, break the tie randomly
            double dice = dblrand();
            EV << "Two ports with occupancy of " << min_queue_occupancy << ". Breaking tie: dice is " << dice << endl;
            if (dice >= 0.5) {
                // 50% update the chosen source idx
                chosen_source_index = *it;
            }
        }
    }
    if (send_header_of_dropped_packet_to_receiver) {
        module_path_string = switch_name + ".eth[" + std::to_string(ie2->getIndex()) + "].mac.queue.mainQueue";
    } else {
        module_path_string = switch_name + ".eth[" + std::to_string(ie2->getIndex()) + "].mac.queue";
    }

    module_path_string = switch_name + ".eth[" + std::to_string(chosen_source_index) + "].mac";
    EV << "Chosen port: " << module_path_string << endl;
    AugmentedEtherMac *mac = check_and_cast<AugmentedEtherMac *>(getModuleByPath(module_path_string.c_str()));
    ie = ifTable->getInterface(chosen_source_index);

    queue_full_path =  module_path_string + ".queue";
    if (send_header_of_dropped_packet_to_receiver) {
        queue_full_path = module_path_string + ".queue.mainQueue";
    }

    // See if you should push the packet or drop it
    if (mac->is_queue_full(packet_length, queue_full_path) ||
                        (mac->is_queue_over_v2_threshold(packet_length, queue_full_path, packet) &&
                        mac->is_packet_tag_larger_than_last_packet(queue_full_path, packet))) {
        EV << "Dropping deflected packet packet to " << queue_full_path << endl;
//        std::cout << simTime() << ": Dropping deflected packet packet to " << queue_full_path << endl;
        light_in_relay_packet_drop_counter++;
        delete packet;
        return;
    }

    // Send packet
    auto mac_header = packet->peekAtFront<EthernetMacHeader>();
    mac->add_on_the_way_packet(b(packet->getBitLength()), mac_header->getIs_v2_dropped_packet_header());
    EV << module_path_string << ": ";
    if (!consider_servers) {
        std::string other_side_input_module_path = getParentModule()->gate(getParentModule()->gateBaseId("ethg$o")+ ie->getIndex())->getPathEndGate()->getFullPath();
        bool is_other_side_input_module_path_server = other_side_input_module_path.find("server") != std::string::npos;
        if (is_other_side_input_module_path_server) {
            throw cRuntimeError("The chosen bouncing port is towards a host!");
        }
    }

    emit(feedBackPacketGeneratedSignal, packet->getId());
    EV << "Sending frame " << packet << " on output interface " << ie->getFullName() << " with destination = " << frame->getDest() << endl;
    numDispatchedNonBPDUFrames++;
    auto oldPacketProtocolTag = packet->removeTag<PacketProtocolTag>();
    packet->clearTags();
    auto newPacketProtocolTag = packet->addTag<PacketProtocolTag>();
    *newPacketProtocolTag = *oldPacketProtocolTag;
    delete oldPacketProtocolTag;
    packet->addTag<InterfaceReq>()->setInterfaceId(ie->getInterfaceId());
    packet->trim();
    emit(packetSentToLowerSignal, packet);
    send(packet, "ifOut");
}

void BouncingIeee8021dRelay::find_interface_to_bounce_randomly_v2(Packet *packet, bool consider_servers, InterfaceEntry *ie2) {
    if (!can_deflect)
        throw cRuntimeError("find_interface_to_bounce_randomly_v2! can deflect is false!");
    // eject the packets to create room for packet
    EV << "find_interface_to_bounce_randomly_v2 called!" << endl;
    std::string module_path_string;
    std::string switch_name = getParentModule()->getFullName();
    b packetPosition = packet->getFrontOffset();
    packet->setFrontIteratorPosition(b(0));
    auto phyHeader = packet->removeAtFront<EthernetPhyHeader>();
    auto ethHeader = packet->removeAtFront<EthernetMacHeader>();
    auto ipHeader = packet->removeAtFront<Ipv4Header>();

    unsigned long seq, ret_count;
    for (unsigned int i = 0; i < ipHeader->getOptionArraySize(); i++) {
        const TlvOptionBase *option = &ipHeader->getOption(i);
        if (option->getType() == IPOPTION_V2_MARKING) {
            auto opt = check_and_cast<const Ipv4OptionV2Marking*>(option);
            seq = opt->getSeq();
            ret_count = opt->getRet_num();
            break;
        }
        // Check if something is returned
        if (i == ipHeader->getOptionArraySize() - 1)
            throw cRuntimeError("Marking cannot be off at this position!");
    }

    EV << "Packet's seq = " << seq << " and ret_count = " << ret_count << endl;

    packet->insertAtFront(ipHeader);
    packet->insertAtFront(ethHeader);
    packet->insertAtFront(phyHeader);
    packet->setFrontIteratorPosition(packetPosition);
    const auto& frame = packet->peekAtFront<EthernetMacHeader>();
    PacketQueue* queue;
    if (send_header_of_dropped_packet_to_receiver) {
        module_path_string = switch_name + ".eth[" + std::to_string(ie2->getIndex()) + "].mac.queue.mainQueue";
    } else {
        module_path_string = switch_name + ".eth[" + std::to_string(ie2->getIndex()) + "].mac.queue";
    }
    if (use_v2_pifo) {
        queue = check_and_cast<V2PIFO *>(getModuleByPath(module_path_string.c_str()));
    } else if (use_pfabric)
        queue = check_and_cast<pFabric *>(getModuleByPath(module_path_string.c_str()));
    module_path_string = switch_name + ".eth[" + std::to_string(ie2->getIndex()) + "].mac";

    AugmentedEtherMac *mac = check_and_cast<AugmentedEtherMac *>(getModuleByPath(module_path_string.c_str()));
    std::string queue_full_path = module_path_string + ".queue";
    if (send_header_of_dropped_packet_to_receiver) {
        queue_full_path = module_path_string + ".queue.mainQueue";
    }
    int extraction_port_interface_id = mac->interfaceEntry->getInterfaceId();
    EV << "Trying to inject packet" << packet->str() << " with seq=" << seq << endl;
    EV << module_path_string << ": ";
    int num_packets_to_eject = queue->getNumPacketsToEject(b(packet->getBitLength()), seq, ret_count,
            mac->on_the_way_packet_num, mac->on_the_way_packet_length);
    EV << "Number of packets to eject is " << num_packets_to_eject << endl;
    std::list<Packet*> ejected_packets;
    bool deflecting_original_packet = false;
    if (num_packets_to_eject < 0) {
        // bounce packet itself
        // we have already generated an SRC if the type is org or org_def
        // so here just check if type is def and generate src in that case because we are deflecting
        // the packet itself
        // we send the SRC here if we're deflecting original packet and if we're not, we send it
        // in the while loop
        deflecting_original_packet = true;
        EV << "Adding the main packets to the list to be bounced" << endl;
        if (src_enabled_packet_type == BOLT_VERTIGO_DEF_PKT)
            bolt_evaluate_if_src_packet_should_be_generated(packet, ie2, ignore_cc_thresh_for_deflected_packets);
        if (dctcp_mark_deflected_packets_only)
            dctcp_mark_ecn_for_deflected_packets(packet);
        ejected_packets.push_back(packet);
    } else {
        // eject packets to make room for the main packet
        EV << "Ejecting packets from the queue to be bounced." << endl;
        module_path_string = switch_name + ".eth[" + std::to_string(ie2->getIndex()) + "].mac";
        AugmentedEtherMac *mac = check_and_cast<AugmentedEtherMac *>(getModuleByPath(module_path_string.c_str()));
        auto frame = packet->peekAtFront<EthernetMacHeader>();
        mac->add_on_the_way_packet(b(packet->getBitLength()), frame->getIs_v2_dropped_packet_header());

        ejected_packets = queue->eject_and_push(num_packets_to_eject);

        // send the main packet to the output queue
        EV << "Sending the main packet " << packet << " on output interface " << ie2->getFullName() << " with destination = " << frame->getDest() << endl;
        numDispatchedNonBPDUFrames++;
        auto oldPacketProtocolTag = packet->removeTag<PacketProtocolTag>();
        packet->clearTags();
        auto newPacketProtocolTag = packet->addTag<PacketProtocolTag>();
        *newPacketProtocolTag = *oldPacketProtocolTag;
        delete oldPacketProtocolTag;
        packet->addTag<InterfaceReq>()->setInterfaceId(ie2->getInterfaceId());
        packet->addTag<PacketInsertionControlTag>()->setInsert_packet_without_checking_the_queue(true);
        packet->trim();
        emit(packetSentToLowerSignal, packet);
        send(packet, "ifOut");
    }

    // bounce the remaining packets
    while (ejected_packets.size() > 0){

        // This forwards or bounces the packet randomly to ports with free space using power of two choices
        InterfaceEntry *ie;
        auto packet = ejected_packets.front();

        if (!deflecting_original_packet &&
                (src_enabled_packet_type == BOLT_VERTIGO_DEF_PKT ||
                        src_enabled_packet_type == BOLT_VERTIGO_ORG_DEF_PKT)) {
            bolt_evaluate_if_src_packet_should_be_generated(packet, ie2, ignore_cc_thresh_for_deflected_packets,
                    false, extraction_port_interface_id);
        }

        if (!deflecting_original_packet && dctcp_mark_deflected_packets_only)
            dctcp_mark_ecn_for_deflected_packets(packet, false);

        if (deflecting_original_packet)
            mark_packet_deflection_tag(packet);
        else
            mark_packet_deflection_tag(packet, false);

        EV << "Bouncing the ejected packet " << packet->str() << endl;
        ejected_packets.pop_front();
        b packet_length = b(packet->getBitLength());
        const auto& frame = packet->peekAtFront<EthernetMacHeader>();
        long min_queue_occupancy = -1, max_queue_occupancy = -1;
        int chosen_source_index = -1;
        std::list<int> chosen_interface_indexes;
        int chosen_interface_counter = 0;

        // we choose two random ports and drop if both of them are full
        chosen_interface_counter = 0;
        std::list<int> port_idx_connected_to_switch_neioghbors_copy = port_idx_connected_to_switch_neioghbors;
        std::unordered_set<int> chosen_interfaces_set;
        while(chosen_interface_counter < random_power_bounce_factor && port_idx_connected_to_switch_neioghbors_copy.size() != 0) {
            int random_idx = rand() % port_idx_connected_to_switch_neioghbors_copy.size();
            EV << "randomly choosing a port for bounce towards neighbor switches. random_idx is " << random_idx << endl;
            std::list<int>::iterator it = port_idx_connected_to_switch_neioghbors_copy.begin();
            std::advance(it, random_idx);
            module_path_string = switch_name + ".eth[" + std::to_string(*it) + "].mac";
            EV << "finding additional ports: " << module_path_string << endl;
            mac = check_and_cast<AugmentedEtherMac *>(getModuleByPath(module_path_string.c_str()));
            queue_full_path = module_path_string + ".queue";
            if (((use_pfabric || use_v2_pifo) && !drop_bounced_in_relay) || !mac->is_queue_full(packet_length, queue_full_path)) {
                // We don't want to add the port only if we are using v2_pifo and we are not planning
                // to use NDP, accordingly, we leave the packet to get dropped in the queue
                chosen_interface_indexes.push_back(*it);
                chosen_interfaces_set.insert(*it);
                EV << "Adding port: " << module_path_string << endl;
            }

            chosen_interface_counter++;
            port_idx_connected_to_switch_neioghbors_copy.erase(it);
            if (port_idx_connected_to_switch_neioghbors.size() == port_idx_connected_to_switch_neioghbors_copy.size())
                throw cRuntimeError("2)You have also changed the size of base array of port_idx_connected_to_switch_neioghbors.");
        }

        // add the memory: the last element in the memory is the latest used
        if (use_memory) {
            EV << "Adding memory" << endl;
            int memory_counter = 0;
            auto memory_it = deflection_memory.end();
            while (memory_counter < random_power_bounce_memory_size) {
                if (memory_it == deflection_memory.begin())
                    throw cRuntimeError("Deflection: How is -- memory_it == memory_list.begin() -- possible?");
                memory_it--;
                memory_counter++;
                int interface_idx = (*memory_it);
                EV << "Interface_idx for deflection is " << interface_idx << endl;
                if (interface_idx >= 0) {
                    // a memory is saved
                    module_path_string = switch_name + ".eth[" + std::to_string(interface_idx) + "].mac";
                    EV << "finding main ports: " << module_path_string << endl;
                    AugmentedEtherMac *mac = check_and_cast<AugmentedEtherMac *>(getModuleByPath(module_path_string.c_str()));
                    std::string other_side_input_module_path = getParentModule()->gate(getParentModule()->gateBaseId("ethg$o")+ interface_idx)->getPathEndGate()->getFullPath();
                    bool is_other_side_input_module_path_server = other_side_input_module_path.find("server") != std::string::npos;
                    if (is_other_side_input_module_path_server)
                        throw cRuntimeError("Why do we have deflection toward server in memory");
                    // I count the chosen ports but only add those with free
                    // ports because those that are full do not matter.
                    std::string queue_full_path = module_path_string + ".queue";
                    if (send_header_of_dropped_packet_to_receiver) {
                        queue_full_path = module_path_string + ".queue.mainQueue";
                    }
                    if (((use_pfabric || use_v2_pifo) && !drop_bounced_in_relay) || !mac->is_queue_full(packet_length, queue_full_path)) {
                        if (chosen_interfaces_set.find(interface_idx) == chosen_interfaces_set.end()) {
                            chosen_interface_indexes.push_back(interface_idx);
                            chosen_interfaces_set.insert(interface_idx);
                            EV << "Adding memory port: " << module_path_string << endl;
                        }
                    }
                }
            }
        }

        if (chosen_interface_indexes.size() == 0) {
            // Cannot bounce this packet, drop it and get to the next packet
            EV << "No available ports was found to bounce the packet, drop the bounced packet." << endl;
//            emit(feedBackPacketDroppedSignal, int(frame->getIs_bursty()));
//            emit(feedBackPacketDroppedPortSignal, ie2->getIndex());
            light_in_relay_packet_drop_counter++;
            delete packet;
            continue;
        }

        // Apply power of n: Find least congested port and forward the packet to that port
        chosen_interface_indexes.sort();
        for (std::list<int>::iterator it=chosen_interface_indexes.begin(); it != chosen_interface_indexes.end(); it++){
            module_path_string = switch_name + ".eth[" + std::to_string(*it) + "].mac";
            AugmentedEtherMac *mac = check_and_cast<AugmentedEtherMac *>(getModuleByPath(module_path_string.c_str()));
            std::string queue_full_path = module_path_string + ".queue";
            if (send_header_of_dropped_packet_to_receiver) {
                queue_full_path = module_path_string + ".queue.mainQueue";
            }
            long queue_occupancy = mac->get_queue_occupancy(queue_full_path);
            EV << "considering " << module_path_string << " with occupancy: " << queue_occupancy << endl;
            if (min_queue_occupancy == -1 || queue_occupancy < min_queue_occupancy) {
                min_queue_occupancy = queue_occupancy;
                chosen_source_index = *it;
            } else if (queue_occupancy == min_queue_occupancy) {
                // two equally full buffers, break the tie randomly
                double dice = dblrand();
                EV << "Two ports with occupancy of " << min_queue_occupancy << ". Breaking tie: dice is " << dice << endl;
                if (dice >= 0.5) {
                    // 50% update the chosen source idx
                    chosen_source_index = *it;
                }
            }
        }
        if (send_header_of_dropped_packet_to_receiver) {
            module_path_string = switch_name + ".eth[" + std::to_string(ie2->getIndex()) + "].mac.queue.mainQueue";
        } else {
            module_path_string = switch_name + ".eth[" + std::to_string(ie2->getIndex()) + "].mac.queue";
        }

        if (use_memory) {
            // if the chosen source index is already in the list we don't need to update the memory
            bool found = false;
            // front is written last, so pop_front would return -1 if there is space in memory
            int last_element_in_mem = deflection_memory.front();
            std::list<int>::iterator max_occupancy_port_in_memory_idx;
            for (auto list_it = deflection_memory.begin();
                    list_it != deflection_memory.end();
                    list_it++) {
                if ((*list_it) == chosen_source_index) {
                    EV << "chosen_source_index for deflection is already in the memory!" << endl;
                    found = true;
                    break;
                }
                if (last_element_in_mem != -1) {
                    // if max_occupancy_port_in_memory_idx has -1 in it which is the initial value in
                    // memory, then we have enough space in memory for a new record and do not need
                    // to remove one, make sure to always push_back
                    // entering this if means that all the initial -1s in memory are over-written
                    module_path_string = switch_name + ".eth[" + std::to_string(*list_it) + "].mac";
                    AugmentedEtherMac *mac = check_and_cast<AugmentedEtherMac *>(getModuleByPath(module_path_string.c_str()));
                    std::string queue_full_path = module_path_string + ".queue";
                    if (send_header_of_dropped_packet_to_receiver) {
                        queue_full_path = module_path_string + ".queue.mainQueue";
                    }
                    long queue_occupancy = mac->get_queue_occupancy(queue_full_path);
                    if (max_queue_occupancy == -1 || queue_occupancy > max_queue_occupancy) {
                        max_queue_occupancy = queue_occupancy;
                        max_occupancy_port_in_memory_idx = list_it;
                    } else if (queue_occupancy == max_queue_occupancy) {
                        // two equally full buffers, break the tie randomly
                        double dice = dblrand();
                        EV << "Two ports in memory with occupancy of " << max_queue_occupancy << ". Breaking tie: dice is " << dice << endl;
                        if (dice >= 0.5) {
                            // 50% update the chosen source idx
                            max_occupancy_port_in_memory_idx = list_it;
                        }
                    }
                    EV << "max_occupancy_port_in_memory_idx: " << (*max_occupancy_port_in_memory_idx) << endl;
                }
            }
            if (!found) {
                if (last_element_in_mem == -1) {
                    // enough space in memory (it still has -1)
                    EV << "Enough space in memory... no need to remove any record!" << endl;
                    deflection_memory.pop_front();
                    deflection_memory.push_back(chosen_source_index);
                } else {
                    // should replace
                    module_path_string = switch_name + ".eth[" + std::to_string(chosen_source_index) + "].mac";
                    AugmentedEtherMac *mac = check_and_cast<AugmentedEtherMac *>(getModuleByPath(module_path_string.c_str()));
                    std::string queue_full_path = module_path_string + ".queue";
                    if (send_header_of_dropped_packet_to_receiver) {
                        queue_full_path = module_path_string + ".queue.mainQueue";
                    }
                    long queue_occupancy = mac->get_queue_occupancy(queue_full_path);
                    if (queue_occupancy < max_queue_occupancy) {
                        // replace it
                        EV << "Replacing " << (*max_occupancy_port_in_memory_idx) << " with " << chosen_source_index << endl;
                        auto memory_replace_it = deflection_memory.erase(max_occupancy_port_in_memory_idx);
                        deflection_memory.push_back(chosen_source_index);
                    }
                }
            }
            if (deflection_memory.size() != random_power_bounce_memory_size)
                throw cRuntimeError("deflection_memory.size() != random_power_bounce_memory_size");
        }

        module_path_string = switch_name + ".eth[" + std::to_string(chosen_source_index) + "].mac";
        EV << "Chosen port: " << module_path_string << endl;
        AugmentedEtherMac *mac = check_and_cast<AugmentedEtherMac *>(getModuleByPath(module_path_string.c_str()));
        ie = ifTable->getInterface(chosen_source_index);

        queue_full_path =  module_path_string + ".queue";
        if (send_header_of_dropped_packet_to_receiver) {
            queue_full_path = module_path_string + ".queue.mainQueue";
        }

        if (send_header_of_dropped_packet_to_receiver && mac->is_queue_full(packet_length, queue_full_path)) {
            std::list<Packet*> ejected_packets;
            // Eject from the queue you are bouncing to
            EV << "Ejecting packets from the queue to to make room for the bounced packet." << endl;
            module_path_string = switch_name + ".eth[" + std::to_string(chosen_source_index) + "].mac";
            AugmentedEtherMac *mac = check_and_cast<AugmentedEtherMac *>(getModuleByPath(module_path_string.c_str()));
            module_path_string += ".queue.mainQueue";
            V2PIFO *queue = check_and_cast<V2PIFO *>(getModuleByPath(module_path_string.c_str()));

            auto packet_dup = packet->dup();
            packet_dup->removeAtFront<EthernetMacHeader>();
            auto ipHeader = packet_dup->removeAtFront<Ipv4Header>();
            delete packet_dup;

            unsigned long seq, ret_count;
            for (unsigned int i = 0; i < ipHeader->getOptionArraySize(); i++) {
                const TlvOptionBase *option = &ipHeader->getOption(i);
                if (option->getType() == IPOPTION_V2_MARKING) {
                    auto opt = check_and_cast<const Ipv4OptionV2Marking*>(option);
                    seq = opt->getSeq();
                    ret_count = opt->getRet_num();
                    break;
                }
                // Check if something is returned
                if (i == ipHeader->getOptionArraySize() - 1)
                    throw cRuntimeError("Marking cannot be off at this position!");
            }
            int num_packets_to_eject_for_bounced_packet = queue->getNumPacketsToEject(b(packet->getBitLength()), seq, ret_count,
                        mac->on_the_way_packet_num, mac->on_the_way_packet_length);
            EV << "Number of packets to eject for bounced packet is " << num_packets_to_eject_for_bounced_packet << endl;

            if (num_packets_to_eject_for_bounced_packet < 0)
                ejected_packets.push_back(packet);
            else {
                ejected_packets = queue->eject_and_push(num_packets_to_eject_for_bounced_packet);

                auto frame = packet->peekAtFront<EthernetMacHeader>();
                if (frame->getIs_v2_dropped_packet_header())
                    throw cRuntimeError("How is the packet and ndp packet but we are bouncing it!");

                mac->add_on_the_way_packet(b(packet->getBitLength()), frame->getIs_v2_dropped_packet_header());
                // send the main packet to the output queue
                EV << "Sending the main packet " << packet << " on output interface " << ie->getFullName() << " with destination = " << frame->getDest() << endl;
                numDispatchedNonBPDUFrames++;
                auto oldPacketProtocolTag = packet->removeTag<PacketProtocolTag>();
                packet->clearTags();
                auto newPacketProtocolTag = packet->addTag<PacketProtocolTag>();
                *newPacketProtocolTag = *oldPacketProtocolTag;
                delete oldPacketProtocolTag;
                packet->addTag<InterfaceReq>()->setInterfaceId(ie->getInterfaceId());
                packet->trim();
                send(packet, "ifOut");
            }

            while (ejected_packets.size() > 0) {
                // remove the payload
                // mark the header
                // Send the packet to the original queue
                auto packet = ejected_packets.front();
                ejected_packets.pop_front();
                auto frame = packet->peekAtFront<EthernetMacHeader>();
                if (frame->getOriginal_interface_id() < 0)
                    throw cRuntimeError("frame->getOriginal_interface_id()");
                auto ie = ifTable->getInterfaceById(frame->getOriginal_interface_id());
                EV << "Packet should be dropped. Sending the header to the receiver." << endl;
//                emit(feedBackPacketDroppedSignal, int(frame->getIs_bursty()));
//                emit(feedBackPacketDroppedPortSignal, ie->getIndex());
                light_in_relay_packet_drop_counter++;
                EV << "main packet is " << packet << endl;
                auto packet_dup = packet->dup();
                b position = packet_dup->getFrontOffset();
                packet_dup->setFrontIteratorPosition(b(0));
                Packet* new_packet = new Packet();
                if (num_packets_to_eject < 0 && num_packets_to_eject_for_bounced_packet < 0) {
                    // The main packet has phy header. If the packet is inserted in a queue and ejected, it doesn't
                    // have phy header
                    new_packet->insertAtBack(packet_dup->removeAtFront<EthernetPhyHeader>());
                }
                auto mac_header = packet_dup->removeAtFront<EthernetMacHeader>();
                mac_header->setIs_v2_dropped_packet_header(true);
                new_packet->insertAtBack(mac_header);
                auto ipv4_header = packet_dup->removeAtFront<Ipv4Header>();
                auto tcp_header = packet_dup->removeAtFront<tcp::TcpHeader>();
                auto ether_fcs = packet_dup->popAtBack<EthernetFcs>(ETHER_FCS_BYTES);
                b new_length = mac_header->getChunkLength() + ipv4_header->getChunkLength() +
                        tcp_header->getChunkLength() + ether_fcs->getChunkLength();
                b old_length = (b)packet->getBitLength();
                b data_length = old_length - new_length;
                ipv4_header->setTotalLengthField(ipv4_header->getTotalLengthField() - data_length);
                new_packet->insertAtBack(ipv4_header);
                new_packet->insertAtBack(tcp_header);
                new_packet->insertAtBack(ether_fcs);
                delete packet_dup;
                new_packet->setFrontIteratorPosition(position);
                EV << "Header of packet is " << new_packet << endl;
                EV << "Sending frame " << new_packet << " on output interface " << ie->getFullName() << " with destination = " << frame->getDest() << endl;
                numDispatchedNonBPDUFrames++;
                auto oldPacketProtocolTag = packet->removeTag<PacketProtocolTag>();
                packet->clearTags();
                new_packet->clearTags();
                auto newPacketProtocolTag = new_packet->addTag<PacketProtocolTag>();
                *newPacketProtocolTag = *oldPacketProtocolTag;
                delete oldPacketProtocolTag;
                new_packet->addTag<InterfaceReq>()->setInterfaceId(ie->getInterfaceId());
                new_packet->trim();
                new_packet->setName(packet->getName());
                EV << "ipv4Header->getTotalLengthField: " << ipv4_header->getTotalLengthField() << endl;
                EV << "new_packet->getDataLength(): " << new_packet->getDataLength() << endl;
                delete packet;
                send(new_packet, "ifOut");
            }
            continue;
        }

        // Send packet
        auto mac_header = packet->peekAtFront<EthernetMacHeader>();
        mac->add_on_the_way_packet(b(packet->getBitLength()), mac_header->getIs_v2_dropped_packet_header());
        EV << module_path_string << ": ";
        if (!consider_servers) {
            std::string other_side_input_module_path = getParentModule()->gate(getParentModule()->gateBaseId("ethg$o")+ ie->getIndex())->getPathEndGate()->getFullPath();
            bool is_other_side_input_module_path_server = other_side_input_module_path.find("server") != std::string::npos;
            if (is_other_side_input_module_path_server) {
                throw cRuntimeError("The chosen bouncing port is towards a host!");
            }
        }

        emit(feedBackPacketGeneratedSignal, packet->getId());
        EV << "Sending frame " << packet << " on output interface " << ie->getFullName() << " with destination = " << frame->getDest() << endl;
        numDispatchedNonBPDUFrames++;
        auto oldPacketProtocolTag = packet->removeTag<PacketProtocolTag>();
        packet->clearTags();
        auto newPacketProtocolTag = packet->addTag<PacketProtocolTag>();
        *newPacketProtocolTag = *oldPacketProtocolTag;
        delete oldPacketProtocolTag;
        packet->addTag<InterfaceReq>()->setInterfaceId(ie->getInterfaceId());
        packet->trim();
        emit(packetSentToLowerSignal, packet);
        send(packet, "ifOut");
    }
}

void BouncingIeee8021dRelay::mark_packet_deflection_tag(Packet *packet, bool has_phy_header) {
    EV << "mark_packet_deflection_tag called for " << packet->str() << endl;
    b position = packet->getFrontOffset();
    packet->setFrontIteratorPosition(b(0));
    Ptr<EthernetPhyHeader> phy_header;
    // if a packet is extracted from queue to be deflected it doesn't have a
    // physical header
    if (has_phy_header)
        phy_header = packet->removeAtFront<EthernetPhyHeader>();
    auto eth_header = packet->removeAtFront<EthernetMacHeader>();

    eth_header->setIs_deflected(true);

    packet->insertAtFront(eth_header);
    if (has_phy_header)
        packet->insertAtFront(phy_header);
    packet->setFrontIteratorPosition(position);
}

void BouncingIeee8021dRelay::dctcp_mark_ecn_for_deflected_packets(Packet *packet, bool has_phy_header) {
    EV << "marking the ecn for the deflected packet!" << endl;
    b position = packet->getFrontOffset();
    packet->setFrontIteratorPosition(b(0));
    Ptr<EthernetPhyHeader> phy_header;
    if (has_phy_header)
        phy_header = packet->removeAtFront<EthernetPhyHeader>();
    auto eth_header = packet->removeAtFront<EthernetMacHeader>();
    auto ip_header = packet->removeAtFront<Ipv4Header>();
    ip_header->setEcn(IP_ECN_CE);
    packet->insertAtFront(ip_header);
    packet->insertAtFront(eth_header);
    if (has_phy_header)
        packet->insertAtFront(phy_header);
    packet->setFrontIteratorPosition(position);
}

void BouncingIeee8021dRelay::bolt_pifo_evaluate_if_src_packet_should_be_generated(Packet *packet, InterfaceEntry *ie, bool ignore_cc_thresh) {
    // This function uses Bolt+PIFO in which we react to the packet that is pushed from before CC_thresh to after it
    // this function is just called when a packet is not being deflected
    EV << "bolt_pifo_evaluate_if_src_packet_should_be_generated for " << packet->str() << endl;
    if (use_bolt && use_pifo_bolt_reaction) {
        std::string switch_name = getParentModule()->getFullName();
        std::string module_path_string = switch_name + ".eth[" + std::to_string(ie->getIndex()) + "].mac";
        AugmentedEtherMac *mac = check_and_cast<AugmentedEtherMac *>(getModuleByPath(module_path_string.c_str()));
        std::string queue_full_path = module_path_string + ".queue";
        // in this case we only use V2PIFO queue
        V2PIFO * queue = check_and_cast<V2PIFO *>(getModuleByPath(queue_full_path.c_str()));
        b queue_len;
        if (queue->update_qlen_periodically) {
            queue_len = queue->periodic_qlen_bytes;
        } else {
            queue_len = queue->getTotalLength();
        }
        int queue_len_pkt_num = queue->getNumPackets();
        b ccThresh = queue->CCThresh;
        if (queue_len >= ccThresh) {
            EV << "queue_len >= ccThresh" << endl;
            std::list<Packet*> packets_to_react = queue->get_list_of_packets_to_react(packet);
            EV << "packet_to_react size is " << packets_to_react.size() << endl;
            while (packets_to_react.size() != 0) {
                auto front_packet = packets_to_react.front();
                std::string packet_name = packet->getName();
                bool has_phy_header = false;
                int extraction_port_interface_id = -1;
                if (packet_name.find("tcpseg") != std::string::npos) {
                    // packets that are already in the queue does not have phy header
                    if (front_packet == packet) {
                        EV << "reacting to the original packet!" << endl;
                        if (packets_to_react.size() != 1)
                            throw cRuntimeError("we are reacting to the original packet and the size of the list is not 1");
                        has_phy_header = true;
                    } else {
                        EV << "reacting to a packet that was inserted in the queue previously!" << endl;
                        extraction_port_interface_id = mac->interfaceEntry->getInterfaceId();
                    }
                    b position = front_packet->getFrontOffset();
                    front_packet->setFrontIteratorPosition(b(0));
                    Ptr<EthernetPhyHeader> phy_header;
                    // if a packet is extracted from queue to be deflected it doesn't have a
                    // physical header
                    if (has_phy_header)
                        phy_header = front_packet->removeAtFront<EthernetPhyHeader>();

                    auto eth_header = front_packet->removeAtFront<EthernetMacHeader>();
                    auto ip_header = front_packet->removeAtFront<Ipv4Header>();
                    auto tcp_header = front_packet->removeAtFront<tcp::TcpHeader>();
                    bool should_generate_src = (!tcp_header->getBolt_dec());

                    tcp_header->setBolt_dec(true);
                    tcp_header->setBolt_inc(false);

                    front_packet->insertAtFront(tcp_header);
                    front_packet->insertAtFront(ip_header);
                    front_packet->insertAtFront(eth_header);
                    if (has_phy_header)
                        front_packet->insertAtFront(phy_header);
                    front_packet->setFrontIteratorPosition(position);

                    if (should_generate_src) {
                        auto packet_dup = front_packet->dup();
                        if(!has_phy_header) {
                            auto phy_header = makeShared<EthernetPhyHeader>();
                            packet_dup->insertAtFront(phy_header);
                            packet_dup->setFrontIteratorPosition(B(8));
    //                        throw cRuntimeError("fuck");
                        }
                        generate_and_send_bolt_src_packet(packet_dup, queue_len_pkt_num, mac->get_link_util(), extraction_port_interface_id);
                        delete packet_dup;
                    }
                }
                packets_to_react.pop_front();
            }
        }
    }
}

void BouncingIeee8021dRelay::bolt_evaluate_if_src_packet_should_be_generated(Packet *packet, InterfaceEntry *ie, bool ignore_cc_thresh, bool has_phy_header, int extraction_port_interface_id) {
    // This function uses Bolt's original technique, reacting to newly arrived packets/deflected packets for us
    EV << "bolt_evaluate_if_src_packet_should_be_generated for " << packet->str() << endl;
    if (use_bolt) {
        std::string packet_name = packet->getName();
        if (packet_name.find("tcpseg") != std::string::npos) {
            // the src should be generated only if the packet is datapacket
            std::string switch_name = getParentModule()->getFullName();
            std::string module_path_string = switch_name + ".eth[" + std::to_string(ie->getIndex()) + "].mac";
            AugmentedEtherMac *mac = check_and_cast<AugmentedEtherMac *>(getModuleByPath(module_path_string.c_str()));
            std::string queue_full_path = module_path_string + ".queue";
            b queue_len;
            b queue_cap_bytes;
            int queue_len_pkt_num;
            b ccThresh;
            b sel_reaction_ccThresh_max;
            double relative_priority;
            if (use_bolt_queue) {
                V2PIFOBoltQueue * queue = check_and_cast<V2PIFOBoltQueue *>(getModuleByPath(queue_full_path.c_str()));
                queue_len = queue->getTotalLength();
                queue_cap_bytes = queue->getMaxTotalLength();
                queue_len_pkt_num = queue->getNumPackets();
                ccThresh = queue->CCThresh;
                if (apply_selective_net_reaction)
                    throw cRuntimeError("Selective reaction is not yet implemented for bolt queue!");
                EV << "the length of " << queue->getFullPath();
            } else if (use_bolt_with_vertigo_queue) {
                V2PIFO * queue = check_and_cast<V2PIFO *>(getModuleByPath(queue_full_path.c_str()));
                if (queue->update_qlen_periodically) {
                    queue_len = queue->periodic_qlen_bytes;
                    queue_len_pkt_num = queue->periodic_qlen_num_packets;
                } else {
                    queue_len = queue->getTotalLength();
                    queue_len_pkt_num = queue->getNumPackets();
                }
                queue_cap_bytes = queue->getMaxTotalLength();
                ccThresh = queue->CCThresh;
                if (apply_selective_net_reaction && queue_len >= ccThresh) {
                    EV << "qlen is " << queue_len << endl;
                    sel_reaction_ccThresh_max = queue->bolt_CCThresh_max_selective_reaction;
                    if (sel_reaction_ccThresh_max <= ccThresh)
                        throw cRuntimeError("sel_reaction_ccThresh_max <= ccThresh");
                    if (ignore_cc_thresh)
                        throw cRuntimeError("ignore_cc_thresh and selective reaction cannot be applied together!");
                    if (selective_net_reaction_type == QUANTILE_SEL_REACTION)
                        relative_priority = queue->sel_reaction_calc_quantile(packet);
                }
                EV << "the length of " << queue->getFullPath();
            }
            bool should_src_be_sent = ignore_cc_thresh;
            if (apply_selective_net_reaction && queue_len >= ccThresh) {
                EV << "Applying selective feedback" << endl;
                should_src_be_sent = should_src_be_sent || (queue_len >= sel_reaction_ccThresh_max);
                if (queue_len < sel_reaction_ccThresh_max) {
                    // we use the condition which is set based on the min and max ccThresh
                    // so we calculate the queue cap and queue len relative to min and max ccThresh
                    EV << "queue_len >= ccThresh && queue_len < sel_reaction_ccThresh_max" << endl;
                    EV << "relative priority is " << relative_priority << endl;
                    b relative_queue_len = queue_len - ccThresh;
                    if (queue_cap_bytes < ccThresh)
                        throw cRuntimeError("queue_cap_bytes < ccThresh");
                    b relative_queue_cap;
                    if (queue_cap_bytes < sel_reaction_ccThresh_max)
                        relative_queue_cap = queue_cap_bytes - ccThresh;
                    else
                        relative_queue_cap = sel_reaction_ccThresh_max - ccThresh;
                    should_src_be_sent = should_src_be_sent || (relative_queue_len > (relative_queue_cap * (1 - sel_reaction_alpha * relative_priority)));
                }
            } else {
                should_src_be_sent = should_src_be_sent || (queue_len >= ccThresh);
            }
            if (should_src_be_sent) {
                EV  << " is " << queue_len << "which is larger than " << ccThresh << endl;
                b position = packet->getFrontOffset();
                packet->setFrontIteratorPosition(b(0));
                Ptr<EthernetPhyHeader> phy_header;
                // if a packet is extracted from queue to be deflected it doesn't have a
                // physical header
                if (has_phy_header)
                    phy_header = packet->removeAtFront<EthernetPhyHeader>();

                auto eth_header = packet->removeAtFront<EthernetMacHeader>();
                auto ip_header = packet->removeAtFront<Ipv4Header>();
                auto tcp_header = packet->removeAtFront<tcp::TcpHeader>();
                bool should_generate_src = (!tcp_header->getBolt_dec());

                tcp_header->setBolt_dec(true);
                tcp_header->setBolt_inc(false);

                packet->insertAtFront(tcp_header);
                packet->insertAtFront(ip_header);
                packet->insertAtFront(eth_header);
                if (has_phy_header)
                    packet->insertAtFront(phy_header);
                packet->setFrontIteratorPosition(position);

                if (should_generate_src) {
                    auto packet_dup = packet->dup();
                    if(!has_phy_header) {
                        auto phy_header = makeShared<EthernetPhyHeader>();
                        packet_dup->insertAtFront(phy_header);
                        packet_dup->setFrontIteratorPosition(B(8));
//                        throw cRuntimeError("fuck");
                    }
                    generate_and_send_bolt_src_packet(packet_dup, queue_len_pkt_num, mac->get_link_util(), extraction_port_interface_id);
                    delete packet_dup;
                }
            }
        }
    }
}

bool BouncingIeee8021dRelay::bolt_is_packet_src(Packet *packet) {
    std::string packet_name = packet->getName();
    return (packet_name.find("SRC") != std::string::npos);
}

void BouncingIeee8021dRelay::generate_and_send_bolt_src_packet(Packet *packet, int queue_occupancy_packet_num, long link_util, int extraction_port_interface_id) {
    // based on bolt's code we use queue occupancy in number of packets not bytes
    EV << "generate_and_send_bolt_src_packet called for " <<
            packet->str() << endl;
    b src_ip_header_len = b(0);
    auto packet_dup = packet->dup();
    b position = packet_dup->getFrontOffset();
    packet_dup->setFrontIteratorPosition(b(0));
    Ptr<EthernetPhyHeader> phy_header;
    // if a packet is extracted from queue to be deflected it doesn't have a
    // physical header
    phy_header = packet_dup->removeAtFront<EthernetPhyHeader>();
    auto eth_header = packet_dup->removeAtFront<EthernetMacHeader>();
    auto ip_header = packet_dup->removeAtFront<Ipv4Header>();
    auto tcp_header = packet_dup->removeAtFront<tcp::TcpHeader>();
    auto ether_fcs = packet_dup->removeAtBack<EthernetFcs>(ETHER_FCS_BYTES);

//    std::cout << simTime() << ", Original packet info: " << endl <<
//            "TCP: src: " << tcp_header->getSrcPort() << ", dst: " << tcp_header->getDestPort() << endl <<
//            "IP: src: " << ip_header->getSrcAddress() << ", dst: " << ip_header->getDestAddress() << endl <<
//            "ETH: src: " << eth_header->getSrc() << ", dst: " << eth_header->getDest() << endl;

    Packet *src_packet = new Packet("SRC");

    unsigned short int src_port = tcp_header->getSrcPort();
    tcp_header->setSrcPort(tcp_header->getDestPort());
    tcp_header->setDestPort(src_port);
    tcp_header->setBolt_tx_time(eth_header->getTime_packet_sent_from_src());
    tcp_header->setSrcbit(true);
    tcp_header->setAckBit(false);
    tcp_header->setLink_util(link_util);
    tcp_header->setQueue_occupancy_packet_num(queue_occupancy_packet_num);
    tcp_header->setCrcMode(inet::CRC_COMPUTED);
    tcp_header->setCrc(0);
    src_ip_header_len += tcp_header->getChunkLength();

    auto src_addr = ip_header->getSrcAddress();
    ip_header->setSrcAddress(ip_header->getDestAddress());
    ip_header->setDestAddress(src_addr);
    // I don't know where this is right
    ip_header->setCrcMode(inet::CRC_COMPUTED);
    ip_header->setCrc(0);
    ip_header->getOptionsForUpdate().deleteOptionByType(IPOPTION_V2_MARKING);
    ip_header->setTimeToLive(TTL);

    unsigned short numOptions = ip_header->getOptionArraySize();
    if (numOptions > 0) {    // options present?
        throw cRuntimeError("Bolt: numOptions for SRC packet IP header should be 0!");
    }
    src_ip_header_len += ip_header->getChunkLength();
    ip_header->setTotalLengthField(B(src_ip_header_len));

    auto src_mac = eth_header->getSrc();
    eth_header->setSrc(eth_header->getDest());
    eth_header->setDest(src_mac);


//    std::cout << "SRC packet info: " << endl <<
//            "TCP: src: " << tcp_header->getSrcPort() << ", dst: " << tcp_header->getDestPort() << endl <<
//            "IP: src: " << ip_header->getSrcAddress() << ", dst: " << ip_header->getDestAddress() << endl <<
//            "ETH: src: " << eth_header->getSrc() << ", dst: " << eth_header->getDest() << endl;
//    std::cout << "-------------------------------" << endl;

    src_packet->insertAtFront(tcp_header);
    src_packet->insertAtFront(ip_header);
    src_packet->insertAtFront(eth_header);
    src_packet->insertAtFront(phy_header);
    src_packet->insertAtBack(ether_fcs);
    src_packet->setFrontOffset(position);

    if (packet_dup->findTag<InterfaceInd>()) {
        auto interface_ind_tag = src_packet->addTag<InterfaceInd>();
        *interface_ind_tag = *packet_dup->getTag<InterfaceInd>();
    } else if (extraction_port_interface_id != -1)
        src_packet->addTagIfAbsent<InterfaceInd>()->setInterfaceId(extraction_port_interface_id);
    else
        throw cRuntimeError("Don't have any interface ind!!!");

    auto packet_protocol_tag = src_packet->addTag<PacketProtocolTag>();
    *packet_protocol_tag = *packet_dup->getTag<PacketProtocolTag>();

    delete packet_dup;

    EV << "src packet is " << src_packet->str() << endl;

    handleAndDispatchFrame(src_packet);
}

void BouncingIeee8021dRelay::dispatch(Packet *packet, InterfaceEntry *ie)
{
//    std::string packet_name = packet->getName();
//    if (packet_name.find("tcpseg") != std::string::npos) {
//        delete packet;
//        return;
//    }
//    drop_cnt--;
//    if (drop_cnt <= 0){
//        delete packet;
//        return;
//    }

//    std::cout << simTime() << endl;

    if (ie != nullptr) {
        b position = packet->getFrontOffset();
        packet->setFrontIteratorPosition(b(0));
        auto phy_header_temp = packet->removeAtFront<EthernetPhyHeader>();
        auto mac_header_temp = packet->removeAtFront<EthernetMacHeader>();
        mac_header_temp->setOriginal_interface_id(ie->getInterfaceId());
        packet->insertAtFront(mac_header_temp);
        packet->insertAtFront(phy_header_temp);
        packet->setFrontIteratorPosition(position);
    }

    const auto& frame = packet->peekAtFront<EthernetMacHeader>();
    std::string switch_name = getParentModule()->getFullName();
    b packet_length = b(packet->getBitLength());

//    if (drop_cnt >=0 && frame->getPayload_length() >= b(100)) {
//        delete packet;
//        drop_cnt--;
//        return;
//    }

    std::string module_path_string;
    InterfaceEntry *ie2 = nullptr;

    if (frame->getIs_v2_dropped_packet_header())
        EV << "Packet is a dropped packet header. No need to bounce it." << endl;

    if (!frame->getIs_v2_dropped_packet_header() && !frame->getDest().isBroadcast()) {
        // If there is enough space on the chosen port simply forward it
        module_path_string = switch_name + ".eth[" + std::to_string(ie->getIndex()) + "].mac";
        EV << "The chosen port path is " << module_path_string << endl;
        AugmentedEtherMac *mac_temp = check_and_cast<AugmentedEtherMac *>(getModuleByPath(module_path_string.c_str()));
        std::string queue_full_path = module_path_string + ".queue";
        if (send_header_of_dropped_packet_to_receiver) {
            queue_full_path = module_path_string + ".queue.mainQueue";
        }



        if (can_deflect && bounce_probabilistically) {
            // Using PABO
            ie2 = find_interface_to_bounce_probabilistically(packet, ie);
        }
        else if (can_deflect && use_vertigo_prio_queue && bounce_randomly_v2) {
            bolt_evaluate_if_src_packet_should_be_generated(packet, ie, ignore_cc_thresh_for_deflected_packets);
            if (mac_temp->is_queue_full(packet_length, queue_full_path) ||
                    (mac_temp->is_queue_over_v2_threshold(packet_length, queue_full_path, packet) &&
                    mac_temp->is_packet_tag_larger_than_last_packet(queue_full_path, packet))) {
//                std::cout << simTime() << ": Applying early deflection" << endl;
                if (bolt_is_packet_src(packet)) {
                    delete packet;
                } else {
                    if (dctcp_mark_deflected_packets_only)
                        dctcp_mark_ecn_for_deflected_packets(packet);
                    mark_packet_deflection_tag(packet);
                    apply_early_deflection(packet, false, ie);
                }
                return;
            } else {
                ie2 = ie;
            }
        } else if (can_deflect && mac_temp->is_queue_full(packet_length, queue_full_path)) {
            if (bounce_randomly) {
                if (bolt_is_packet_src(packet)) {
                    // do not deflect SRC packets
                    ie2 = ie;
                } else {
                    // Using DIBS --> Randomly bouncing to a not full switch port
                    // Bolt: ignoring cc_thresh for deflected packets.
                    bolt_evaluate_if_src_packet_should_be_generated(packet, ie, ignore_cc_thresh_for_deflected_packets);
                    if (dctcp_mark_deflected_packets_only)
                        dctcp_mark_ecn_for_deflected_packets(packet);
                    mark_packet_deflection_tag(packet);
                    ie2 = find_interface_to_bounce_randomly(packet);
                }
            } else if (bounce_naively) {
                if (bolt_is_packet_src(packet)) {
                    // do not deflect SRC packets
                    ie2 = ie;
                } else {
                    // Bolt: ignoring cc_thresh for deflected packets.
                    bolt_evaluate_if_src_packet_should_be_generated(packet, ie, ignore_cc_thresh_for_deflected_packets);
                    if (dctcp_mark_deflected_packets_only)
                        dctcp_mark_ecn_for_deflected_packets(packet);
                    mark_packet_deflection_tag(packet);
                    ie2 = find_interface_to_bounce_naively();
                }
            } else if (bounce_on_same_path){
                // Using main version of Vertigo
                ie2 = find_interface_to_bounce_on_the_same_path(packet, ie);
            } else if (bounce_randomly_v2) {
                if (bolt_is_packet_src(packet)) {
                    // do not deflect SRC packets
                    ie2 = ie;
                } else {
                    // Bounce the packet to source using power of n choices.
                    // Not considering ports towards servers
                    EV << "Frames src is " << frame->getSrc() << " and frame's dst is " << frame->getDest() << endl;
                    if (src_enabled_packet_type == BOLT_VERTIGO_ORG_PKT ||
                            src_enabled_packet_type == BOLT_VERTIGO_ORG_DEF_PKT) {
                        // Bolt: ignoring cc_thresh for deflected packets.
                        bolt_evaluate_if_src_packet_should_be_generated(packet, ie, ignore_cc_thresh_for_deflected_packets);
                    }
                    find_interface_to_bounce_randomly_v2(packet, false, ie);
                    return;
                }
            } else {
                ie2 = nullptr;
                EV << "No bouncing method chosen! Normally drop the packet!" << endl;
            }
            if (ie2 == nullptr && (use_vertigo_prio_queue || use_v2_pifo || use_pfabric)) {
                // if this is true, we handle the drop in the mac layer and not the relay unit
                ie2 = ie;
            }
            if (ie2 == nullptr) {
//                emit(feedBackPacketDroppedSignal, int(frame->getIs_bursty()));
//                emit(feedBackPacketDroppedPortSignal, ie->getIndex());
                light_in_relay_packet_drop_counter++;
                delete packet;
                return;
            }
        } else {
            // This is when you normally sending the packet to the output queue
            // without any deflection or something...
            // here we implement the src process for bolt
            // Bolt: For normally forwarding packets we should not ignore CC_thresh
            // if you don't want SRC to be generated for normal packets, just set the CC thresh high
            // so that it's never hit
            if (use_pifo_bolt_reaction) {
                bolt_pifo_evaluate_if_src_packet_should_be_generated(packet, ie, false);
            } else {
                // original bolt reaction
                bolt_evaluate_if_src_packet_should_be_generated(packet, ie, false);
            }
            ie2 = ie;
        }
    } else {
        ie2 = ie;
    }

    if (ie2->getInterfaceId() != ie->getInterfaceId()) {
        EV << "The output interface has changed as a result of bouncing." << endl;
        emit(feedBackPacketGeneratedSignal, packet->getId());
    }

    module_path_string = switch_name + ".eth[" + std::to_string(ie2->getIndex()) + "].mac";
    AugmentedEtherMac *mac = check_and_cast<AugmentedEtherMac *>(getModuleByPath(module_path_string.c_str()));
    auto mac_header = packet->peekAtFront<EthernetMacHeader>();
    mac->add_on_the_way_packet(packet_length, mac_header->getIs_v2_dropped_packet_header());

    EV << "Sending frame " << packet << " on output interface " << ie2->getFullName() << " with destination = " << frame->getDest() << endl;

    numDispatchedNonBPDUFrames++;
    auto oldPacketProtocolTag = packet->removeTag<PacketProtocolTag>();
    packet->clearTags();
    auto newPacketProtocolTag = packet->addTag<PacketProtocolTag>();
    *newPacketProtocolTag = *oldPacketProtocolTag;
    delete oldPacketProtocolTag;
    packet->addTag<InterfaceReq>()->setInterfaceId(ie2->getInterfaceId());
    packet->trim();
    emit(packetSentToLowerSignal, packet);
    send(packet, "ifOut");
}

void BouncingIeee8021dRelay::learn(MacAddress srcAddr, int arrivalInterfaceId)
{
    Ieee8021dInterfaceData *port = getPortInterfaceData(arrivalInterfaceId);

    EV << "SEPEHR: Is learning." << endl;

    if (!isStpAware || port->isLearning())
        macTable->updateTableWithAddress(arrivalInterfaceId, srcAddr);
}

void BouncingIeee8021dRelay::sendUp(Packet *packet)
{
    EV_INFO << "Sending frame " << packet << " to the upper layer" << endl;
    send(packet, "upperLayerOut");
}

Ieee8021dInterfaceData *BouncingIeee8021dRelay::getPortInterfaceData(unsigned int interfaceId)
{
    if (isStpAware) {
        InterfaceEntry *gateIfEntry = ifTable->getInterfaceById(interfaceId);
        Ieee8021dInterfaceData *portData = gateIfEntry ? gateIfEntry->getProtocolData<Ieee8021dInterfaceData>() : nullptr;

        if (!portData)
            throw cRuntimeError("Ieee8021dInterfaceData not found for port = %d", interfaceId);

        return portData;
    }
    return nullptr;
}

void BouncingIeee8021dRelay::start()
{
    ie = chooseInterface();
    if (ie) {
        bridgeAddress = ie->getMacAddress(); // get the bridge's MAC address
        registerAddress(bridgeAddress); // register bridge's MAC address
    }
    else
        throw cRuntimeError("No non-loopback interface found!");
}

void BouncingIeee8021dRelay::stop()
{
    ie = nullptr;
}

InterfaceEntry *BouncingIeee8021dRelay::chooseInterface()
{
    // TODO: Currently, we assume that the first non-loopback interface is an Ethernet interface
    //       since relays work on EtherSwitches.
    //       NOTE that, we don't check if the returning interface is an Ethernet interface!
    for (int i = 0; i < ifTable->getNumInterfaces(); i++) {
        InterfaceEntry *current = ifTable->getInterface(i);
        if (!current->isLoopback())
            return current;
    }

    return nullptr;
}

void BouncingIeee8021dRelay::finish()
{
    recordScalar("number of received BPDUs from STP module", numReceivedBPDUsFromSTP);
    recordScalar("number of received frames from network (including BPDUs)", numReceivedNetworkFrames);
    recordScalar("number of dropped frames (including BPDUs)", numDroppedFrames);
    recordScalar("number of delivered BPDUs to the STP module", numDeliveredBDPUsToSTP);
    recordScalar("number of dispatched BPDU frames to the network", numDispatchedBDPUFrames);
    recordScalar("number of dispatched non-BDPU frames to the network", numDispatchedNonBPDUFrames);
}


