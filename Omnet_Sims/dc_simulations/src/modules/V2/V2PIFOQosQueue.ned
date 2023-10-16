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

package dc_simulations.modules.V2;

import inet.queueing.contract.IPacketClassifier;
import inet.queueing.contract.IPacketQueue;
import inet.queueing.contract.IPacketScheduler;
import inet.queueing.queue.CompoundPacketQueue;
import inet.queueing.queue.DropTailQueue;
import dc_simulations.modules.V2.V2PIFO;


module V2PIFOQosQueue extends CompoundPacketQueue like IPacketQueue
{
    parameters:
        @display("i=block/queue");
    submodules:
        classifier: V2PIFOClassifier {
            parameters:
                @display("p=100,200");
        }
        //The main queue that packets are forwarded to
        mainQueue: V2PIFO {
            @display("p=187,145");
        }
        // The queue that the header of dropped packets are forwarded to
        droppedPacketsQueue: DropTailQueue {
            parameters:
                @display("p=250,300;q=l2queue");
        }
        scheduler: V2PIFOPriorityScheduler {
            parameters:
                @display("p=400,200");
        }
    connections:
        in --> classifier.in;
        classifier.out++ --> droppedPacketsQueue.in; // out[0] --> droppedPacketsQueue
        droppedPacketsQueue.out --> scheduler.in++;
        classifier.out++ --> mainQueue.in; //out[1] --> mainQueue
        mainQueue.out --> scheduler.in++;
        scheduler.out --> out;
}

