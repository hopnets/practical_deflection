//
// Copyright (C) 2012 Opensim Ltd.
// Author: Tamas Borbely
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

package dc_simulations.modules.PABO;

import inet.queueing.contract.IPacketClassifier;
import inet.queueing.contract.IPacketQueue;
import inet.queueing.contract.IPacketScheduler;
import inet.queueing.queue.CompoundPacketQueue;
import inet.queueing.queue.DropTailQueue;

//
// Queue module that gives the PAUSE frames a higher priority.
//
module PABOEtherQueue extends DropTailQueue
{
    parameters:
        comparatorClass = default("inet::EthernetFrameTypeComparator");
}

//
// Queue module that gives the PAUSE frames a higher priority,
// and can be parametrized with an ~IPacketQueue for serving the
// data frames.
//
module PABOEtherQosQueue extends CompoundPacketQueue like IPacketQueue
{
    parameters:
        @display("i=block/queue");
    submodules:
        classifier: PABOEtherFrameClassifier {
            parameters:
                @display("p=100,200");
        }
        //bounceBackQueue: storing bounced back frames
        bounceBackQueue: <default("DropTailQueue")> like IPacketQueue {
            @display("p=187,145");
        }
        dataQueue: <default("DropTailQueue")> like IPacketQueue {
            parameters:
                @display("p=250,300;q=l2queue");
        }
        scheduler: <default("PABOPriorityScheduler")> like IPacketScheduler {
            parameters:
                @display("p=400,200");
        }
    connections:
        in --> classifier.in;
        // In the current version of scheduler, bounceBackQeueu would have higher priority than dataQueue because it is added first.
        classifier.out++ --> bounceBackQueue.in; // out[0] --> bouncebackqueue: These actually indicate the priorities in the current scheduler
        classifier.out++ --> dataQueue.in; //out[1] --> dataqueue
        bounceBackQueue.out --> scheduler.in++;
        dataQueue.out --> scheduler.in++;
        scheduler.out --> out;
}

