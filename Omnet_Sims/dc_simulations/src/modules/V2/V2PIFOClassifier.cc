//
// Copyright (C) 2011 Zoltan Bojthe
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
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#include "inet/linklayer/ethernet/EtherMac.h"
#include "./V2PIFOClassifier.h"

using namespace inet;

Define_Module(V2PIFOClassifier);

int V2PIFOClassifier::classifyPacket(Packet *packet)
{
    EV << "Packet is being classified: " << packet << endl;
    auto frame = packet->peekAtFront<EthernetMacHeader>();
    if(frame->getIs_v2_dropped_packet_header()) {
        // Send to BounceBackQueue
        EV << "packet is v2 dropped packet header" << endl;
        return DROPPED_PACKET_HEADER;
    }
    else {
        // Send to DataQeueu
        EV << "packet is a normal packet" << endl;
        return NORMAL_PACKET;
    }
}

