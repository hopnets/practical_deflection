//
// In this version of V2PIFOPrioQueue, the higher the value of priority variable, the lower the packet's priority
//

#include "inet/common/ModuleAccess.h"
#include "inet/common/Simsignals.h"
#include "inet/queueing/function/PacketComparatorFunction.h"
#include "inet/queueing/function/PacketDropperFunction.h"
#include "./V2PIFOPrioQueue.h"

using namespace inet;
using namespace queueing;

#define SPPIFO 0
#define AIFO 1
#define RED 2

#define TS_LOW 0
#define TS_HIGH 1


#define MICE_FLOW_SIZE 800000   // 100KB = 800000b


Define_Module(V2PIFOPrioQueue);


simsignal_t V2PIFOPrioQueue::packetDropSeqSignal = registerSignal("packetDropSeq");
simsignal_t V2PIFOPrioQueue::packetDropRetCountSignal = registerSignal("packetDropRetCount");
simsignal_t V2PIFOPrioQueue::packetDropTotalPayloadLenSignal = registerSignal("packetDropTotalPayloadLength");

V2PIFOPrioQueue::~V2PIFOPrioQueue()
{
    recordScalar("lightInQueuePacketDropCount", light_in_queue_packet_drop_count);
    recordScalar("lightAllQueueingTime", all_packets_queueing_time_sum / num_all_packets);
    recordScalar("lightMiceQueueingTime", mice_packets_queueing_time_sum / num_mice_packets);
}

