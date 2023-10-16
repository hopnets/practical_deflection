#include "includes/sd_defines.p4"
#include "includes/sd_headers.p4"

// typedef bit<INGRESS_CTR_WIDTH> bypass_ctr_index_t;
// typedef bit<INGRESS_CTR_WIDTH> bcast_ctr_index_t;
// typedef bit<INGRESS_CTR_WIDTH> bypass_fwd_ctr_index_t;


control Forward(inout header_t hdr, 
                inout ingress_intrinsic_metadata_for_tm_t ig_intr_tm_md, 
                inout ingress_intrinsic_metadata_for_deparser_t ig_intr_dprsr_md,
                inout metadata_t ig_md) {


    action drop() { 
        ig_intr_dprsr_md.drop_ctl = 1; 
    }

    action get_fw_port_idx_action(PortId_t idx, bit<16> fw_port_idx) {
        ig_intr_tm_md.ucast_egress_port = idx;
        ig_md.fw_port_idx = fw_port_idx;
    }

    table get_fw_port_idx_table {
        key = {
            hdr.ipv4.dstAddr : exact;
        }
        actions = {
            get_fw_port_idx_action;
            drop;
        }
        const default_action = drop;
        size = TABLE_SIZE;
    }

    action fw_l2_action(PortId_t idx) {
        ig_intr_tm_md.ucast_egress_port = idx;
    }

    action broadcast() {
        ig_intr_tm_md.mcast_grp_a = 1;
        ig_intr_tm_md.rid = 0xFFFF;
        ig_intr_tm_md.level2_exclusion_id = ig_md.port_properties.l2_xid;
        ig_intr_tm_md.bypass_egress = 1w1;
    }

    table fw_l2_table {
        key = {
            hdr.ethernet.dstAddr : exact;
        }
        actions = {
            fw_l2_action;
            broadcast;
            drop;
        }
        size = TABLE_SIZE;
        default_action = broadcast();
    }

    apply {
        hdr.ipv4.ttl = hdr.ipv4.ttl - 1;

        if (!hdr.ipv4.isValid()) {
            fw_l2_table.apply();
        } else {
            get_fw_port_idx_table.apply();
        }
    }
}

// worker packets

control BeeRecirculate(inout header_t hdr, 
                        inout ingress_intrinsic_metadata_for_tm_t ig_intr_tm_md, 
                        in ingress_intrinsic_metadata_t ig_intr_md) {

    action bee_recirculate_action() {
        ig_intr_tm_md.ucast_egress_port[8:7] = ig_intr_md.ingress_port[8:7];
        ig_intr_tm_md.ucast_egress_port[6:0] = 68;
    }

    table bee_recirculate_table {
        actions = {
            bee_recirculate_action;
        }
        const default_action = bee_recirculate_action;
        size = TABLE_SIZE;
    }

    apply {
        bee_recirculate_table.apply();
    }
}
