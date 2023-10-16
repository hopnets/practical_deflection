//
// This program is property of its copyright holder. All rights reserved.
// 

import inet.common.INETDefs;
import inet.common.TagBase;

namespace inet;

class InterfaceTagBase extends TagBase
{
    int interfaceId = -1; // a locally unique (network node) identifier that determines the interface registered in the ~InterfaceTable  
}

class PacketInsertionControlTag extends TagBase
{
    bool insert_packet_without_checking_the_queue; // if true, packet is cleared to be inserted right away
}

//
// Determines the outgoing interface where the packet should leave the network node.
//
class InterfaceReq extends InterfaceTagBase
{
}

//
// Specifies the incoming interface where the packet has arrived at the network node.
//
class InterfaceInd extends InterfaceTagBase
{
}
