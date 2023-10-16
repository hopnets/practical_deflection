#include "includes/distpd_defines.p4"
#include "includes/distpd_headers.p4"

typedef bit<INGRESS_CTR_WIDTH> bypass_ctr_index_t;
typedef bit<INGRESS_CTR_WIDTH> bcast_ctr_index_t;
typedef bit<INGRESS_CTR_WIDTH> bypass_fwd_ctr_index_t;

enum bit<12> master_mode_t {
    PD = 0X0,
    SWITCH = 0x1
}


control Routing(inout header_t hdr, 
                inout ingress_intrinsic_metadata_for_tm_t ig_intr_tm_md, 
                inout ingress_intrinsic_metadata_for_deparser_t ig_intr_dprsr_md,
                inout metadata_t ig_md) {

    action get_fw_idx_port_action(PortId_t idx) {
        ig_intr_tm_md.ucast_egress_port = idx;
    }

    action drop_pkt() { 
        ig_intr_dprsr_md.drop_ctl = 1; 
    }

    table get_fw_idx_port_table {
        key = {
            hdr.ipv4.dstAddr : exact;
        }
        actions = {
            // resubmit_action;
            get_fw_idx_port_action;
            drop_pkt;
        }
        const default_action = drop_pkt;
        size = TABLE_SIZE;
    }

    apply {
        hdr.ipv4.ttl = hdr.ipv4.ttl - 1;
        get_fw_idx_port_table.apply();
    }
}

control DeflectRouting(inout header_t hdr,  
                inout metadata_t ig_md,
                inout ingress_intrinsic_metadata_for_deparser_t ig_intr_dprsr_md,
                inout ingress_intrinsic_metadata_for_tm_t ig_intr_tm_md) {

    action deflect_get_fw_idx_port_action(PortId_t idx) {
        ig_md.deflect_ucast_egress_port = idx;
    }

    table deflect_get_fw_idx_port_table {
        key = {
            hdr.ipv4.dstAddr : exact;
        }
        actions = {
            deflect_get_fw_idx_port_action;
        }
        size = TABLE_SIZE;
    }

    apply {
        deflect_get_fw_idx_port_table.apply();
    }
}



control DefaultRouting(inout header_t hdr,  
                inout metadata_t ig_md,
                inout ingress_intrinsic_metadata_for_deparser_t ig_intr_dprsr_md,
                inout ingress_intrinsic_metadata_for_tm_t ig_intr_tm_md) {

    bypass_ctr_index_t bp_ctr_index = 0;
    bcast_ctr_index_t bcast_counter_idx = 0;
    bypass_fwd_ctr_index_t bp_fwd_ctr_index_d = 1;
    Counter<bit<CTR_WIDTH>, bypass_ctr_index_t>(INGRESS_CTR_SIZE, CounterType_t.PACKETS_AND_BYTES) bypass_ctr;
    Counter<bit<CTR_WIDTH>, bypass_fwd_ctr_index_t>(INGRESS_CTR_SIZE, CounterType_t.PACKETS_AND_BYTES) bypass_fwd_ctr;
    Counter<bit<CTR_WIDTH>, bcast_ctr_index_t>(INGRESS_CTR_SIZE, CounterType_t.PACKETS_AND_BYTES) bcast_counter;

    action drop() { 
        ig_intr_dprsr_md.drop_ctl = 1;
        // bypass_ctr.count(bp_ctr_index);
    }

    action default_fw_action(PortId_t idx) {
        ig_intr_tm_md.ucast_egress_port = idx;
        bypass_fwd_ctr.count(bp_fwd_ctr_index_d);
    }


    table default_fw_table {
        key = {
            hdr.ipv4.dstAddr : exact;
        }
        actions = {
            default_fw_action;
            drop;
        }
        size = TABLE_SIZE;
    }

    action default_fw_l2_action(PortId_t idx) {
        ig_intr_tm_md.ucast_egress_port = idx;
    }

    action broadcast() {
        ig_intr_tm_md.mcast_grp_a = 1;
        ig_intr_tm_md.rid = 0xFFFF;
        ig_intr_tm_md.level2_exclusion_id = ig_md.port_properties.l2_xid;
        ig_intr_tm_md.bypass_egress = 1w1;
    }


    table default_fw_l2_table {
        key = {
            hdr.ethernet.dstAddr : exact;
        }
        actions = {
            default_fw_l2_action;
            broadcast;
            drop;
        }
        size = TABLE_SIZE;
        default_action = broadcast();
    }

    apply {
        bypass_ctr.count(bp_ctr_index);
        // hdr.ipv4.ttl = hdr.ipv4.ttl - 1;

        if (!hdr.ipv4.isValid()) {
            default_fw_l2_table.apply();
        } else {
            default_fw_table.apply();
        }
        // } else if (default_fw_table.apply().miss) {
        //     default_fw_l2_table.apply();
        // }
    }
}


control GetMinRelPrioQueueLen (inout metadata_t ig_md) {

    action get_min_rel_prio_queue_len_action() {
        // // min(meta.min_value, meta.count_all, meta.queue_length);
        // if (ig_md.rel_prio < ig_md.queue_length) {
        //     ig_md.min_value_rel_prio_queue_len = ig_md.rel_prio;
        // } else {
        //     ig_md.min_value_rel_prio_queue_len = ig_md.queue_length;
        // }
        ig_md.min_value_rel_prio_queue_len = min(ig_md.rel_prio, ig_md.queue_length);
    }

    table get_min_rel_prio_queue_len_table {
        actions = {
            get_min_rel_prio_queue_len_action;
        }
        const default_action = get_min_rel_prio_queue_len_action;
        size = TABLE_SIZE;
    }

    apply {
        get_min_rel_prio_queue_len_table.apply();
    }

}

control DeflectGetMinRelPrioQueueLen (inout metadata_t ig_md) {

    action deflect_get_min_rel_prio_queue_len_action() {
        // if (ig_md.count_all < ig_md.deflect_queue_length) {
        //     ig_md.deflect_min_value_rel_prio_queue_len = ig_md.deflect_rel_prio;
        // } else {
        //     ig_md.deflect_min_value_rel_prio_queue_len = ig_md.deflect_queue_length;
        // }
        ig_md.deflect_min_value_rel_prio_queue_len = min(ig_md.rel_prio, ig_md.deflect_queue_length);
    }

    table deflect_get_min_rel_prio_queue_len_table {
        actions = {
            deflect_get_min_rel_prio_queue_len_action;
        }
        const default_action = deflect_get_min_rel_prio_queue_len_action;
        size = TABLE_SIZE;
    }

    apply {
        deflect_get_min_rel_prio_queue_len_table.apply();
    }

}

// worker packets

/*
    * The source and destination of worker packets actually does not matter
    * what matters is that based on hdr.bee.ucast_egress_port and hdr.bee.qid, 
    * it reads some information from the register array at egress and writes
    * in the register array at ingress port based on the same information
    * hdr.bee.ucast_egress_port and hdr.bee.qid MUST remain unchanged.
    * Note that the packets that write into the register array at egress are not
    * worker packets. They are packets.
*/
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
