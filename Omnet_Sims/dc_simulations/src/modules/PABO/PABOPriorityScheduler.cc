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

#include "./PABOPriorityScheduler.h"

using namespace inet;
using namespace queueing;

Define_Module(PABOPriorityScheduler);

void PABOPriorityScheduler::initialize(int stage)
{
    PacketSchedulerBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL)
        for (auto provider : providers)
            collections.push_back(dynamic_cast<IPacketCollection *>(provider));
}

int PABOPriorityScheduler::getNumPackets()
{
    int size = 0;
    for (auto collection : collections)
        size += collection->getNumPackets();
    return size;
}

b PABOPriorityScheduler::getTotalLength()
{
    b totalLength(0);
    for (auto collection : collections)
        totalLength += collection->getTotalLength();
    return totalLength;
}

Packet *PABOPriorityScheduler::getPacket(int index)
{
    EV << "PABOPriorityScheduler::getPacket called." << endl;
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

void PABOPriorityScheduler::removePacket(Packet *packet)
{
    EV << "PABOPriorityScheduler::removePacket called." << endl;
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

int PABOPriorityScheduler::schedulePacket()
{
    //SEPEHR: The input gates are exactly in the order you provided in QosQueue. So in this case if you have put the bounceback
    // queue first, there is nothing left to do because automatically it would first try to read from bounceback queue.
    for (int i = 0; i < (int)providers.size(); i++) {
        if (providers[i]->canPopSomePacket(inputGates[i]->getPathStartGate())) {
            EV << "Found a packet to pop from provider" << i << endl;
            return i;
        }
    }
    return -1;
}


