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
#include "./PABOEtherFrameClassifier.h"


using namespace inet;

Define_Module(PABOEtherFrameClassifier);

int PABOEtherFrameClassifier::classifyPacket(Packet *packet)
{
    //TODO find out what you should return
    EV << "Packet is being classified: " << packet << endl;
    auto frame = packet->peekAtFront<EthernetMacHeader>();
    if(frame->getBouncedHop()>0) {
        // Send to BounceBackQueue
        EV << "Sending the packet to bounceBackQueue" << endl;
        return 0;
    }
    else {
        // Send to DataQeueu
        EV << "Sending the packet to dataQueue" << endl;
        return 1;
    }
}

