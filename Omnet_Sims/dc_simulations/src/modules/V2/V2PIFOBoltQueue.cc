//
// In this version of V2PIFOBoltQueue, the higher the value of priority variable, the lower the packet's priority
//

#include "V2PIFOBoltQueue.h"

#include "inet/common/ModuleAccess.h"
#include "inet/common/Simsignals.h"
#include "inet/queueing/function/PacketComparatorFunction.h"
#include "inet/queueing/function/PacketDropperFunction.h"
#include <math.h>

using namespace inet;
using namespace queueing;

#define PERCENTILE 0
#define DISTRIBUTION 1

#define EXPON 0


#define MICE_FLOW_SIZE 800000   // 100KB = 800000b


Define_Module(V2PIFOBoltQueue);


simsignal_t V2PIFOBoltQueue::packetDropSeqSignal = registerSignal("packetDropSeq");
simsignal_t V2PIFOBoltQueue::packetDropRetCountSignal = registerSignal("packetDropRetCount");
simsignal_t V2PIFOBoltQueue::packetDropTotalPayloadLenSignal = registerSignal("packetDropTotalPayloadLength");

V2PIFOBoltQueue::~V2PIFOBoltQueue()
{
    recordScalar("lightInQueuePacketDropCount", light_in_queue_packet_drop_count);
    recordScalar("lightAllQueueingTime", all_packets_queueing_time_sum / num_all_packets);
    recordScalar("lightMiceQueueingTime", mice_packets_queueing_time_sum / num_mice_packets);
}

void V2PIFOBoltQueue::initialize(int stage) {
    PacketQueue::initialize(stage);
    bounce_randomly_v2 = getAncestorPar("bounce_randomly_v2");
    denominator_for_retrasnmissions = getAncestorPar("denominator_for_retrasnmissions");

    CCThresh = b(par("CCThresh"));
    mtu = int(par("mtu"));

    // Deterministic
    k = par("k");

    // AIFO (percentile)
    quantile_wind_size = par("quantile_wind_size");
    aifo_count = 0;
    aifo_sample_count = par("aifo_sample_count");

    bool use_vertigo_prio_queue = getAncestorPar("use_vertigo_prio_queue");
    if (use_vertigo_prio_queue) {
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

        if (relative_priority_calculation_type == PERCENTILE && aifo_sample_count == -1)
            throw cRuntimeError("relative_priority_calculation_type == PERCENTILE && aifo_sample_count == -1");

        if (relative_priority_calculation_type == PERCENTILE && quantile_wind_size < 1)
            throw cRuntimeError("relative_priority_calculation_type == PERCENTILE && quantile_wind_size < 0");

        if (k < 0)
            throw cRuntimeError("k < 0");
    }

    if (CCThresh == b(-1))
        throw cRuntimeError("CCThresh == b(-1) for bolt!!");

    if (mtu == -1)
        throw cRuntimeError("mtu == -1 for bolt!!");

    if (dataCapacity == b(-1) && packetCapacity == -1)
        throw cRuntimeError("dataCapacity == b(-1) && packetCapacity == -1. No queue capacity?");
}

unsigned long V2PIFOBoltQueue::calculate_priority(unsigned long seq, unsigned long ret_count) {

    // note that in extract priority we return 0 if the packet is not tcpseg
    // so this funciton is only called for data packets
    if (!bounce_randomly_v2 || denominator_for_retrasnmissions <= 0) {
        return seq;
    }

    unsigned long priority = seq;
    for (int i = 0; i < ret_count; i++) {
        // todo: here is where we apply the function
        priority = (unsigned long) (priority / denominator_for_retrasnmissions);
    }
    if (priority <= 0)
        priority = 1;
    return priority;
}

unsigned long V2PIFOBoltQueue::extract_priority(Packet *packet, bool is_packet_being_dropped) {


    // This should be added, whether marking is on or not!
    if(is_packet_being_dropped) {
        EV << "Packet dropped in extract priority!" << endl;
        light_in_queue_packet_drop_count++;
        // the packet is dropped, the priority does not matter
        return 0;
    }

    std::string packet_name = packet->getName();

    if (packet_name.find("tcpseg") == std::string::npos) {
        // prioritize SYN, ACK, SRC over data packet
        EV << "control packet, prio: 0" << endl;
        return 0;
    }

    unsigned long priority;
    unsigned long seq, ret_count;
    auto packet_dup = packet->dup();
    auto etherheader = packet_dup->removeAtFront<EthernetMacHeader>();
    auto ipv4header = packet_dup->peekAtFront<Ipv4Header>();
    delete packet_dup;

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
            EV << "data packet, prio: " << priority << endl;
            return priority;
        }
    }

    // The marking component is probably off!
    throw cRuntimeError("Marking component is off which makes no sense for prio_queues. How are you assiging prio queues?");
}

