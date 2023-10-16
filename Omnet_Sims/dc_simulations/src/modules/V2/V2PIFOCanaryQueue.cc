//
// In this version of V2PIFOCanaryQueue, the higher the value of priority variable, the lower the packet's priority
//

#include "V2PIFOCanaryQueue.h"

#include "inet/common/ModuleAccess.h"
#include "inet/common/Simsignals.h"
#include "inet/queueing/function/PacketComparatorFunction.h"
#include "inet/queueing/function/PacketDropperFunction.h"
#include <math.h>

using namespace inet;
using namespace queueing;

#define PROBABILISTIC 0
#define DETERMINISTIC 1

#define PERCENTILE 0
#define DISTRIBUTION 1

#define EXPON 0

#define TS_LOW 0
#define TS_HIGH 1


#define MICE_FLOW_SIZE 800000   // 100KB = 800000b


Define_Module(V2PIFOCanaryQueue);


simsignal_t V2PIFOCanaryQueue::packetDropSeqSignal = registerSignal("packetDropSeq");
simsignal_t V2PIFOCanaryQueue::packetDropRetCountSignal = registerSignal("packetDropRetCount");
simsignal_t V2PIFOCanaryQueue::packetDropTotalPayloadLenSignal = registerSignal("packetDropTotalPayloadLength");

V2PIFOCanaryQueue::~V2PIFOCanaryQueue()
{
    recordScalar("lightInQueuePacketDropCount", light_in_queue_packet_drop_count);
    recordScalar("lightAllQueueingTime", all_packets_queueing_time_sum / num_all_packets);
    recordScalar("lightMiceQueueingTime", mice_packets_queueing_time_sum / num_mice_packets);
}

void V2PIFOCanaryQueue::initialize(int stage) {
    PacketQueue::initialize(stage);
    dctcp_thresh = par("dctcp_thresh");
    bounce_randomly_v2 = getAncestorPar("bounce_randomly_v2");
    denominator_for_retrasnmissions = getAncestorPar("denominator_for_retrasnmissions");

    std::string where_to_mark_packets = par("where_to_mark_packets");
    if (where_to_mark_packets.compare("enqueue") == 0) {
        mark_packets_in_enqueue = true;
    } else if (where_to_mark_packets.compare("dequeue") == 0) {
        mark_packets_in_enqueue = false;
    } else
        throw cRuntimeError("where to mark packet is neither enqueue nor dequeue!");

    std::string algorithm_type_str = par("algorithm_type");
    if (algorithm_type_str.compare("PROBABILISTIC") == 0)
        algorithm_type = PROBABILISTIC;
    else if (algorithm_type_str.compare("DETERMINISTIC") == 0)
        algorithm_type = DETERMINISTIC;
    else
        throw cRuntimeError("No algorithm type for canary!");

    std::string relative_priority_calculation_type_str = par("relative_priority_calculation_type");
    if (relative_priority_calculation_type_str.compare("PERCENTILE") == 0)
        relative_priority_calculation_type = PERCENTILE;
    else if (relative_priority_calculation_type_str.compare("DISTRIBUTION") == 0)
        relative_priority_calculation_type = DISTRIBUTION;
    else
        throw cRuntimeError("No relative priority calculation paradigm for canary!");

    if (relative_priority_calculation_type == DISTRIBUTION) {
        std::string relative_priority_distribution_type_str = par("relative_priority_distribution_type");
        if (relative_priority_distribution_type_str.compare("EXPONENTIAL") == 0) {
            relative_priority_distribution_type = EXPON;
        } else {
            throw cRuntimeError("Unknown relative_priority_distribution_type!");
        }
    }

    // Probabilistic
    minth = par("minth");

    // Deterministic
    k = par("k");

    // AIFO (percentile)
    quantile_wind_size = par("quantile_wind_size");
    aifo_count = 0;
    aifo_sample_count = par("aifo_sample_count");

    if (minth < 0.0)
        throw cRuntimeError("minth parameter must not be negative");

    if (relative_priority_calculation_type == PERCENTILE && aifo_sample_count == -1)
        throw cRuntimeError("relative_priority_calculation_type == PERCENTILE && aifo_sample_count == -1");

    if (relative_priority_calculation_type == PERCENTILE && quantile_wind_size < 1)
        throw cRuntimeError("relative_priority_calculation_type == PERCENTILE && quantile_wind_size < 0");

    if (k < 0)
        throw cRuntimeError("k < 0");

    if (dataCapacity == b(-1) && packetCapacity == -1)
        throw cRuntimeError("dataCapacity == b(-1) && packetCapacity == -1. No queue capacity?");

    just_prioritize_bursty_flows = par("just_prioritize_bursty_flows");

    if (stage == INITSTAGE_LOCAL) {
        update_qlen_periodically = par("update_qlen_periodically");
        if (using_buffer && update_qlen_periodically)
            throw cRuntimeError("using_buffer && update_qlen_periodically are not yet implemented together!");
        if (update_qlen_periodically) {
            qlen_update_period = par("qlen_update_period");
            if (qlen_update_period <= 0)
                throw cRuntimeError("qlen_update_period <= 0");
            updateQueueOccupancyMsg = new cMessage("updateQueueOccupancy");
            scheduleAt(simTime()+qlen_update_period, updateQueueOccupancyMsg);
            periodic_qlen_num_packets = 0;
            periodic_qlen_bytes = b(0);
        }
    }
}

