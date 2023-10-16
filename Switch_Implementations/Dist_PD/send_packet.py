from scapy.all import *
#from zmq import IPV6

AIFO_PORT = 9000
BEE_PORT = 9999

class BEE(Packet):
    name = "bee"
    fields_desc = [
        BitField("ucast_egress_port", size=32, default=0),
        BitField("qid", size=32, default=0),
        BitField("queue_length", size=32, default=0),
        BitField("M", size=32, default=0),
    ]

class CANARYD(Packet):
    name = "bee"
    fields_desc = [
        BitField("ucast_egress_port", size=32, default=0),
        BitField("qid", size=32, default=0),
        BitField("queue_length", size=32, default=0),
        BitField("M", size=32, default=0),
    ]


#FNS1
PORTS_1 = [128, 144, 160, 176, 140]
FNS1_INTERFACE_1 = "enp5s0f1"
#FNS2
PORTS_2 = [136, 180]
FNS1_INTERFACE_2 = "ens1f0"
for i in PORTS_1:
    p_canaryp = Ether()/IP()/UDP(dport=BEE_PORT)/BEE(ucast_egress_port=i, qid=0, queue_length=0, M=0)/"A"
    hexdump(p_canaryp)
    p_canaryp.show()
    print('--------------------------------')
    sendp(p_canaryp, iface=FNS1_INTERFACE_1)