void V2PIFOPrioQueue::initialize(int stage) {
    PacketQueue::initialize(stage);
    dctcp_thresh = par("dctcp_thresh");
    bounce_randomly_v2 = getAncestorPar("bounce_randomly_v2");
    denominator_for_retrasnmissions = getAncestorPar("denominator_for_retrasnmissions");

    num_queues = par("queue_num");
    if (num_queues <= 0)
        throw cRuntimeError("num_queues <= 0");

    deflection_threshold = par("deflection_threshold");

    std::string where_to_mark_packets = par("where_to_mark_packets");
    if (where_to_mark_packets.compare("enqueue") == 0) {
        mark_packets_in_enqueue = true;
    } else if (where_to_mark_packets.compare("dequeue") == 0) {
        mark_packets_in_enqueue = false;
    } else
        throw cRuntimeError("where to mark packet is neither enqueue nor dequeue!");

    std::string priority_mapping_scheme_str = par("priority_mapping_scheme");
    if (priority_mapping_scheme_str.compare("SPPIFO") == 0)
        priority_mapping_scheme = SPPIFO;
    else if (priority_mapping_scheme_str.compare("AIFO") == 0)
        priority_mapping_scheme = AIFO;
    else if (priority_mapping_scheme_str.compare("RED") == 0)
        priority_mapping_scheme = RED;
    else
        throw cRuntimeError("No priority mapping scheme identified!");

    // AIFO
    aifo_k = par("aifo_k");
    quantile_wind_size = par("quantile_wind_size");
    aifo_count = 0;
    aifo_sample_count = par("aifo_sample_count");

    // RED
    red_wq = par("red_wq");
    if (red_wq < 0.0 || red_wq > 1.0)
        throw cRuntimeError("Invalid value for wq parameter: %g", red_wq);
    red_minth = par("red_minth");
    red_maxth = par("red_maxth");
    red_maxp = par("red_maxp");
    red_pkrate = par("red_pkrate");
    red_count = -1;
    if (red_minth < 0.0)
        throw cRuntimeError("red_minth parameter must not be negative");
    if (red_maxth < 0.0)
        throw cRuntimeError("red_maxth parameter must not be negative");
    if (red_minth >= red_maxth)
        throw cRuntimeError("red_minth must be smaller than red_maxth");
    if (red_maxp < 0.0 || red_maxp > 1.0)
        throw cRuntimeError("Invalid value for red_maxp parameter: %g", red_maxp);
    if (red_pkrate < 0.0)
        throw cRuntimeError("Invalid value for red_pkrate parameter: %g", red_pkrate);

    // WRED
    deploy_wred = par("deploy_wred");
    rank_grouping_cut_off = par("rank_grouping_cut_off");
    hp_red_wq = par("hp_red_wq");
    if (hp_red_wq < 0.0 || hp_red_wq > 1.0)
        throw cRuntimeError("Invalid value for hp_wq parameter: %g", hp_red_wq);
    hp_red_minth = par("hp_red_minth");
    hp_red_maxth = par("hp_red_maxth");
    hp_red_maxp = par("hp_red_maxp");
    hp_red_pkrate = par("hp_red_pkrate");
    hp_red_count = -1;
    if (hp_red_minth < 0.0)
        throw cRuntimeError("hp_red_minth parameter must not be negative");
    if (hp_red_maxth < 0.0)
        throw cRuntimeError("hp_red_maxth parameter must not be negative");
    if (hp_red_minth >= hp_red_maxth)
        throw cRuntimeError("hp_red_minth must be smaller than hp_red_maxth");
    if (hp_red_maxp < 0.0 || hp_red_maxp > 1.0)
        throw cRuntimeError("Invalid value for hp_red_maxp parameter: %g", hp_red_maxp);
    if (hp_red_pkrate < 0.0)
        throw cRuntimeError("Invalid value for hp_red_pkrate parameter: %g", hp_red_pkrate);

    if ((priority_mapping_scheme == AIFO || priority_mapping_scheme == RED) && num_queues != 1)
        throw cRuntimeError("priority_mapping_scheme == AIFO && num_queues != 1");

    if (priority_mapping_scheme == AIFO && aifo_sample_count == -1)
        throw cRuntimeError("priority_mapping_scheme == AIFO && aifo_sample_count == -1");

    if (priority_mapping_scheme == AIFO && quantile_wind_size < 1)
        throw cRuntimeError("priority_mapping_scheme == AIFO && quantile_wind_size < 0");

    if (priority_mapping_scheme == AIFO && (aifo_k < 0 || aifo_k > 1))
        throw cRuntimeError("priority_mapping_scheme == AIFO && aifo_k < 0");

    if (prio_queues.size() == 0) {
        for (int i = 0; i < num_queues; i++) {
            std::list<Packet*> *queue = new std::list<Packet*>;
            prio_queues.push_back(*queue);
            prio_queues_bit_length.push_back(0);
            if (priority_mapping_scheme == SPPIFO)
                sppifo_queue_bounds.push_back(0);
        }
    }
    if (prio_queues.size() != num_queues)
        throw cRuntimeError("prio_queues.size() != num_queues");

    per_queue_packetCapacity = par("per_queue_packetCapacity");
    per_queue_dataCapacity = b(par("per_queue_dataCapacity"));

    if (per_queue_dataCapacity != b(-1))
        dataCapacity = b(num_queues * per_queue_dataCapacity.get());
    else if (per_queue_packetCapacity != -1)
        packetCapacity = num_queues * per_queue_packetCapacity;

    if (dataCapacity == b(-1) && packetCapacity == -1)
        throw cRuntimeError("dataCapacity == b(-1) && packetCapacity == -1. No queue capacity?");

    just_prioritize_bursty_flows = par("just_prioritize_bursty_flows");

}

unsigned long V2PIFOPrioQueue::calculate_priority(unsigned long seq, unsigned long ret_count) {
    if (!bounce_randomly_v2 || denominator_for_retrasnmissions <= 0) {
        return seq;
    }

    unsigned long priority = seq;
    for (int i = 0; i < ret_count; i++) {
        // todo: here is where we apply the function
        priority = (unsigned long) (priority / denominator_for_retrasnmissions);
    }
    if (priority < 0)
        priority = 0;
    return priority;
}

unsigned long V2PIFOPrioQueue::extract_priority(Packet *packet, bool is_packet_being_dropped) {
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
        // the packet is dropped, the priority does not matter
        return 0;
    }

    if (just_prioritize_bursty_flows) {
        std::string packet_name = packet->getName();

        if (packet_name.find("tcpseg") == std::string::npos) {
            // prioritize SYN, ACK, SRC over data packet
            EV << "control packet, prio: 0" << endl;
            return 0;
        }

        if (etherheader->getIs_bursty()) {
            return 1;
        } else {
            return 2;
        }
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

    // The marking component is probably off!
    throw cRuntimeError("Marking component is off which makes no sense for prio_queues. How are you assiging prio queues?");
}

