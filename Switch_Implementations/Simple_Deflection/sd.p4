#include <core.p4>
#if __TARGET_TOFINO__ == 2
#include <t2na.p4>
#else
#include <tna.p4>
#endif

#include "includes/sd_defines.p4"
#include "includes/sd_headers.p4"
#include "includes/sd_parser.p4"
#include "sd_controls.p4"

typedef bit<INGRESS_CTR_WIDTH> ingress_ctr_index_t;
typedef bit<INGRESS_CTR_WIDTH> egress_ctr_index_t;
typedef bit<INGRESS_CTR_WIDTH> drop_ctr_index_t;
typedef bit<INGRESS_CTR_WIDTH> deflect_ctr_index_t;
typedef bit<INGRESS_CTR_WIDTH> bee_ctr_index_t;

/*
    Switch ingress pipeline
*/
control SwitchIngress(
        inout header_t hdr,
        inout metadata_t ig_md,
        in ingress_intrinsic_metadata_t ig_intr_md,
        in ingress_intrinsic_metadata_from_parser_t ig_intr_prsr_md,
        inout ingress_intrinsic_metadata_for_deparser_t ig_intr_dprsr_md,
        inout ingress_intrinsic_metadata_for_tm_t ig_intr_tm_md) {


            #define IG_QUEUE_OCC_INFO_REG(i) \
                Register<bit<8>, _>(1) ig_queue_occ_info_reg_##i##;

            #define IG_REG_AC_GET_QUEUE_OCC_INFO(i) \
                RegisterAction<bit<8>, _, bit<8>>(ig_queue_occ_info_reg_##i##) get_ig_queue_occ_info_reg_alu_##i## = { \
                    void apply(inout bit<8> register_data, out bit<8> result) { \
                        result = register_data; \
                    } \
                };

            #define IG_REG_AC_SET_QUEUE_OCC_INFO(i) \
                RegisterAction<bit<8>, _, void>(ig_queue_occ_info_reg_##i##) set_ig_queue_occ_info_reg_alu_##i## = { \
                    void apply(inout bit<8> register_data) { \
                        register_data = hdr.bee.queue_occ_info; \
                    } \
                };

            #define IG_ACTION_GET_QUEUE_OCC_INFO(i) \
                action get_ig_queue_occ_info_action_##i##() { \
                    ig_md.is_queue_full_##i## = get_ig_queue_occ_info_reg_alu_##i##.execute(0); \
                }
            
            #define IG_ACTION_SET_QUEUE_OCC_INFO(i) \
                action set_ig_queue_occ_info_action_##i##() { \
                    set_ig_queue_occ_info_reg_alu_##i##.execute(0); \                        
                }

            #define IG_TABLE_SET_QUEUE_OCC_INFO(i) \
                table set_ig_queue_occ_info_table_##i## { \
                    actions = { \
                        set_ig_queue_occ_info_action_##i##; \
                    } \
                    const default_action = set_ig_queue_occ_info_action_##i##; \
                    size = TABLE_SIZE; \
                }

            #define IG_TABLE_GET_QUEUE_OCC_INFO(i) \
                table get_ig_queue_occ_info_table_##i## { \
                    actions = { \
                        get_ig_queue_occ_info_action_##i##; \
                    } \
                    const default_action = get_ig_queue_occ_info_action_##i##; \
                    size = TABLE_SIZE; \
                }

            IG_QUEUE_OCC_INFO_REG(0)
            IG_QUEUE_OCC_INFO_REG(1)
            IG_QUEUE_OCC_INFO_REG(2)
            IG_QUEUE_OCC_INFO_REG(3)
            IG_QUEUE_OCC_INFO_REG(4)
            IG_QUEUE_OCC_INFO_REG(5)
            IG_QUEUE_OCC_INFO_REG(6)
            IG_QUEUE_OCC_INFO_REG(7)

            IG_REG_AC_GET_QUEUE_OCC_INFO(0)
            IG_REG_AC_GET_QUEUE_OCC_INFO(1)
            IG_REG_AC_GET_QUEUE_OCC_INFO(2)
            IG_REG_AC_GET_QUEUE_OCC_INFO(3)
            IG_REG_AC_GET_QUEUE_OCC_INFO(4)
            IG_REG_AC_GET_QUEUE_OCC_INFO(5)
            IG_REG_AC_GET_QUEUE_OCC_INFO(6)
            IG_REG_AC_GET_QUEUE_OCC_INFO(7)

            IG_REG_AC_SET_QUEUE_OCC_INFO(0)
            IG_REG_AC_SET_QUEUE_OCC_INFO(1)
            IG_REG_AC_SET_QUEUE_OCC_INFO(2)
            IG_REG_AC_SET_QUEUE_OCC_INFO(3)
            IG_REG_AC_SET_QUEUE_OCC_INFO(4)
            IG_REG_AC_SET_QUEUE_OCC_INFO(5)
            IG_REG_AC_SET_QUEUE_OCC_INFO(6)
            IG_REG_AC_SET_QUEUE_OCC_INFO(7)

            IG_ACTION_GET_QUEUE_OCC_INFO(0)
            IG_ACTION_GET_QUEUE_OCC_INFO(1)
            IG_ACTION_GET_QUEUE_OCC_INFO(2)
            IG_ACTION_GET_QUEUE_OCC_INFO(3)
            IG_ACTION_GET_QUEUE_OCC_INFO(4)
            IG_ACTION_GET_QUEUE_OCC_INFO(5)
            IG_ACTION_GET_QUEUE_OCC_INFO(6)
            IG_ACTION_GET_QUEUE_OCC_INFO(7)

            IG_ACTION_SET_QUEUE_OCC_INFO(0)
            IG_ACTION_SET_QUEUE_OCC_INFO(1)
            IG_ACTION_SET_QUEUE_OCC_INFO(2)
            IG_ACTION_SET_QUEUE_OCC_INFO(3)
            IG_ACTION_SET_QUEUE_OCC_INFO(4)
            IG_ACTION_SET_QUEUE_OCC_INFO(5)
            IG_ACTION_SET_QUEUE_OCC_INFO(6)
            IG_ACTION_SET_QUEUE_OCC_INFO(7)

            IG_TABLE_SET_QUEUE_OCC_INFO(0)
            IG_TABLE_SET_QUEUE_OCC_INFO(1)
            IG_TABLE_SET_QUEUE_OCC_INFO(2)
            IG_TABLE_SET_QUEUE_OCC_INFO(3)
            IG_TABLE_SET_QUEUE_OCC_INFO(4)
            IG_TABLE_SET_QUEUE_OCC_INFO(5)
            IG_TABLE_SET_QUEUE_OCC_INFO(6)
            IG_TABLE_SET_QUEUE_OCC_INFO(7)

            IG_TABLE_GET_QUEUE_OCC_INFO(0)
            IG_TABLE_GET_QUEUE_OCC_INFO(1)
            IG_TABLE_GET_QUEUE_OCC_INFO(2)
            IG_TABLE_GET_QUEUE_OCC_INFO(3)
            IG_TABLE_GET_QUEUE_OCC_INFO(4)
            IG_TABLE_GET_QUEUE_OCC_INFO(5)
            IG_TABLE_GET_QUEUE_OCC_INFO(6)
            IG_TABLE_GET_QUEUE_OCC_INFO(7)


            Forward() forward;
            // Forward() forward2;
            BeeRecirculate() bee_recirculate;

            // =e
            ingress_ctr_index_t ingress_ctr_index = 0;
            deflect_ctr_index_t deflect_ctr_index = 0;
            drop_ctr_index_t drop_ctr_index = 0;
            bee_ctr_index_t bee_ctr_index = 0;
            Counter< bit<CTR_WIDTH>, ingress_ctr_index_t >(
            INGRESS_CTR_SIZE, CounterType_t.PACKETS_AND_BYTES) ingress_ctr;
            Counter< bit<CTR_WIDTH>, deflect_ctr_index_t >(
            INGRESS_CTR_SIZE, CounterType_t.PACKETS_AND_BYTES) deflect_ctr;
            Counter< bit<CTR_WIDTH>, drop_ctr_index_t >(
            INGRESS_CTR_SIZE, CounterType_t.PACKETS_AND_BYTES) drop_ctr;
            Counter< bit<CTR_WIDTH>, bee_ctr_index_t >(
            INGRESS_CTR_SIZE, CounterType_t.PACKETS_AND_BYTES) bee_ctr;

            Register<bit<32>, _>(1) ucast_port_debug;

            RegisterAction<bit<32>, _, void>(ucast_port_debug) ucast_port_debug_alu = {
                void apply(inout bit<32> register_data) {
                    // if(hdr.bee.queue_occ_info > 0) {
                        register_data = (bit<32>)ig_md.output_port_idx;
                    // }
                }
            };
            

            // drop
            action drop() { 
                ig_intr_dprsr_md.drop_ctl = 1; 
            }

            action set_deflect_eggress_port_action(PortId_t idx) {
                ig_intr_tm_md.ucast_egress_port = idx;
            }

            table set_deflect_eggress_port_table {
                key = {
                    ig_md.output_port_idx : exact;
                }
                actions = {
                    set_deflect_eggress_port_action;
                    drop;
                }
                size = TABLE_SIZE;
                default_action = drop();
            }

            Random<bit<5>>() rnd;

            action generate_random_number_action() { 
                ig_md.random_number = rnd.get(); 
            }

            table generate_random_number_table {
                actions = {
                    generate_random_number_action;
                }
                const default_action = generate_random_number_action;
                size = TABLE_SIZE;
            }

                        
            apply {

                if (hdr.bee.isValid()) {
                    // // Worker packets should be recirculated
                    if (hdr.bee.port_idx_in_reg == 0) {
                        set_ig_queue_occ_info_table_0.apply();
                    } else if (hdr.bee.port_idx_in_reg == 1) {
                        set_ig_queue_occ_info_table_1.apply();
                    } else if (hdr.bee.port_idx_in_reg == 2) {
                        if (hdr.bee.queue_occ_info == 1) {
                            drop_ctr.count(drop_ctr_index);
                        }
                        set_ig_queue_occ_info_table_2.apply();
                    } else if (hdr.bee.port_idx_in_reg == 3) {
                        set_ig_queue_occ_info_table_3.apply();
                    } else if (hdr.bee.port_idx_in_reg == 4) {
                        set_ig_queue_occ_info_table_4.apply();
                    } else if (hdr.bee.port_idx_in_reg == 5) {
                        set_ig_queue_occ_info_table_5.apply();
                    } else if (hdr.bee.port_idx_in_reg == 6) {
                        set_ig_queue_occ_info_table_6.apply();
                    } else if (hdr.bee.port_idx_in_reg == 7) {
                        set_ig_queue_occ_info_table_7.apply();
                    }

                    bee_recirculate.apply(hdr, ig_intr_tm_md, ig_intr_md);
                    bee_ctr.count(bee_ctr_index);

                } else {
                    ingress_ctr.count(ingress_ctr_index);
                    forward.apply(hdr, ig_intr_tm_md, ig_intr_dprsr_md, ig_md);
                    if (hdr.ipv4.isValid() && (hdr.ipv4.protocol == IP_PROTOCOLS_TCP || hdr.ipv4.protocol == IP_PROTOCOLS_UDP)) {
                        
                        get_ig_queue_occ_info_table_7.apply();
                        get_ig_queue_occ_info_table_6.apply();
                        get_ig_queue_occ_info_table_5.apply();
                        get_ig_queue_occ_info_table_4.apply();
                        get_ig_queue_occ_info_table_3.apply();
                        get_ig_queue_occ_info_table_2.apply();
                        get_ig_queue_occ_info_table_1.apply();
                        get_ig_queue_occ_info_table_0.apply();
                        
                        if (ig_md.fw_port_idx == 0) {
                            if (ig_md.is_queue_full_0 == 0) {
                                ig_md.is_fw_port_full = 0;
                            } else {
                                ig_md.is_fw_port_full = 1;
                            }
                        } else if (ig_md.fw_port_idx == 1) {
                            if (ig_md.is_queue_full_1 == 0) {
                                ig_md.is_fw_port_full = 0;
                            } else {
                                ig_md.is_fw_port_full = 1;
                            }
                        } else if (ig_md.fw_port_idx == 2) {
                            if (ig_md.is_queue_full_2 == 0) {
                                ig_md.is_fw_port_full = 0;
                            } else {
                                ig_md.is_fw_port_full = 1;
                            }
                        } else if (ig_md.fw_port_idx == 3) {
                            if (ig_md.is_queue_full_3 == 0) {
                                ig_md.is_fw_port_full = 0;
                            } else {
                                ig_md.is_fw_port_full = 1;
                            }
                        } else if (ig_md.fw_port_idx == 4) {
                            if (ig_md.is_queue_full_4 == 0) {
                                ig_md.is_fw_port_full = 0;
                            } else {
                                ig_md.is_fw_port_full = 1;
                            }
                        } else if (ig_md.fw_port_idx == 5) {
                            if (ig_md.is_queue_full_5 == 0) {
                                ig_md.is_fw_port_full = 0;
                            } else {
                                ig_md.is_fw_port_full = 1;
                            }
                        } else if (ig_md.fw_port_idx == 6) {
                            if (ig_md.is_queue_full_6 == 0) {
                                ig_md.is_fw_port_full = 0;
                            } else {
                                ig_md.is_fw_port_full = 1;
                            }
                        } else if (ig_md.fw_port_idx == 7) {
                            if (ig_md.is_queue_full_7 == 0) {
                                ig_md.is_fw_port_full = 0;
                            } else {
                                ig_md.is_fw_port_full = 1;
                            }
                        }
  

                        generate_random_number_table.apply();
                        // // make sure not to consider ports toward neighboring switches
                        // ig_md.temp_queue_occ_info = (ig_md.temp_queue_occ_info | NEIGHBOR_SWITCH_INDICATOR);
                        ig_md.is_queue_full_0 = ig_md.is_queue_full_0 | NEIGHBOR_SWITCH_INDICATOR_0;
                        ig_md.is_queue_full_1 = ig_md.is_queue_full_1 | NEIGHBOR_SWITCH_INDICATOR_1;
                        ig_md.is_queue_full_2 = ig_md.is_queue_full_2 | NEIGHBOR_SWITCH_INDICATOR_2;
                        ig_md.is_queue_full_3 = ig_md.is_queue_full_3 | NEIGHBOR_SWITCH_INDICATOR_3;
                        ig_md.is_queue_full_4 = ig_md.is_queue_full_4 | NEIGHBOR_SWITCH_INDICATOR_4;
                        ig_md.is_queue_full_5 = ig_md.is_queue_full_5 | NEIGHBOR_SWITCH_INDICATOR_5;
                        ig_md.is_queue_full_6 = ig_md.is_queue_full_6 | NEIGHBOR_SWITCH_INDICATOR_6;
                        ig_md.is_queue_full_7 = ig_md.is_queue_full_7 | NEIGHBOR_SWITCH_INDICATOR_7;

                        if (ig_md.is_fw_port_full == 1) {
                            // queue is full
                            if (ig_md.random_number <= 0 && ig_md.is_queue_full_0 == 0) {
                                ig_md.output_port_idx = 0;
                            } else if (ig_md.random_number <= 1 && ig_md.is_queue_full_1 == 0) {
                                ig_md.output_port_idx = 1;
                            } else if (ig_md.random_number <= 2 && ig_md.is_queue_full_2 == 0) {
                                ig_md.output_port_idx = 2;
                            } else if (ig_md.random_number <= 3 && ig_md.is_queue_full_3 == 0) {
                                ig_md.output_port_idx = 3;
                            } else if (ig_md.random_number <= 4 && ig_md.is_queue_full_4 == 0) {
                                ig_md.output_port_idx = 4;
                            } else if (ig_md.random_number <= 5 && ig_md.is_queue_full_5 == 0) {
                                ig_md.output_port_idx = 5;
                            } else if (ig_md.random_number <= 6 && ig_md.is_queue_full_6 == 0) {
                                ig_md.output_port_idx = 6;
                            } else if (ig_md.random_number <= 7 && ig_md.is_queue_full_7 == 0) {
                                ig_md.output_port_idx = 7;
                            } else {
                                // it's a loop check
                                if (ig_md.is_queue_full_0 == 0) {
                                    ig_md.output_port_idx = 0;
                                } else if (ig_md.is_queue_full_1 == 0) {
                                    ig_md.output_port_idx = 1;
                                } else if (ig_md.is_queue_full_2 == 0) {
                                    ig_md.output_port_idx = 2;
                                } else if (ig_md.is_queue_full_3 == 0) {
                                    ig_md.output_port_idx = 3;
                                } else if (ig_md.is_queue_full_4 == 0) {
                                    ig_md.output_port_idx = 4;
                                } else if (ig_md.is_queue_full_5 == 0) {
                                    ig_md.output_port_idx = 5;
                                } else if (ig_md.is_queue_full_6 == 0) {
                                    ig_md.output_port_idx = 6;
                                } else if (ig_md.is_queue_full_7 == 0) {
                                    ig_md.output_port_idx = 7;
                                }
                            }

                            set_deflect_eggress_port_table.apply();
                            deflect_ctr.count(deflect_ctr_index);
                            ucast_port_debug_alu.execute(0);    // what is the value for idx
                        }
                    } 
                // else {
                //     forward2.apply(hdr, ig_intr_tm_md, ig_intr_dprsr_md, ig_md);
                }
            }
        }


