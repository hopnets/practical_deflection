#include <core.p4>
#if __TARGET_TOFINO__ == 2
#include <t2na.p4>
#else
#include <tna.p4>
#endif

#include "includes/quantilepd_defines.p4"
#include "includes/quantilepd_headers.p4"
#include "includes/quantilepd_parser.p4"
#include "quantilepd_controls.p4"

typedef bit<INGRESS_CTR_WIDTH> ingress_ctr_index_t;
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

            bit<12> master_mode = master_mode_t.SWITCH;


            Routing() routing;
            DeflectRouting() deflection_routing;
            DefaultRouting() default_routing;
            BeeRecirculate() bee_recirculate;
            GetQuantile() get_quantile;
            GetMin() get_min;
            DeflectGetMin() deflect_get_min;

            // drop
            action drop() { 
                ig_intr_dprsr_md.drop_ctl = 1; 
            }

            // Registers FW
            #define REG_QUEUE_LEN(i) \
                Register<bit<32>, _>(NUM_QUEUES) ig_queue_length_reg_##i##;

            #define REG_AC_GET_IG_QUEUE_LEN(i) \
                RegisterAction<bit<32>, _, bit<32>>(ig_queue_length_reg_##i##) get_ig_queue_length_alu_##i## = { \
                    void apply(inout bit<32> register_data, out bit<32> result) { \
                        result = register_data; \
                    } \
                };

            #define REG_AC_SET_IG_QUEUE_LEN(i) \
                RegisterAction<bit<32>, _, void>(ig_queue_length_reg_##i##) set_ig_queue_length_alu_##i## = { \
                    void apply(inout bit<32> register_data) { \
                        register_data = hdr.bee.queue_length; \
                    } \
                };

            // #define ACTION_GET_IG_QUEUE_LEN(i) \
            //     action get_ig_queue_length_action_##i##(bit<32> index) { \
            //         ig_md.queue_length_##i## = get_ig_queue_length_alu_##i##.execute(index)[15:0]; \
            //     }
            
            // Setting the queue occupancy registers based on worker packets
            #define ACTION_SET_IG_QUEUE_LEN(i) \
                action set_ig_queue_length_action_##i##(bit<32> index) { \
                    set_ig_queue_length_alu_##i##.execute(index); \                        
                }

            #define TABLE_SET_IG_QUEUE_LEN(i) \
                table set_ig_queue_length_table_##i## { \
                    key = { \
                        hdr.bee.ucast_egress_port: exact; \
                        hdr.bee.qid: exact; \
                    } \
                    actions = { \
                        set_ig_queue_length_action_##i##; \
                    } \
                    size = TABLE_SIZE; \
                }

            
            REG_QUEUE_LEN(0)
            REG_QUEUE_LEN(1)

            REG_AC_GET_IG_QUEUE_LEN(0)
            REG_AC_GET_IG_QUEUE_LEN(1)

            REG_AC_SET_IG_QUEUE_LEN(0)
            REG_AC_SET_IG_QUEUE_LEN(1)

            // ACTION_GET_IG_QUEUE_LEN(0)
            // ACTION_GET_IG_QUEUE_LEN(1)

            ACTION_SET_IG_QUEUE_LEN(0)
            ACTION_SET_IG_QUEUE_LEN(1)

            TABLE_SET_IG_QUEUE_LEN(0)
            TABLE_SET_IG_QUEUE_LEN(1)

            /* 
                * Getting the queue occupancy registers to route the packets
                * Why we have same key for all set_ig_queue_length_tables but 
                * different keys for get_ig_queue_length_tables?
                * It's because when a bee packet arrives at ingress, we update
                * the information for only one queue in all four registers
                * but when we want to read the information to do load balancing, 
                * we need the queue occupancy of four different queues
            */

            action get_ig_queue_length_action(bit<32> index) {
                ig_md.queue_length = get_ig_queue_length_alu_0.execute(index)[15:0];
            }

            action deflect_get_ig_queue_length_action(bit<32> index) {
                ig_md.deflect_queue_length = get_ig_queue_length_alu_1.execute(index)[15:0];
            }

            table get_ig_queue_length_table {
                key = {
                    ig_intr_tm_md.ucast_egress_port: exact;
                    ig_intr_tm_md.qid: exact;
                }
                actions = {
                    get_ig_queue_length_action;
                }
                size = TABLE_SIZE;
            }

            table deflect_get_ig_queue_length_table {
                key = {
                    ig_md.deflect_ucast_egress_port: exact;
                    ig_intr_tm_md.qid: exact;
                }
                actions = {
                    deflect_get_ig_queue_length_action;
                }
                size = TABLE_SIZE;
            }

            
            // get flow priority based on ipv4 srcAddr
            action get_flow_priority_action(bit<32> rank) {
                ig_md.rank = rank;
            }

            table get_flow_priority_table {
                key = {
                    hdr.ipv4.srcAddr : exact;
                    hdr.ipv4.dstAddr : exact;
                }
                actions = {
                    // resubmit_action;
                    get_flow_priority_action;
                }
                size = TABLE_SIZE;
            }

            // Get quantile (rank among the items in the window)
            // TODO: I don't think we need to tables for tail when deflecting
            Register<pair16, _>(1) tail_reg;
            RegisterAction<pair16, _, bit<16>>(tail_reg) get_tail_alu = {
                void apply(inout pair16 register_data, out bit<16> result) {
                    if (register_data.low < 16 * SAMPLE_COUNT - 1) {
                        register_data.low = register_data.low + 1;
                    } else {
                        register_data.low = 0;
                    }
                    result = register_data.low;
                }
            };

            action get_tail_action() {
                ig_md.tail = get_tail_alu.execute(0);
            }

            table get_tail_table {
                actions = {
                    get_tail_action;
                }
                const default_action = get_tail_action;
                size = TABLE_SIZE;
            }

            table drop_table {
                actions = {
                    drop;
                }
                const default_action = drop;
                size = TABLE_SIZE;
            }

            // action subtract_queue_lengths_action() {
            //     // ig_md.tmp_queue_length_0 = ig_md.tmp_queue_length_0 - ig_md.queue_length_1;
            //     // ig_md.tmp_queue_length_2 = ig_md.tmp_queue_length_2 - ig_md.queue_length_3;
            //     ig_md.queue_length = C - ig_md.queue_length;
            //     ig_md.deflect_queue_length = C - ig_md.deflect_queue_length;
            // }

            // table subtract_queue_lengths_table {
            //     actions = {
            //         subtract_queue_lengths_action;
            //     }
            //     const default_action = subtract_queue_lengths_action;
            //     size = TABLE_SIZE;
            // }

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
                    if(ig_md.queue_length != 0){
                        register_data = (bit<32>)ig_md.queue_length;
                    }
                }
            };

                        
            apply {

                master_mode = ig_md.port_properties.port_vid;

                if(master_mode == master_mode_t.SWITCH || !hdr.ipv4.isValid()){
                    default_routing.apply(hdr, ig_md, ig_intr_dprsr_md, ig_intr_tm_md);
                }
                else {
                    if (hdr.bee.isValid()) {
                        // Only for bee(worker) packets
                        /*
                            * We don't need routing for these packets
                            * We set the output port based on 
                        */

                        /*
                            * Assuming that the worker packets have the information about the queue
                            * occupancies, we store the information they are moving around in 4 registers
                            * Why do we need 4 registers: For Po2 forwarding and deflection, we need to read
                            * 4 queue occupancies at max: two for forwarding and two for deflection, with 1 
                            * register, this requires four stages out of 12
                        */
                        set_ig_queue_length_table_0.apply();
                        set_ig_queue_length_table_1.apply();

                        // e
                        // if(hdr.bee.queue_length > 0)
                        // {
                        //     ucast_port_debug_alu.execute(0);
                        // }
                        bee_ctr.count(0);


                        // Worker packets should be recirculated
                        bee_recirculate.apply(hdr, ig_intr_tm_md, ig_intr_md);
                    

                    // } else if (hdr.tcp.isValid() || hdr.udp.isValid()) {
                    // } else if (hdr.ipv4.isValid()) {
                    } else if (hdr.ipv4.isValid() && (hdr.ipv4.protocol == IP_PROTOCOLS_TCP || hdr.ipv4.protocol == IP_PROTOCOLS_UDP)) {
                        
                        ingress_ctr.count(ingress_ctr_index);   // What is happening to bee packets?!
                        
                        // get flow priority using ipv4 srcAddr
                        get_flow_priority_table.apply();

                        // Select two valid random port indexes for FW and two random ports for DEF
                        routing.apply(hdr, ig_intr_tm_md, ig_intr_dprsr_md, ig_md);
                        deflection_routing.apply(hdr, ig_md, ig_intr_dprsr_md, ig_intr_tm_md);

                        // We transform the inequition into (1-K)*C*quantile<=C-c
                        // Get queue occupancy c for two forwarding ports and two deflection ports
                        get_ig_queue_length_table.apply();    // FW
                        deflect_get_ig_queue_length_table.apply();    // DEF
                        
                        ucast_port_debug_alu.execute(0);

                        // The if statements below calculate C-c
                        if(ig_md.queue_length < C){
                            ig_md.queue_length = C - ig_md.queue_length;
                        }
                        else{
                            ig_md.queue_length = 0;
                        }
                        if(ig_md.deflect_queue_length < C){
                            ig_md.deflect_queue_length = C - ig_md.deflect_queue_length;
                        }
                        else{
                            ig_md.deflect_queue_length = 0;
                        }

                        // subtract_queue_lengths_table.apply();

                        /*
                            Get quantile (rank among the items in the window)
                            Since, FOR NOW (TODO), we are only keeping one window (and one tail) for all ports
                            we don't need to get quantile multiple times
                        */
                        get_tail_table.apply();
                        get_quantile.apply(ig_md);
                        get_min.apply(ig_md);
                        deflect_get_min.apply(ig_md);

                        // drop_ctr.count(ingress_ctr_index);

                        // ingress_ctr.count(2);

                        if (ig_md.min_value == ig_md.queue_length) {
                            /*
                                The packet should be dropped, get ready for deflection
                                update the output port
                            */
                            

                            if(master_mode == master_mode_t.PD && ig_md.deflect_min_value != ig_md.deflect_queue_length)  //todo check
                            {
                                deflect_ctr.count(deflect_ctr_index);
                                ig_intr_tm_md.ucast_egress_port = ig_md.deflect_ucast_egress_port;

                            }
                            else {
                                    drop_table.apply();
                                    drop_ctr.count(0);
                            }

                        }
                    }

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

            // GetEgQueueLength() get_eg_queue_length;
            // SetEgQueueLength() set_eg_queue_length;
            Counter< bit<CTR_WIDTH>, bee_ctr_index_t >(
            INGRESS_CTR_SIZE, CounterType_t.PACKETS_AND_BYTES) bee_ctr;

            Register<bit<32>, _>(NUM_QUEUES) eg_queue_length_reg;

            RegisterAction<bit<32>, _, bit<32>>(eg_queue_length_reg) get_eg_queue_length_alu = {
                void apply(inout bit<32> register_data, out bit<32> result) {
                    result = register_data;
                }
            };

            action get_eg_queue_length_action(bit<32> index) {
                hdr.bee.queue_length = get_eg_queue_length_alu.execute(index);
            }

            // Worker packets read the information from the register array
            table get_eg_queue_length_table {
                key = {
                    hdr.bee.ucast_egress_port: exact;
                    hdr.bee.qid: exact;
                }

                actions = {
                    get_eg_queue_length_action;
                }
                // const default_action = get_eg_queue_length_action;
                size = TABLE_SIZE;
            }

            RegisterAction<bit<32>, _, void>(eg_queue_length_reg) set_eg_queue_length_alu = {
                void apply(inout bit<32> register_data) {
                    register_data = (bit<32>)eg_intr_md.deq_qdepth;
                }
            };
            action set_eg_queue_length_action(bit<32> index) {
                // TODO: black box, deal with later
                // set_eg_queue_length_alu.execute_stateful_alu(index);
                set_eg_queue_length_alu.execute(index);
            }
            table set_eg_queue_length_table {
                key = {
                    eg_intr_md.egress_port: exact;
                    eg_intr_md.egress_qid: exact;
                }
                actions = {
                    set_eg_queue_length_action;
                }

                size = TABLE_SIZE;
                // const default_action = set_eg_queue_length_action;
            }

            Register<bit<8>, _>(1) eg_ucast_port_debug;

            RegisterAction<bit<8>, _, void>(eg_ucast_port_debug) eg_ucast_port_debug_alu = {
                void apply(inout bit<8> register_data) {
                    register_data = (bit<8>)eg_intr_md.egress_port;
                }
            };

            apply {

                if (hdr.bee.isValid()) {
                    // At the egress, worker packets should only read from the queue occupancy register array
                    get_eg_queue_length_table.apply();
                    bee_ctr.count(0);

                } else {
                    // At the egress, data packets should write into the queue occupancy register array
                    set_eg_queue_length_table.apply();
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