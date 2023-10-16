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

#ifndef __INET_V2PACKETBUFFER_H
#define __INET_V2PACKETBUFFER_H

#include "inet/queueing/buffer/PacketBuffer.h"

using namespace inet;
using namespace queueing;

class INET_API V2PacketBuffer : public PacketBuffer
{
    protected:
        virtual void initialize(int stage) override;

    public:

        long on_the_way_packet_num = 0;
        b on_the_way_packet_length = b(0);
        double dt_alpha;

        virtual void addPacket(Packet *packet) override;
        virtual void removePacket(Packet *packet) override;
        virtual bool isOverloadedDT(int qlen_pkt_num, b qlen_bytes);
        virtual bool isOverloaded(int qlen_pkt_num = -1, b qlen_bytes = b(-1));
        virtual bool findPacket(Packet *packet);
        virtual bool is_buffer_full(b packet_length);
        virtual bool is_queue_full_DT(b packet_length, long on_the_way_packet_num = 0, b on_the_way_packet_length = b(0), int qlen_pkt_num = -1, b qlen_bytes = b(-1));
        virtual bool is_queue_full(b packet_length, long on_the_way_packet_num = 0, b on_the_way_packet_length = b(0), int qlen_pkt_num = -1, b qlen_bytes = b(-1));
        virtual int get_UB();
};

#endif // ifndef __V2INET_PACKETBUFFER_H