/*
    Switch Egress pipeline
*/
control SwitchEgress(
        inout header_t hdr,
        inout metadata_t eg_md,
        in egress_intrinsic_metadata_t eg_intr_md,
        in egress_intrinsic_metadata_from_parser_t eg_intr_from_prsr,
        inout egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr,
        inout egress_intrinsic_metadata_for_output_port_t eg_intr_md_for_oport) {

            #define QUEUE_OCC_INFO_REG(i) \
                Register<bit<8>, _>(1) queue_occ_info_reg_##i##;

            #define REG_AC_GET_QUEUE_OCC_INFO(i) \
                RegisterAction<bit<8>, _, bit<8>>(queue_occ_info_reg_##i##) get_queue_occ_info_reg_alu_##i## = { \
                    void apply(inout bit<8> register_data, out bit<8> result) { \
                        result = register_data; \
                    } \
                };

            #define REG_AC_SET_QUEUE_OCC_INFO(i) \
                RegisterAction<bit<8>, _, void>(queue_occ_info_reg_##i##) set_queue_occ_info_reg_alu_##i## = { \
                    void apply(inout bit<8> register_data) { \
                        if (eg_md.is_fw_port_full == 1) { \
                            register_data = 1; \
                        } else { \
                            register_data = 0; \
                        } \
                    } \
                };

            #define REG_AC_SET_QUEUE_OCC_INFO2(i) \
                RegisterAction<bit<8>, _, void>(queue_occ_info_reg_##i##) set_queue_occ_info_reg_alu_##i## = { \
                    void apply(inout bit<8> register_data) { \
                        if (eg_md.queue_length2 == C) { \
                            register_data = 1; \
                        } else { \
                            register_data = 0; \
                        } \
                    } \
                };

            #define ACTION_GET_QUEUE_OCC_INFO(i) \
                action get_queue_occ_info_action_##i##() { \
                    hdr.bee.queue_occ_info = get_queue_occ_info_reg_alu_##i##.execute(0); \
                }
            
            #define ACTION_SET_QUEUE_OCC_INFO(i) \
                action set_queue_occ_info_action_##i##() { \
                    set_queue_occ_info_reg_alu_##i##.execute(0); \                        
                }

            #define TABLE_SET_QUEUE_OCC_INFO(i) \
                table set_queue_occ_info_table_##i## { \
                    actions = { \
                        set_queue_occ_info_action_##i##; \
                    } \
                    const default_action = set_queue_occ_info_action_##i##; \
                    size = TABLE_SIZE; \
                }

            #define TABLE_GET_QUEUE_OCC_INFO(i) \
                table get_queue_occ_info_table_##i## { \
                    actions = { \
                        get_queue_occ_info_action_##i##; \
                    } \
                    const default_action = get_queue_occ_info_action_##i##; \
                    size = TABLE_SIZE; \
                }

            QUEUE_OCC_INFO_REG(0)
            QUEUE_OCC_INFO_REG(1)
            QUEUE_OCC_INFO_REG(2)
            QUEUE_OCC_INFO_REG(3)
            QUEUE_OCC_INFO_REG(4)
            QUEUE_OCC_INFO_REG(5)
            QUEUE_OCC_INFO_REG(6)
            QUEUE_OCC_INFO_REG(7)

            REG_AC_GET_QUEUE_OCC_INFO(0)
            REG_AC_GET_QUEUE_OCC_INFO(1)
            REG_AC_GET_QUEUE_OCC_INFO(2)
            REG_AC_GET_QUEUE_OCC_INFO(3)
            REG_AC_GET_QUEUE_OCC_INFO(4)
            REG_AC_GET_QUEUE_OCC_INFO(5)
            REG_AC_GET_QUEUE_OCC_INFO(6)
            REG_AC_GET_QUEUE_OCC_INFO(7)

            REG_AC_SET_QUEUE_OCC_INFO(0)
            REG_AC_SET_QUEUE_OCC_INFO(1)
            REG_AC_SET_QUEUE_OCC_INFO(2)
            REG_AC_SET_QUEUE_OCC_INFO(3)
            REG_AC_SET_QUEUE_OCC_INFO(4)
            REG_AC_SET_QUEUE_OCC_INFO(5)
            REG_AC_SET_QUEUE_OCC_INFO(6)
            REG_AC_SET_QUEUE_OCC_INFO(7)

            ACTION_GET_QUEUE_OCC_INFO(0)
            ACTION_GET_QUEUE_OCC_INFO(1)
            ACTION_GET_QUEUE_OCC_INFO(2)
            ACTION_GET_QUEUE_OCC_INFO(3)
            ACTION_GET_QUEUE_OCC_INFO(4)
            ACTION_GET_QUEUE_OCC_INFO(5)
            ACTION_GET_QUEUE_OCC_INFO(6)
            ACTION_GET_QUEUE_OCC_INFO(7)

            ACTION_SET_QUEUE_OCC_INFO(0)
            ACTION_SET_QUEUE_OCC_INFO(1)
            ACTION_SET_QUEUE_OCC_INFO(2)
            ACTION_SET_QUEUE_OCC_INFO(3)
            ACTION_SET_QUEUE_OCC_INFO(4)
            ACTION_SET_QUEUE_OCC_INFO(5)
            ACTION_SET_QUEUE_OCC_INFO(6)
            ACTION_SET_QUEUE_OCC_INFO(7)

            TABLE_SET_QUEUE_OCC_INFO(0)
            TABLE_SET_QUEUE_OCC_INFO(1)
            TABLE_SET_QUEUE_OCC_INFO(2)
            TABLE_SET_QUEUE_OCC_INFO(3)
            TABLE_SET_QUEUE_OCC_INFO(4)
            TABLE_SET_QUEUE_OCC_INFO(5)
            TABLE_SET_QUEUE_OCC_INFO(6)
            TABLE_SET_QUEUE_OCC_INFO(7)

            TABLE_GET_QUEUE_OCC_INFO(0)
            TABLE_GET_QUEUE_OCC_INFO(1)
            TABLE_GET_QUEUE_OCC_INFO(2)
            TABLE_GET_QUEUE_OCC_INFO(3)
            TABLE_GET_QUEUE_OCC_INFO(4)
            TABLE_GET_QUEUE_OCC_INFO(5)
            TABLE_GET_QUEUE_OCC_INFO(6)
            TABLE_GET_QUEUE_OCC_INFO(7)

            action subtract_queue_lengths_action() {
                eg_md.queue_length = min(eg_md.queue_length, C);
                // eg_md.queue_length2 = min(eg_md.queue_length2, C);
            }

            table subtract_queue_lengths_table {
                actions = {
                    subtract_queue_lengths_action;
                }
                const default_action = subtract_queue_lengths_action;
                size = TABLE_SIZE;
            }

            action get_eg_port_idx_in_reg_action(bit<16> index) {
                eg_md.port_idx_in_reg = index;
            }
            table get_eg_port_idx_in_reg_table {
                key = {
                    eg_intr_md.egress_port: exact;
                }
                actions = {
                    get_eg_port_idx_in_reg_action;
                }

                size = TABLE_SIZE;
                // const default_action = set_eg_queue_length_action;
            }

            Register<bit<32>, _>(1) eg_ucast_port_debug;

            RegisterAction<bit<32>, _, void>(eg_ucast_port_debug) eg_ucast_port_debug_alu = {
                void apply(inout bit<32> register_data) {
                    register_data = eg_md.queue_length;
                }
            };

            egress_ctr_index_t egress_ctr_index = 0;
            drop_ctr_index_t drop_ctr_index = 0;
            Counter< bit<CTR_WIDTH>, egress_ctr_index_t >(
            INGRESS_CTR_SIZE, CounterType_t.PACKETS_AND_BYTES) egress_ctr;
            Counter< bit<CTR_WIDTH>, drop_ctr_index_t >(
            INGRESS_CTR_SIZE, CounterType_t.PACKETS_AND_BYTES) drop_ctr;

            apply {
                if (hdr.bee.isValid()) {
                    // At the egress, worker packets should only read from the queue occupancy register array
                    if (hdr.bee.port_idx_in_reg == 0) {
                        get_queue_occ_info_table_0.apply();
                    } else if (hdr.bee.port_idx_in_reg == 1) {
                        get_queue_occ_info_table_1.apply();
                    } else if (hdr.bee.port_idx_in_reg == 2) {
                        // drop_ctr.count(0);
                        get_queue_occ_info_table_2.apply();
                    } else if (hdr.bee.port_idx_in_reg == 3) {
                        get_queue_occ_info_table_3.apply();
                    } else if (hdr.bee.port_idx_in_reg == 4) {
                        get_queue_occ_info_table_4.apply();
                    } else if (hdr.bee.port_idx_in_reg == 5) {
                        get_queue_occ_info_table_5.apply();
                    } else if (hdr.bee.port_idx_in_reg == 6) {
                        get_queue_occ_info_table_6.apply();
                    } else if (hdr.bee.port_idx_in_reg == 7) {
                        get_queue_occ_info_table_7.apply();
                    }
                    // eg_ucast_port_debug_alu.execute(0);
                } else {
                    if (hdr.ipv4.isValid() && (hdr.ipv4.protocol == IP_PROTOCOLS_TCP || hdr.ipv4.protocol == IP_PROTOCOLS_UDP)) {

                        // At the egress, data packets should write into the queue occupancy register array
                        eg_md.queue_length = (bit<32>)eg_intr_md.deq_qdepth;
                        subtract_queue_lengths_table.apply();
                        // eg_md.queue_length2 = (bit<32>)eg_intr_md.deq_qdepth;
                        if (eg_md.queue_length == C) {
                            eg_md.is_fw_port_full = 1;
                        } else {
                            eg_md.is_fw_port_full = 0;
                        }
                         
                        get_eg_port_idx_in_reg_table.apply();
                        
                        if (eg_md.port_idx_in_reg == 0) {
                            set_queue_occ_info_table_0.apply();
                        } else if (eg_md.port_idx_in_reg == 1) {
                            set_queue_occ_info_table_1.apply();
                        } else if (eg_md.port_idx_in_reg == 2) {
                            if (eg_md.is_fw_port_full == 1) {
                                egress_ctr.count(egress_ctr_index);
                            } else {
                                drop_ctr.count(0);
                            }
                            eg_ucast_port_debug_alu.execute(0);
                            set_queue_occ_info_table_2.apply();
                        } else if (eg_md.port_idx_in_reg == 3) {
                            set_queue_occ_info_table_3.apply();
                        } else if (eg_md.port_idx_in_reg == 4) {
                            set_queue_occ_info_table_4.apply();
                        } else if (eg_md.port_idx_in_reg == 5) {
                            set_queue_occ_info_table_5.apply();
                        } else if (eg_md.port_idx_in_reg == 6) {
                            set_queue_occ_info_table_6.apply();
                        } else if (eg_md.port_idx_in_reg == 7) {
                            set_queue_occ_info_table_7.apply();
                        }
                        // set_queue_occ_info_reg_table.apply();
                    }
                }
            }
    
    }


//switch architecture
Pipeline(SwitchIngressParser(),
         SwitchIngress(),
         SwitchIngressDeparser(),
         SwitchEgressParser(),
         SwitchEgress(),
         SwitchEgressDeparser()) pipe;

Switch(pipe) main;