void V2PIFOBoltQueue::set_BW_bps(double bandwidth) {
    if (bw == -1) {
        bw = bandwidth;
        EV << getFullPath() << " --> bandwidth set to " << bw << endl;
    }
}

void V2PIFOBoltQueue::calculate_suply_token(int packet_size_bits) {
    simtime_t now = simTime();
    double inter_arrival_time = (now - last_sm_time).dbl();
    if (inter_arrival_time <= 0) {
        inter_arrival_time = (now - last_sm_time_backup).dbl();
        if (inter_arrival_time <= 0)
            throw cRuntimeError("This is practically impossible!");
    } else {
        last_sm_time_backup = last_sm_time;
        last_sm_time = now;
    }
    EV << "bw is " << bw << endl;
    double supply = bw * inter_arrival_time;
    // demand is packet_size
    EV << "The old sm_token is " << sm_token;
    sm_token += (supply - packet_size_bits);
    sm_token = std::min(sm_token, mtu);
    EV << " and the new one is " << sm_token << endl;
}

void V2PIFOBoltQueue::pushPacket(Packet *packet, cGate *gate)
{
    Enter_Method("pushPacket");
    EV << "pushPacket is called in V2PIFOBoltQueue" << endl;
    emit(packetPushedSignal, packet);
    EV_INFO << "Pushing packet " << packet->getName() << " into the queue." << endl;

    bool is_packet_deflected = get_packet_deflection_tag(packet);

    unsigned long priority = extract_priority(packet, false);
    EV << "priority is " << priority << ". finding where to push the packet." << endl;

    if (buffer != nullptr)
        throw cRuntimeError("buffer != nullptr is not implemented for V2PIFOBoltQueue");
    else {
        int queue_occupancy = getNumPackets();
        auto eth_header = packet->removeAtFront<EthernetMacHeader>();
        eth_header->setQueue_occupancy(queue_occupancy);
        packet->insertAtFront(eth_header);


        bool should_insert_packet;
        // AIFO might consider it as full sooner than normal
        if (bounce_randomly_v2) {
            // Early deflection is on so we drop packets
            // both forwarding and early deflection in relay unit
            // so no need to check here again to see if we should drop the packet.
            should_insert_packet = true;
        } else {
            should_insert_packet = (!isOverloaded());
        }

        if (should_insert_packet) {

            sum_ranks_in_queue += priority;

            if (priority == 0) {
                // None data packets -- Prioritize
                EV << "Inserting the packet at the front of the queue" << endl;
                signal_packet_queue.push_back(packet);
            } else {
                if (!is_packet_deflected) {
                    calculate_suply_token(packet->getTotalLength().get());
                    EV << packet->str() << endl;
                    b position = packet->getFrontOffset();
                    packet->setFrontIteratorPosition(b(0));
                    auto eth_header = packet->removeAtFront<EthernetMacHeader>();
                    auto ip_header = packet->removeAtFront<Ipv4Header>();
                    auto tcp_header = packet->removeAtFront<tcp::TcpHeader>();

                    // if (getTotalLength() > CCThresh) is handled in relay unit
                    // just assign tokens to non-deflected packets, you don't need to reset bolt_inc
                    // for deflected packets because it is set to false when they are deflected
                    if (getTotalLength() < CCThresh) {
                        if (tcp_header->getBolt_last()) {
                            if (!tcp_header->getBolt_first()) {
                                pru_token++;
                            }
                        } else if (tcp_header->getBolt_inc()) {
                            if (pru_token > 0) {
                                pru_token--;
                            } else if (sm_token >= mtu) {
                                sm_token -= mtu;
                            } else {
                                tcp_header->setBolt_inc(false);
                            }
                        }
                    }

                    packet->insertAtFront(tcp_header);
                    packet->insertAtFront(ip_header);
                    packet->insertAtFront(eth_header);
                    packet->setFrontIteratorPosition(position);
                }

                EV << "Inserting the packet at the end of the queue" << endl;
                data_packet_queue.push_back(packet);
            }
            queue.insert(packet);
    //            priority_of_last_packet_inserted_in_queue = priority;
    //            std::cout << "packet is " << packet->str() << endl;
    //            std::cout << "--------------------------" << endl;

            EV_INFO << "SEPEHR: A packet is inserted into the queue. Queue length: "
                    << getNumPackets() << " & packetCapacity: " << packetCapacity <<
                    ", Queue data occupancy is " << getTotalLength() <<
                    " and dataCapacity is " << dataCapacity << endl;
        } else {
            // packet should get dropped
            if (bounce_randomly_v2) {
                throw cRuntimeError("Packets are dropped at relay for these cases not queue!");
            }
            priority = extract_priority(packet, true);
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

Packet *V2PIFOBoltQueue::popPacket(cGate *gate)
{
    Enter_Method("popPacket");
    EV << "popPacket is called in V2PIFOBoltQueue" << endl;
    EV << "Initial queue len: " << getNumPackets() << endl;
    Packet* popped_packet;
    if (signal_packet_queue.size() != 0) {
        popped_packet = check_and_cast<Packet *>(queue.remove(signal_packet_queue.front()));
        signal_packet_queue.pop_front();
    } else {
        popped_packet = check_and_cast<Packet *>(queue.remove(data_packet_queue.front()));
        data_packet_queue.pop_front();
    }

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


    return popped_packet;
}

void V2PIFOBoltQueue::removePacket(Packet *packet)
{
    Enter_Method("removePacket");
    EV << "removePacket is called in V2PIFOBoltQueue" << endl;
    unsigned long priority = extract_priority(packet, false);
    if (priority == 0) {
        signal_packet_queue.remove(packet);
    } else
        data_packet_queue.remove(packet);

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

long V2PIFOBoltQueue::get_queue_occupancy(long on_the_way_packet_num, b on_the_way_packet_length)
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

bool V2PIFOBoltQueue::is_queue_full(b packet_length, long on_the_way_packet_num, b on_the_way_packet_length) {
    EV << "V2PIFOBoltQueue::is_queue_full" << endl;

    bool is_queue_full = (getMaxNumPackets() != -1 && getNumPackets() + on_the_way_packet_num >= getMaxNumPackets()) ||
            (getMaxTotalLength() != b(-1) && (getTotalLength() + on_the_way_packet_length + packet_length) >= getMaxTotalLength());
    EV << "Checking if queue is full" << endl;
    if (getMaxNumPackets() != -1)
        EV << "The queue capacity is " << getMaxNumPackets() << ", There are currently " << getNumPackets() << " packets inside the queue and " << on_the_way_packet_num << " packets on the way. Is the queue full? " << is_queue_full << endl;
    else if (getMaxTotalLength() != b(-1))
        EV << "The queue capacity is " << getMaxTotalLength() << ", Queue length is " << getTotalLength() << " and packet length is " << packet_length << " and " << on_the_way_packet_length << " bytes on the way. Is the queue full? " << is_queue_full << endl;
    return is_queue_full;
}

void V2PIFOBoltQueue::update_quantile_list(Packet* packet) {
    unsigned long priority = extract_priority(packet, false);
    if (quantile_list.size() == quantile_wind_size)
        quantile_list.pop_front();
//    std::cout << simTime() << ": pushing " << priority << " into quantile_list" << endl;
    quantile_list.push_back(priority);
}

double V2PIFOBoltQueue::calc_quantile(Packet* packet) {
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

double V2PIFOBoltQueue::calculate_relative_priority_dist_expon(Packet* packet) {
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

double V2PIFOBoltQueue::calculate_relative_priority_dist(Packet* packet) {
    switch (relative_priority_distribution_type) {
        case EXPON:
            return calculate_relative_priority_dist_expon(packet);
            break;
        default:
            throw cRuntimeError("relative_priority_distribution_type not known!");
    }
    throw cRuntimeError("calculate_relative_priority_dist under construction!");
}

double V2PIFOBoltQueue::calculate_relative_priority(Packet* packet) {
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
        EV << base_relative_prio << endl;
        throw cRuntimeError("base_relative_prio < 0 || base_relative_prio > 1");
    }
    double relative_prio = (1.0-k) * base_relative_prio;
    return relative_prio;
}

bool V2PIFOBoltQueue::is_over_v2_threshold_full(b packet_length, Packet* packet, long on_the_way_packet_num, b on_the_way_packet_length) {
    int Q;  // queue size
    int q;  // queue length
    if (packetCapacity != -1){
        Q= packetCapacity;
        q = getNumPackets();
    }
    else if (dataCapacity != b(-1)){
        Q = dataCapacity.get();
        q = getTotalLength().get();
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

bool V2PIFOBoltQueue::isFullPrioQueue(int queue_idx, int packet_len, Packet* packet)
{
    EV << "V2PIFOBoltQueue::isFullPrioQueue" << endl;

    if (is_queue_full(b(packet_len), 0, b(0))) {
//        std::cout << "t = " << simTime() << ". Queue is overall full!" << endl;
        return true;
    }

    // no neet to check for over_v2_threshold if we are not bouncing
    if (!bounce_randomly_v2)
        return false;

    return is_over_v2_threshold_full(b(packet_len), packet, 0, b(0));
}

bool V2PIFOBoltQueue::is_packet_tag_larger_than_last_packet(Packet* packet) {
    return true;
}
