//
// In this version of V2PIFO, the higher the value of priority variable, the lower the packet's priority
//

#include "inet/common/ModuleAccess.h"
#include "inet/common/Simsignals.h"
#include "inet/queueing/function/PacketComparatorFunction.h"
#include "inet/queueing/function/PacketDropperFunction.h"
#include "./V2PIFO.h"
#include <sqlite3.h>
#include "inet/linklayer/common/InterfaceTag_m.h"

using namespace inet;
using namespace queueing;

#define LAS 0
#define SRPT 1
#define FIFO 2

#define MICE_FLOW_SIZE 800000   // 100KB = 800000b


Define_Module(V2PIFO);

bool popped_marking_off_error = false;

simsignal_t V2PIFO::packetDropSeqSignal = registerSignal("packetDropSeq");
simsignal_t V2PIFO::packetDropRetCountSignal = registerSignal("packetDropRetCount");
simsignal_t V2PIFO::packetDropTotalPayloadLenSignal = registerSignal("packetDropTotalPayloadLength");
simsignal_t V2PIFO::packetRankSignal = registerSignal("packetRank");

V2PIFO::~V2PIFO()
{
    cancelAndDelete(updateQueueOccupancyMsg);
    recordScalar("lightInQueuePacketDropCount", light_in_queue_packet_drop_count);
    recordScalar("lightAllQueueingTime", all_packets_queueing_time_sum / num_all_packets);
    recordScalar("lightMiceQueueingTime", mice_packets_queueing_time_sum / num_mice_packets);
}

static int callback_incremental_deployment(void *data, int argc, char **argv, char **azColName){
   int i;
   V2PIFO *queue_object = (V2PIFO*) data;

   for(i = 0; i<argc; i++){
      if (std::strcmp(argv[i], "") != 0) {
          int result = std::stoi(argv[i]);
          if (result != 1 && result != 0) {
              throw cRuntimeError("Incremental deflection is neither 0 nor 1!");
          }
          queue_object->deployed_with_deflection = result;
          return 0;
      }
   }

   throw cRuntimeError("No result found for incremental deflection identifier!");
   return -1;
}

void V2PIFO::read_inc_deflection_properties(std::string incremental_deployment_identifier, std::string input_file_name)
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

