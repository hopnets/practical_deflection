#ifndef _HEADERS_
#define _HEADERS_

typedef bit<8> ip_protocol_t;
const ip_protocol_t IP_PROTOCOLS_ICMP = 1;
const ip_protocol_t IP_PROTOCOLS_IPV4 = 4;
const ip_protocol_t IP_PROTOCOLS_TCP = 6;
const ip_protocol_t IP_PROTOCOLS_UDP = 17;
const ip_protocol_t IP_PROTOCOLS_IPV6 = 41;
const ip_protocol_t IP_PROTOCOLS_SRV6 = 43;
const ip_protocol_t IP_PROTOCOLS_NONXT = 59;

struct port_metadata_t {
    bit<3> port_pcp;
    bit<12> port_vid;
    bit<9> l2_xid;
}

header ethernet_h {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header ipv4_h {
    bit<4> version;
    bit<4> ihl;
    bit<8> diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<3> flags;
    bit<13> fragOffset;
    bit<8> ttl;
    bit<8> protocol;
    bit<16> hdrChecksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
}

header tcp_h {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<32> seqNo;
    bit<32> ackNo;
    bit<4> dataOffset;
    bit<3> res;
    bit<3> ecn;
    bit<6> ctrl;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgent_ptr;
}

header udp_h {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<16> pkt_length;
    bit<16> checksum;
}

header bee_h {
    bit<32> port_idx_in_reg;
    bit<8> queue_occ_info;
}

struct metadata_t {

    bit <5> random_number;

    bit<32> queue_length;
    bit<32> queue_length2;
    bit<16> fw_port_idx; 

    port_metadata_t port_properties;

    bit<32> output_port_idx;

    // bit<8> is_queue_full;
    bit<8> is_queue_full_0;
    bit<8> is_queue_full_1;
    bit<8> is_queue_full_2;
    bit<8> is_queue_full_3;
    bit<8> is_queue_full_4;
    bit<8> is_queue_full_5;
    bit<8> is_queue_full_6;
    bit<8> is_queue_full_7;

    // bit<32> temp_queue_occ_info;
    bit<16> port_idx_in_reg;
    bit<1> is_fw_port_full;

}


struct header_t {
    ethernet_h ethernet;
    ipv4_h ipv4;
    tcp_h tcp;
    udp_h udp;
    bee_h bee;
}

// struct empty_header_t {}

// struct pair32 {
//     bit<32>     low;
//     bit<32>     high;
// }

struct pair16 {
    bit<16>     low;
    bit<16>     high;
}


#endif /* _HEADERS_ */