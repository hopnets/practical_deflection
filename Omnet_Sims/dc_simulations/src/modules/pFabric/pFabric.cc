//
// In this version of pFabric, the higher the value of priority variable, the lower the packet's priority
//

#include "inet/common/ModuleAccess.h"
#include "inet/common/Simsignals.h"
#include "inet/queueing/function/PacketComparatorFunction.h"
#include "inet/queueing/function/PacketDropperFunction.h"
#include "./pFabric.h"
#include "inet/transportlayer/tcp_common/TcpHeader_m.h"

using namespace inet;
using namespace queueing;

#define MICE_FLOW_SIZE 800000   // 100KB = 800000b

Define_Module(pFabric);

simsignal_t pFabric::packetDropSeqSignal = registerSignal("packetDropSeq");
simsignal_t pFabric::packetDropRetCountSignal = registerSignal("packetDropRetCount");
simsignal_t pFabric::packetDropTotalPayloadLenSignal = registerSignal("packetDropTotalPayloadLength");

pFabric::~pFabric()
{
    recordScalar("lightInQueuePacketDropCount", light_in_queue_packet_drop_count);
    recordScalar("lightAllQueueingTime", all_packets_queueing_time_sum / num_all_packets);
    recordScalar("lightMiceQueueingTime", mice_packets_queueing_time_sum / num_mice_packets);
}

void pFabric::initialize(int stage) {
    PacketQueue::initialize(stage);
    dctcp_thresh = par("dctcp_thresh");
    bounce_randomly_v2 = getAncestorPar("bounce_randomly_v2");
    denominator_for_retrasnmissions = getAncestorPar("denominator_for_retrasnmissions");
}

int pFabric::getNumPacketsToEject(b packet_length, long seq, long ret_count,
        long on_the_way_packet_num, b on_the_way_packet_length) {

    // Ejects packets with the lowest priorities, also add fifo ejecting
    EV << "getNumPacketsToEject called for packet with lenght: " << packet_length <<
            ", seq: " << seq << ", and ret_count: " << ret_count << endl;

    unsigned long priority = calculate_priority(seq, ret_count);
    int num_packets_to_eject = 0;
    b required_length = packet_length;
    long queue_occupancy = get_queue_occupancy(on_the_way_packet_num, on_the_way_packet_length);
    int max_capacity;
    if (getMaxNumPackets() != -1) {
        max_capacity = getMaxNumPackets();
    } else {
        max_capacity = getMaxTotalLength().get();
    }
    EV << "max capacity is " << max_capacity << endl;

    if (priority_sorted_packet_queue.size() == 0)
        throw cRuntimeError("How is this possible? The queue is empty but you're ejecting a packet!");
    std::multimap<unsigned long, Packet*>::iterator map_it =
            priority_sorted_packet_queue.end();
    map_it--;
    int num_packets = getNumPackets();
    while(num_packets > 0) {
        num_packets--; // makes sure that there is no infinite while loop in the code
        EV << "considering packet " << map_it->second->str() << " with priority=" << map_it->first << endl;
        if (map_it->first > priority) {
            // packet has higher priority
            EV << "packet has higher priority" << endl;
            num_packets_to_eject++;
            if (getMaxNumPackets() != -1) {
                queue_occupancy--;
            } else {
                queue_occupancy -= map_it->second->getBitLength();
            }
            if ((getMaxNumPackets() != -1 && max_capacity - queue_occupancy >= 1) ||
                    (getMaxNumPackets() == -1 && max_capacity - queue_occupancy >= packet_length.get()))
                return num_packets_to_eject;
        } else {
            EV << "not enough packets with lower priority to be ejected" << endl;
            // not enough packets with lower priority to be ejected
            return -1;
        }
        // check for mismatch between actual queue and priority_sorted_packet_queue
        if (map_it == priority_sorted_packet_queue.begin() && num_packets > 0)
            throw cRuntimeError("map_it == priority_sorted_packet_queue.begin() && num_packets > 0");
        map_it--;
    }
    return -1;
}