void V2PIFO::initialize(int stage) {
    PacketQueue::initialize(stage);
    dctcp_thresh = par("dctcp_thresh");
    dctcp_mark_deflected_packets_only = getAncestorPar("dctcp_mark_deflected_packets_only");
    incremental_deployment = getAncestorPar("incremental_deployment");
    std::string incremental_deployment_file_name = getAncestorPar("incremental_deployment_file_name");
    int repetition_num = atoi(getEnvir()->getConfigEx()->getVariable(CFGVAR_REPETITION));
    std::string rep_num_string = std::to_string(repetition_num);
    incremental_deployment_file_name += ("_" + rep_num_string + "_rep.db");
    cModule *switch_module = getParentModule()->getParentModule()->getParentModule();
    denominator_for_retrasnmissions = getAncestorPar("denominator_for_retrasnmissions");

    CCThresh = b(par("CCThresh"));
    mtu = int(par("mtu"));
    use_bolt_queue = getAncestorPar("use_bolt_queue");
    use_bolt_with_vertigo_queue = getAncestorPar("use_bolt_with_vertigo_queue");
    use_bolt = use_bolt_queue || use_bolt_with_vertigo_queue;

    // selective reaction
    apply_selective_net_reaction = getAncestorPar("apply_selective_net_reaction");
    bolt_CCThresh_max_selective_reaction = b(par("bolt_CCThresh_max_selective_reaction"));
    sel_reaction_quantile_wind_size = par("sel_reaction_quantile_wind_size");
    sel_reaction_count = 0;
    sel_reaction_sample_count = par("sel_reaction_sample_count");
    if (apply_selective_net_reaction) {
        if (sel_reaction_quantile_wind_size < 1 || sel_reaction_sample_count < 1)
            throw cRuntimeError("sel_reaction_quantile_wind_size < 1 || sel_reaction_sample_count < 1");
    }

    if (use_bolt && !use_bolt_with_vertigo_queue) {
        throw cRuntimeError("use_bolt_with_vertigo_queue is false in V2PIFO! This is kinda weird!");
    }

    if (incremental_deployment) {
        std::string incremental_deployment_identifier =
                    switch_module->getName() +
                    std::to_string(switch_module->getIndex());

        read_inc_deflection_properties(incremental_deployment_identifier, incremental_deployment_file_name);
        if (deployed_with_deflection == 1) {
            // read the deflection settings
            bounce_randomly_v2 = getAncestorPar("bounce_randomly_v2");

            std::string dropper_type_str = par("dropper_type");
            if (dropper_type_str.compare("LAS") == 0)
                dropper_type = LAS;
            else if (dropper_type_str.compare("SRPT") == 0)
                dropper_type = SRPT;
            else if (dropper_type_str.compare("FIFO") == 0)
                dropper_type = FIFO;
            else
                throw cRuntimeError("No dropper type identified!");

            std::string scheduler_type_str = par("scheduler_type");
            if (scheduler_type_str.compare("LAS") == 0)
                scheduler_type = LAS;
            else if (scheduler_type_str.compare("SRPT") == 0)
                scheduler_type = SRPT;
            else if (scheduler_type_str.compare("FIFO") == 0)
                scheduler_type = FIFO;
            else
                throw cRuntimeError("No dropper type identified!");
        } else {
            // set the values as default (FIFO, no deflection)
            bounce_randomly_v2 = false;
            dropper_type = FIFO;
            scheduler_type = FIFO;
        }
    } else {
        bounce_randomly_v2 = getAncestorPar("bounce_randomly_v2");

        std::string dropper_type_str = par("dropper_type");
        if (dropper_type_str.compare("LAS") == 0)
            dropper_type = LAS;
        else if (dropper_type_str.compare("SRPT") == 0)
            dropper_type = SRPT;
        else if (dropper_type_str.compare("FIFO") == 0)
            dropper_type = FIFO;
        else
            throw cRuntimeError("No dropper type identified!");

        std::string scheduler_type_str = par("scheduler_type");
        if (scheduler_type_str.compare("LAS") == 0)
            scheduler_type = LAS;
        else if (scheduler_type_str.compare("SRPT") == 0)
            scheduler_type = SRPT;
        else if (scheduler_type_str.compare("FIFO") == 0)
            scheduler_type = FIFO;
        else
            throw cRuntimeError("No dropper type identified!");
    }

    bool bounce_naively = getAncestorPar("bounce_naively");
    if (bounce_naively && (scheduler_type != FIFO || dropper_type != FIFO))
        throw cRuntimeError("bounce_naively && (scheduler_type != FIFO || dropper_type != FIFO)");


    std::string where_to_mark_packets = par("where_to_mark_packets");
    if (where_to_mark_packets.compare("enqueue") == 0) {
        mark_packets_in_enqueue = true;
    } else if (where_to_mark_packets.compare("dequeue") == 0) {
        mark_packets_in_enqueue = false;
    } else
        throw cRuntimeError("where to mark packet is neither enqueue nor dequeue!");

    if (use_bolt && CCThresh == b(-1))
        throw cRuntimeError("CCThresh == b(-1) for bolt!!");

    if (use_bolt && mtu == -1)
        throw cRuntimeError("mtu == -1 for bolt!!");

    if (use_bolt && (scheduler_type == FIFO || dropper_type == FIFO)) {
        throw cRuntimeError("Bolt should at least prioritize control packets!");
    }

    if (dctcp_mark_deflected_packets_only && dctcp_thresh >= 0)
        throw cRuntimeError("dctcp_mark_deflected_packets_only && dctcp_thresh >= 0");

    just_prioritize_bursty_flows = par("just_prioritize_bursty_flows");
    if (just_prioritize_bursty_flows &&
            (scheduler_type != SRPT || dropper_type != SRPT)) {
        throw cRuntimeError("just_prioritize_bursty_flows && (!scheduler_type != SRPT || dropper_type != SRPT)");
    }

    if (stage == INITSTAGE_LOCAL) {

        std::string buffer_name = par("bufferModule");
        std::string buffer_full_name = buffer_name;
        int buffer_idx = getParentModule()->getParentModule()
                ->getParentModule()->getIndex();
        buffer_full_name += "[" + std::to_string(buffer_idx) + "]";
        try {
            buffer = check_and_cast<V2PacketBuffer*>(getModuleByPath(buffer_full_name.c_str()));
        } catch (...) {
            buffer = nullptr;
        }

        using_buffer = (buffer != nullptr);

        if (buffer_idx == 0 && getParentModule()->getParentModule()
                ->getIndex() == 0) {
            if (using_buffer)
                std::cout << "Buffer is loaded!" << endl;
            else
                std::cout << "No buffer is loaded!" << endl;
        }
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

void V2PIFO::sel_reaction_update_quantile_list(Packet* packet) {
    if (sel_reaction_count == 0) {
        unsigned long priority = extract_priority(packet, false);
        if (sel_reaction_quantile_list.size() == sel_reaction_quantile_wind_size)
            sel_reaction_quantile_list.pop_front();
    //    std::cout << simTime() << ": pushing " << priority << " into quantile_list" << endl;
        sel_reaction_quantile_list.push_back(priority);
    }
    sel_reaction_count++;
    sel_reaction_count %= sel_reaction_sample_count;
}

double V2PIFO::sel_reaction_calc_quantile(Packet* packet) {
    EV << "V2PIFO::sel_reaction_calc_quantile called." << endl;
    unsigned long priority = extract_priority(packet, false);
    int sum = 0;
    for (auto it = sel_reaction_quantile_list.begin(); it != sel_reaction_quantile_list.end(); it++) {
        if ((*it) < priority)
            sum += 1;
    }

    return sum * 1.0 / sel_reaction_quantile_wind_size;
}

void V2PIFO::set_BW_bps(double bandwidth) {
    if (bw == -1) {
        bw = bandwidth;
    }
}

void V2PIFO::handleMessage(cMessage *message) {
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

bool V2PIFO::isOverloaded() {
    bool is_overloaded;
    EV << "V2PIFO::isOverloaded --> qlen_pkt_num: " << getNumPackets() << ", qlen_bytes: " << getTotalLength() << endl;
    if (using_buffer)
        is_overloaded = buffer->isOverloaded(getNumPackets(), getTotalLength());
    else {
        is_overloaded = PacketQueue::isOverloaded();
    }
    if (is_overloaded)
        EV << "The queue is overloaded" << endl;
    return is_overloaded;
}

void V2PIFO::calculate_suply_token(int packet_size_bits) {
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
//    std::cout << "bw is " << bw << endl;
    double supply = bw * inter_arrival_time;
    // demand is packet_size
//    std::cout << "The old sm_token is " << sm_token;
    sm_token += (supply - packet_size_bits);
    sm_token = std::min(sm_token, mtu);
//    std::cout << " and the new one is " << sm_token << endl;
}

void V2PIFO::check_correctness() {
    int num_packets_in_hash_table = 0;
    // All packets in a list have the same priority?
    for (std::map<unsigned long, std::list<Packet*>>::iterator it =
            sorted_queued_packet_hash_table.begin(); it != sorted_queued_packet_hash_table.end();
            it++) {
        if (it->second.size() <= 0)
            throw cRuntimeError("How can there be an empty list in your hash table?");
        num_packets_in_hash_table += it->second.size();
        unsigned long last;
        bool seen = false;
        for (std::list<Packet*>::iterator it2 = it->second.begin(); it2 != it->second.end(); it2++) {
            unsigned long priority = extract_priority(*it2, false);
            if (seen && last != priority)
                throw cRuntimeError("Packets in the list of one priority do not actually have the same priority");
            seen = true;
            last = priority;
        }
    }
    if (getNumPackets() != num_packets_in_hash_table)
        throw cRuntimeError("mismatch between queue and num_packets_in_hash_table: getNumPackets() != num_packets_in_hash_table");

    // The number of packets is the same in the queue and hash table
    std::map<unsigned long, int> priority_seen_count;
    for (int j = 0; j < getNumPackets(); j++) {
        auto temp_packet = check_and_cast<Packet *>(queue.get(j));
        unsigned long priority = extract_priority(temp_packet, false);
        auto priority_found = priority_seen_count.find(priority);
        int priority_idx;
        if (priority_found != priority_seen_count.end()) {
            priority_found->second++;
            priority_idx = priority_found->second;
        } else {
            priority_seen_count.insert(std::pair<unsigned long, int>(priority, 0));
            priority_idx = 0;
        }
        auto temp_priority_found = sorted_queued_packet_hash_table.find(priority);
        auto packet_iterator = temp_priority_found->second.begin();
        std::advance(packet_iterator, priority_idx);
        if (temp_packet != (*packet_iterator))
            throw cRuntimeError("Mismatch between packets in queue and packets in hash table!");
    }
}

int V2PIFO::getNumPacketsToEject(b packet_length, long seq, long ret_count,
        long on_the_way_packet_num, b on_the_way_packet_length) {
    // pass bitlength to this because the phy header is removed when the packet is inserted in the queue
    // Ejects packets with the lowest priorities, also add fifo ejecting
    EV << "getNumPacketsToEject called for packet with lenght: " << packet_length <<
            ", seq: " << seq << ", and ret_count: " << ret_count << endl;
    if (dropper_type == FIFO) {
        // If you're acting FIFO, there is no sense in ejecting any packets, how are you deciding
        // which packet to eject? You should just bounce/drop the received packet itself
        return -1;
    }
    unsigned long priority = calculate_priority(seq, ret_count);
    int num_packets_to_eject = 0;
    b required_length = packet_length;
    long queue_occupancy = get_queue_occupancy(on_the_way_packet_num, on_the_way_packet_length);
    long max_capacity = -1;
    // maximum number of packets/bytes in the queue
    bool use_packet_num, use_packet_bytes;
    if (using_buffer) {
        use_packet_num = getMaxNumPackets() != -1 || buffer->getMaxNumPackets() != -1;
        use_packet_bytes = (getMaxTotalLength() != b(-1) || buffer->getMaxTotalLength() != b(-1));
        max_capacity = queue_occupancy;
    } else {
        use_packet_num = getMaxNumPackets() != -1;
        use_packet_bytes = getMaxTotalLength() != b(-1);
        if (use_packet_num) {
            max_capacity = getMaxNumPackets();
        } else {
            max_capacity = getMaxTotalLength().get();
        }
    }
    if (use_packet_num == use_packet_bytes)
        throw cRuntimeError("We are either considering both packet num and bytes or none of them!");

    if (max_capacity == -1)
        throw cRuntimeError("max_capacity == -1");

    EV << "max capacity is " << max_capacity << endl;

    if (sorted_queued_packet_hash_table.size() == 0)
        throw cRuntimeError("How is this possible? The queue is empty but you're ejecting a packet!");
    std::map<unsigned long, std::list<Packet*>>::iterator map_it =
            sorted_queued_packet_hash_table.end();
    map_it--;
    if (map_it->second.size() == 0) {
        throw cRuntimeError("There shouldn't be any empty lists in our hash map1!");
    }
    std::list<Packet*>::iterator list_it = map_it->second.end();
    list_it--; // we made sure that list isn't empty
    int num_packets = getNumPackets();
    while(num_packets > 0) {
        num_packets--; // makes sure that there is no infinite while loop in the code
        EV << "considering packet " << (*list_it)->str() << " with priority=" << map_it->first << endl;
        if (map_it->first > priority) {
            // packet has lower priority
            EV << "packet has higher remaining bytes" << endl;
            num_packets_to_eject++;
            if (use_packet_num) {
                queue_occupancy--;
            } else {
                queue_occupancy -= (*list_it)->getBitLength();
            }
            if ((use_packet_num && max_capacity - queue_occupancy >= 1) ||
                    (use_packet_bytes && max_capacity - queue_occupancy >= packet_length.get()))
                return num_packets_to_eject;
        } else {
            EV << "not enough packets with lower priority to be ejected" << endl;
            // not enough packets with lower priority to be ejected
            return -1;
        }
        if (list_it == map_it->second.begin()) {
            // check if we reached the first packet of first list
            EV << "check if we reached the first packet of first list" << endl;
            if (map_it == sorted_queued_packet_hash_table.begin())
                return -1;
            map_it--;
            if (map_it->second.size() == 0) {
                throw cRuntimeError("There shouldn't be any empty lists in our hash map2!");
            }
            list_it = map_it->second.end();
            list_it--;
        } else {
            list_it--;
        }
    }
    return -1;
}

std::list<Packet*> V2PIFO::eject_and_push(int num_packets_to_eject) {
    if (dropper_type == FIFO) {
        // If you're acting FIFO, there is no sense in ejecting any packets, how are you deciding
        // which packet to eject? You should just bounce/drop the received packet itself
        throw cRuntimeError("Based on getNumPacketsToEject's definition, this function should never be called"
                " if the dropper/bouncer is FIFO.");
    }
    std::list<Packet*> packets;
    for (int i = 0; i < num_packets_to_eject; i++) {
        std::map<unsigned long, std::list<Packet*>>::iterator map_it =
                    sorted_queued_packet_hash_table.end();
        map_it--;
        if (map_it->second.size() == 0) {
            throw cRuntimeError("There shouldn't be any empty lists in our hash map3!");
        }
        packets.push_back(check_and_cast<Packet *>(queue.remove(map_it->second.back())));
        if (using_buffer)
            buffer->removePacket(map_it->second.back());
        map_it->second.pop_back();
        if (map_it->second.size() == 0)
            sorted_queued_packet_hash_table.erase(map_it->first);
    }
    if (packets.size() != num_packets_to_eject)
        throw cRuntimeError("packets.size() != num_packets_to_eject");
    return packets;
}

unsigned long V2PIFO::calculate_priority(unsigned long seq, unsigned long ret_count) {
    if (!bounce_randomly_v2 || denominator_for_retrasnmissions <= 0) {
        return seq;
    }

    unsigned long priority = seq;
    for (int i = 0; i < ret_count; i++) {
        // todo: here is where we apply the function
        priority = (unsigned long) (priority / denominator_for_retrasnmissions);
    }

    if (priority <= 0) {
        if (use_bolt)
            priority = 1;
        else
            priority = 0;
    }
    return priority;
}

unsigned long V2PIFO::extract_priority(Packet *packet, bool is_packet_being_dropped, bool record_rank) {
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

    if (use_bolt || just_prioritize_bursty_flows) {
        std::string packet_name = packet->getName();

        if (packet_name.find("tcpseg") == std::string::npos) {
            // prioritize SYN, ACK, SRC over data packet
            EV << "control packet, prio: 0" << endl;
            return 0;
        }

        if (just_prioritize_bursty_flows) {
            if (etherheader->getIs_bursty()) {
                return 1;
            } else {
                return 2;
            }
        }
    }

    for (unsigned int i = 0; i < ipv4header->getOptionArraySize(); i++) {
        const TlvOptionBase *option = &ipv4header->getOption(i);
        if (option->getType() == IPOPTION_V2_MARKING) {
            auto opt = check_and_cast<const Ipv4OptionV2Marking*>(option);
            seq = opt->getSeq();

/*
            if (record_rank && !is_packet_being_dropped)
                cSimpleModule::emit(packetRankSignal, seq);
*/

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
    if (!popped_marking_off_error) {
        popped_marking_off_error = true;
        std::cout << "Option not found in V2PIFO, marking is probably off. Setting prio to 1 for all packets." << endl;
    }
    return 1;
}

std::list<Packet*> V2PIFO::get_list_of_packets_to_react(Packet *new_packet) {
    if (using_buffer)
        throw cRuntimeError("using buffer has not yet been implemented in V2PIFO::get_list_of_packets_to_react");
    EV << "get_list_of_packets_to_react called!" << endl;
    std::list<Packet*> list_of_packets_we_shoud_react_to;
    int new_packet_len = new_packet->getBitLength();
    unsigned long new_packet_prio = extract_priority(new_packet);
    int total_packet_length = 0;
    if (scheduler_type == FIFO) {
        // FIFO
        throw cRuntimeError("get_list_of_packets_to_react not implemented for scheduler_type == FIFO");
    } else {
        // SRPT or LAS
        std::map<unsigned long, std::list<Packet*>>::iterator map_it =
                    sorted_queued_packet_hash_table.begin();
        if (sorted_queued_packet_hash_table.size() == 0 || map_it->second.size() == 0) {
            throw cRuntimeError("BoltPIFO: We made sure there are some packets, the hash should not be empty");
        }
        std::list<Packet*>::iterator list_it = map_it->second.begin();
        int num_packets = getNumPackets();
        while(num_packets > 0) {
            num_packets--; // makes sure that there is no infinite while loop in the code
            if (total_packet_length >= CCThresh.get())
                break;
            total_packet_length += (*list_it)->getBitLength();
            if (total_packet_length + new_packet_len > CCThresh.get()) {
                unsigned long packet_prio = extract_priority(*list_it);
                if (packet_prio > new_packet_prio) {
                    list_of_packets_we_shoud_react_to.push_back(*list_it);
                }
            }
            list_it++;
            if (list_it == map_it->second.end()) {
                // check if we reached the last packet of list
                map_it++;
                if (map_it == sorted_queued_packet_hash_table.end())
                    break;
                if (map_it->second.size() == 0) {
                    throw cRuntimeError("BoltPIFO: There shouldn't be any empty lists in our hash map2!");
                }
                list_it = map_it->second.begin();
            }
        }

        if (list_of_packets_we_shoud_react_to.size() == 0)
            list_of_packets_we_shoud_react_to.push_back(new_packet);
    }
    if (list_of_packets_we_shoud_react_to.size() == 0)
        throw cRuntimeError("The list should not be empty by this point!");
    return list_of_packets_we_shoud_react_to;
}

void V2PIFO::pushPacket(Packet *packet, cGate *gate)
{

    Enter_Method("pushPacket");
    EV << "pushPacket is called in V2PIFO" << endl;
    emit(packetPushedSignal, packet);
    EV_INFO << "Pushing packet " << packet->getName() << " into the queue." << endl;
    bool is_packet_deflected = get_packet_deflection_tag(packet);
    bool packet_is_cleared = false;
    if (using_buffer) {
        auto packet_insertion_control_tag = packet->findTag<PacketInsertionControlTag>();
        if (packet_insertion_control_tag != nullptr) {
            EV << "packet_insertion_control_tag found!";
            packet_is_cleared = packet_insertion_control_tag->getInsert_packet_without_checking_the_queue();
            packet->removeTag<PacketInsertionControlTag>();
        }
    }

    // see if you should mark packet
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

    //calculate priority
    unsigned long priority = extract_priority(packet, false, true);
    EV << "priority is " << priority << ". finding where to push the packet." << endl;

    if (use_bolt) {
        if (!is_packet_deflected && priority != 0) {
            // data packet
            calculate_suply_token(packet->getTotalLength().get());
//            std::cout << packet->str() << endl;
            b position = packet->getFrontOffset();
            packet->setFrontIteratorPosition(b(0));
            auto eth_header = packet->removeAtFront<EthernetMacHeader>();
            auto ip_header = packet->removeAtFront<Ipv4Header>();
            auto tcp_header = packet->removeAtFront<tcp::TcpHeader>();

            // if (getTotalLength() > CCThresh) is handled in relay unit
            // just assign tokens to non-deflected packets, you don't need to reset bolt_inc
            // for deflected packets because it is set to false when they are deflected
            b qlen_pkt_bytes;
            if (update_qlen_periodically)
                qlen_pkt_bytes = periodic_qlen_bytes;
            else
                qlen_pkt_bytes = getTotalLength();
            if (qlen_pkt_bytes < CCThresh) {
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
    }

    // we don't need to consider on_the_way_packets in this case
    long queue_occupancy_before_pushing_packet = get_queue_occupancy(0, b(0));

    // update selective reaction list
    if (apply_selective_net_reaction) {
        std::string packet_name = packet->getName();
        if (packet_name.find("tcpseg") != std::string::npos) {
            sel_reaction_update_quantile_list(packet);
        }
    }

    // push packet
    auto priority_found = sorted_queued_packet_hash_table.find(priority);
    if (priority_found != sorted_queued_packet_hash_table.end()) {
        priority_found->second.push_back(packet);
    } else {
        std::list<Packet*> packets;
        packets.push_back(packet);
        sorted_queued_packet_hash_table.insert(std::pair<unsigned long,
                std::list<Packet*>>(priority, packets));
    }
    EV << "Inserting the packet at the end of the queue" << endl;
    queue.insert(packet);
    if (using_buffer)
        buffer->addPacket(packet);

    // TODO temp: check the correctness
//    check_correctness();

    EV_INFO << "SEPEHR: A packet is inserted into the queue. Queue length: "
            << getNumPackets() << " & packetCapacity: " << packetCapacity <<
            ", Queue data occupancy is " << getTotalLength() <<
            " and dataCapacity is " << dataCapacity << endl;

    int num_packets = getNumPackets();
    while (isOverloaded() &&
            num_packets > 0) {
        if (using_buffer &&
                (packet_is_cleared ||
                get_queue_occupancy(0, b(0)) <= queue_occupancy_before_pushing_packet)) {
            /*
             * In shared buffer mechanism, a queue's capacity through time might change
             * Now assume that we want to insert one packet and the queue becomes overloaded
             * We can't just drop packets as long as the queue is overloaded, what if
             * the queue capacity has changed through time and now is less than before
             * this would mean extracting packets from the queue and dropping them
             * when DT reduces the capacity which is not how DT work
             * so the first condition makes sure that we drop as long as the
             * occupancy is more than the occupancy before pushing the packet in
             * this is like bringing the queue back to its previous steady state
             */
            break;
        }
        num_packets--; // avoiding infinite loops
        unsigned long priority;
        Packet *packet;
        if (dropper_type == FIFO) {
            // drop the last packet received
            EV << "Drop the last packet received" << endl;
            packet = check_and_cast<Packet *>(queue.remove(queue.get(getNumPackets() - 1)));
            priority = extract_priority(packet, true);
            auto priority_found = sorted_queued_packet_hash_table.find(priority);
            if (priority_found == sorted_queued_packet_hash_table.end() ||
                    priority_found->second.size() == 0)
                throw cRuntimeError("Priority doesn't exist or its list is emptly!");
            if (packet != priority_found->second.back())
                throw cRuntimeError("packet != priority_found->second.back(). Mismatch between the packets of hash table and queue!");
            priority_found->second.pop_back(); // the packet that is received last, it stored last in the list of each priority
            if (priority_found->second.size() == 0) {
                sorted_queued_packet_hash_table.erase(priority_found->first);
            }
        } else {
            // drop the lowest priority packet
            EV << "Drop the lowest priority packet" << endl;
            std::map<unsigned long, std::list<Packet*>>::iterator it = sorted_queued_packet_hash_table.end();
            it--;
            priority = it->first;
            if (priority != extract_priority(it->second.back(), true)) {
                // this checks if the priority of the packet we are dropping is equal
                // to its key in hash table
                // it also emit appropriate dropped signals
                throw cRuntimeError("Priority mismatch between packet and hash key!");
            }
            packet = check_and_cast<Packet *>(queue.remove(it->second.back()));
            if (packet != it->second.back())
                throw cRuntimeError("packet != it->second.back(). Mismatch between the packets of hash table and queue!");
            it->second.pop_back();
            if (it->second.size() == 0) {
                sorted_queued_packet_hash_table.erase(it->first);
            }
        }

        // for packet drops we emit 4 other signals indicated in extract_priority()
//            PacketDropDetails details;
//            details.setReason(QUEUE_OVERFLOW);
//            emit(packetDroppedSignal, packet, &details);
        EV << "Queue is overloaded. Dropping the packet in queue with priority: " << priority<< endl;
        if (using_buffer)
            buffer->removePacket(packet);
        delete packet;
    }
//        check_correctness();

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

Packet *V2PIFO::popPacket(cGate *gate)
{
    Enter_Method("popPacket");
    EV << "popPacket is called in V2PIFO" << endl;
    EV << "Initial queue len: " << getNumPackets() << endl;
    Packet* popped_packet;
    unsigned long priority;
    if (scheduler_type == FIFO) {
        // forward the packet at the beginning of the queue
        popped_packet = PacketQueue::popPacket(gate);
        priority = extract_priority(popped_packet, false);
        auto priority_found = sorted_queued_packet_hash_table.find(priority);
        if (priority_found == sorted_queued_packet_hash_table.end() ||
                priority_found->second.size() == 0)
            throw cRuntimeError("PopPacket: Priority doesn't exist or its list is empty!");
        if (popped_packet != priority_found->second.front())
            throw cRuntimeError("popped_packet != priority_found->second.front()");
        priority_found->second.pop_front(); // the packet that is received last, it stored last in the list of each priority
        if (priority_found->second.size() == 0) {
            sorted_queued_packet_hash_table.erase(priority_found->first);
        }
    } else {
        // forward the packet with highest priority
        EV << "Forward the packet with highest priority" << endl;
        std::map<unsigned long, std::list<Packet*>>::iterator it =
                sorted_queued_packet_hash_table.begin();
        priority = it->first;
        popped_packet = check_and_cast<Packet *>(queue.remove(it->second.front()));
        if (popped_packet != it->second.front())
            throw cRuntimeError("popped_packet != it->second.front()");
        it->second.pop_front();
        if (it->second.size() == 0) {
            sorted_queued_packet_hash_table.erase(it->first);
        }
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

    if (using_buffer)
        buffer->removePacket(popped_packet);
    return popped_packet;
}

void V2PIFO::removePacket(Packet *packet)
{
    Enter_Method("removePacket");
    EV << "removePacket is called in V2PIFO" << endl;
    unsigned long priority = extract_priority(packet, false);
    auto priority_found = sorted_queued_packet_hash_table.find(priority);
    if (priority_found == sorted_queued_packet_hash_table.end() ||
            priority_found->second.size() == 0)
        throw cRuntimeError("The packet that is supposed to be removed doesn't exist!");
    priority_found->second.remove(packet);
    if (priority_found->second.size() == 0) {
        sorted_queued_packet_hash_table.erase(priority_found->first);
    }
    if (using_buffer)
        buffer->removePacket(packet);
    PacketQueue::removePacket(packet);
    emit(packetRemovedSignal, packet);
    if (packetCapacity != -1)
        cSimpleModule::emit(customQueueLengthSignal, getNumPackets());
    else
        cSimpleModule::emit(customQueueLengthSignalPacketBytes, getTotalLength().get());

    // TODO temp: check the correctness
//    check_correctness();
}

long V2PIFO::get_queue_occupancy(long on_the_way_packet_num, b on_the_way_packet_length)
{
    EV << "V2PIFO::get_queue_occupancy" << endl;
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
    if (using_buffer) {
        if (buffer->getMaxNumPackets() != -1)
            return (getNumPackets() + on_the_way_packet_num);
        if (buffer->getMaxTotalLength() != b(-1))
            return (getTotalLength() + on_the_way_packet_length).get();
    }
    throw cRuntimeError("No queue/buffer capacity specified!");
}

bool V2PIFO::is_queue_full(b packet_length, long on_the_way_packet_num, b on_the_way_packet_length) {
    EV << "Checking if queue is full" << endl;
    if (using_buffer) {
        return buffer->is_queue_full(packet_length, on_the_way_packet_num, on_the_way_packet_length, getNumPackets(), getTotalLength());
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
