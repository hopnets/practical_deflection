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

package dc_simulations.modules.V2;

import inet.queueing.queue.PacketQueue;
import inet.queueing.contract.IPacketQueue;

// NOTE: Using V2 pifo is a little bit different than normal queues, 
// because in most cases of normal queues we drop the packet
// in relay unit but v2pifo drops them in queue 

// Instead of implementing pure scheduling, here we use a specific
// number of fifo queues for approximation

simple V2PIFOBoltQueue extends PacketQueue
{
	parameters:
        @class(V2PIFOBoltQueue);
        
        string relative_priority_calculation_type = default("");	// percentile, dist-based
        string relative_priority_distribution_type = default("");	// exponential
        
        // percentile-based relative prio calculation 
        int quantile_wind_size = default(-1);
        int aifo_sample_count = default(-1);
        
        // DETERMINISTIC
        double k = default(-1);
        
		int CCThresh @unit(b) = default(-1b);
		int mtu = default(-1);

        @signal[packetDropSeq]();
        @signal[packetDropRetCount]();
        @signal[packetDropTotalPayloadLength]();
        
        @statistic[droppktSeqs](title="seqs of the dropped packets"; source="packetDropSeq"; record=vector; interpolationmode=none);
        @statistic[droppktRetCount](title="ret count of the dropped packets"; source="packetDropRetCount"; record=vector; interpolationmode=none);
        @statistic[pktDropTotalPayloadLen](title="total payload length of BG packets dropped in v2 queue"; source="packetDropTotalPayloadLength"; record=vector; interpolationmode=none);
}