std::list<Packet*> pFabric::eject_and_push(int num_packets_to_eject) {
    // TODO: Under construction
    std::list<Packet*> packets;
    for (int i = 0; i < num_packets_to_eject; i++) {
        std::multimap<unsigned long, Packet*>::iterator map_it =
                priority_sorted_packet_queue.end();
        map_it--;
        packets.push_back(map_it->second);
        queue.remove(map_it->second);
        unsigned long hash_of_flow = extract_hash_of_flow(map_it->second);
        auto flow_fifo_queue_found = fifo_per_flow_queue.find(hash_of_flow);
        // hash of flow should exist
        if (flow_fifo_queue_found == fifo_per_flow_queue.end())
            throw cRuntimeError("flow_fifo_queue_found == fifo_per_flow_queue.end()");
        flow_fifo_queue_found->second.remove(map_it->second);
        if (flow_fifo_queue_found->second.size() <= 0)
            fifo_per_flow_queue.erase(flow_fifo_queue_found);
        priority_sorted_packet_queue.erase(map_it);
    }
    if (packets.size() != num_packets_to_eject)
        throw cRuntimeError("packets.size() != num_packets_to_eject");
    return packets;
}

unsigned long pFabric::calculate_priority(unsigned long seq, unsigned long ret_count) {
    if (!bounce_randomly_v2 || denominator_for_retrasnmissions <= 0)
        return seq;

    // only decrease the priority for retransmission if we are using our technique
    unsigned long priority = seq;
    for (int i = 0; i < ret_count; i++) {
        // todo: here is where we apply the function
        priority = (unsigned long) (priority / denominator_for_retrasnmissions);
    }
    if (priority < 0)
        priority = 0;
    return priority;
}

unsigned long pFabric::extract_hash_of_flow(Packet *packet) {
    auto packet_dup = packet->dup();
    packet_dup->removeAtFront<EthernetMacHeader>();
    auto ipv4header = packet_dup->removeAtFront<Ipv4Header>();
    auto tcpheader = packet_dup->peekAtFront<tcp::TcpHeader>();
    std::string src_ip = ipv4header->getSourceAddress().str();
    std::string dst_ip = ipv4header->getDestinationAddress().str();
    std::string src_port = std::to_string(tcpheader->getSourcePort());
    std::string dst_port = std::to_string(tcpheader->getDestinationPort());
    std::string tcp_seq_num = std::to_string(tcpheader->getSequenceNo());
    std::string packet_name = packet_dup->getName();
    std::string tcp_ack_num = std::to_string(tcpheader->getAckNo());
    unsigned long hash_of_flow = flow_hash(src_ip + dst_ip + src_port + dst_port);
    delete packet_dup;
    EV << "Hash of flow is " << hash_of_flow << endl;
    return hash_of_flow;
}

unsigned long pFabric::extract_priority(Packet *packet, bool is_packet_being_dropped) {
    unsigned long priority;
    unsigned long seq, ret_count;
    auto packet_dup = packet->dup();
    auto etherheader = packet_dup->removeAtFront<EthernetMacHeader>();
    auto ipv4header = packet_dup->peekAtFront<Ipv4Header>();
    delete packet_dup;

    // This should be added, whether marking is on or not!
    if(is_packet_being_dropped) {
        EV << "Packet dropped in extract priority!" << endl;
        light_in_queue_packet_drop_count++;
    }

    for (unsigned int i = 0; i < ipv4header->getOptionArraySize(); i++) {
        const TlvOptionBase *option = &ipv4header->getOption(i);
        if (option->getType() == IPOPTION_V2_MARKING) {
            auto opt = check_and_cast<const Ipv4OptionV2Marking*>(option);
            seq = opt->getSeq();
            ret_count = opt->getRet_num();
            priority = calculate_priority(seq, ret_count);
            if (is_packet_being_dropped) {
//                cSimpleModule::emit(packetDropSeqSignal, seq);
//                cSimpleModule::emit(packetDropRetCountSignal, ret_count);
//                cSimpleModule::emit(packetDropTotalPayloadLenSignal, etherheader->getTotal_length().get());
            }
            return priority;
        }
    }
    throw cRuntimeError("By now priority should've been returned! Maybe you didn't turn on the marking. For VIFO, marking should always be on, even if the scheduling is FIFO.");
}

