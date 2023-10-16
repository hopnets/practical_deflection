from scapy.all import *
# from zmq import IPV6

BEE_PORT = 9999

class BEE(Packet):
    name = "bee"
    fields_desc = [
        BitField("port_idx_in_reg", size=32, default=0),
        BitField("queue_occ_info", size=8, default=0)
    ]

class DIBS(Packet):
    name = "dibs"
    fields_desc = [
        BitField("port_idx_in_reg", size=32, default=0),
        BitField("queue_occ_info", size=8, default=0)
    ]


PORT_NUM = 8
for i in range(PORT_NUM):
    p_dibs = Ether()/IP()/UDP(dport=BEE_PORT)/DIBS(port_idx_in_reg=i, queue_occ_info=0)/"A"
    hexdump(p_dibs)
    p_dibs.show()
    print('--------------------------------')
    sendp(p_dibs, iface="enp5s0f1")