#include "includes/quantilepd_defines.p4"
#include "includes/quantilepd_headers.p4"

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

    action drop_quantilepd() { 
        ig_intr_dprsr_md.drop_ctl = 1; 
    }

    table get_fw_idx_port_table {
        key = {
            hdr.ipv4.dstAddr : exact;
        }
        actions = {
            // resubmit_action;
            get_fw_idx_port_action;
            drop_quantilepd;
        }
        const default_action = drop_quantilepd;
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


// Get quantile (rank among the items in the window)
// None of the tables have any keys in GetQuantile
control GetQuantile(inout metadata_t ig_md) {

    #define REG_WIN(i,j) \
        Register<bit<32>, _>(1) window_##i##_##j##_register;

    #define BLACKBOX_CHECK_WINDOW(i,j) \
        RegisterAction<bit<32>, _, bit<1>>(window_##i##_##j##_register) check_win_##i##_##j##_alu = { \
                void apply(inout bit<32> register_data, out bit<1> result) { \
                    if (ig_md.rank < register_data) { \
                        result = 1; \
                    } else { \
                        result = 0; \
                    } \
                     \
                    if (ig_md.tail == (i*4 + j) * SAMPLE_COUNT) { \
                        register_data = ig_md.rank; \
                    } \
                } \
            };

    #define ACTION_CHECK_WINDOW(i,j) \
        action check_win_##i##_##j##_action() { \
            ig_md.count_##i##_##j##_let = check_win_##i##_##j##_alu.execute(0); \
        }

    #define TABLE_CHECK_WINDOW(i,j) \
        table check_win_##i##_##j##_table { \
            actions = { \
                check_win_##i##_##j##_action; \
            } \
            const default_action = check_win_##i##_##j##_action; \
            size = TABLE_SIZE; \
        }

    #define TABLE_SUM(i,j) \
        table sum_##i##_##j##_table { \
            actions = { \
                sum_##i##_##j##_action; \
            } \
            const default_action = sum_##i##_##j##_action; \
            size = TABLE_SIZE; \
        }

    #define ACTION_SUM(i,j) \
        action sum_##i##_##j##_action() { \
            ig_md.count_0_##j##_let = ig_md.count_0_##j##_let + ig_md.count_##i##_##j##_let; \
        }

    #define BLACKBOX_CHECK_WINDOW_34(i,j, k) \
        RegisterAction<bit<32>, _, bit<1>>(window_##i##_##j##_register) check_win_##i##_##j##_alu = { \
                void apply(inout bit<32> register_data, out bit<1> result) { \
                    if (ig_md.rank < register_data) { \
                        result = 1; \
                    } else { \
                        result = 0; \
                    } \
                     \
                    if (ig_md.tail == (i*4 + j) * SAMPLE_COUNT) { \
                        register_data = ig_md.rank; \
                    } \
                } \
            };

    action sum_1_0_action() {
        ig_md.count_0_0_let = ig_md.count_0_0_let + ig_md.count_1_0_let;
    }
    action sum_1_1_action() {
        ig_md.count_0_1_let = ig_md.count_0_1_let + ig_md.count_1_1_let;
    }
    action sum_1_2_action() {
        ig_md.count_0_2_let = ig_md.count_0_2_let + ig_md.count_1_2_let;
    }
    action sum_1_3_action() {
        ig_md.count_0_3_let = ig_md.count_0_3_let + ig_md.count_1_3_let;
    }

    action sum_2_0_action() {
        ig_md.count_0_0_let = ig_md.count_0_0_let + ig_md.count_2_0_let;
    }
    action sum_2_1_action() {
        ig_md.count_0_1_let = ig_md.count_0_1_let + ig_md.count_2_1_let;
    }
    action sum_2_2_action() {
        ig_md.count_0_2_let = ig_md.count_0_2_let + ig_md.count_2_2_let;
    }
    action sum_2_3_action() {
        ig_md.count_0_3_let = ig_md.count_0_3_let + ig_md.count_2_3_let;
    }

    action sum_3_0_action() {
        ig_md.count_0_0_let = ig_md.count_0_0_let + ig_md.count_1_0_let;
    }
    action sum_3_1_action() {
        ig_md.count_0_1_let = ig_md.count_0_1_let + ig_md.count_1_1_let;
    }
    action sum_3_2_action() {
        ig_md.count_0_2_let = ig_md.count_0_2_let + ig_md.count_1_2_let;
    }
    action sum_3_3_action() {
        ig_md.count_0_3_let = ig_md.count_0_3_let + ig_md.count_1_3_let;
    }

    action sum_4_0_action() {
        ig_md.count_0_0_let = ig_md.count_0_0_let + ig_md.count_2_0_let;
    }
    action sum_4_1_action() {
        ig_md.count_0_1_let = ig_md.count_0_1_let + ig_md.count_2_1_let;
    }
    action sum_4_2_action() {
        ig_md.count_0_2_let = ig_md.count_0_2_let + ig_md.count_2_2_let;
    }
    action sum_4_3_action() {
        ig_md.count_0_3_let = ig_md.count_0_3_let + ig_md.count_2_3_let;
    }

    REG_WIN(0,0)
    REG_WIN(0,1)
    REG_WIN(0,2)
    REG_WIN(0,3)
    REG_WIN(1,0)
    REG_WIN(1,1)
    REG_WIN(1,2)
    REG_WIN(1,3)
    REG_WIN(2,0)
    REG_WIN(2,1)
    REG_WIN(2,2)
    REG_WIN(2,3)
    REG_WIN(3,0)
    REG_WIN(3,1)
    REG_WIN(3,2)
    REG_WIN(3,3)
    REG_WIN(4,0)
    REG_WIN(4,1)
    REG_WIN(4,2)
    REG_WIN(4,3)

    BLACKBOX_CHECK_WINDOW(0,0)
    BLACKBOX_CHECK_WINDOW(0,1)
    BLACKBOX_CHECK_WINDOW(0,2)
    BLACKBOX_CHECK_WINDOW(0,3)
    BLACKBOX_CHECK_WINDOW(1,0)
    BLACKBOX_CHECK_WINDOW(1,1)
    BLACKBOX_CHECK_WINDOW(1,2)
    BLACKBOX_CHECK_WINDOW(1,3)
    BLACKBOX_CHECK_WINDOW(2,0)
    BLACKBOX_CHECK_WINDOW(2,1)
    BLACKBOX_CHECK_WINDOW(2,2)
    BLACKBOX_CHECK_WINDOW(2,3)
    // BLACKBOX_CHECK_WINDOW(3,0)
    // BLACKBOX_CHECK_WINDOW(3,1)
    // BLACKBOX_CHECK_WINDOW(3,2)
    // BLACKBOX_CHECK_WINDOW(3,3)
    // BLACKBOX_CHECK_WINDOW(4,0)
    // BLACKBOX_CHECK_WINDOW(4,1)
    // BLACKBOX_CHECK_WINDOW(4,2)
    // BLACKBOX_CHECK_WINDOW(4,3)
    BLACKBOX_CHECK_WINDOW_34(3,0,1)
    BLACKBOX_CHECK_WINDOW_34(3,1,1)
    BLACKBOX_CHECK_WINDOW_34(3,2,1)
    BLACKBOX_CHECK_WINDOW_34(3,3,1)
    BLACKBOX_CHECK_WINDOW_34(4,0,2)
    BLACKBOX_CHECK_WINDOW_34(4,1,2)
    BLACKBOX_CHECK_WINDOW_34(4,2,2)
    BLACKBOX_CHECK_WINDOW_34(4,3,2)

    ACTION_SUM(0,0)
    ACTION_SUM(0,1)
    ACTION_SUM(0,2)
    ACTION_SUM(0,3)
    // ACTION_SUM(1,0)
    // ACTION_SUM(1,1)
    // ACTION_SUM(1,2)
    // ACTION_SUM(1,3)
    // ACTION_SUM(2,0)
    // ACTION_SUM(2,1)
    // ACTION_SUM(2,2)
    // ACTION_SUM(2,3)
    // ACTION_SUM(3,0)
    // ACTION_SUM(3,1)
    // ACTION_SUM(3,2)
    // ACTION_SUM(3,3)
    // ACTION_SUM(4,0)
    // ACTION_SUM(4,1)
    // ACTION_SUM(4,2)
    // ACTION_SUM(4,3)

    ACTION_CHECK_WINDOW(0,0)
    ACTION_CHECK_WINDOW(0,1)
    ACTION_CHECK_WINDOW(0,2)
    ACTION_CHECK_WINDOW(0,3)
    ACTION_CHECK_WINDOW(1,0)
    ACTION_CHECK_WINDOW(1,1)
    ACTION_CHECK_WINDOW(1,2)
    ACTION_CHECK_WINDOW(1,3)
    ACTION_CHECK_WINDOW(2,0)
    ACTION_CHECK_WINDOW(2,1)
    ACTION_CHECK_WINDOW(2,2)
    ACTION_CHECK_WINDOW(2,3)
    ACTION_CHECK_WINDOW(3,0)
    ACTION_CHECK_WINDOW(3,1)
    ACTION_CHECK_WINDOW(3,2)
    ACTION_CHECK_WINDOW(3,3)
    ACTION_CHECK_WINDOW(4,0)
    ACTION_CHECK_WINDOW(4,1)
    ACTION_CHECK_WINDOW(4,2)
    ACTION_CHECK_WINDOW(4,3)

    TABLE_SUM(0,0)
    TABLE_SUM(0,1)
    TABLE_SUM(0,2)
    TABLE_SUM(0,3)
    TABLE_SUM(1,0)
    TABLE_SUM(1,1)
    TABLE_SUM(1,2)
    TABLE_SUM(1,3)
    TABLE_SUM(2,0)
    TABLE_SUM(2,1)
    TABLE_SUM(2,2)
    TABLE_SUM(2,3)
    TABLE_SUM(3,0)
    TABLE_SUM(3,1)
    TABLE_SUM(3,2)
    TABLE_SUM(3,3)
    TABLE_SUM(4,0)
    TABLE_SUM(4,1)
    TABLE_SUM(4,2)
    TABLE_SUM(4,3)

    TABLE_CHECK_WINDOW(0,0)
    TABLE_CHECK_WINDOW(0,1)
    TABLE_CHECK_WINDOW(0,2)
    TABLE_CHECK_WINDOW(0,3)
    TABLE_CHECK_WINDOW(1,0)
    TABLE_CHECK_WINDOW(1,1)
    TABLE_CHECK_WINDOW(1,2)
    TABLE_CHECK_WINDOW(1,3)
    TABLE_CHECK_WINDOW(2,0)
    TABLE_CHECK_WINDOW(2,1)
    TABLE_CHECK_WINDOW(2,2)
    TABLE_CHECK_WINDOW(2,3)
    TABLE_CHECK_WINDOW(3,0)
    TABLE_CHECK_WINDOW(3,1)
    TABLE_CHECK_WINDOW(3,2)
    TABLE_CHECK_WINDOW(3,3)
    TABLE_CHECK_WINDOW(4,0)
    TABLE_CHECK_WINDOW(4,1)
    TABLE_CHECK_WINDOW(4,2)
    TABLE_CHECK_WINDOW(4,3)

    action sum_0_1_2_3_action() {
        // add(meta.count_0_0_let, meta.count_0_0_let, meta.count_0_1_let);
        // add(meta.count_0_2_let, meta.count_0_2_let, meta.count_0_3_let);
        ig_md.count_0_0_let = ig_md.count_0_0_let + ig_md.count_0_1_let;
        ig_md.count_0_2_let = ig_md.count_0_2_let + ig_md.count_0_3_let;
    }

    table sum_0_1_2_3_table {
        actions = {
            sum_0_1_2_3_action;
        }
        const default_action = sum_0_1_2_3_action;
        size = TABLE_SIZE;
    }

    action count_mul_action() {
        // shift_left(ig_md.count_all, ig_md.count_all, 10);
        ig_md.count_all = ig_md.count_all << 10;
    }

    table count_mul_table {
        actions = {
            count_mul_action;
        }
        const default_action = count_mul_action;
        size = TABLE_SIZE;
    }

    apply{
        // get quantile and update the window
        // stage x
        check_win_0_0_table.apply();
        check_win_0_1_table.apply();
        check_win_0_2_table.apply();
        check_win_0_3_table.apply();


        // stage x+1
        check_win_1_0_table.apply();
        check_win_1_1_table.apply();
        check_win_1_2_table.apply();
        check_win_1_3_table.apply();


        // stage x+2
        check_win_2_0_table.apply();
        check_win_2_1_table.apply();
        check_win_2_2_table.apply();
        check_win_2_3_table.apply();
        sum_1_0_table.apply();
        sum_1_1_table.apply();
        sum_1_2_table.apply();
        sum_1_3_table.apply();

        // stage x+3
        check_win_3_0_table.apply();
        check_win_3_1_table.apply();
        check_win_3_2_table.apply();
        check_win_3_3_table.apply();
        sum_2_0_table.apply();
        sum_2_1_table.apply();
        sum_2_2_table.apply();
        sum_2_3_table.apply();

        // stage x+4
        check_win_4_0_table.apply();
        check_win_4_1_table.apply();
        check_win_4_2_table.apply();
        check_win_4_3_table.apply();
        sum_3_0_table.apply();
        sum_3_1_table.apply();
        sum_3_2_table.apply();
        sum_3_3_table.apply();

        sum_0_1_2_3_table.apply();

        // get the rank
        // sum_all_table.apply();
        if (ig_md.count_0_0_let == 0 && ig_md.count_0_2_let == 0) {
            ig_md.count_all = 0;
        } else if ((ig_md.count_0_0_let == 1 && ig_md.count_0_2_let == 0) || 
            (ig_md.count_0_0_let == 0 && ig_md.count_0_2_let == 1)) {
            ig_md.count_all = 1;
        } else {
            ig_md.count_all = 2;
        }

        // get the rank times 2^10 / 2^11, i.e., (1-K)*C*quantile
        count_mul_table.apply();
    }
}

control GetMin (inout metadata_t ig_md) {

    action get_min_action() {
        // min(meta.min_value, meta.count_all, meta.queue_length);
        if (ig_md.count_all < ig_md.queue_length) {
            ig_md.min_value = ig_md.count_all;
        } else {
            ig_md.min_value = ig_md.queue_length;
        }
    }

    table get_min_table {
        actions = {
            get_min_action;
        }
        const default_action = get_min_action;
        size = TABLE_SIZE;
    }

    apply {
        get_min_table.apply();
    }

}

control DeflectGetMin (inout metadata_t ig_md) {

    action deflect_get_min_action() {
        if (ig_md.count_all < ig_md.deflect_queue_length) {
            ig_md.deflect_min_value = ig_md.count_all;
        } else {
            ig_md.deflect_min_value = ig_md.deflect_queue_length;
        }
    }

    table deflect_get_min_table {
        actions = {
            deflect_get_min_action;
        }
        const default_action = deflect_get_min_action;
        size = TABLE_SIZE;
    }

    apply {
        deflect_get_min_table.apply();
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
    * worker packets.
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
