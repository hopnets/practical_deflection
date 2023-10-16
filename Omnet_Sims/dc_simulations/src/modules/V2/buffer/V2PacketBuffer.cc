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

#include "./V2PacketBuffer.h"
#include <bits/stdc++.h>

using namespace inet;
using namespace queueing;

Define_Module(V2PacketBuffer);

/*
 * Definition of general functions
 */

void V2PacketBuffer::initialize(int stage)
{
    PacketBuffer::initialize(stage);
    dt_alpha = par("dt_alpha");
}

void V2PacketBuffer::addPacket(Packet *packet)
{
    Enter_Method("addPacket");
    EV_INFO << "Adding packet " << packet->getName() << " to the buffer.\n";
    emit(packetAddedSignal, packet);
    totalLength += packet->getTotalLength();
    packets.push_back(packet);
    updateDisplayString();
}

void V2PacketBuffer::removePacket(Packet *packet)
{
    Enter_Method("removePacket");
    EV_INFO << "Removing packet " << packet->getName() << " from the buffer.\n";
    EV << "Buffers length before removing: " << getTotalLength() << endl;
    emit(packetRemovedSignal, packet);
    totalLength -= packet->getTotalLength();
    auto removed_packet = find(packets.begin(), packets.end(), packet);
    if (removed_packet == packets.end())
        throw cRuntimeError("How is this possible? The packet does not exist in the buffer!");
    packets.erase(removed_packet);
    updateDisplayString();
    EV << "Buffers length after removing: " << getTotalLength() << endl;
}

bool V2PacketBuffer::findPacket(Packet *packet)
{
    EV << "V2PacketBuffer::findPacket" << endl;
    auto it = std::find(packets.begin(), packets.end(), packet);
    return (it != packets.end());
}
bool V2PacketBuffer::is_queue_full_DT(b packet_length, long on_the_way_packet_num, b on_the_way_packet_length, int qlen_pkt_num, b qlen_bytes) {
    EV << "V2PacketBuffer::is_queue_full_DT" << endl;

    int UB = get_UB(); // The part of buffer that is yet not occupied
    if (getMaxNumPackets() != -1) {
        EV << "Queue occupancy is " << qlen_pkt_num + on_the_way_packet_num << endl;
        EV << "UB * alpha is " << UB * dt_alpha << endl;
        // we should also consider on the way packets
        if (qlen_pkt_num + on_the_way_packet_num + 1 >= (UB - this->on_the_way_packet_num - 1) * dt_alpha) {
            EV << "queue is full" << endl;
            return true;
        } else {
            EV << "queue is not full" << endl;
            return false;
        }
    }
    if (getMaxTotalLength() != b(-1)) {
        EV << "Queue occupancy is " << (qlen_bytes + on_the_way_packet_length + packet_length).get() << endl;
        EV << "UB * alpha is " << UB * dt_alpha << endl;
        // we should also consider on the way packets
        if ((qlen_bytes + on_the_way_packet_length + packet_length).get() >= (UB - this->on_the_way_packet_length.get() - packet_length.get()) * dt_alpha) {
            EV << "queue is full" << endl;
            return true;
        } else {
            EV << "queue is not full" << endl;
            return false;
        }
    }
    throw cRuntimeError("Buffer has no capacity limit!");
}

bool V2PacketBuffer::is_queue_full(b packet_length, long on_the_way_packet_num, b on_the_way_packet_length, int qlen_pkt_num, b qlen_bytes) {
    if (is_buffer_full(packet_length))
        return true;

    if (qlen_pkt_num == -1 && qlen_bytes == b(-1))
        throw cRuntimeError("V2PacketBuffer::is_queue_full --> no qlen given!");
    return is_queue_full_DT(packet_length, on_the_way_packet_num, on_the_way_packet_length, qlen_pkt_num, qlen_bytes);
}

bool V2PacketBuffer::is_buffer_full(b packet_length)
{
    EV << "V2PacketBuffer::is_buffer_full" << endl;
    if (getMaxNumPackets() != -1)
        return (getNumPackets() + on_the_way_packet_num) >= getMaxNumPackets();
    else if (getMaxTotalLength() != b(-1))
        return (getTotalLength() + on_the_way_packet_length + packet_length) >= getMaxTotalLength();
    throw cRuntimeError("is_buffer_full --> The buffer has no limit!");
}

// get the unallocated part of the buffer
int V2PacketBuffer::get_UB()
{
    EV << "V2PacketBuffer::get_UB" << endl;
    int ub = -1;
    if (getMaxNumPackets() != -1) {
        int max_num_packets = getMaxNumPackets();
        int num_packets = getNumPackets();
        ub = max_num_packets - num_packets;
        EV << "max_num_packets: " << max_num_packets <<
                ", num_packets: " << num_packets <<
                ", on_the_way_packet_num: " << on_the_way_packet_num << endl;
    }
    if (getMaxTotalLength() != b(-1)) {
        b max_total_length = getMaxTotalLength();
        b total_length = getTotalLength();
        ub = (max_total_length - total_length).get();
        EV << "max_total_length: " << max_total_length <<
                ", total_length: " << total_length <<
                ", on_the_way_packet_length: " << on_the_way_packet_length
                << endl;
    }
    if (ub < 0)
        throw cRuntimeError("UB < 0");
    EV << "UB: " << ub << endl;
    return ub;
}

bool V2PacketBuffer::isOverloadedDT(int qlen_pkt_num, b qlen_bytes) {
    EV << "V2PacketBuffer::isOverloadedDT called!" << endl;
    int UB = get_UB(); // The part of buffer that is yet not occupied
    if (getMaxNumPackets() != -1) {
        EV << "qlen_pkt_num is " << qlen_pkt_num << endl;
        if (qlen_pkt_num >= UB * dt_alpha) {
            return true;
        } else {
            return false;
        }
    }
    if (getMaxTotalLength() != b(-1)) {
        EV << "qlen bits is " << qlen_bytes.get() << endl;
        if (qlen_bytes.get() >= UB * dt_alpha) {
            return true;
        } else {
            return false;
        }
    }
    throw cRuntimeError("V2PacketBuffer::isOverloadedDT --> We should not reach this point!");

}

bool V2PacketBuffer::isOverloaded(int qlen_pkt_num, b qlen_bytes)
{
    // Is buffer overloaded?
    EV << "V2PacketBuffer::isOverloaded" << endl;
    if (PacketBuffer::isOverloaded())
        return true;

    if (qlen_pkt_num == -1 && qlen_bytes == b(-1))
        throw cRuntimeError("V2PacketBuffer::isOverloaded --> The queue occupancy information is not transfered!");
    return isOverloadedDT(qlen_pkt_num, qlen_bytes);
}


