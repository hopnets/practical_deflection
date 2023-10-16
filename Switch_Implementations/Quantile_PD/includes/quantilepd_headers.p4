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
    bit<32> ucast_egress_port;
    bit<32> qid;
    bit<32> queue_length;
}

struct metadata_t {

    bit<8> prefix_id;
    bit<8> deflect_prefix_id;

    /////////////////////////////
    bit<32> ucast_egress_port;
    bit<32> qid;

    bit<16> order;
    bit<16> tail;
    bit<16> queue_length;
    bit<16> min_value;


    bit<16> count_all;

    // TODO: We don't need deflect queue id
    bit<16> deflect_queue_length;
    bit<16> deflect_min_value;

    bit<32> rank;

    // todo: should be bit 1;
    bit<1> count_0_0_let;
    bit<1> count_0_1_let;
    bit<1> count_0_2_let;
    bit<1> count_0_3_let;
    bit<1> count_1_0_let;
    bit<1> count_1_1_let;
    bit<1> count_1_2_let;
    bit<1> count_1_3_let;
    bit<1> count_2_0_let;
    bit<1> count_2_1_let;
    bit<1> count_2_2_let;
    bit<1> count_2_3_let;
    bit<1> count_3_0_let;
    bit<1> count_3_1_let;
    bit<1> count_3_2_let;
    bit<1> count_3_3_let;
    bit<1> count_4_0_let;
    bit<1> count_4_1_let;
    bit<1> count_4_2_let;
    bit<1> count_4_3_let;

    // bit<1> checksum_err;

    PortId_t deflect_ucast_egress_port;

    port_metadata_t port_properties;
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