void pFabric::pushPacket(Packet *packet, cGate *gate)
{
    // first see if you should mark packet
    if (dctcp_thresh >= 0) {
        EV << "dctcp_thresh is " << dctcp_thresh << endl;
        if (getNumPackets() >= dctcp_thresh) {
            std::string protocol = packet->getName();
            if (protocol.find("tcpseg") != std::string::npos){
                EcnMarker::setEcn(packet, IP_ECN_CE);
                EV << "SOUGOL: The ECN is marked for this packet!" << endl;
            }
        }
    }

    Enter_Method("pushPacket");
    EV << "pushPacket is called in pFabric" << endl;
    emit(packetPushedSignal, packet);
    EV_INFO << "Pushing packet " << packet->getName() << " into the queue." << endl;

    unsigned long hash_of_flow = extract_hash_of_flow(packet);
    //calculate priority
    unsigned long priority = extract_priority(packet, false);
    EV << "priority is " << priority << ". finding where to push the packet." << endl;

    // insert the packet in the sorted queue
    priority_sorted_packet_queue.insert(std::pair<unsigned long, Packet*>(priority, packet));

    // insert the packet in fifo per flow queue
    auto flow_record_exists = fifo_per_flow_queue.find(hash_of_flow);
    if (flow_record_exists == fifo_per_flow_queue.end()) {
        // New packet for new flow
        std::list<Packet*> temp_list;
        temp_list.push_back(packet);
        fifo_per_flow_queue.insert(std::pair<unsigned long, std::list<Packet*>>(hash_of_flow, temp_list));
    } else {
        // New packet for old flow
        flow_record_exists->second.push_back(packet);
    }

    EV << "Inserting the packet at the end of the queue" << endl;
    queue.insert(packet);

    EV_INFO << "SEPEHR: A packet is inserted into the queue. Queue length: "
            << getNumPackets() << " & packetCapacity: " << packetCapacity <<
            ", Queue data occupancy is " << getTotalLength() <<
            " and dataCapacity is " << dataCapacity << endl;

    if (buffer != nullptr)
        buffer->addPacket(packet);
    else {
        int num_packets = getNumPackets();
        while (isOverloaded() && num_packets > 0) {
            num_packets--; // avoiding infinite loops
            auto worst_prio_packet_it = priority_sorted_packet_queue.end();
            worst_prio_packet_it--;
            unsigned long hash_of_flow_of_worst_prio_packet = extract_hash_of_flow(worst_prio_packet_it->second);
            auto fifo_flow_of_worst_prio_packet_it = fifo_per_flow_queue.find(hash_of_flow_of_worst_prio_packet);
            // We should have the fifo record for the flow
            if (fifo_flow_of_worst_prio_packet_it == fifo_per_flow_queue.end())
                throw cRuntimeError("fifo_flow_of_worst_prio_packet_it == fifo_per_flow_queue.end()");

            fifo_flow_of_worst_prio_packet_it->second.remove(worst_prio_packet_it->second);
            if (fifo_flow_of_worst_prio_packet_it->second.size() <= 0)
                fifo_per_flow_queue.erase(fifo_flow_of_worst_prio_packet_it);
            queue.remove(worst_prio_packet_it->second);
            // record it being dropped
            unsigned long priority = extract_priority(worst_prio_packet_it->second, true);
            EV << "Queue is overloaded. Dropping the packet in queue with priority: " << priority<< endl;
            //remove the packet
            delete worst_prio_packet_it->second;
            priority_sorted_packet_queue.erase(worst_prio_packet_it);
        }
    }

    updateDisplayString();
    if (packetCapacity != -1)
        cSimpleModule::emit(customQueueLengthSignal, getNumPackets());
    else
        cSimpleModule::emit(customQueueLengthSignalPacketBytes, getTotalLength().get());
    if (collector != nullptr && getNumPackets() != 0){
        EV << "SEPEHR: Handling can pop packet." << endl;
        collector->handleCanPopPacket(outputGate);
    }
}

Packet *pFabric::popPacket(cGate *gate)
{
    Enter_Method("popPacket");
    EV << "popPacket is called in pFabric" << endl;
    EV << "Initial queue len: " << getNumPackets() << endl;

    auto best_prio_packet_it = priority_sorted_packet_queue.begin();
    unsigned long hash_of_flow_of_best_prio_packet = extract_hash_of_flow(best_prio_packet_it->second);
    auto fifo_flow_of_best_prio_packet_it = fifo_per_flow_queue.find(hash_of_flow_of_best_prio_packet);
    // check if fifo queues exist for the flow
    if (fifo_flow_of_best_prio_packet_it == fifo_per_flow_queue.end())
        throw cRuntimeError("fifo_flow_of_best_prio_packet_it == fifo_per_flow_queue.end()");

    Packet* popped_packet = fifo_flow_of_best_prio_packet_it->second.front();
    fifo_flow_of_best_prio_packet_it->second.pop_front();
    if (fifo_flow_of_best_prio_packet_it->second.size() <= 0)
        fifo_per_flow_queue.erase(fifo_flow_of_best_prio_packet_it);
    queue.remove(popped_packet);

    unsigned long popped_packet_priority = extract_priority(popped_packet, false);
    auto popped_packet_priority_it = priority_sorted_packet_queue.find(popped_packet_priority);
    // priority should exist in the sorted queue
    if (popped_packet_priority_it == priority_sorted_packet_queue.end())
        throw cRuntimeError("popped_packet_priority_it == priority_sorted_packet_queue.end()");
    bool packet_found = false;
    while (popped_packet_priority_it != priority_sorted_packet_queue.end() &&
            popped_packet_priority_it->first == popped_packet_priority) {
        if (popped_packet_priority_it->second == popped_packet) {
            packet_found = true;
            break;
        }
        popped_packet_priority_it++;
    }

    if (!packet_found)
        throw cRuntimeError("Packet was not found in the sorted queue!");
    priority_sorted_packet_queue.erase(popped_packet_priority_it);


    emit(packetPoppedSignal, popped_packet);
    simtime_t queueing_time = simTime() - popped_packet->getArrivalTime();
    all_packets_queueing_time_sum += queueing_time.dbl();
    num_all_packets++;
    auto eth_header = popped_packet->peekAtFront<EthernetMacHeader>();
    b flow_len = eth_header->getTotal_length();
    b payload_len = eth_header->getPayload_length();
    if (payload_len > b(0) && flow_len.get() <= MICE_FLOW_SIZE) {
        mice_packets_queueing_time_sum += queueing_time.dbl();
        num_mice_packets++;
    }
    if (packetCapacity != -1)
        cSimpleModule::emit(customQueueLengthSignal, getNumPackets());
    else
        cSimpleModule::emit(customQueueLengthSignalPacketBytes, getTotalLength().get());
    EV << "Final queue len: " << getNumPackets() << endl;
    return popped_packet;
}

