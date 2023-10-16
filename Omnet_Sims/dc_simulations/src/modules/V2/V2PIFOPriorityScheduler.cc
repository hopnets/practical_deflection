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

#include "./V2PIFOPriorityScheduler.h"

using namespace inet;
using namespace queueing;

Define_Module(V2PIFOPriorityScheduler);

void V2PIFOPriorityScheduler::initialize(int stage)
{
    PacketSchedulerBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        for (auto provider : providers)
            collections.push_back(dynamic_cast<IPacketCollection *>(provider));

        bool found_main_queue = false;
        for (auto collection : collections) {
            std::string queue_module_full_path = collection->get_full_path();
            if (queue_module_full_path.find("mainQueue") != std::string::npos) {
                collection->queue_type = NORMAL_QUEUE;
                found_main_queue = true;
            } else {
                collection->queue_type = DROPPED_PACKET_HEADER_QUEUE;
            }
        }
        if (!found_main_queue)
            throw cRuntimeError("V2PIFOPriorityScheduler::initialize: No main queue found!");
    }
}

int V2PIFOPriorityScheduler::getNumPackets()
{
    int size = 0;
    for (auto collection : collections) {
        int num_packets = collection->getNumPackets();
        size += num_packets;
    }
    return size;
}

b V2PIFOPriorityScheduler::getTotalLength()
{
    b totalLength(0);
    for (auto collection : collections)
        totalLength += collection->getTotalLength();
    return totalLength;
}

Packet *V2PIFOPriorityScheduler::getPacket(int index)
{
    int origIndex = index;
    for (auto collection : collections) {
        auto numPackets = collection->getNumPackets();
        if (index < numPackets)
            return collection->getPacket(index);
        else
            index -= numPackets;
    }
    throw cRuntimeError("Index %i out of range", origIndex);
}

void V2PIFOPriorityScheduler::removePacket(Packet *packet)
{
    Enter_Method("removePacket");
    for (auto collection : collections) {
        int numPackets = collection->getNumPackets();
        for (int j = 0; j < numPackets; j++) {
            if (collection->getPacket(j) == packet) {
                collection->removePacket(packet);
                return;
            }
        }
    }
    throw cRuntimeError("Cannot find packet");
}

int V2PIFOPriorityScheduler::schedulePacket()
{
    EV << "V2PIFOPriorityScheduler::schedulePacket()" << endl;
    //SEPEHR: The input gates are exactly in the order you provided in QosQueue. So in this case if you have put the droppedPacketsQueue
    // queue first, there is nothing left to do because automatically it would first try to read from droppedPacketsQueue queue.
    for (int i = 0; i < (int)providers.size(); i++) {
        if (providers[i]->canPopSomePacket(inputGates[i]->getPathStartGate())) {
            EV << "Found a packet to pop from provider" << i << endl;
            return i;
        }
    }
    return -1;
}


