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
#include "LSIeee8021dRelay.h"

using namespace inet;

Define_Module(LSIeee8021dRelay);

LSIeee8021dRelay::LSIeee8021dRelay()
{
}

void LSIeee8021dRelay::initialize(int stage)
{
    LayeredProtocolBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        // statistics
        numDispatchedBDPUFrames = numDispatchedNonBPDUFrames = numDeliveredBDPUsToSTP = 0;
        numReceivedBPDUsFromSTP = numReceivedNetworkFrames = numDroppedFrames = 0;
        isStpAware = par("hasStp");

        macTable = getModuleFromPar<LSIMacAddressTable>(par("macTableModule"), this);
        ifTable = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);

        learn_mac_addresses = par("learn_mac_addresses");
        bounce_back_random_port = par("bounce_back_random_port");
    }
    else if (stage == INITSTAGE_LINK_LAYER) {
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
        if (bounce_back_random_port) {
            std::string other_side_input_module_path;
            bool is_other_side_input_module_path_server;
            for (int i = 0; i < ifTable->getNumInterfaces(); i++) {
               other_side_input_module_path = getParentModule()->gate(getParentModule()->gateBaseId("ethg$o")+ i)->getPathEndGate()->getFullPath();
               is_other_side_input_module_path_server = (other_side_input_module_path.find("server") != std::string::npos);
               if (!is_other_side_input_module_path_server) {
                   std::cout << other_side_input_module_path << " is not a host." << endl;
                   port_idx_connected_to_switch_neioghbors.push_back(i);
               }
            }
        }
    }
}

void LSIeee8021dRelay::registerAddress(MacAddress mac)
{
    registerAddresses(mac, mac);
}

void LSIeee8021dRelay::registerAddresses(MacAddress startMac, MacAddress endMac)
{
    registeredMacAddresses.insert(MacAddressPair(startMac, endMac));
}

void LSIeee8021dRelay::handleLowerPacket(Packet *packet)
{
    // messages from network
    numReceivedNetworkFrames++;
    std::string switch_name = this->getParentModule()->getFullName();

    EV_INFO << "Received " << packet << " from network." << endl;
    delete packet->removeTagIfPresent<DispatchProtocolReq>();
    handleAndDispatchFrame(packet);
}

void LSIeee8021dRelay::handleUpperPacket(Packet *packet)
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
            broadcast(packet, -1);
        } else {
            InterfaceEntry *ie = ifTable->getInterfaceById(interfaceId);
            chooseDispatchType(packet, ie);
        }
    }
}

bool LSIeee8021dRelay::isForwardingInterface(InterfaceEntry *ie)
{
    if (isStpAware) {
        if (!ie->getProtocolData<Ieee8021dInterfaceData>())
            throw cRuntimeError("Ieee8021dInterfaceData not found for interface %s", ie->getFullName());
        return ie->getProtocolData<Ieee8021dInterfaceData>()->isForwarding();
    }
    return true;
}

void LSIeee8021dRelay::broadcast(Packet *packet, int arrivalInterfaceId)
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

void LSIeee8021dRelay::handleAndDispatchFrame(Packet *packet)
{

    const auto& frame = packet->peekAtFront<EthernetMacHeader>();
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
            broadcast(packet, arrivalInterfaceId);
        }
        else {
            //for (std::list<int>::iterator it=outputInterfaceId.begin(); it != outputInterfaceId.end(); ++it){
            //TODO: HERE I USED the first path
            InterfaceEntry *outputInterface = ifTable->getInterfaceById(*outputInterfaceId.begin());
            if (frame->getAllow_same_input_output() || *outputInterfaceId.begin() != arrivalInterfaceId) {
                if(frame->getAllow_same_input_output()) {
                    EV_DETAIL << "SEPEHR: Allowing output same as input" << endl;
                    EV << "Packet is " << packet->str() << endl;
                    if (frame->isFB()) {
                        EV_DETAIL << "Output port is same as input port, " << packet->getFullName() << " destination = " << frame->getDest() << ", but frame is a feedback so it's OK" << endl;
                        auto frame = packet->removeAtFront<EthernetMacHeader>();
                        frame->setAllow_same_input_output(false);
                        packet->insertAtFront(frame);
                    } else {
                        b packetPosition = packet->getFrontOffset();
                        packet->setFrontIteratorPosition(b(0));
                        auto phyHeader = packet->removeAtFront<EthernetPhyHeader>();
                        auto frame = packet->removeAtFront<EthernetMacHeader>();
                        frame->setAllow_same_input_output(false);
                        packet->insertAtFront(frame);
                        auto frame2 = packet->peekAtFront<EthernetMacHeader>();
                        EV << "SEPEHR: check if allow same input is false: " << frame2->getAllow_same_input_output()<< endl;
                        packet->insertAtFront(phyHeader);
                        packet->setFrontIteratorPosition(packetPosition);
                        EV << "SEPEHR: new packet is: " << packet << endl;
                    }
                }
                if (isForwardingInterface(outputInterface))
                    chooseDispatchType(packet, outputInterface);
                else {
                    EV_INFO << "Output interface " << *outputInterface->getFullName() << " is not forwarding. Discarding!" << endl;
                    numDroppedFrames++;
                    delete packet;
                }
            }
            else {
                EV_DETAIL << "Output port is same as input port, " << packet->getFullName() << " destination = " << frame->getDest() << ", discarding frame " << frame << endl;
                numDroppedFrames++;
                delete packet;
            }


        }
    }
}