void pFabric::removePacket(Packet *packet)
{
    Enter_Method("removePacket");
    EV << "removePacket is called in pFabric" << endl;
    unsigned long priority = extract_priority(packet);
    unsigned long hash_of_flow = extract_hash_of_flow(packet);
    auto priority_found_in_sorted_queue = priority_sorted_packet_queue.find(priority);
    if (priority_found_in_sorted_queue == priority_sorted_packet_queue.end())
        throw cRuntimeError("priority_found_in_sorted_queue == priority_sorted_packet_queue.end()");
    bool packet_found = false;
    while (priority_found_in_sorted_queue != priority_sorted_packet_queue.end() &&
            priority_found_in_sorted_queue->first == priority) {
        if (priority_found_in_sorted_queue->second == packet) {
            packet_found = true;
            break;
        }
        priority_found_in_sorted_queue++;
    }

    if (!packet_found)
        throw cRuntimeError("packet that is supposed to be removed does'nt exist in priority_sorted_packet_queue");

    auto flow_found = fifo_per_flow_queue.find(hash_of_flow);
    if (flow_found == fifo_per_flow_queue.end())
        throw cRuntimeError("flow_found == fifo_per_flow_queue.end()");
    flow_found->second.remove(packet);
    PacketQueue::removePacket(packet);
    emit(packetRemovedSignal, packet);
    if (packetCapacity != -1)
        cSimpleModule::emit(customQueueLengthSignal, getNumPackets());
    else
        cSimpleModule::emit(customQueueLengthSignalPacketBytes, getTotalLength().get());

}

long pFabric::get_queue_occupancy(long on_the_way_packet_num, b on_the_way_packet_length)
{
    EV << "pFabric::get_queue_occupancy" << endl;
    if (getMaxNumPackets() != -1) {
        return (getNumPackets() + on_the_way_packet_num);
    }
    else if (getMaxTotalLength() != b(-1)) {
        return (getTotalLength() + on_the_way_packet_length).get();
    } else {
        throw cRuntimeError("No queue capacity specified! WTF?");
    }
}

bool pFabric::is_queue_full(b packet_length, long on_the_way_packet_num, b on_the_way_packet_length) {
    EV << "pFabric::is_queue_full" << endl;
    bool is_queue_full = (getMaxNumPackets() != -1 && getNumPackets() + on_the_way_packet_num >= getMaxNumPackets()) ||
            (getMaxTotalLength() != b(-1) && (getTotalLength() + on_the_way_packet_length + packet_length) >= getMaxTotalLength());
    EV << "Checking if queue is full" << endl;
    if (getMaxNumPackets() != -1)
        EV << "The queue capacity is " << getMaxNumPackets() << ", There are currently " << getNumPackets() << " packets inside the queue and " << on_the_way_packet_num << " packets on the way. Is the queue full? " << is_queue_full << endl;
    else if (getMaxTotalLength() != b(-1))
        EV << "The queue capacity is " << getMaxTotalLength() << ", Queue length is " << getTotalLength() << " and packet length is " << packet_length << " and " << on_the_way_packet_length << " bytes on the way. Is the queue full? " << is_queue_full << endl;
    return is_queue_full;
}