void V2PIFOCanaryQueue::handleMessage(cMessage *message) {
    EV << "V2PIFO::handleMessage called" << endl;
    if (message->isSelfMessage()) {
        cancelEvent(updateQueueOccupancyMsg);
        periodic_qlen_num_packets = getNumPackets();
        periodic_qlen_bytes = getTotalLength();
        scheduleAt(simTime()+qlen_update_period, updateQueueOccupancyMsg);
        return;
    }
    PacketQueue::handleMessage(message);
}

unsigned long V2PIFOCanaryQueue::calculate_priority(unsigned long seq, unsigned long ret_count) {
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

unsigned long V2PIFOCanaryQueue::extract_priority(Packet *packet, bool is_packet_being_dropped) {
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
//            if (is_packet_being_dropped) {
//                cSimpleModule::emit(packetDropSeqSignal, seq);
//                cSimpleModule::emit(packetDropRetCountSignal, ret_count);
//                cSimpleModule::emit(packetDropTotalPayloadLenSignal, etherheader->getTotal_length().get());
//            }
            return priority;
        }
    }

    // The marking component is probably off!
    throw cRuntimeError("Marking component is off which makes no sense for prio_queues. How are you assiging prio queues?");
}

void V2PIFOCanaryQueue::pushPacket(Packet *packet, cGate *gate)
{

    Enter_Method("pushPacket");
    EV << "pushPacket is called in V2PIFOCanaryQueue" << endl;
    emit(packetPushedSignal, packet);
    EV_INFO << "Pushing packet " << packet->getName() << " into the queue." << endl;

    unsigned long priority = extract_priority(packet, false);
    EV << "priority is " << priority << ". finding where to push the packet." << endl;

    if (buffer != nullptr)
        throw cRuntimeError("buffer != nullptr is not implemented for V2PIFOCanaryQueue");
    else {
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

        sum_ranks_in_queue += priority;
        prio_queue.push_back(packet);
        EV << "Inserting the packet at the end of the queue" << endl;
        queue.insert(packet);
//            priority_of_last_packet_inserted_in_queue = priority;
//            std::cout << "packet is " << packet->str() << endl;
//            std::cout << "--------------------------" << endl;

        EV_INFO << "SEPEHR: A packet is inserted into the queue. Queue length: "
                << getNumPackets() << " & packetCapacity: " << packetCapacity <<
                ", Queue data occupancy is " << getTotalLength() <<
                " and dataCapacity is " << dataCapacity << endl;

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

Packet *V2PIFOCanaryQueue::popPacket(cGate *gate)
{
    Enter_Method("popPacket");
    EV << "popPacket is called in V2PIFOCanaryQueue" << endl;
    EV << "Initial queue len: " << getNumPackets() << endl;
    Packet* popped_packet = check_and_cast<Packet *>(queue.remove(prio_queue.front()));
    prio_queue.pop_front();

    unsigned long priority = extract_priority(popped_packet, false);
    sum_ranks_in_queue -= priority;

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

    return popped_packet;
}

void V2PIFOCanaryQueue::removePacket(Packet *packet)
{
    Enter_Method("removePacket");
    EV << "removePacket is called in V2PIFOCanaryQueue" << endl;
    prio_queue.remove(packet);

    unsigned long priority = extract_priority(packet, false);
    sum_ranks_in_queue -= priority;

    PacketQueue::removePacket(packet);
    emit(packetRemovedSignal, packet);
    if (packetCapacity != -1)
        cSimpleModule::emit(customQueueLengthSignal, getNumPackets());
    else
        cSimpleModule::emit(customQueueLengthSignalPacketBytes, getTotalLength().get());

    // TODO temp: check the correctness
//    check_correctness();
}

long V2PIFOCanaryQueue::get_queue_occupancy(long on_the_way_packet_num, b on_the_way_packet_length)
{
    if (getMaxNumPackets() != -1) {
        int num_packets;
        if (update_qlen_periodically)
            num_packets = periodic_qlen_num_packets;
        else
            num_packets = getNumPackets();
        return (num_packets + on_the_way_packet_num);
    }
    if (getMaxTotalLength() != b(-1)) {
        b num_bits;
        if (update_qlen_periodically)
            num_bits = periodic_qlen_bytes;
        else
            num_bits = getTotalLength();
        return (num_bits + on_the_way_packet_length).get();
    }
    throw cRuntimeError("No queue capacity specified!");
}

bool V2PIFOCanaryQueue::is_queue_full(b packet_length, long on_the_way_packet_num, b on_the_way_packet_length) {
    EV << "V2PIFOCanaryQueue::is_queue_full" << endl;
    if (using_buffer) {
        throw cRuntimeError("buffer is not yet implemented in canary queue!");
    } else {
        EV << "V2PIFO::is_queue_full" << endl;
        bool is_queue_full = false;

        if (getMaxNumPackets() != -1) {
            int num_packets;
            if (update_qlen_periodically) {
                num_packets = periodic_qlen_num_packets;
            } else {
                num_packets = getNumPackets();
            }
            is_queue_full = (num_packets + on_the_way_packet_num >= getMaxNumPackets());
            EV << "The queue capacity is " << getMaxNumPackets() << ", There are currently " << num_packets << " packets inside the queue and " << on_the_way_packet_num << " packets on the way. Is the queue full? " << is_queue_full << endl;
        } else if (getMaxTotalLength() != b(-1)) {
            b qlen_packet_bytes;
            if (update_qlen_periodically) {
                qlen_packet_bytes = periodic_qlen_bytes;
            } else {
                qlen_packet_bytes = getTotalLength();
            }
            is_queue_full = ((qlen_packet_bytes + on_the_way_packet_length + packet_length) >= getMaxTotalLength());
            EV << "The queue capacity is " << getMaxTotalLength() << ", Queue length is " << qlen_packet_bytes << " and packet length is " << packet_length << " and " << on_the_way_packet_length << " bytes on the way. Is the queue full? " << is_queue_full << endl;
        } else
            throw cRuntimeError("The queue does not have any cap!");
        return is_queue_full;
    }
}

void V2PIFOCanaryQueue::update_quantile_list(Packet* packet) {
    unsigned long priority = extract_priority(packet, false);
    if (quantile_list.size() == quantile_wind_size)
        quantile_list.pop_front();
//    std::cout << simTime() << ": pushing " << priority << " into quantile_list" << endl;
    quantile_list.push_back(priority);
}

double V2PIFOCanaryQueue::calc_quantile(Packet* packet) {
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

double V2PIFOCanaryQueue::calculate_relative_priority_dist_expon(Packet* packet) {
    int num_packets = getNumPackets();
    unsigned long priority = extract_priority(packet, false);
    if (num_packets == 0 || priority == 0)
        return 0;
    double mean = sum_ranks_in_queue / (1.0 * num_packets);
    double lambda = 1.0/mean;
    double cdf = 1 - exp(-1 * lambda * priority);
//    std::cout << "sum_ranks_in_queue: " << sum_ranks_in_queue << ", num_packets: " << num_packets << ", mean: " << mean << ", priority: "<< priority << ", base relative priority: " << cdf << endl;
    return cdf;
}

double V2PIFOCanaryQueue::calculate_relative_priority_dist(Packet* packet) {
    switch (relative_priority_distribution_type) {
        case EXPON:
            return calculate_relative_priority_dist_expon(packet);
            break;
        default:
            throw cRuntimeError("relative_priority_distribution_type not known!");
    }
    throw cRuntimeError("calculate_relative_priority_dist under construction!");
}

double V2PIFOCanaryQueue::calculate_relative_priority(Packet* packet) {
    double base_relative_prio;
    switch (relative_priority_calculation_type) {
        case PERCENTILE:
            base_relative_prio = calc_quantile(packet);
            break;
        case DISTRIBUTION:
            base_relative_prio = calculate_relative_priority_dist(packet);
            break;
        default:
            throw cRuntimeError("calculate_relative_priority: no familiar paradigm!");
    }

    if (!(base_relative_prio >= 0 && base_relative_prio <= 1)) {
        std::cout << base_relative_prio << endl;
        throw cRuntimeError("base_relative_prio < 0 || base_relative_prio > 1");
    }
    double relative_prio = (1.0-k) * base_relative_prio;
    return relative_prio;
}

bool V2PIFOCanaryQueue::is_over_v2_threshold_full_deterministic(b packet_length, Packet* packet, long on_the_way_packet_num, b on_the_way_packet_length) {
    int Q;  // queue size
    int q;  // queue length
    if (packetCapacity != -1){
        Q= packetCapacity;
        if (update_qlen_periodically) {
            q = periodic_qlen_num_packets;
        } else {
            q = getNumPackets();
        }
    }
    else if (dataCapacity != b(-1)){
        Q = dataCapacity.get();
        if (update_qlen_periodically) {
            q = periodic_qlen_bytes.get();
        } else {
            q = getTotalLength().get();
        }
    }
    double relative_prio = calculate_relative_priority(packet);
    if (!(relative_prio >= 0 && relative_prio <= 1))
        throw cRuntimeError("!(relative_prio >= 0 && relative_prio <= 1)");

    if (q <= Q * (1 - relative_prio)) {
        // when we have deflection, a full queue should indicate that the packet should be deflected
        // in AIFO admitting it does not change the fact that it gets droped but with deflection it does
        // however, is queue full is already checked before this function is called, so here
        // just return false
        return false;
    }
    EV << "queue occupancy is " << get_queue_occupancy(0, b(0)) << ", Q is " << Q << ", and q is " << q <<
            ", relative prio is " << relative_prio <<  endl;
//    int test = 0;
//    unsigned long pkt_prio = extract_priority(packet, false);
//    for (std::list<Packet*>::iterator it = prio_queue.begin(); it != prio_queue.end(); it++) {
//        unsigned long test_prio = extract_priority(*it, false);
//        if (test_prio < pkt_prio)
//            test++;
////        std::cout << test_prio << endl;
//    }
//    if (q > Q * ((1.0-k) * (test * 1.0) / getNumPackets())) {
//        std::cout << "queue occupancy is " << get_queue_occupancy(0, b(0)) << ", Q is " << Q << ", and q is " << q <<
//                    ", relative prio is " << relative_prio <<  endl;
//        std::cout << "mean: " << sum_ranks_in_queue * 1.0 / getNumPackets() << endl;
//        std::cout << "prio: " << extract_priority(packet, false) << endl;
//        std::cout << "dist:" << relative_prio << ", quantile: " << (1.0-k) * (test * 1.0) / getNumPackets() << endl;
//        std::cout << "difference: " << relative_prio - ((1.0-k) * (test * 1.0) / getNumPackets()) << endl;
////        std::cout << (q <= Q * ((1.0-k) * (test * 1.0) / getNumPackets())) << endl;
//        for (std::list<Packet*>::iterator it = prio_queue.begin(); it != prio_queue.end(); it++) {
//            unsigned long test_prio = extract_priority(*it, false);
//            std::cout << test_prio << endl;
//        }
//        std::cout << "----------------------------------------" << endl;
//
//    }
//    throw cRuntimeError("Gotcha!");
    return true;
}

bool V2PIFOCanaryQueue::is_over_v2_threshold_full_probabilistic(b packet_length, Packet* packet, long on_the_way_packet_num, b on_the_way_packet_length) {

    EV << "V2PIFOCanaryQueue::is_over_v2_threshold_full_probabilistic talking!" << endl;
    int Q;  // queue size
    int q;  // queue length
    if (packetCapacity != -1){
        Q= packetCapacity;
        if (update_qlen_periodically) {
            q = periodic_qlen_num_packets;
        } else {
            q = getNumPackets();
        }
    }
    else if (dataCapacity != b(-1)){
        Q = dataCapacity.get();
        if (update_qlen_periodically) {
            q = periodic_qlen_bytes.get();
        } else {
            q = getTotalLength().get();
        }
        // we switch these to bytes because minth is in bytes
        q /= 8;
        Q /= 8;
    }

    EV << "Dealing with LOW priority traffic class" << endl;
    EV << "Q: " << Q << ", q: " << q << ", and min_th:" << minth << endl;
    if (minth <= q && q < Q) {
        double relative_prio = calculate_relative_priority(packet);
        double pb = relative_prio * (q - minth) / (Q - minth);
        if (!(pb >= 0 && pb <= 1))
            throw cRuntimeError("pb < 0 or pb > 1");
        double dice = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        EV << "Deciding randomly to mark the packet with pb = " << pb << " and dice = " << dice << endl;
        if (dice < pb) {
            EV << "returning RANDOMLY_ABOVE_LIMIT" << endl;
//            std::cout << "1) Q: " << Q << ", q: " << q << ", minth: " << minth << ", pb: " << pb << ", relative prio: " << relative_prio << endl;
            return true;
        }
        else {
            EV << "returning RANDOMLY_BELOW_LIMIT" << endl;
            return false;
        }
    }
    else if (q >= Q) {
//        std::cout << "2) Q: " << Q << ", q: " << q << ", minth: " << minth << endl;
        EV << "returning ABOVE_MAX_LIMIT" << endl;
        return true;
    }
    EV << "returning BELOW_MIN_LIMIT" << endl;
    return false;
}

bool V2PIFOCanaryQueue::is_over_v2_threshold_full(b packet_length, Packet* packet, long on_the_way_packet_num, b on_the_way_packet_length) {
    switch (algorithm_type) {
        case PROBABILISTIC:
            return is_over_v2_threshold_full_probabilistic(packet_length, packet, on_the_way_packet_num, on_the_way_packet_length);
            break;
        case DETERMINISTIC:
            return is_over_v2_threshold_full_deterministic(packet_length, packet, on_the_way_packet_num, on_the_way_packet_length);
            break;
        default:
            throw cRuntimeError("No algorithm_type!");
    }
}

bool V2PIFOCanaryQueue::isFullPrioQueue(int queue_idx, int packet_len, Packet* packet)
{
    EV << "V2PIFOCanaryQueue::isFullPrioQueue" << endl;

    if (is_queue_full(b(packet_len), 0, b(0))) {
//        std::cout << "t = " << simTime() << ". Queue is overall full!" << endl;
        return true;
    }

    return is_over_v2_threshold_full(b(packet_len), packet, 0, b(0));
}

bool V2PIFOCanaryQueue::is_packet_tag_larger_than_last_packet(Packet* packet) {
    return true;
}