unsigned int LSIeee8021dRelay::Compute_CRC16_Simple(std::list<int> bytes, int bytes_size){
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

std::list<int> LSIeee8021dRelay::getInfoFlowByteArray(std::string srcAddr, std::string destAddr, int srcPort, int destPort) {
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

void LSIeee8021dRelay::chooseDispatchType(Packet *packet, InterfaceEntry *ie){
    bool useECMP = this->getAncestorPar("useECMP");
    const auto& frame = packet->peekAtFront<EthernetMacHeader>();
    Chunk::enableImplicitChunkSerialization = true;
    std::string protocol = packet->getName();
    bool is_packet_arp_or_broadcast = (protocol.find("arp") != std::string::npos) || (frame->getDest().isBroadcast());
    int ipv4HeaderOffset = 0, tcpHeaderOffset = 0;
    if (frame->isFB()){
        // In this case there is no physical header
        ipv4HeaderOffset = 112; // 14B
        tcpHeaderOffset = 272; // 34B
    }
    else {
        // We have physical header
        ipv4HeaderOffset = 176;// 14B + 8B
        tcpHeaderOffset = 336;// 34B + 8B
        if (!is_packet_arp_or_broadcast){
            EV << "SEPEHR: Should reduce packet's ttl." << endl;
            b packetPosition = packet->getFrontOffset();
            packet->setFrontIteratorPosition(b(0));
            auto phyHeader = packet->removeAtFront<EthernetPhyHeader>();
            auto ethHeader = packet->removeAtFront<EthernetMacHeader>();
            auto ipHeader = packet->removeAtFront<Ipv4Header>();
            short int ttl = ipHeader->getTimeToLive() - 1;
            EV << "SEPEHR: packet's old ttl is: " << ipHeader->getTimeToLive() << " and it's new ttl is: " << ttl << endl;
            ipHeader->setTimeToLive(ttl);
            packet->insertAtFront(ipHeader);
            packet->insertAtFront(ethHeader);
            packet->insertAtFront(phyHeader);
            packet->setFrontIteratorPosition(packetPosition);
        }
    }
    std::list<int> destInterfaceIds = macTable->getInterfaceIdForAddress(frame->getDest());
    int portNum = destInterfaceIds.size();
    if (bounce_back_random_port && frame->isFB()) {
        std::list<int> port_idx_connected_to_switch_neioghbors_copy = port_idx_connected_to_switch_neioghbors;
        while (true) {
            int random_index = rand() % port_idx_connected_to_switch_neioghbors_copy.size();
            std::list<int>::iterator it = port_idx_connected_to_switch_neioghbors_copy.begin();
            std::advance(it, random_index);
            InterfaceEntry *ie2 = ifTable->getInterface(*it);
            if (ie->getInterfaceId() == ie2->getInterfaceId()) {
                port_idx_connected_to_switch_neioghbors_copy.erase(it);
                if (port_idx_connected_to_switch_neioghbors_copy.size() == port_idx_connected_to_switch_neioghbors.size())
                    throw cRuntimeError("You changed the sizes of both array, the copy and the main. WTF?");
                continue;
            }
            EV << "SEPEHR: output port id should have been " << ie->getInterfaceId() << " but as it was full, the port is randomly changed to " << ie2->getInterfaceId() << endl;
            dispatch(packet, ie2);
            break;
        }

    }
    else if ((!is_packet_arp_or_broadcast) && (portNum > 1) && useECMP){
        EV << "SOUGOL: This is the Packet Name: " << protocol << endl;
        EV << "SEPEHR: The number of available ports for this packet is " << portNum << endl;
        EV << "SEPEHR: source mac address: " << frame->getSrc().str() << " and dest mac address: " << frame->getDest().str() << endl;
        destInterfaceIds.sort();
        const auto& packetDup = packet->dup();
        packetDup->setFrontIteratorPosition(b(0));
        EV << "SEPEHR: The packetDup is: " << packetDup << endl;
        EV << "SOUGOL: This is not ARP" << endl;
        packetDup->setFrontIteratorPosition(b(ipv4HeaderOffset));
        const auto& ipv4Header = packetDup->peekAtFront<Ipv4Header>();
        packetDup->setFrontIteratorPosition(b(tcpHeaderOffset));
        const auto& tcpHeader = packetDup->peekAtFront<tcp::TcpHeader>();
        EV << "S&S: The flow info is: ( src_ip: " << ipv4Header->getSourceAddress() << ", dest_ip: " << ipv4Header->getDestinationAddress() << ", src_port: " << tcpHeader->getSourcePort() << ", dest_port: " << tcpHeader->getDestinationPort() << " )" << endl;
        EV << "SEPEHR: Switch IS using ECMP for this packet!" << endl;
        //TODO There is something here which may create a re-ordering problem with HotPotato packets that are
        //TODO going back, would not necessarily be forwarded to exactly the same port that they have came from but all FB packets would go through the same path surely
        std::list<int> bytes = getInfoFlowByteArray(ipv4Header->getSourceAddress().str(), ipv4Header->getDestinationAddress().str(), tcpHeader->getSourcePort(), tcpHeader->getDestinationPort());
        int bytes_size = bytes.size();
        EV << "SEPEHR: bytes size extracted from flow: "<< bytes_size << endl;
        unsigned int crc = Compute_CRC16_Simple(bytes, bytes_size);
        EV << "SEPEHR: CRC is: " << crc << endl;
        int outputPortNum = crc % portNum;
        EV << "SEPEHR: output port number is: " << outputPortNum << endl;
        std::list<int>::iterator it = destInterfaceIds.begin();
        std::advance(it, outputPortNum);
        EV << "SEPEHR: output interface ID is: " << *it << endl;
        int arrivalInterfaceId = packet->getTag<InterfaceInd>()->getInterfaceId();
        InterfaceEntry *ie2 = ifTable->getInterfaceById(*it);
        delete packetDup;
        dispatch(packet, ie2);
    }
    else {
        EV << "SEPEHR: Switch IS NOT using ECMP for this packet!" << endl;
        dispatch(packet, ie);
    }
}

void LSIeee8021dRelay::dispatch(Packet *packet, InterfaceEntry *ie)
{
    const auto& frame = packet->peekAtFront<EthernetMacHeader>();
    EV_INFO << "Sending frame " << packet << " on output interface " << ie->getFullName() << " with destination = " << frame->getDest() << endl;

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

void LSIeee8021dRelay::learn(MacAddress srcAddr, int arrivalInterfaceId)
{
    Ieee8021dInterfaceData *port = getPortInterfaceData(arrivalInterfaceId);

    EV << "SEPEHR: Is learning." << endl;

    if (!isStpAware || port->isLearning())
        macTable->updateTableWithAddress(arrivalInterfaceId, srcAddr);
}

void LSIeee8021dRelay::sendUp(Packet *packet)
{
    EV_INFO << "Sending frame " << packet << " to the upper layer" << endl;
    send(packet, "upperLayerOut");
}

Ieee8021dInterfaceData *LSIeee8021dRelay::getPortInterfaceData(unsigned int interfaceId)
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

void LSIeee8021dRelay::start()
{
    ie = chooseInterface();
    if (ie) {
        bridgeAddress = ie->getMacAddress(); // get the bridge's MAC address
        registerAddress(bridgeAddress); // register bridge's MAC address
    }
    else
        throw cRuntimeError("No non-loopback interface found!");
}

void LSIeee8021dRelay::stop()
{
    ie = nullptr;
}

InterfaceEntry *LSIeee8021dRelay::chooseInterface()
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

void LSIeee8021dRelay::finish()
{
    recordScalar("number of received BPDUs from STP module", numReceivedBPDUsFromSTP);
    recordScalar("number of received frames from network (including BPDUs)", numReceivedNetworkFrames);
    recordScalar("number of dropped frames (including BPDUs)", numDroppedFrames);
    recordScalar("number of delivered BPDUs to the STP module", numDeliveredBDPUsToSTP);
    recordScalar("number of dispatched BPDU frames to the network", numDispatchedBDPUFrames);
    recordScalar("number of dispatched non-BDPU frames to the network", numDispatchedNonBPDUFrames);
}


