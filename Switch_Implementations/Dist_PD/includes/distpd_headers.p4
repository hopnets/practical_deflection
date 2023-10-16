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
    bit<32> M;
}

@flexible
header distpd_bridged_metadata_h {
    bit<32> rank;
    bit<32> M;
}

struct metadata_t {

    bit<32> rel_prio;
    bit<32> deflect_rel_prio;

    /////////////////////////////
    bit<32> ucast_egress_port;
    bit<32> qid;

    // bit<16> order;
    bit<32> queue_length;
    bit<32> m;
    bit<32> min_value_rel_prio_queue_len;

    // TODO: We don't need deflect queue id
    bit<32> deflect_queue_length;
    bit<32> deflect_m;
    bit<32> deflect_min_value_rel_prio_queue_len;
        // for egress
    bit<32> new_m;

    // bit<32> rank;

    // bit<1> checksum_err;

    PortId_t deflect_ucast_egress_port;

    // PortId_t rand_port_1_idx; 
    // PortId_t deflect_rand_port_1_idx;

    port_metadata_t port_properties;


}


struct header_t {
    distpd_bridged_metadata_h bridged_md;
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