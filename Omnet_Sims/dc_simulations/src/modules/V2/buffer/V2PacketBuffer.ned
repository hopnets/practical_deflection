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


package dc_simulations.modules.V2.buffer;

import inet.queueing.buffer.PacketBuffer;

//
// This module provides packet storage for sharing and optimizing storage space
// between multiple packet queues. When a packet buffer becomes overloaded, the
// packet dropping strategy can drop any number of packets from any number of
// connected packet queues.
//
// @see ~IPacketQueue
//
simple V2PacketBuffer extends PacketBuffer
{
	parameters:
        @class(V2PacketBuffer);
        double dt_alpha = default(1);
        
        @signal[sharedMemQueueLenByte](type=long);
		@statistic[sharedMemQueueLengthByte](title="The shared mem length in bytes"; source="sharedMemQueueLenByte"; record=vector; interpolationmode=none);
}