int V2PIFOPrioQueue::update_sppifo_tags(int queue_idx, unsigned long priority) {
    if (priority_mapping_scheme != SPPIFO)
        throw cRuntimeError("update_sppifo_tags called but priority_mapping_scheme is not SPPIFO");
    auto sppifo_queue_bound_it = sppifo_queue_bounds.begin();
    if (queue_idx < 0) {
        // push down
        int cost = (*sppifo_queue_bound_it) - priority;
        if (cost <= 0)
            throw cRuntimeError("cost <= 0 does not make any sense!");
        for (auto sppifo_queue_bound_it2 = sppifo_queue_bounds.begin();
                sppifo_queue_bound_it2 != sppifo_queue_bounds.end();
                sppifo_queue_bound_it2++) {
            (*sppifo_queue_bound_it2) = (*sppifo_queue_bound_it2) - cost;
            if ((*sppifo_queue_bound_it2) < 0)
                throw cRuntimeError("(*sppifo_queue_bound_it2) < 0 is an invalid bound");
        }
        return 0;
    } else {
        // push up
        std::advance(sppifo_queue_bound_it, queue_idx);
        (*sppifo_queue_bound_it) = priority;
        return queue_idx;
    }
}

int V2PIFOPrioQueue::map_priority_to_queue_idx_sppifo(unsigned long priority) {
    auto sppifo_queue_bound_it = sppifo_queue_bounds.end();
    int queue_idx = sppifo_queue_bounds.size();

    // push up
    while (queue_idx > 0) {
        sppifo_queue_bound_it--;
        queue_idx--;
        if (priority >= (*sppifo_queue_bound_it))
            return queue_idx;
    }

    return -1;
}

int V2PIFOPrioQueue::map_priority_to_queue_idx(unsigned long priority) {
    if (num_queues == 1)
        return 0;
    if (priority_mapping_scheme == SPPIFO)
        return map_priority_to_queue_idx_sppifo(priority);
    else if (priority_mapping_scheme == AIFO) {
        // we only have one queue and its idx is 0
        return 0;
    } else if (priority_mapping_scheme == RED) {
        // we only have one queue and its idx is 0
        return 0;
    }
    throw cRuntimeError("V2PIFOPrioQueue::map_priority_to_queue_idx: No scheme found!");
}

void V2PIFOPrioQueue::pushPacket(Packet *packet, cGate *gate)
{

    Enter_Method("pushPacket");
    EV << "pushPacket is called in V2PIFOPrioQueue" << endl;
    emit(packetPushedSignal, packet);
    EV_INFO << "Pushing packet " << packet->getName() << " into the queue." << endl;

    unsigned long priority = extract_priority(packet, false);
    EV << "priority is " << priority << ". finding where to push the packet." << endl;
    int queue_idx = map_priority_to_queue_idx(priority);

    if (queue_idx >= num_queues) {
        throw cRuntimeError("queue_idx >= num_queues");
    }

    if (buffer != nullptr)
        throw cRuntimeError("buffer != nullptr is not implemented for V2PIFOPrioQueue");
    else {
        bool should_insert_packet;
        // AIFO might consider it as full sooner than normal
        if (bounce_randomly_v2 && (priority_mapping_scheme == AIFO || priority_mapping_scheme == RED)) {
            // For AIFO and RED, we are already checking queue occupancies for
            // both forwarding and early deflection in relay unit
            // so no need to check here again to see if we should drop the packet.
            should_insert_packet = true;
        } else {
            should_insert_packet = (queue_idx < 0 && !isFullPrioQueue(0, packet->getBitLength(), packet)) ||
                    (queue_idx >= 0 && !isFullPrioQueue(queue_idx, packet->getBitLength(), packet));
        }
        if (should_insert_packet) {
            // packet is gonna be inserted
            // see if you should mark packet
            switch (priority_mapping_scheme) {
                case SPPIFO:
                    queue_idx = update_sppifo_tags(queue_idx, priority);
                    break;
                case AIFO:
                    queue_idx = 0;
                    break;
                case RED:
                    queue_idx = 0;
                    break;
                default:
                    throw cRuntimeError("No defined priority_mapping_scheme!");
            }
            int queue_occupancy = getNumPackets();
            auto eth_header = packet->removeAtFront<EthernetMacHeader>();
            eth_header->setQueue_occupancy(queue_occupancy);
            packet->insertAtFront(eth_header);

            if (mark_packets_in_enqueue) {
                if (dctcp_thresh >= 0) {
                    EV << "dctcp_thresh is " << dctcp_thresh << endl;
                    if (queue_occupancy >= dctcp_thresh) {
                        EV << "marking at enqueue" << endl;
                        std::string protocol = packet->getName();
                        if (protocol.find("tcpseg") != std::string::npos){
                            EcnMarker::setEcn(packet, IP_ECN_CE);
                            EV << "SOUGOL: The ECN is marked for this packet!" << endl;
                        }
                    }
                }
            }


            auto it = prio_queues.begin();
            std::advance(it, queue_idx);
            it->push_back(packet);
            EV << "Inserting the packet at the end of the queue" << endl;
            queue.insert(packet);
            auto it2 = prio_queues_bit_length.begin();
            std::advance(it2, queue_idx);
            (*it2) = (*it2) + packet->getBitLength();
            priority_of_last_packet_inserted_in_queue = priority;
//            std::cout << "packet is " << packet->str() << endl;
//            std::cout << "--------------------------" << endl;

            EV_INFO << "SEPEHR: A packet is inserted into the queue. Queue length: "
                    << getNumPackets() << " & packetCapacity: " << packetCapacity <<
                    ", Queue data occupancy is " << getTotalLength() <<
                    " and dataCapacity is " << dataCapacity << endl;
        } else {
            // packet should get dropped
            if (bounce_randomly_v2 && (priority_mapping_scheme == AIFO || priority_mapping_scheme == RED)) {
                throw cRuntimeError("Packets are dropped at relay for these cases not queue!");
            }
            priority = extract_priority(packet, true);
            EV << "Dropped packet with queue_idx: " << queue_idx << endl;
            delete packet;
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

Packet *V2PIFOPrioQueue::popPacket(cGate *gate)
{
    Enter_Method("popPacket");
    EV << "popPacket is called in V2PIFOPrioQueue" << endl;
    EV << "Initial queue len: " << getNumPackets() << endl;
    Packet* popped_packet;
    auto it2 = prio_queues_bit_length.begin();
    for (auto it = prio_queues.begin(); it != prio_queues.end(); it++) {
        if (it->size() > 0) {
            popped_packet = check_and_cast<Packet *>(queue.remove(it->front()));
            it->pop_front();
            (*it2) = (*it2) - popped_packet->getBitLength();
            break;
        }
        it2++;
    }

    // TODO temp: check the correctness
//    check_correctness();
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

    if (!mark_packets_in_enqueue) {
        if (dctcp_thresh >= 0) {
            EV << "dctcp_thresh is " << dctcp_thresh << endl;
            auto eth_header = popped_packet->peekAtFront<EthernetMacHeader>();
            if (eth_header->getQueue_occupancy() >= dctcp_thresh) {
                EV << "marking at dequeue" << endl;
                std::string protocol = popped_packet->getName();
                if (protocol.find("tcpseg") != std::string::npos){
                    EcnMarker::setEcn(popped_packet, IP_ECN_CE);
                    EV << "SOUGOL: The ECN is marked for this popped_packet!" << endl;
                }
            }
        }
    }

    if (priority_mapping_scheme == RED) {
        int queueLength = red_get_queue_length();
        if (queueLength == 0)
            red_q_time = simTime();
    }

    return popped_packet;
}

void V2PIFOPrioQueue::removePacket(Packet *packet)
{
    Enter_Method("removePacket");
    EV << "removePacket is called in V2PIFOPrioQueue" << endl;
    bool found = false;
    auto it3 = prio_queues_bit_length.begin();
    for (auto it = prio_queues.begin(); it != prio_queues.end(); it++) {
        for (auto it2 = it->begin(); it2 != it->end(); it2++) {
            if (*it2 == packet) {
                found = true;
                it->remove(packet);
                (*it3) = (*it3) - packet->getBitLength();
                break;
            }
        }
        if (found)
            break;
        it3++;
    }
    if (!found)
        throw cRuntimeError("Did not find the packet that was supposed to be removed!");
    PacketQueue::removePacket(packet);
    emit(packetRemovedSignal, packet);
    if (packetCapacity != -1)
        cSimpleModule::emit(customQueueLengthSignal, getNumPackets());
    else
        cSimpleModule::emit(customQueueLengthSignalPacketBytes, getTotalLength().get());

    // TODO temp: check the correctness
//    check_correctness();
}

long V2PIFOPrioQueue::get_queue_occupancy(long on_the_way_packet_num, b on_the_way_packet_length)
{
    if (getMaxNumPackets() != -1) {
        return (getNumPackets() + on_the_way_packet_num);
    }
    else if (getMaxTotalLength() != b(-1)) {
        return (getTotalLength() + on_the_way_packet_length).get();
    } else {
        throw cRuntimeError("No queue capacity specified!");
    }
}

bool V2PIFOPrioQueue::is_queue_full(b packet_length, long on_the_way_packet_num, b on_the_way_packet_length) {
    EV << "V2PIFOPrioQueue::is_queue_full" << endl;

    bool is_queue_full = (getMaxNumPackets() != -1 && getNumPackets() + on_the_way_packet_num >= getMaxNumPackets()) ||
            (getMaxTotalLength() != b(-1) && (getTotalLength() + on_the_way_packet_length + packet_length) >= getMaxTotalLength());
    EV << "Checking if queue is full" << endl;
    if (getMaxNumPackets() != -1)
        EV << "The queue capacity is " << getMaxNumPackets() << ", There are currently " << getNumPackets() << " packets inside the queue and " << on_the_way_packet_num << " packets on the way. Is the queue full? " << is_queue_full << endl;
    else if (getMaxTotalLength() != b(-1))
        EV << "The queue capacity is " << getMaxTotalLength() << ", Queue length is " << getTotalLength() << " and packet length is " << packet_length << " and " << on_the_way_packet_length << " bytes on the way. Is the queue full? " << is_queue_full << endl;
    return is_queue_full;
}

void V2PIFOPrioQueue::update_quantile_list(Packet* packet) {
    unsigned long priority = extract_priority(packet, false);
    if (quantile_list.size() == quantile_wind_size)
        quantile_list.pop_front();
//    std::cout << simTime() << ": pushing " << priority << " into quantile_list" << endl;
    quantile_list.push_back(priority);
}

double V2PIFOPrioQueue::calc_quantile(Packet* packet) {
    unsigned long priority = extract_priority(packet, false);
    int sum = 0;
    for (auto it = quantile_list.begin(); it != quantile_list.end(); it++) {
        if ((*it) < priority)
            sum += 1;
    }
//    std::cout << simTime() << ": priority is " << priority << " and quantile is " << (sum * 1.0 / quantile_wind_size) << endl;
    if (aifo_count == 0){
        // doesn't matter if packet is admitted or not, we update the list
        update_quantile_list(packet);
    }

    aifo_count++;
    aifo_count %= aifo_sample_count;

    return sum * 1.0 / quantile_wind_size;
}

bool V2PIFOPrioQueue::is_over_v2_threshold_full_aifo(b packet_length, Packet* packet, long on_the_way_packet_num, b on_the_way_packet_length) {
    int C;  // queue size
    int c;  // queue length
    if (per_queue_packetCapacity != -1) {
        C = per_queue_packetCapacity;
        auto it = prio_queues.begin();
        c = it->size();

    }
    else if (per_queue_dataCapacity != b(-1)) {
        C = per_queue_dataCapacity.get();
        auto it = prio_queues_bit_length.begin();
        c = (*it);
    }
    else if (packetCapacity != -1){
        C = packetCapacity;
        c = getNumPackets();
    }
    else if (dataCapacity != b(-1)){
        C = dataCapacity.get();
        c = getTotalLength().get();
    }
    double quantile = calc_quantile(packet);
//        std::cout << "queue occupancy is " << get_queue_occupancy(0, b(0)) << ", C is " << C << ", and c is " << c <<
//                ", quantile is " << quantile << ", and ((1.0/(1-aifo_k)) * ((C-c) * 1.0 / C)) is " <<
//                ((1.0/(1-aifo_k)) * ((C-c) * 1.0 / C)) << endl;

    EV << "queue occupancy is " << get_queue_occupancy(0, b(0)) << ", C is " << C << ", and c is " << c <<
            ", quantile is " << quantile << ", and ((1.0/(1-aifo_k)) * ((C-c) * 1.0 / C)) is " <<
            ((1.0/(1-aifo_k)) * ((C-c) * 1.0 / C)) << endl;
    if (quantile <= ((1.0/(1-aifo_k)) * ((C-c) * 1.0 / C))) {
        // when we have deflection, a full queue should indicate that the packet should be deflected
        // in AIFO admitting it does not change the fact that it gets droped but with deflection it does
        // however, is queue full is already checked before this function is called, so here
        // just return false
//            std::cout << "packet should be admitted!" << endl;
        return false;
    }

//        std::cout << "-------------------------------" << endl;
//        for (auto it = quantile_list.begin(); it != quantile_list.end(); it++) {
//            std::cout << (*it) << endl;
//        }
//        std::cout << "-------------------------------" << endl;



//        std::cout << "packet should not be admitted!" << endl;
    return true;
}

bool V2PIFOPrioQueue::is_over_v2_threshold_full_sppifo(b packet_length, Packet* packet, long on_the_way_packet_num, b on_the_way_packet_length) {
    if (deflection_threshold <= 0) {
        // is queue full is already checked in here so if the deflection_threshold is
        // not defined and queue is not full, return false
        return false;
    }
    EV << "V2PIFOPrioQueue::is_over_v2_threshold_full" << endl;
    bool is_over_deflection_threshold = (getMaxNumPackets() != -1 && getNumPackets() + on_the_way_packet_num >= deflection_threshold) ||
            (getMaxTotalLength() != b(-1) && (getTotalLength() + on_the_way_packet_length + packet_length).get() >= deflection_threshold);
    if (getMaxNumPackets() != -1)
        EV << "The deflection threshold is " << deflection_threshold << ", There are currently " << getNumPackets() << " packets inside the queue and " << on_the_way_packet_num << " packets on the way. is_over_deflection_threshold? " << is_over_deflection_threshold << endl;
    else if (getMaxTotalLength() != b(-1))
        EV << "The deflection threshold is " << deflection_threshold << ", Queue length is " << getTotalLength() << " and packet length is " << packet_length << " and " << on_the_way_packet_length << " bytes on the way. is_over_deflection_threshold? " << is_over_deflection_threshold << endl;

//    if (is_over_deflection_threshold)
//        std::cout << "t = " << simTime() << ". The deflection threshold is " << deflection_threshold << ", Queue length is " << getTotalLength() << " and packet length is " << packet_length << " and " << on_the_way_packet_length << " bytes on the way. is_over_deflection_threshold? " << is_over_deflection_threshold << endl;
    return is_over_deflection_threshold;
}

int V2PIFOPrioQueue::red_get_queue_length() {
    int queueLength;
    if (per_queue_packetCapacity != -1) {
        auto it = prio_queues.begin();
        queueLength = it->size();

    }
    else if (per_queue_dataCapacity != b(-1)) {
        auto it = prio_queues_bit_length.begin();
        queueLength = (*it);
        queueLength /= 8; // changing queueLength to bytes
    }
    else if (packetCapacity != -1){
        queueLength = getNumPackets();
    }
    else if (dataCapacity != b(-1)){
        queueLength = getTotalLength().get();
        queueLength /= 8; // changing queueLength to bytes
    } else
        throw cRuntimeError("No queue length is being returned!");
    return queueLength;
}

int V2PIFOPrioQueue::wred_get_traffic_class(Packet* packet) {
    if (!deploy_wred)
        return TS_LOW;

    unsigned long priority = extract_priority(packet, false);

    if (priority <= rank_grouping_cut_off)
        return TS_HIGH;
    return TS_LOW;

}

bool V2PIFOPrioQueue::is_over_v2_threshold_full_red(b packet_length, Packet* packet, long on_the_way_packet_num, b on_the_way_packet_length) {

    EV << "RedDropper::doRandomEarlyDetection talking!" << endl;
    int queueLength = red_get_queue_length();


    EV << "queueLength is " << queueLength << endl;

    if (queueLength > 0) {
        // TD: This following calculation is only useful when the queue is not empty!
        red_avg = (1 - red_wq) * red_avg + red_wq * queueLength;
        hp_red_avg = (1 - hp_red_wq) * hp_red_avg + hp_red_wq * queueLength;
    }
    else {
        // TD: Added behaviour for empty queue.
        const double m = SIMTIME_DBL(simTime() - red_q_time) * red_pkrate;
        red_avg = pow(1 - red_wq, m) * red_avg;
        hp_red_avg = pow(1 - hp_red_wq, m) * hp_red_avg;
    }

    EV << "red_avg is: " << red_avg << endl;
    EV << "hp_red_avg is: " << hp_red_avg << endl;

    int traffic_class = wred_get_traffic_class(packet);

    if (traffic_class == TS_HIGH) {
        if (hp_red_minth <= hp_red_avg && hp_red_avg < hp_red_maxth) {
            EV << "Dealing with HIGH priority traffic class" << endl;
            hp_red_count++;
            const double pb = hp_red_maxp * (hp_red_avg - hp_red_minth) / (hp_red_maxth - hp_red_minth);
            const double pa = pb / (1 - hp_red_count * pb); // TD: Adapted to work as in [Floyd93].
            double dice = dblrand();
            EV << "Deciding randomly to mark the packet with pa = " << pa << " and dice = " << dice << endl;
            if (dice < pa) {
                EV << "Random early packet (avg queue len=" << hp_red_avg << ", pa=" << pa << ")\n";
                hp_red_count = 0;
                EV << "returning RANDOMLY_ABOVE_LIMIT" << endl;
                return true;
            }
            else {
                EV << "returning RANDOMLY_BELOW_LIMIT" << endl;
                return false;
            }
        }
        else if (hp_red_avg >= hp_red_maxth) {
            EV << "Avg queue len " << hp_red_avg << " >= maxth.\n";
            EV << "returning ABOVE_MAX_LIMIT" << endl;
            hp_red_count = 0;
            return true;
        }
        else {
            hp_red_count = -1;
        }

        EV << "returning BELOW_MIN_LIMIT" << endl;
        return false;
    } else {
        EV << "Dealing with LOW priority traffic class" << endl;
        if (red_minth <= red_avg && red_avg < red_maxth) {
            red_count++;
            const double pb = red_maxp * (red_avg - red_minth) / (red_maxth - red_minth);
            const double pa = pb / (1 - red_count * pb); // TD: Adapted to work as in [Floyd93].
            double dice = dblrand();
            EV << "Deciding randomly to mark the packet with pa = " << pa << " and dice = " << dice << endl;
            if (dice < pa) {
                EV << "Random early packet (avg queue len=" << red_avg << ", pa=" << pa << ")\n";
                red_count = 0;
                EV << "returning RANDOMLY_ABOVE_LIMIT" << endl;
                return true;
            }
            else {
                EV << "returning RANDOMLY_BELOW_LIMIT" << endl;
                return false;
            }
        }
        else if (red_avg >= red_maxth) {
            EV << "Avg queue len " << red_avg << " >= maxth.\n";
            EV << "returning ABOVE_MAX_LIMIT" << endl;
            red_count = 0;
            return true;
        }
        else {
            red_count = -1;
        }

        EV << "returning BELOW_MIN_LIMIT" << endl;
        return false;
    }
    throw cRuntimeError("This line should never be reached!");
}

bool V2PIFOPrioQueue::is_over_v2_threshold_full(b packet_length, Packet* packet, long on_the_way_packet_num, b on_the_way_packet_length) {
    if (priority_mapping_scheme == AIFO) {
        return is_over_v2_threshold_full_aifo(packet_length, packet, on_the_way_packet_num, on_the_way_packet_length);

    } else if (priority_mapping_scheme == SPPIFO) {
        return is_over_v2_threshold_full_sppifo(packet_length, packet, on_the_way_packet_num, on_the_way_packet_length);
    } else if (priority_mapping_scheme == RED) {
        return is_over_v2_threshold_full_red(packet_length, packet, on_the_way_packet_num, on_the_way_packet_length);
    }
    throw cRuntimeError("No priority_mapping_scheme!");
}

bool V2PIFOPrioQueue::isFullPrioQueue(int queue_idx, int packet_len, Packet* packet)
{
    EV << "V2PIFOPrioQueue::isFullPrioQueue" << endl;

    if (is_queue_full(b(packet_len), 0, b(0))) {
//        std::cout << "t = " << simTime() << ". Queue is overall full!" << endl;
        return true;
    }
    bool is_prio_queue_full;
    if (per_queue_packetCapacity != -1) {
        auto it = prio_queues.begin();
        std::advance(it, queue_idx);
        is_prio_queue_full = it->size() >= per_queue_packetCapacity;
        EV << "per_queue_packetCapacity: " << per_queue_packetCapacity << ", queue length: " << it->size() << endl;
    } else if (per_queue_dataCapacity != b(-1)) {
        auto it2 = prio_queues_bit_length.begin();
        std::advance(it2, queue_idx);
        is_prio_queue_full = (*it2 + packet_len) >= per_queue_dataCapacity.get();
        EV << "per_queue_dataCapacity: " << per_queue_dataCapacity.get() << ", queue length: " << (*it2 + packet_len) << endl;
    } else if (packetCapacity != -1) {
        // todo: for now this simulates equal queue sizes for all priority queues
        auto it = prio_queues.begin();
        std::advance(it, queue_idx);
        is_prio_queue_full = it->size() >= (packetCapacity / num_queues);
        EV << "packetCapacity: " << (packetCapacity / num_queues) << ", queue length: " << it->size() << endl;
    } else if (dataCapacity != b(-1)) {
        // todo: for now this simulates equal queue sizes for all priority queues
        auto it2 = prio_queues_bit_length.begin();
        std::advance(it2, queue_idx);
        is_prio_queue_full = (*it2 + packet_len) >= (dataCapacity.get() / num_queues);
        EV << "dataCapacity: " << (dataCapacity.get() / num_queues) << ", queue length: " << (*it2 + packet_len) << endl;
    } else
        throw cRuntimeError("No capacity!");
    EV << "V2PIFOPrioQueue::isFullPrioQueue? " << is_prio_queue_full << endl;

    // AIFO might consider it as full sooner than normal
    if (priority_mapping_scheme == AIFO || priority_mapping_scheme == RED) {
        // either is true we should drop it
        is_prio_queue_full = is_prio_queue_full || is_over_v2_threshold_full(b(packet_len), packet, 0, b(0));
    }

    return is_prio_queue_full;
}

bool V2PIFOPrioQueue::is_packet_tag_larger_than_last_packet(Packet* packet) {
    if (priority_mapping_scheme == AIFO || priority_mapping_scheme == RED) {
        // if over_threshold returns true, it means that we should deflect or
        // drop the packet for AIFO/RED so in here we just return true to apply early deflection
        return true;
    }
    else if (priority_mapping_scheme == SPPIFO) {
        EV << "V2PIFOPrioQueue::is_packet_tag_larger_than_last_packet called." << endl;
        unsigned long priority = extract_priority(packet, false);
        EV << "priority_of_last_packet_inserted_in_queue is " <<
                priority_of_last_packet_inserted_in_queue << ", priority is " << priority << endl;
        return priority > priority_of_last_packet_inserted_in_queue;
    }
    throw cRuntimeError("No priority mapping scheme indicated...!");
}
