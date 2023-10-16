#include <bf_rt/bf_rt_info.hpp>
#include <bf_rt/bf_rt_init.hpp>
#include <bf_rt/bf_rt_common.h>
#include <bf_rt/bf_rt_table_key.hpp>
#include <bf_rt/bf_rt_table_data.hpp>
#include <bf_rt/bf_rt_table.hpp>
#include <bf_rt/bf_rt_table_operations.hpp>
#include <getopt.h>
#include <unistd.h>
#include <chrono>
#include <iostream>
#include <algorithm>
#include <arpa/inet.h>
#include <cmath>

#ifdef __cplusplus
extern "C" {
#endif
#include <bf_switchd/bf_switchd.h>
#include <bf_pm/bf_pm_intf.h>
#ifdef __cplusplus
}
#endif
#include <unistd.h>
#include <sys/types.h>

#define INIT_STATUS_TCP_PORT    7777
#define ALL_PIPES 0xffff


struct options {
	char hostname[100];
};

char* getCmdOption(char ** begin, char ** end, const std::string & option)
{
    char ** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end)
    {
        return *itr;
    }
    return 0;
}

bf_status_t cp_run(bf_switchd_context_t *ctx, struct options *opt)
{
    bf_status_t status = 0;
    char *bfshell = (char *) "/home/USER/bf-sde-9.7.0/install/bin/bfshell -b bfrt_asic.py -i";
    char *progname = (char *) "distpd";
    const bfrt::BfRtInfo *bfrtInfo = nullptr;
	const bfrt::BfRtTable *port_table = nullptr;
	const bfrt::BfRtTable *ing_table = nullptr;
	uint32_t ip_addr;


	std::shared_ptr<bfrt::BfRtSession> session;
	std::unique_ptr<bfrt::BfRtTableKey> table_key;
	std::unique_ptr<bfrt::BfRtTableData> table_data;
	bf_rt_target_t dev_tgt;
	bf_rt_id_t set_output, action_id;
	std::vector<bf_rt_id_t> ids, dataFields;

	uint64_t *port_list;
	uint64_t *mac_port_list;
	uint32_t num_ports = 0;

	u_int32_t switch_get_flow_priority_entries = 0;
	std::string *switch_get_flow_priority_entries_array_keys1;
	std::string *switch_get_flow_priority_entries_array_keys2;
	u_int64_t *switch_get_flow_priority_entries_array_prios;

	uint32_t switch_get_fw_idx_port_entries = 0, switch_deflect_get_fw_idx_port_entries = 0;
	std::string *switch_get_fw_idx_port_addrs, *switch_deflect_get_fw_idx_port_addrs;
	uint64_t *switch_get_fw_idx_port_indices, *switch_deflect_get_fw_idx_port_indices;

	uint32_t switch_get_ig_queue_length_table_entries = 0, switch_get_ig_m_table_0_entries = 0;
	uint32_t switch_deflect_get_ig_queue_length_table_entries = 0, switch_get_ig_m_table_1_entries = 0;
	uint16_t *switch_get_ig_queue_length_table_qid, *switch_get_ig_m_table_0_qid;
	uint16_t *switch_get_ig_queue_length_table_portids, *switch_get_ig_m_table_0_portids;
	uint64_t *switch_get_ig_queue_length_table_indices, *switch_get_ig_m_table_0_indices;
	uint16_t *switch_deflect_get_ig_queue_length_table_qid, *switch_get_ig_m_table_1_qid;
	uint16_t *switch_deflect_get_ig_queue_length_table_portids, *switch_get_ig_m_table_1_portids;
	uint64_t *switch_deflect_get_ig_queue_length_table_indices, *switch_get_ig_m_table_1_indices;

	uint32_t switch_set_ig_queue_length_table_0_entries = 0, switch_set_ig_queue_length_table_1_entries = 0;
	uint32_t switch_set_ig_m_table_0_entries = 0, switch_set_ig_m_table_1_entries = 0;
	uint16_t *switch_set_ig_queue_length_table_0_qid, *switch_set_ig_queue_length_table_1_qid;
	uint16_t *switch_set_ig_queue_length_table_0_portids, *switch_set_ig_queue_length_table_1_portids;
	uint64_t *switch_set_ig_queue_length_table_0_indices, *switch_set_ig_queue_length_table_1_indices;
	uint16_t *switch_set_ig_m_table_0_qid, *switch_set_ig_m_table_1_qid;
	uint16_t *switch_set_ig_m_table_0_portids, *switch_set_ig_m_table_1_portids;
	uint64_t *switch_set_ig_m_table_0_indices, *switch_set_ig_m_table_1_indices;

	uint32_t switch_get_rel_prio_table_entries = 0;
	uint32_t switch_get_rel_prio_table_entries_all = 0;
	uint64_t *switch_get_rel_prio_table_m_starts;
	uint64_t *switch_get_rel_prio_table_m_ends;
	uint64_t *switch_get_rel_prio_table_rank_starts;
	uint64_t *switch_get_rel_prio_table_rank_ends;
	uint64_t *switch_get_rel_prio_table_rel_prios;

	uint32_t switch_set_eg_queue_length_table_entries = 0;
	uint16_t *switch_set_eg_queue_length_table_qid;
	uint16_t *switch_set_eg_queue_length_table_ports;
	uint64_t *switch_set_eg_queue_length_table_indices;

	uint32_t switch_get_eg_queue_length_table_entries = 0;
	uint16_t *switch_get_eg_queue_length_table_qid;
	uint16_t *switch_get_eg_queue_length_table_ports;
	uint64_t *switch_get_eg_queue_length_table_indices;

	uint32_t switch_set_eg_m_table_entries = 0;
	uint16_t *switch_set_eg_m_table_qid;
	uint16_t *switch_set_eg_m_table_ports;
	uint64_t *switch_set_eg_m_table_indices;

	uint32_t switch_get_eg_m_table_entries = 0;
	uint16_t *switch_get_eg_m_table_qid;
	uint16_t *switch_get_eg_m_table_ports;
	uint64_t *switch_get_eg_m_table_indices;

	uint32_t switch_get_newm_table_entries = 0;
	uint32_t switch_get_newm_table_entries_all = 0;
	uint64_t *switch_get_newm_table_m_starts;
	uint64_t *switch_get_newm_table_m_ends;
	uint64_t *switch_get_newm_table_rank_starts;
	uint64_t *switch_get_newm_table_rank_ends;
	uint64_t *switch_get_newm_table_newms;

	uint32_t switch_default_routing_entries;	// =e
	std::string *switch_default_routing_entries_array_keys;
	uint64_t *switch_default_routing_destinations;

	//////////////////////////////////////////////////////// VARIABLES //////////////////////////////////////////////////////////////////
	///////////////////////// MUST CHANGE THESE ACOORDING TO YOUR PHYSICAL SETUP ////////////////////////////////////////////////////////
	uint32_t fns1_num_ports = 5, fns2_num_ports = 2;
	uint64_t fns1_port_list[fns1_num_ports] = {128, 144, 160, 176, 140};
	uint64_t fns1_mac_port_list[fns1_num_ports] = {23, 21, 19, 17, 25};
	uint64_t fns2_port_list[fns2_num_ports] = {136, 180};
	uint64_t fns2_mac_port_list[fns2_num_ports] = {25, 17};
	// 
	uint64_t C = 1024 * 80;
	double	alpha = 0.1;

	// get_flow_priority_table
	u_int32_t fns1_get_flow_priority_entries = 8, fns2_get_flow_priority_entries = 8;
	std::string fns1_get_flow_priority_entries_array_keys1[fns1_get_flow_priority_entries] = {"10.2.2.3", "10.2.2.5", "10.2.2.101", "10.2.2.6", "10.2.2.6", "10.2.2.6", "10.2.2.6", "10.2.2.9"};
	std::string fns1_get_flow_priority_entries_array_keys2[fns1_get_flow_priority_entries] = {"10.2.2.6", "10.2.2.6", "10.2.2.6", "10.2.2.3", "10.2.2.5", "10.2.2.101", "10.2.2.9", "10.2.2.6"};
	u_int64_t fns1_get_flow_priority_entries_array_prios[fns1_get_flow_priority_entries] = {45, 2, 45, 45, 2, 45, 45, 45};
	std::string fns2_get_flow_priority_entries_array_keys1[fns2_get_flow_priority_entries] = {"10.2.2.3", "10.2.2.5", "10.2.2.101", "10.2.2.6", "10.2.2.6", "10.2.2.6", "10.2.2.6", "10.2.2.9"};
	std::string fns2_get_flow_priority_entries_array_keys2[fns2_get_flow_priority_entries] = {"10.2.2.6", "10.2.2.6", "10.2.2.6", "10.2.2.3", "10.2.2.5", "10.2.2.101", "10.2.2.9", "10.2.2.6"};
	u_int64_t fns2_get_flow_priority_entries_array_prios[fns2_get_flow_priority_entries] = {45, 2, 45, 45, 2, 45, 45, 45};

	// get_fw_idx_port
	u_int32_t fns1_get_fw_idx_port_entries = 5, fns2_get_fw_idx_port_entries = 5;
	std::string fns1_get_fw_idx_port_port_addrs[fns1_get_fw_idx_port_entries] = {"10.2.2.3", "10.2.2.5", "10.2.2.6", "10.2.2.9", "10.2.2.101"};
	u_int64_t fns1_get_fw_idx_port_indices[fns1_get_fw_idx_port_entries] = {128, 144, 160, 140, 140};
	std::string fns2_get_fw_idx_port_port_addrs[fns2_get_fw_idx_port_entries] = {"10.2.2.3", "10.2.2.5", "10.2.2.6", "10.2.2.9", "10.2.2.101"};
	u_int64_t fns2_get_fw_idx_port_indices[fns2_get_fw_idx_port_entries] = {136, 136, 136, 180, 180}; 
	// deflect_get_fw_idx_port
	u_int32_t fns1_deflect_get_fw_idx_port_entries = 5, fns2_deflect_get_fw_idx_port_entries = 5;
	std::string fns1_deflect_get_fw_idx_port_addrs[fns1_deflect_get_fw_idx_port_entries] = {"10.2.2.3", "10.2.2.5", "10.2.2.6", "10.2.2.9", "10.2.2.101"};
	u_int64_t fns1_deflect_get_fw_idx_port_indices[fns1_deflect_get_fw_idx_port_entries] = {140, 140, 140, 140, 140};
	std::string fns2_deflect_get_fw_idx_port_addrs[fns2_deflect_get_fw_idx_port_entries] = {"10.2.2.3", "10.2.2.5", "10.2.2.6", "10.2.2.9", "10.2.2.101"};
	u_int64_t fns2_deflect_get_fw_idx_port_indices[fns2_deflect_get_fw_idx_port_entries] = {136, 136, 136, 136, 136}; 

	// get_ig_queue_length_table
	u_int32_t fns1_get_ig_queue_length_table_entries = 5, fns2_get_ig_queue_length_table_entries = 2;
	uint16_t fns1_get_ig_queue_length_table_qid[fns1_get_ig_queue_length_table_entries] = {0, 0, 0, 0, 0};
	uint16_t fns1_get_ig_queue_length_table_portids[fns1_get_ig_queue_length_table_entries] = {128, 144, 160, 176, 140};
	uint64_t fns1_get_ig_queue_length_table_indices[fns1_get_ig_queue_length_table_entries] = {0, 1, 2, 3, 4};
	uint16_t fns2_get_ig_queue_length_table_qid[fns2_get_ig_queue_length_table_entries] = {0, 0};
	uint16_t fns2_get_ig_queue_length_table_portids[fns2_get_ig_queue_length_table_entries] = {136, 180};	
	uint64_t fns2_get_ig_queue_length_table_indices[fns2_get_ig_queue_length_table_entries] = {0, 1};

	// get_ig_m_table_0
	u_int32_t fns1_get_ig_m_table_0_entries = 5, fns2_get_ig_m_table_0_entries = 2;
	uint16_t fns1_get_ig_m_table_0_qid[fns1_get_ig_m_table_0_entries] = {0, 0, 0, 0, 0};
	uint16_t fns1_get_ig_m_table_0_rand_ports[fns1_get_ig_m_table_0_entries] = {128, 144, 160, 176, 140};
	uint64_t fns1_get_ig_m_table_0_indices[fns1_get_ig_m_table_0_entries] = {0, 1, 2, 3, 4};
	uint16_t fns2_get_ig_m_table_0_qid[fns2_get_ig_m_table_0_entries] = {0, 0};
	uint16_t fns2_get_ig_m_table_0_rand_ports[fns2_get_ig_m_table_0_entries] = {136, 180};
	uint64_t fns2_get_ig_m_table_0_indices[fns2_get_ig_m_table_0_entries] = {0, 1};

	// get_ig_m_table_1
	u_int32_t fns1_get_ig_m_table_1_entries = 5, fns2_get_ig_m_table_1_entries = 2;
	uint16_t fns1_get_ig_m_table_1_qid[fns1_get_ig_m_table_1_entries] = {0, 0, 0, 0, 0};
	uint16_t fns1_get_ig_m_table_1_rand_ports[fns1_get_ig_m_table_1_entries] = {128, 144, 160, 176, 140};
	uint64_t fns1_get_ig_m_table_1_indices[fns1_get_ig_m_table_1_entries] = {0, 1, 2, 3, 4};
	uint16_t fns2_get_ig_m_table_1_qid[fns2_get_ig_m_table_1_entries] = {0, 0};
	uint16_t fns2_get_ig_m_table_1_rand_ports[fns2_get_ig_m_table_1_entries] = {136, 180};
	uint64_t fns2_get_ig_m_table_1_indices[fns2_get_ig_m_table_1_entries] = {0, 1};

	// set_ig_m_table_0
	u_int32_t fns1_set_ig_m_table_0_entries = 5, fns2_set_ig_m_table_0_entries = 2;
	uint16_t fns1_set_ig_m_table_0_qid[fns1_set_ig_m_table_0_entries] = {0, 0, 0, 0, 0};
	uint16_t fns1_set_ig_m_table_0_rand_ports[fns1_set_ig_m_table_0_entries] = {128, 144, 160, 176, 140};
	uint64_t fns1_set_ig_m_table_0_indices[fns1_set_ig_m_table_0_entries] = {0, 1, 2, 3, 4};
	uint16_t fns2_set_ig_m_table_0_qid[fns2_set_ig_m_table_0_entries] = {0, 0};
	uint16_t fns2_set_ig_m_table_0_rand_ports[fns2_set_ig_m_table_0_entries] = {136, 180};
	uint64_t fns2_set_ig_m_table_0_indices[fns2_set_ig_m_table_0_entries] = {0, 1};

	// set_ig_m_table_1
	u_int32_t fns1_set_ig_m_table_1_entries = 5, fns2_set_ig_m_table_1_entries = 2;
	uint16_t fns1_set_ig_m_table_1_qid[fns1_set_ig_m_table_1_entries] = {0, 0, 0, 0, 0};
	uint16_t fns1_set_ig_m_table_1_rand_ports[fns1_set_ig_m_table_1_entries] = {128, 144, 160, 176, 140};
	uint64_t fns1_set_ig_m_table_1_indices[fns1_set_ig_m_table_1_entries] = {0, 1, 2, 3, 4};
	uint16_t fns2_set_ig_m_table_1_qid[fns2_set_ig_m_table_1_entries] = {0, 0};
	uint16_t fns2_set_ig_m_table_1_rand_ports[fns2_set_ig_m_table_1_entries] = {136, 180};
	uint64_t fns2_set_ig_m_table_1_indices[fns2_set_ig_m_table_1_entries] = {0, 1};

	// deflect_get_ig_queue_length_table
	u_int32_t fns1_deflect_get_ig_queue_length_table_entries = 5, fns2_deflect_get_ig_queue_length_table_entries = 2;
	uint16_t fns1_deflect_get_ig_queue_length_table_qid[fns1_deflect_get_ig_queue_length_table_entries] = {0, 0, 0, 0, 0};
	uint16_t fns1_deflect_get_ig_queue_length_table_portids[fns1_deflect_get_ig_queue_length_table_entries] = {128, 144, 160, 176, 140};
	uint64_t fns1_deflect_get_ig_queue_length_table_indices[fns1_deflect_get_ig_queue_length_table_entries] = {0, 1, 2, 3, 4};
	uint16_t fns2_deflect_get_ig_queue_length_table_qid[fns2_deflect_get_ig_queue_length_table_entries] = {0, 0};
	uint16_t fns2_deflect_get_ig_queue_length_table_portids[fns2_deflect_get_ig_queue_length_table_entries] = {136, 180};
	uint64_t fns2_deflect_get_ig_queue_length_table_indices[fns2_deflect_get_ig_queue_length_table_entries] = {0, 1};

	// set_ig_queue_length_table_0
	u_int32_t fns1_set_ig_queue_length_table_0_entries = 5, fns2_set_ig_queue_length_table_0_entries = 2;
	uint16_t fns1_set_ig_queue_length_table_0_qid[fns1_set_ig_queue_length_table_0_entries] = {0, 0, 0, 0, 0};
	uint16_t fns1_set_ig_queue_length_table_0_portids[fns1_set_ig_queue_length_table_0_entries] = {128, 144, 160, 176, 140};
	uint64_t fns1_set_ig_queue_length_table_0_indices[fns1_set_ig_queue_length_table_0_entries] = {0, 1, 2, 3, 4};
	uint16_t fns2_set_ig_queue_length_table_0_qid[fns2_set_ig_queue_length_table_0_entries] = {0, 0};
	uint16_t fns2_set_ig_queue_length_table_0_portids[fns2_set_ig_queue_length_table_0_entries] = {136, 180};	
	uint64_t fns2_set_ig_queue_length_table_0_indices[fns2_set_ig_queue_length_table_0_entries] = {0, 1};
	// set_ig_queue_length_table_1
	u_int32_t fns1_set_ig_queue_length_table_1_entries = 5, fns2_set_ig_queue_length_table_1_entries = 2;
	uint16_t fns1_set_ig_queue_length_table_1_qid[fns1_set_ig_queue_length_table_1_entries] = {0, 0, 0, 0, 0};
	uint16_t fns1_set_ig_queue_length_table_1_portids[fns1_set_ig_queue_length_table_1_entries] = {128, 144, 160, 176, 140};
	uint64_t fns1_set_ig_queue_length_table_1_indices[fns1_set_ig_queue_length_table_1_entries] = {0, 1, 2, 3, 4};
	uint16_t fns2_set_ig_queue_length_table_1_qid[fns2_set_ig_queue_length_table_1_entries] = {0, 0};
	uint16_t fns2_set_ig_queue_length_table_1_portids[fns2_set_ig_queue_length_table_1_entries] = {136, 180};
	uint64_t fns2_set_ig_queue_length_table_1_indices[fns2_set_ig_queue_length_table_1_entries] = {0, 1};

	// switch_get_rel_prio_table_0_entries	
	uint32_t fns1_get_rel_prio_table_entries = 5, fns2_get_rel_prio_table_entries = 5;
	uint32_t fns1_get_rel_prio_table_entries_all = fns1_get_rel_prio_table_entries * fns1_get_rel_prio_table_entries;
	uint32_t fns2_get_rel_prio_table_entries_all = fns2_get_rel_prio_table_entries * fns2_get_rel_prio_table_entries;
	uint64_t fns1_get_rel_prio_table_m_starts[fns1_get_rel_prio_table_entries_all];
	uint64_t fns1_get_rel_prio_table_m_ends[fns1_get_rel_prio_table_entries_all];
	uint64_t fns1_get_rel_prio_table_rank_starts[fns1_get_rel_prio_table_entries_all];
	uint64_t fns1_get_rel_prio_table_rank_ends[fns1_get_rel_prio_table_entries_all];
	uint64_t fns1_get_rel_prio_table_rel_prios[fns1_get_rel_prio_table_entries_all];
	uint64_t fns2_get_rel_prio_table_m_starts[fns2_get_rel_prio_table_entries_all];
	uint64_t fns2_get_rel_prio_table_m_ends[fns2_get_rel_prio_table_entries_all];
	uint64_t fns2_get_rel_prio_table_rank_starts[fns2_get_rel_prio_table_entries_all];
	uint64_t fns2_get_rel_prio_table_rank_ends[fns2_get_rel_prio_table_entries_all];
	uint64_t fns2_get_rel_prio_table_rel_prios[fns2_get_rel_prio_table_entries_all];
	int get_rel_prio_index_all = 0;
	for(int i = 0; i < fns1_get_rel_prio_table_entries; i++)
	{
		for(int j = 0; j < fns1_get_rel_prio_table_entries; j++)
		{
			fns1_get_rel_prio_table_m_starts[get_rel_prio_index_all] = (2 << i) + 1;
			fns1_get_rel_prio_table_m_ends[get_rel_prio_index_all] = 2 << (i+1);
			fns1_get_rel_prio_table_rank_starts[get_rel_prio_index_all] = (2 << j) + 1;
			fns1_get_rel_prio_table_rank_ends[get_rel_prio_index_all] = 2 << (j+1);
			double mid1 = (fns1_get_rel_prio_table_rank_starts[get_rel_prio_index_all]+fns1_get_rel_prio_table_rank_ends[get_rel_prio_index_all]) / 2;
            double mid2 = (fns1_get_rel_prio_table_m_starts[get_rel_prio_index_all]+fns1_get_rel_prio_table_m_ends[get_rel_prio_index_all]) / 2;
			fns1_get_rel_prio_table_rel_prios[get_rel_prio_index_all] = (uint64_t) floor(C*alpha*(1-exp(-(mid1/mid2))));
			fns2_get_rel_prio_table_m_starts[get_rel_prio_index_all] = (2 << i) + 1;
			fns2_get_rel_prio_table_m_ends[get_rel_prio_index_all] = 2 << (i+1);
			fns2_get_rel_prio_table_rank_starts[get_rel_prio_index_all] = (2 << j) + 1;
			fns2_get_rel_prio_table_rank_ends[get_rel_prio_index_all] = 2 << (j+1);
			fns2_get_rel_prio_table_rel_prios[get_rel_prio_index_all] = fns1_get_rel_prio_table_rel_prios[get_rel_prio_index_all];
			get_rel_prio_index_all++;
		}
	}


	// default_routing
	u_int32_t fns1_default_routing_entries = 5, fns2_default_routing_entries = 5;
	std::string fns1_default_routing_entries_array_keys[fns1_default_routing_entries] = {"10.2.2.3", "10.2.2.5", "10.2.2.6", "10.2.2.9", "10.2.2.101"};
	u_int64_t fns1_default_routing_entries_destinations[fns1_default_routing_entries] = {128, 144, 160, 140, 140};
	std::string fns2_default_routing_entries_array_keys[fns2_default_routing_entries] = {"10.2.2.3", "10.2.2.5", "10.2.2.6", "10.2.2.9", "10.2.2.101"};
	u_int64_t fns2_default_routing_entries_destinations[fns2_default_routing_entries] = {136, 136, 136, 180, 180};
	// set_eg_queue_length_table
	u_int32_t fns1_set_eg_queue_length_table_entries = 5, fns2_set_eg_queue_length_table_entries = 2;
	uint16_t fns1_set_eg_queue_length_table_qid[fns1_set_eg_queue_length_table_entries] = {0, 0, 0, 0, 0};
	uint16_t fns1_set_eg_queue_length_table_rand_ports[fns1_set_eg_queue_length_table_entries] = {128, 144, 160, 176, 140};
	uint64_t fns1_set_eg_queue_length_table_indices[fns1_set_eg_queue_length_table_entries] = {0, 1, 2, 3, 4};
	uint16_t fns2_set_eg_queue_length_table_qid[fns2_set_eg_queue_length_table_entries] = {0, 0};
	uint16_t fns2_set_eg_queue_length_table_rand_ports[fns2_set_eg_queue_length_table_entries] = {136, 180};	
	uint64_t fns2_set_eg_queue_length_table_indices[fns2_set_eg_queue_length_table_entries] = {0, 1};

	// get_eg_queue_length_table
	u_int32_t fns1_get_eg_queue_length_table_entries = 5, fns2_get_eg_queue_length_table_entries = 2;
	uint16_t fns1_get_eg_queue_length_table_qid[fns1_get_eg_queue_length_table_entries] = {0, 0, 0, 0, 0};
	uint16_t fns1_get_eg_queue_length_table_rand_ports[fns1_get_eg_queue_length_table_entries] = {128, 144, 160, 176, 140};
	uint64_t fns1_get_eg_queue_length_table_indices[fns1_get_eg_queue_length_table_entries] = {0, 1, 2, 3, 4};
	uint16_t fns2_get_eg_queue_length_table_qid[fns2_get_eg_queue_length_table_entries] = {0, 0};
	uint16_t fns2_get_eg_queue_length_table_rand_ports[fns2_get_eg_queue_length_table_entries] = {136, 180};	
	uint64_t fns2_get_eg_queue_length_table_indices[fns2_get_eg_queue_length_table_entries] = {0, 1};

	// get_eg_m_table
	u_int32_t fns1_get_eg_m_table_entries = 5, fns2_get_eg_m_table_entries = 2;
	uint16_t fns1_get_eg_m_table_qid[fns1_get_eg_m_table_entries] = {0, 0, 0, 0, 0};
	uint16_t fns1_get_eg_m_table_rand_ports[fns1_get_eg_m_table_entries] = {128, 144, 160, 176, 140};
	uint64_t fns1_get_eg_m_table_indices[fns1_get_eg_m_table_entries] = {0, 1, 2, 3, 4};
	uint16_t fns2_get_eg_m_table_qid[fns2_get_eg_m_table_entries] = {0, 0};
	uint16_t fns2_get_eg_m_table_rand_ports[fns2_get_eg_m_table_entries] = {136, 180};	
	uint64_t fns2_get_eg_m_table_indices[fns2_get_eg_m_table_entries] = {0, 1};

	// set_eg_m_table
	u_int32_t fns1_set_eg_m_table_entries = 5, fns2_set_eg_m_table_entries = 2;
	uint16_t fns1_set_eg_m_table_qid[fns1_set_eg_m_table_entries] = {0, 0, 0, 0, 0};
	uint16_t fns1_set_eg_m_table_rand_ports[fns1_set_eg_m_table_entries] = {128, 144, 160, 176, 140};
	uint64_t fns1_set_eg_m_table_indices[fns1_set_eg_m_table_entries] = {0, 1, 2, 3, 4};
	uint16_t fns2_set_eg_m_table_qid[fns2_set_eg_m_table_entries] = {0, 0};
	uint16_t fns2_set_eg_m_table_rand_ports[fns2_set_eg_m_table_entries] = {136, 180};	
	uint64_t fns2_set_eg_m_table_indices[fns2_set_eg_m_table_entries] = {0, 1};

	// get_newm_table
	uint32_t fns1_get_newm_table_entries = 5, fns2_get_newm_table_entries = 5;
	uint32_t fns1_get_newm_table_entries_all = fns1_get_newm_table_entries * fns1_get_newm_table_entries;
	uint32_t fns2_get_newm_table_entries_all = fns2_get_newm_table_entries * fns2_get_newm_table_entries;
	uint64_t fns1_get_newm_table_m_starts[fns1_get_newm_table_entries_all];
	uint64_t fns1_get_newm_table_m_ends[fns1_get_newm_table_entries_all];
	uint64_t fns1_get_newm_table_rank_starts[fns1_get_newm_table_entries_all];
	uint64_t fns1_get_newm_table_rank_ends[fns1_get_newm_table_entries_all];
	uint64_t fns1_get_newm_table_newms[fns1_get_newm_table_entries_all];
	uint64_t fns2_get_newm_table_m_starts[fns2_get_newm_table_entries_all];
	uint64_t fns2_get_newm_table_m_ends[fns2_get_newm_table_entries_all];
	uint64_t fns2_get_newm_table_rank_starts[fns2_get_newm_table_entries_all];
	uint64_t fns2_get_newm_table_rank_ends[fns2_get_newm_table_entries_all];
	uint64_t fns2_get_newm_table_newms[fns2_get_newm_table_entries_all];
	// (49 * m + rank) / 50
	get_rel_prio_index_all = 0;
	for(int i = 0; i < fns1_get_newm_table_entries; i++)
	{
		for(int j = 0; j < fns1_get_newm_table_entries; j++)
		{
			fns1_get_newm_table_m_starts[get_rel_prio_index_all] = (2 << i) + 1;
			fns1_get_newm_table_m_ends[get_rel_prio_index_all] = 2 << (i+1);
			fns1_get_newm_table_rank_starts[get_rel_prio_index_all] = (2 << j) + 1;
			fns1_get_newm_table_rank_ends[get_rel_prio_index_all] = 2 << (j+1);
			double mid1 = (fns1_get_newm_table_m_starts[get_rel_prio_index_all]+fns1_get_newm_table_m_ends[get_rel_prio_index_all]) / 2;
            double mid2 = (fns1_get_newm_table_rank_starts[get_rel_prio_index_all]+fns1_get_newm_table_rank_ends[get_rel_prio_index_all]) / 2;
			uint64_t new_m =  (uint64_t) floor((49*mid1 + mid2)/50);
			std::cout << mid1 << " " << " " << mid2 << " : " <<  new_m << std::endl;
			fns1_get_newm_table_newms[get_rel_prio_index_all] = new_m;
			fns2_get_newm_table_m_starts[get_rel_prio_index_all] = (2 << i) + 1;
			fns2_get_newm_table_m_ends[get_rel_prio_index_all] = 2 << (i+1);
			fns2_get_newm_table_rank_starts[get_rel_prio_index_all] = (2 << j) + 1;
			fns2_get_newm_table_rank_ends[get_rel_prio_index_all] = 2 << (j+1);
			fns2_get_newm_table_newms[get_rel_prio_index_all] = fns1_get_newm_table_newms[get_rel_prio_index_all];
			get_rel_prio_index_all++;
		}
	}

	

	///////////////////////////////////////////////////////////////////////////////////////////////
	if(strcmp(opt->hostname, "fns1") == 0)
	{
		num_ports = fns1_num_ports;
		port_list = fns1_port_list;
		mac_port_list = fns1_mac_port_list;

		switch_get_flow_priority_entries = fns1_get_flow_priority_entries;
		switch_get_flow_priority_entries_array_keys1 = fns1_get_flow_priority_entries_array_keys1;
		switch_get_flow_priority_entries_array_keys2 = fns1_get_flow_priority_entries_array_keys2;
		switch_get_flow_priority_entries_array_prios = fns1_get_flow_priority_entries_array_prios;

		switch_get_fw_idx_port_entries = fns1_get_fw_idx_port_entries;
		switch_deflect_get_fw_idx_port_entries = fns1_deflect_get_fw_idx_port_entries;
		switch_get_fw_idx_port_addrs = fns1_get_fw_idx_port_port_addrs;
		switch_deflect_get_fw_idx_port_addrs = fns1_deflect_get_fw_idx_port_addrs;
		switch_get_fw_idx_port_indices = fns1_get_fw_idx_port_indices;
		switch_deflect_get_fw_idx_port_indices = fns1_deflect_get_fw_idx_port_indices;

		switch_get_ig_queue_length_table_entries = fns1_get_ig_queue_length_table_entries;
		switch_get_ig_m_table_0_entries = fns1_get_ig_m_table_0_entries;
		switch_deflect_get_ig_queue_length_table_entries = fns1_deflect_get_ig_queue_length_table_entries; 
		switch_get_ig_m_table_1_entries = fns1_get_ig_m_table_1_entries;
		switch_get_ig_queue_length_table_qid = fns1_get_ig_queue_length_table_qid;
		switch_get_ig_m_table_0_qid = fns1_get_ig_m_table_0_qid;
		switch_get_ig_queue_length_table_portids = fns1_get_ig_queue_length_table_portids;
		switch_get_ig_m_table_0_portids = fns1_get_ig_m_table_0_rand_ports;
		switch_get_ig_queue_length_table_indices = fns1_get_ig_queue_length_table_indices;
		switch_get_ig_m_table_0_indices = fns1_get_ig_m_table_0_indices;
		switch_deflect_get_ig_queue_length_table_qid = fns1_deflect_get_ig_queue_length_table_qid;
		switch_get_ig_m_table_1_qid = fns1_get_ig_m_table_1_qid;
		switch_deflect_get_ig_queue_length_table_portids = fns1_deflect_get_ig_queue_length_table_portids;
		switch_get_ig_m_table_1_portids = fns1_get_ig_m_table_1_rand_ports;
		switch_deflect_get_ig_queue_length_table_indices = fns1_deflect_get_ig_queue_length_table_indices;
		switch_get_ig_m_table_1_indices = fns1_get_ig_m_table_1_indices;

		switch_set_ig_queue_length_table_0_entries = fns1_set_ig_queue_length_table_0_entries;
		switch_set_ig_m_table_0_entries = fns1_set_ig_m_table_0_entries;
		switch_set_ig_queue_length_table_1_entries = fns1_set_ig_queue_length_table_1_entries; 
		switch_set_ig_m_table_1_entries = fns1_set_ig_m_table_1_entries;
		switch_set_ig_queue_length_table_0_qid = fns1_set_ig_queue_length_table_0_qid;
		switch_set_ig_m_table_0_qid = fns1_set_ig_m_table_0_qid;
		switch_set_ig_queue_length_table_0_portids = fns1_set_ig_queue_length_table_0_portids;
		switch_set_ig_m_table_0_portids = fns1_set_ig_m_table_0_rand_ports;
		switch_set_ig_queue_length_table_0_indices = fns1_set_ig_queue_length_table_0_indices;
		switch_set_ig_m_table_0_indices = fns1_set_ig_m_table_0_indices;
		switch_set_ig_queue_length_table_1_qid = fns1_set_ig_queue_length_table_1_qid;
		switch_set_ig_m_table_1_qid = fns1_set_ig_m_table_1_qid;
		switch_set_ig_queue_length_table_1_portids = fns1_set_ig_queue_length_table_1_portids;
		switch_set_ig_m_table_1_portids = fns1_set_ig_m_table_1_rand_ports;
		switch_set_ig_queue_length_table_1_indices = fns1_set_ig_queue_length_table_1_indices;
		switch_set_ig_m_table_1_indices = fns1_set_ig_m_table_1_indices;

		switch_get_rel_prio_table_entries = fns1_get_rel_prio_table_entries;
		switch_get_rel_prio_table_entries_all = fns1_get_rel_prio_table_entries_all;
		switch_get_rel_prio_table_m_starts = fns1_get_rel_prio_table_m_starts;
		switch_get_rel_prio_table_m_ends = fns1_get_rel_prio_table_m_ends;
		switch_get_rel_prio_table_rank_starts = fns1_get_rel_prio_table_rank_starts;
		switch_get_rel_prio_table_rank_ends = fns1_get_rel_prio_table_rank_ends;
		switch_get_rel_prio_table_rel_prios = fns1_get_rel_prio_table_rel_prios;

		switch_set_eg_queue_length_table_entries = fns1_set_eg_queue_length_table_entries;
		switch_set_eg_queue_length_table_qid = fns1_set_eg_queue_length_table_qid;
		switch_set_eg_queue_length_table_ports = fns1_set_eg_queue_length_table_rand_ports;
		switch_set_eg_queue_length_table_indices = fns1_set_eg_queue_length_table_indices;

		switch_get_eg_queue_length_table_entries = fns1_get_eg_queue_length_table_entries;
		switch_get_eg_queue_length_table_qid = fns1_get_eg_queue_length_table_qid;
		switch_get_eg_queue_length_table_ports = fns1_get_eg_queue_length_table_rand_ports;
		switch_get_eg_queue_length_table_indices = fns1_get_eg_queue_length_table_indices;

		switch_set_eg_m_table_entries = fns1_set_eg_m_table_entries;
		switch_set_eg_m_table_qid = fns1_set_eg_m_table_qid;
		switch_set_eg_m_table_ports = fns1_set_eg_m_table_rand_ports;
		switch_set_eg_m_table_indices = fns1_set_eg_m_table_indices;

		switch_get_eg_m_table_entries = fns1_get_eg_m_table_entries;
		switch_get_eg_m_table_qid = fns1_get_eg_m_table_qid;
		switch_get_eg_m_table_ports = fns1_get_eg_m_table_rand_ports;
		switch_get_eg_m_table_indices = fns1_get_eg_m_table_indices;

		switch_get_newm_table_entries = fns1_get_newm_table_entries;
		switch_get_newm_table_entries_all = fns1_get_newm_table_entries_all;
		switch_get_newm_table_m_starts = fns1_get_newm_table_m_starts;
		switch_get_newm_table_m_ends = fns1_get_newm_table_m_ends;
		switch_get_newm_table_rank_starts = fns1_get_newm_table_rank_starts;
		switch_get_newm_table_rank_ends = fns1_get_newm_table_rank_ends;
		switch_get_newm_table_newms = fns1_get_newm_table_newms;

		switch_default_routing_entries = fns1_default_routing_entries;
		switch_default_routing_entries_array_keys = fns1_default_routing_entries_array_keys;
		switch_default_routing_destinations = fns1_default_routing_entries_destinations;
		switch_set_eg_queue_length_table_entries = fns1_set_eg_queue_length_table_entries;

	}
	else if(strcmp(opt->hostname, "fns2") == 0)
	{
		num_ports = fns2_num_ports;
		port_list = fns2_port_list;
		mac_port_list = fns2_mac_port_list;
		switch_get_flow_priority_entries = fns2_get_flow_priority_entries;
		switch_get_flow_priority_entries_array_keys1 = fns2_get_flow_priority_entries_array_keys1;
		switch_get_flow_priority_entries_array_keys2 = fns2_get_flow_priority_entries_array_keys2;
		switch_get_flow_priority_entries_array_prios = fns2_get_flow_priority_entries_array_prios;

		switch_get_fw_idx_port_entries = fns2_get_fw_idx_port_entries;
		switch_deflect_get_fw_idx_port_entries = fns2_deflect_get_fw_idx_port_entries;
		switch_get_fw_idx_port_addrs = fns2_get_fw_idx_port_port_addrs;
		switch_deflect_get_fw_idx_port_addrs = fns2_deflect_get_fw_idx_port_addrs;
		switch_get_fw_idx_port_indices = fns2_get_fw_idx_port_indices;
		switch_deflect_get_fw_idx_port_indices = fns2_deflect_get_fw_idx_port_indices;

		switch_get_ig_queue_length_table_entries = fns2_get_ig_queue_length_table_entries;
		switch_get_ig_m_table_0_entries = fns2_get_ig_m_table_0_entries;
		switch_deflect_get_ig_queue_length_table_entries = fns2_deflect_get_ig_queue_length_table_entries; 
		switch_get_ig_m_table_1_entries = fns2_get_ig_m_table_1_entries;
		switch_get_ig_queue_length_table_qid = fns2_get_ig_queue_length_table_qid;
		switch_get_ig_m_table_0_qid = fns2_get_ig_m_table_0_qid;
		switch_get_ig_queue_length_table_portids = fns2_get_ig_queue_length_table_portids;
		switch_get_ig_m_table_0_portids = fns2_get_ig_m_table_0_rand_ports;
		switch_get_ig_queue_length_table_indices = fns2_get_ig_queue_length_table_indices;
		switch_get_ig_m_table_0_indices = fns2_get_ig_m_table_0_indices;
		switch_deflect_get_ig_queue_length_table_qid = fns2_deflect_get_ig_queue_length_table_qid;
		switch_get_ig_m_table_1_qid = fns2_get_ig_m_table_1_qid;
		switch_deflect_get_ig_queue_length_table_portids = fns2_deflect_get_ig_queue_length_table_portids;
		switch_get_ig_m_table_1_portids = fns2_get_ig_m_table_1_rand_ports;
		switch_deflect_get_ig_queue_length_table_indices = fns2_deflect_get_ig_queue_length_table_indices;
		switch_get_ig_m_table_1_indices = fns2_get_ig_m_table_1_indices;

		switch_set_ig_queue_length_table_0_entries = fns2_set_ig_queue_length_table_0_entries;
		switch_set_ig_m_table_0_entries = fns2_set_ig_m_table_0_entries;
		switch_set_ig_queue_length_table_1_entries = fns2_set_ig_queue_length_table_1_entries; 
		switch_set_ig_m_table_1_entries = fns2_set_ig_m_table_1_entries;
		switch_set_ig_queue_length_table_0_qid = fns2_set_ig_queue_length_table_0_qid;
		switch_set_ig_m_table_0_qid = fns2_set_ig_m_table_0_qid;
		switch_set_ig_queue_length_table_0_portids = fns2_set_ig_queue_length_table_0_portids;
		switch_set_ig_m_table_0_portids = fns2_set_ig_m_table_0_rand_ports;
		switch_set_ig_queue_length_table_0_indices = fns2_set_ig_queue_length_table_0_indices;
		switch_set_ig_m_table_0_indices = fns2_set_ig_m_table_0_indices;
		switch_set_ig_queue_length_table_1_qid = fns2_set_ig_queue_length_table_1_qid;
		switch_set_ig_m_table_1_qid = fns2_set_ig_m_table_1_qid;
		switch_set_ig_queue_length_table_1_portids = fns2_set_ig_queue_length_table_1_portids;
		switch_set_ig_m_table_1_portids = fns2_set_ig_m_table_1_rand_ports;
		switch_set_ig_queue_length_table_1_indices = fns2_set_ig_queue_length_table_1_indices;
		switch_set_ig_m_table_1_indices = fns2_set_ig_m_table_1_indices;

		switch_get_rel_prio_table_entries = fns2_get_rel_prio_table_entries;
		switch_get_rel_prio_table_entries_all = fns2_get_rel_prio_table_entries_all;
		switch_get_rel_prio_table_m_starts = fns2_get_rel_prio_table_m_starts;
		switch_get_rel_prio_table_m_ends = fns2_get_rel_prio_table_m_ends;
		switch_get_rel_prio_table_rank_starts = fns2_get_rel_prio_table_rank_starts;
		switch_get_rel_prio_table_rank_ends = fns2_get_rel_prio_table_rank_ends;
		switch_get_rel_prio_table_rel_prios = fns2_get_rel_prio_table_rel_prios;

		switch_set_eg_queue_length_table_entries = fns2_set_eg_queue_length_table_entries;
		switch_set_eg_queue_length_table_qid = fns2_set_eg_queue_length_table_qid;
		switch_set_eg_queue_length_table_ports = fns2_set_eg_queue_length_table_rand_ports;
		switch_set_eg_queue_length_table_indices = fns2_set_eg_queue_length_table_indices;

		switch_get_eg_queue_length_table_entries = fns2_get_eg_queue_length_table_entries;
		switch_get_eg_queue_length_table_qid = fns2_get_eg_queue_length_table_qid;
		switch_get_eg_queue_length_table_ports = fns2_get_eg_queue_length_table_rand_ports;
		switch_get_eg_queue_length_table_indices = fns2_get_eg_queue_length_table_indices;

		switch_set_eg_m_table_entries = fns2_set_eg_m_table_entries;
		switch_set_eg_m_table_qid = fns2_set_eg_m_table_qid;
		switch_set_eg_m_table_ports = fns2_set_eg_m_table_rand_ports;
		switch_set_eg_m_table_indices = fns2_set_eg_m_table_indices;

		switch_get_eg_m_table_entries = fns2_get_eg_m_table_entries;
		switch_get_eg_m_table_qid = fns2_get_eg_m_table_qid;
		switch_get_eg_m_table_ports = fns2_get_eg_m_table_rand_ports;
		switch_get_eg_m_table_indices = fns2_get_eg_m_table_indices;

		switch_get_newm_table_entries = fns2_get_newm_table_entries;
		switch_get_newm_table_entries_all = fns2_get_newm_table_entries_all;
		switch_get_newm_table_m_starts = fns2_get_newm_table_m_starts;
		switch_get_newm_table_m_ends = fns2_get_newm_table_m_ends;
		switch_get_newm_table_rank_starts = fns2_get_newm_table_rank_starts;
		switch_get_newm_table_rank_ends = fns2_get_newm_table_rank_ends;
		switch_get_newm_table_newms = fns2_get_newm_table_newms;

		switch_default_routing_entries = fns2_default_routing_entries;
		switch_default_routing_entries_array_keys = fns2_default_routing_entries_array_keys;
		switch_default_routing_destinations = fns2_default_routing_entries_destinations;
		switch_set_eg_queue_length_table_entries = fns2_set_eg_queue_length_table_entries;
	}
	else {
		printf("Running on unknown switch hostname. It should be either fns1 or fns2, but it is %s. Exitting ...\n", opt->hostname);
		exit(1);
	}

	auto &devMgr = bfrt::BfRtDevMgr::getInstance();
	int tbl_entry;
	uint64_t selector_max_group_size = 2;
	bf_rt_id_t selector_data_field_ids[3], data_entry_id = 0, key_field_id = 0;
	uint32_t selector_data_index = 0;
	std::vector<bf_rt_id_t> selector_int_array;
	std::vector<bool> selector_bool_array;
	selector_bool_array.push_back(true);
	uint64_t mcast_rid = 0xFFFF;
	uint64_t memory_register_default_value = 0xFFFF;
	uint64_t port_pcp_vid = 0;

	std::vector<uint32_t> mcast_lag_ids;
	std::vector<uint32_t> mcast_dev_ports;
	std::vector<uint32_t> mgid_node_ids;
	std::vector<bool> mgid_xid_valid;
	std::vector<uint32_t> mgid_xids;
	std::vector<uint32_t> prune_vector;

	dev_tgt.dev_id = 0;
	dev_tgt.pipe_id = ALL_PIPES;

	auto bf_status = devMgr.bfRtInfoGet(dev_tgt.dev_id, progname, &bfrtInfo);
	assert(bf_status == BF_SUCCESS);

	session = bfrt::BfRtSession::sessionCreate();

    std::cout << "Listing all tables ..." << std::endl;

    std::vector<const bfrt::BfRtTable *> table_vec;
    bf_status = bfrtInfo->bfrtInfoGetTables(&table_vec);
    assert(bf_status == BF_SUCCESS);
    std::vector<const bfrt::BfRtTable *>::iterator it = table_vec.begin();
    for (it = table_vec.begin() ; it != table_vec.end(); ++it)
    {
        std::string name;
        (*it)->tableNameGet(&name);
        std::cout << "Table " << name << std::endl;
    }
        
	std::cout << "PORT table entries:" << std::endl;

	bf_status = bfrtInfo->bfrtTableFromNameGet("$PORT", &port_table);
	assert(bf_status == BF_SUCCESS);

	port_table->keyFieldIdListGet(&ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i)
    	printf("  Key field %d\n", *i);
	ids.clear();

    for (unsigned int idx = 0; idx < num_ports; idx++) {
        bf_pal_front_port_handle_t port_hdl;
        bf_status = bf_pm_port_dev_port_to_front_panel_port_get(dev_tgt.dev_id,
                                                port_list[idx],
                                                &port_hdl);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_pm_port_add(dev_tgt.dev_id, &port_hdl,
                                    BF_SPEED_40G, BF_FEC_TYP_NONE);
        assert(bf_status == BF_SUCCESS);
        bf_status = bf_pm_port_enable(dev_tgt.dev_id, &port_hdl);
        assert(bf_status == BF_SUCCESS);
        printf("Port %lu is enabled successfully!\n", port_list[idx]);
    }

	    ////////////////////////////////////////////////////////////////////
    std::cout << "Setting up broadcast engine. Populating pre.node entries:" << std::endl;

	bf_status = bfrtInfo->bfrtTableFromNameGet("$pre.node", &ing_table);
	assert(bf_status == BF_SUCCESS);
	bf_status = ing_table->keyAllocate(&table_key);
	assert(bf_status == BF_SUCCESS);

	ing_table->keyFieldIdListGet(&ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i)
    	printf("  Key field %d\n", *i);
	ids.clear();
	ing_table->actionIdListGet( &ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		ing_table->actionNameGet(*i, &s);
		printf("  action: %d ", *i);
		std::cout << s << std::endl;
		action_id = *i;
	}
	ids.clear();
	ing_table->dataFieldIdListGet(&ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		bfrt::DataType t;
		size_t size;
		ing_table->dataFieldNameGet(*i, action_id, &s);
		ing_table->dataFieldDataTypeGet(*i, action_id, &t);
		ing_table->dataFieldSizeGet(*i, action_id, &size);
		printf("  data field %d: ", *i);
		std::cout << s << " " << static_cast<int>(t) << " " << size << std::endl;

	}
	ids.clear();
	bf_status = ing_table->dataAllocate(&table_data);
	assert(bf_status == BF_SUCCESS);
	bf_status = table_key->setValue(0x01, 1);
	assert(bf_status == BF_SUCCESS);
	bf_status = table_data->setValue(0x01, mcast_rid);
	assert(bf_status == BF_SUCCESS);
	bf_status = table_data->setValue(0x02, mcast_lag_ids);
	assert(bf_status == BF_SUCCESS);
	for(int i = 0; i < num_ports; i++)
		mcast_dev_ports.push_back(port_list[i]);
	bf_status = table_data->setValue(0x03, mcast_dev_ports);
	assert(bf_status == BF_SUCCESS);
	bf_status = ing_table->tableEntryAdd(*session, dev_tgt, *table_key, *table_data);
	std::cout << "PRE NODE entries added." << std::endl;
	///////////////////////////////////////////////////////////////////////////////////////////////////////
	    ////////////////////////////////////////////////////////////////////
    std::cout << "Setting up broadcast engine. Populating pre.mgid entries:" << std::endl;

	bf_status = bfrtInfo->bfrtTableFromNameGet("$pre.mgid", &ing_table);
	assert(bf_status == BF_SUCCESS);
	bf_status = ing_table->keyAllocate(&table_key);
	assert(bf_status == BF_SUCCESS);

	ing_table->keyFieldIdListGet(&ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i)
    	printf("  Key field %d\n", *i);
	ids.clear();
	ing_table->actionIdListGet( &ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		ing_table->actionNameGet(*i, &s);
		printf("  action: %d ", *i);
		std::cout << s << std::endl;
		action_id = *i;
	}
	ids.clear();
	ing_table->dataFieldIdListGet(&ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		bfrt::DataType t;
		size_t size;
		ing_table->dataFieldNameGet(*i, action_id, &s);
		ing_table->dataFieldDataTypeGet(*i, action_id, &t);
		ing_table->dataFieldSizeGet(*i, action_id, &size);
		printf("  data field %d: ", *i);
		std::cout << s << " " << static_cast<int>(t) << " " << size << std::endl;

	}
	ids.clear();
	bf_status = ing_table->dataAllocate(&table_data);
	assert(bf_status == BF_SUCCESS);
	bf_status = table_key->setValue(0x01, 1);
	assert(bf_status == BF_SUCCESS);
	mgid_node_ids.push_back(0x1);
	bf_status = table_data->setValue(0x01, mgid_node_ids);
	assert(bf_status == BF_SUCCESS);
	mgid_xid_valid.push_back(false);
	bf_status = table_data->setValue(0x02, mgid_xid_valid);
	assert(bf_status == BF_SUCCESS);
	mgid_xids.push_back(0);
	bf_status = table_data->setValue(0x03, mgid_xids);
	assert(bf_status == BF_SUCCESS);
	bf_status = ing_table->tableEntryAdd(*session, dev_tgt, *table_key, *table_data);
	assert(bf_status == BF_SUCCESS);
	std::cout << "PRE MGID entries added." << std::endl;
	std::cout << "Multicast domain 1 configured." << std::endl;
	///////////////////////////////////////////////////////////////////////////////////////////////////////
		    ////////////////////////////////////////////////////////////////////
    std::cout << "Setting up broadcast engine. Populating PORT METADATA:" << std::endl;

	bf_status = bfrtInfo->bfrtTableFromNameGet("SwitchIngressParser.$PORT_METADATA", &ing_table);
	assert(bf_status == BF_SUCCESS);
	bf_status = ing_table->keyAllocate(&table_key);
	assert(bf_status == BF_SUCCESS);

	ing_table->keyFieldIdListGet(&ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
    	printf("  Key field %d\n", *i);
		key_field_id = *i;
	}
	ids.clear();
	ing_table->actionIdListGet( &ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		ing_table->actionNameGet(*i, &s);
		printf("  action: %d ", *i);
		std::cout << s << std::endl;
		action_id = *i;
	}
	ids.clear();
	ing_table->dataFieldIdListGet(&ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		bfrt::DataType t;
		size_t size;
		ing_table->dataFieldNameGet(*i, action_id, &s);
		ing_table->dataFieldDataTypeGet(*i, action_id, &t);
		ing_table->dataFieldSizeGet(*i, action_id, &size);
		printf("  data field %d: ", *i);
		std::cout << s << " " << static_cast<int>(t) << " " << size << std::endl;

	}
	ids.clear();
	bf_status = ing_table->dataAllocate(&table_data);
	assert(bf_status == BF_SUCCESS);
	uint64_t mcport;
	uint64_t vid_val = 0;
	for(int i = 0; i < num_ports; i++)
	{
		bf_status = table_key->setValue(key_field_id, port_list[i]);
		assert(bf_status == BF_SUCCESS);
		bf_status = table_data->setValue(0x01, port_pcp_vid);
		assert(bf_status == BF_SUCCESS);
		bf_status = table_data->setValue(0x02, vid_val);
		assert(bf_status == BF_SUCCESS);
		bf_status = table_data->setValue(0x03, mac_port_list[i]);
		assert(bf_status == BF_SUCCESS);
		bf_status = ing_table->tableEntryAdd(*session, dev_tgt, *table_key, *table_data);
		assert(bf_status == BF_SUCCESS);
	}

	bf_status = ing_table->tableEntryAdd(*session, dev_tgt, *table_key, *table_data);
	std::cout << "PORT METADATA entries added." << std::endl;
	///////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////
    std::cout << "Setting up broadcast engine. Populating pre.prune:" << std::endl;

	bf_status = bfrtInfo->bfrtTableFromNameGet("$pre.prune", &ing_table);
	assert(bf_status == BF_SUCCESS);
	bf_status = ing_table->keyAllocate(&table_key);
	assert(bf_status == BF_SUCCESS);

	ing_table->keyFieldIdListGet(&ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
    	printf("  Key field %d\n", *i);
		key_field_id = *i;
	}
	ids.clear();
	ing_table->actionIdListGet( &ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		ing_table->actionNameGet(*i, &s);
		printf("  action: %d ", *i);
		std::cout << s << std::endl;
		action_id = *i;
	}
	ids.clear();
	ing_table->dataFieldIdListGet(&ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		bfrt::DataType t;
		size_t size;
		ing_table->dataFieldNameGet(*i, action_id, &s);
		ing_table->dataFieldDataTypeGet(*i, action_id, &t);
		ing_table->dataFieldSizeGet(*i, action_id, &size);
		printf("  data field %d: ", *i);
		std::cout << s << " " << static_cast<int>(t) << " " << size << std::endl;

	}
	ids.clear();
	bf_status = ing_table->dataAllocate(&table_data);
	assert(bf_status == BF_SUCCESS);
	for(int i = 0; i < num_ports; i++)
	{
		bf_status = table_key->setValue(key_field_id, mac_port_list[i]);
		assert(bf_status == BF_SUCCESS);
		prune_vector.push_back(port_list[i]);
		bf_status = table_data->setValue(0x01, prune_vector);
		assert(bf_status == BF_SUCCESS);
		bf_status = ing_table->tableEntryAdd(*session, dev_tgt, *table_key, *table_data);
		assert(bf_status == BF_SUCCESS);
		prune_vector.clear();
	}

	bf_status = ing_table->tableEntryAdd(*session, dev_tgt, *table_key, *table_data);
	std::cout << "PRE PRUNE entries added." << std::endl;
	std::cout << "Broadcast setup is complete." << std::endl;
	///////////////////////////////////////////////////////////////////////////////////////////////////////
	// 	assert(bf_status == BF_SUCCESS);
	// for(tbl_entry=0; tbl_entry < switch_read_prefix_id_entries; tbl_entry++)
	// {
	// 	inet_pton(AF_INET, switch_read_prefix_id_entries_array_keys[tbl_entry].c_str(), &ip_addr);
	// 	printf("Converted IP addr value = %x\n", ntohl(ip_addr));
	// 	bf_status = table_key->setValue(0x01, ntohl(ip_addr));
	// 	assert(bf_status == BF_SUCCESS);
	// 	bf_status = table_data->setValue(1, switch_read_prefix_id_entries_array_prefixes[tbl_entry]);
	// 	assert(bf_status == BF_SUCCESS);
	// 	bf_status = ing_table->tableEntryAdd(*session, dev_tgt, *table_key, *table_data);
	// 	assert(bf_status == BF_SUCCESS);
	// }
    // std::cout << "PRE NODE entries added." << std::endl;
	////////////////////////////////////////////////////////////////////

	// https://github.com/stratum/stratum/blob/cb4ff5cfbb05523b1c50a5eaacbc4615bc845241/stratum/hal/lib/barefoot/bf_sde_wrapper.cc#L2062
    ////////////////////////////////////////////////////////////////////
	    ////////////////////////////////////////////////////////////////////
    std::cout << "Populating get_flow_priority_table entries:" << std::endl;

	bf_status = bfrtInfo->bfrtTableFromNameGet("SwitchIngress.get_flow_priority_table", &ing_table);
	assert(bf_status == BF_SUCCESS);
	bf_status = ing_table->keyAllocate(&table_key);
	assert(bf_status == BF_SUCCESS);

	ing_table->keyFieldIdListGet(&ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i)
    	printf("  Key field %d\n", *i);
	ids.clear();
	ing_table->actionIdListGet( &ids);
	action_id = 0;
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		ing_table->actionNameGet(*i, &s);
		printf("  action: %d ", *i);
		std::cout << s << std::endl;
		if(action_id == 0)
			action_id = *i;
	}
	ids.clear();
	ing_table->dataFieldIdListGet(action_id, &ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		bfrt::DataType t;
		size_t size;
		ing_table->dataFieldNameGet(*i, action_id, &s);
		ing_table->dataFieldDataTypeGet(*i, action_id, &t);
		ing_table->dataFieldSizeGet(*i, action_id, &size);
		printf("  data field %d: ", *i);
		std::cout << s << " " << static_cast<int>(t) << " " << size << std::endl;

	}
	ids.clear();
	bf_status = ing_table->dataAllocate(action_id, &table_data);
	assert(bf_status == BF_SUCCESS);
	for(tbl_entry=0; tbl_entry < switch_get_flow_priority_entries; tbl_entry++)
	{
		inet_pton(AF_INET, switch_get_flow_priority_entries_array_keys1[tbl_entry].c_str(), &ip_addr);
		printf("Coverted IP addr value = %x\n", ntohl(ip_addr));
		bf_status = table_key->setValue(0x01, ntohl(ip_addr));
		assert(bf_status == BF_SUCCESS);
		inet_pton(AF_INET, switch_get_flow_priority_entries_array_keys2[tbl_entry].c_str(), &ip_addr);
		printf("Coverted IP addr value = %x\n", ntohl(ip_addr));
		bf_status = table_key->setValue(0x02, ntohl(ip_addr));
		assert(bf_status == BF_SUCCESS);
		bf_status = table_data->setValue(1, switch_get_flow_priority_entries_array_prios[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = ing_table->tableEntryAdd(*session, dev_tgt, *table_key, *table_data);
		assert(bf_status == BF_SUCCESS);
	}
    std::cout << "get_flow_priority_table entries added." << std::endl;
	////////////////////////////////////////////////////////////////////
    std::cout << "Populating get_fw_idx_port entries:" << std::endl;

	bf_status = bfrtInfo->bfrtTableFromNameGet("SwitchIngress.routing.get_fw_idx_port_table", &ing_table);
	assert(bf_status == BF_SUCCESS);
	bf_status = ing_table->keyAllocate(&table_key);
	assert(bf_status == BF_SUCCESS);

	ing_table->keyFieldIdListGet(&ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i)
    	printf("  Key field %d\n", *i);
	ids.clear();
	ing_table->actionIdListGet( &ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		ing_table->actionNameGet(*i, &s);
		printf("  action: %d ", *i);
		std::cout << s << std::endl;
		action_id = *i;
	}
	ids.clear();
	ing_table->dataFieldIdListGet(action_id, &ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		bfrt::DataType t;
		size_t size;
		ing_table->dataFieldNameGet(*i, action_id, &s);
		ing_table->dataFieldDataTypeGet(*i, action_id, &t);
		ing_table->dataFieldSizeGet(*i, action_id, &size);
		printf("  data field %d: ", *i);
		std::cout << s << " " << static_cast<int>(t) << " " << size << std::endl;

	}
	ids.clear();
	bf_status = ing_table->dataAllocate(action_id, &table_data);
	assert(bf_status == BF_SUCCESS);
	for(tbl_entry=0; tbl_entry < switch_get_fw_idx_port_entries; tbl_entry++)
	{
		inet_pton(AF_INET, switch_get_fw_idx_port_addrs[tbl_entry].c_str(), &ip_addr);
		printf("Coverted IP addr value = %x\n", ntohl(ip_addr));
		bf_status = table_key->setValue(0x01, ntohl(ip_addr));
		assert(bf_status == BF_SUCCESS);
		bf_status = table_data->setValue(1, switch_get_fw_idx_port_indices[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = ing_table->tableEntryAdd(*session, dev_tgt, *table_key, *table_data);
		assert(bf_status == BF_SUCCESS);
	}
    std::cout << "get_fw_idx_port entries added." << std::endl;
	////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////
    std::cout << "Populating deflect_get_fw_idx_port entries:" << std::endl;

	bf_status = bfrtInfo->bfrtTableFromNameGet("SwitchIngress.deflection_routing.deflect_get_fw_idx_port_table", &ing_table);
	assert(bf_status == BF_SUCCESS);
	bf_status = ing_table->keyAllocate(&table_key);
	assert(bf_status == BF_SUCCESS);

	ing_table->keyFieldIdListGet(&ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i)
    	printf("  Key field %d\n", *i);
	ids.clear();
	ing_table->actionIdListGet( &ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		ing_table->actionNameGet(*i, &s);
		printf("  action: %d ", *i);
		std::cout << s << std::endl;
		action_id = *i;
	}
	ids.clear();
	ing_table->dataFieldIdListGet(action_id, &ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		bfrt::DataType t;
		size_t size;
		ing_table->dataFieldNameGet(*i, action_id, &s);
		ing_table->dataFieldDataTypeGet(*i, action_id, &t);
		ing_table->dataFieldSizeGet(*i, action_id, &size);
		printf("  data field %d: ", *i);
		std::cout << s << " " << static_cast<int>(t) << " " << size << std::endl;

	}
	ids.clear();
	bf_status = ing_table->dataAllocate(action_id, &table_data);
	assert(bf_status == BF_SUCCESS);
	for(tbl_entry=0; tbl_entry < switch_deflect_get_fw_idx_port_entries; tbl_entry++)
	{
		inet_pton(AF_INET, switch_deflect_get_fw_idx_port_addrs[tbl_entry].c_str(), &ip_addr);
		printf("Coverted IP addr value = %x\n", ntohl(ip_addr));
		bf_status = table_key->setValue(0x01, ntohl(ip_addr));
		assert(bf_status == BF_SUCCESS);
		bf_status = table_data->setValue(1, switch_deflect_get_fw_idx_port_indices[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = ing_table->tableEntryAdd(*session, dev_tgt, *table_key, *table_data);
		assert(bf_status == BF_SUCCESS);
	}
    std::cout << "deflect_get_fw_idx_port entries added." << std::endl;
	////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////
    std::cout << "Populating get_ig_queue_length_table entries:" << std::endl;

	bf_status = bfrtInfo->bfrtTableFromNameGet("SwitchIngress.get_ig_queue_length_table", &ing_table);
	assert(bf_status == BF_SUCCESS);
	bf_status = ing_table->keyAllocate(&table_key);
	assert(bf_status == BF_SUCCESS);

	ing_table->keyFieldIdListGet(&ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i) {
		bfrt::DataType dt;
		ing_table->keyFieldDataTypeGet(*i, &dt);
		printf("  Key field %d, datatype = %d\n", *i, static_cast<int>(dt));

	}
	ids.clear();
	ing_table->actionIdListGet( &ids);
	action_id = 0;
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		ing_table->actionNameGet(*i, &s);
		printf("  action: %d ", *i);
		std::cout << s << std::endl;
		if(action_id == 0)
			action_id = *i;
	}
	ids.clear();
	ing_table->dataFieldIdListGet(action_id, &ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		bfrt::DataType t;
		size_t size;
		ing_table->dataFieldNameGet(*i, action_id, &s);
		ing_table->dataFieldDataTypeGet(*i, action_id, &t);
		ing_table->dataFieldSizeGet(*i, action_id, &size);
		printf("  data field %d: ", *i);
		std::cout << s << " " << static_cast<int>(t) << " " << size << std::endl;

	}
	ids.clear();
	bf_status = ing_table->dataAllocate(action_id, &table_data);
	assert(bf_status == BF_SUCCESS);
	for(tbl_entry=0; tbl_entry < switch_get_ig_queue_length_table_entries; tbl_entry++)
	{
		bf_status = table_key->setValue(0x02, switch_get_ig_queue_length_table_qid[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = table_key->setValue(0x01, switch_get_ig_queue_length_table_portids[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = table_data->setValue(1, switch_get_ig_queue_length_table_indices[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = ing_table->tableEntryAdd(*session, dev_tgt, *table_key, *table_data);
		assert(bf_status == BF_SUCCESS);
	}
    std::cout << "get_ig_queue_length_table entries added." << std::endl;
	////////////////////////////////////////////////////////////////////
			////////////////////////////////////////////////////////////////////
    std::cout << "Populating deflect_get_ig_queue_length_table entries:" << std::endl;

	bf_status = bfrtInfo->bfrtTableFromNameGet("SwitchIngress.deflect_get_ig_queue_length_table", &ing_table);
	assert(bf_status == BF_SUCCESS);
	bf_status = ing_table->keyAllocate(&table_key);
	assert(bf_status == BF_SUCCESS);

	ing_table->keyFieldIdListGet(&ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i) {
		bfrt::DataType dt;
		ing_table->keyFieldDataTypeGet(*i, &dt);
		printf("  Key field %d, datatype = %d\n", *i, static_cast<int>(dt));

	}
	ids.clear();
	ing_table->actionIdListGet( &ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		ing_table->actionNameGet(*i, &s);
		printf("  action: %d ", *i);
		std::cout << s << std::endl;
		action_id = *i;
	}
	ids.clear();
	ing_table->dataFieldIdListGet(action_id, &ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		bfrt::DataType t;
		size_t size;
		ing_table->dataFieldNameGet(*i, action_id, &s);
		ing_table->dataFieldDataTypeGet(*i, action_id, &t);
		ing_table->dataFieldSizeGet(*i, action_id, &size);
		printf("  data field %d: ", *i);
		std::cout << s << " " << static_cast<int>(t) << " " << size << std::endl;

	}
	ids.clear();
	bf_status = ing_table->dataAllocate(action_id, &table_data);
	assert(bf_status == BF_SUCCESS);
	for(tbl_entry=0; tbl_entry < switch_deflect_get_ig_queue_length_table_entries; tbl_entry++)
	{
		bf_status = table_key->setValue(0x02, switch_deflect_get_ig_queue_length_table_qid[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = table_key->setValue(0x01, switch_deflect_get_ig_queue_length_table_portids[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = table_data->setValue(1, switch_deflect_get_ig_queue_length_table_indices[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = ing_table->tableEntryAdd(*session, dev_tgt, *table_key, *table_data);
		assert(bf_status == BF_SUCCESS);
	}
    std::cout << "deflect_get_ig_queue_length_table entries added." << std::endl;
	////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////
    std::cout << "Populating get_ig_m_table_0 entries:" << std::endl;

	bf_status = bfrtInfo->bfrtTableFromNameGet("SwitchIngress.get_ig_m_table_0", &ing_table);
	assert(bf_status == BF_SUCCESS);
	bf_status = ing_table->keyAllocate(&table_key);
	assert(bf_status == BF_SUCCESS);

	ing_table->keyFieldIdListGet(&ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i) {
		bfrt::DataType dt;
		ing_table->keyFieldDataTypeGet(*i, &dt);
		printf("  Key field %d, datatype = %d\n", *i, static_cast<int>(dt));

	}
	ids.clear();
	ing_table->actionIdListGet( &ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		ing_table->actionNameGet(*i, &s);
		printf("  action: %d ", *i);
		std::cout << s << std::endl;
		action_id = *i;
	}
	ids.clear();
	ing_table->dataFieldIdListGet(action_id, &ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		bfrt::DataType t;
		size_t size;
		ing_table->dataFieldNameGet(*i, action_id, &s);
		ing_table->dataFieldDataTypeGet(*i, action_id, &t);
		ing_table->dataFieldSizeGet(*i, action_id, &size);
		printf("  data field %d: ", *i);
		std::cout << s << " " << static_cast<int>(t) << " " << size << std::endl;

	}
	ids.clear();
	bf_status = ing_table->dataAllocate(action_id, &table_data);
	assert(bf_status == BF_SUCCESS);
	for(tbl_entry=0; tbl_entry < switch_get_ig_m_table_0_entries; tbl_entry++)
	{
		bf_status = table_key->setValue(0x02, switch_get_ig_m_table_0_qid[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = table_key->setValue(0x01, switch_get_ig_m_table_0_portids[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = table_data->setValue(1, switch_get_ig_m_table_0_indices[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = ing_table->tableEntryAdd(*session, dev_tgt, *table_key, *table_data);
		assert(bf_status == BF_SUCCESS);
	}
    std::cout << "get_ig_m_table_0 entries added." << std::endl;
	////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////
    std::cout << "Populating get_ig_m_table_1 entries:" << std::endl;

	bf_status = bfrtInfo->bfrtTableFromNameGet("SwitchIngress.get_ig_m_table_1", &ing_table);
	assert(bf_status == BF_SUCCESS);
	bf_status = ing_table->keyAllocate(&table_key);
	assert(bf_status == BF_SUCCESS);

	ing_table->keyFieldIdListGet(&ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i) {
		bfrt::DataType dt;
		ing_table->keyFieldDataTypeGet(*i, &dt);
		printf("  Key field %d, datatype = %d\n", *i, static_cast<int>(dt));

	}
	ids.clear();
	ing_table->actionIdListGet( &ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		ing_table->actionNameGet(*i, &s);
		printf("  action: %d ", *i);
		std::cout << s << std::endl;
		action_id = *i;
	}
	ids.clear();
	ing_table->dataFieldIdListGet(action_id, &ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		bfrt::DataType t;
		size_t size;
		ing_table->dataFieldNameGet(*i, action_id, &s);
		ing_table->dataFieldDataTypeGet(*i, action_id, &t);
		ing_table->dataFieldSizeGet(*i, action_id, &size);
		printf("  data field %d: ", *i);
		std::cout << s << " " << static_cast<int>(t) << " " << size << std::endl;

	}
	ids.clear();
	bf_status = ing_table->dataAllocate(action_id, &table_data);
	assert(bf_status == BF_SUCCESS);
	for(tbl_entry=0; tbl_entry < switch_get_ig_m_table_1_entries; tbl_entry++)
	{
		bf_status = table_key->setValue(0x02, switch_get_ig_m_table_1_qid[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = table_key->setValue(0x01, switch_get_ig_m_table_1_portids[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = table_data->setValue(1, switch_get_ig_m_table_1_indices[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = ing_table->tableEntryAdd(*session, dev_tgt, *table_key, *table_data);
		assert(bf_status == BF_SUCCESS);
	}
    std::cout << "get_ig_m_table_1 entries added." << std::endl;
	////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////
    std::cout << "Populating set_ig_queue_length_table_0 entries:" << std::endl;

	bf_status = bfrtInfo->bfrtTableFromNameGet("SwitchIngress.set_ig_queue_length_table_0", &ing_table);
	assert(bf_status == BF_SUCCESS);
	bf_status = ing_table->keyAllocate(&table_key);
	assert(bf_status == BF_SUCCESS);

	ing_table->keyFieldIdListGet(&ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i) {
		bfrt::DataType dt;
		ing_table->keyFieldDataTypeGet(*i, &dt);
		printf("  Key field %d, datatype = %d\n", *i, static_cast<int>(dt));

	}
	ids.clear();
	action_id = 0;
	ing_table->actionIdListGet( &ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		ing_table->actionNameGet(*i, &s);
		printf("  action: %d ", *i);
		std::cout << s << std::endl;
		if(action_id == 0)
			action_id = *i;
	}
	ids.clear();
	ing_table->dataFieldIdListGet(action_id, &ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		bfrt::DataType t;
		size_t size;
		ing_table->dataFieldNameGet(*i, action_id, &s);
		ing_table->dataFieldDataTypeGet(*i, action_id, &t);
		ing_table->dataFieldSizeGet(*i, action_id, &size);
		printf("  data field %d: ", *i);
		std::cout << s << " " << static_cast<int>(t) << " " << size << std::endl;

	}
	ids.clear();
	bf_status = ing_table->dataAllocate(action_id, &table_data);
	assert(bf_status == BF_SUCCESS);
	for(tbl_entry=0; tbl_entry < switch_set_ig_queue_length_table_0_entries; tbl_entry++)
	{
		bf_status = table_key->setValue(0x02, switch_set_ig_queue_length_table_0_qid[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = table_key->setValue(0x01, switch_set_ig_queue_length_table_0_portids[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = table_data->setValue(1, switch_set_ig_queue_length_table_0_indices[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = ing_table->tableEntryAdd(*session, dev_tgt, *table_key, *table_data);
		assert(bf_status == BF_SUCCESS);
	}
    std::cout << "set_ig_queue_length_table_0 entries added." << std::endl;
	////////////////////////////////////////////////////////////////////
			////////////////////////////////////////////////////////////////////
    std::cout << "Populating set_ig_queue_length_table_1 entries:" << std::endl;

	bf_status = bfrtInfo->bfrtTableFromNameGet("SwitchIngress.set_ig_queue_length_table_1", &ing_table);
	assert(bf_status == BF_SUCCESS);
	bf_status = ing_table->keyAllocate(&table_key);
	assert(bf_status == BF_SUCCESS);

	ing_table->keyFieldIdListGet(&ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i) {
		bfrt::DataType dt;
		ing_table->keyFieldDataTypeGet(*i, &dt);
		printf("  Key field %d, datatype = %d\n", *i, static_cast<int>(dt));

	}
	ids.clear();
	ing_table->actionIdListGet( &ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		ing_table->actionNameGet(*i, &s);
		printf("  action: %d ", *i);
		std::cout << s << std::endl;
		action_id = *i;
	}
	ids.clear();
	ing_table->dataFieldIdListGet(action_id, &ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		bfrt::DataType t;
		size_t size;
		ing_table->dataFieldNameGet(*i, action_id, &s);
		ing_table->dataFieldDataTypeGet(*i, action_id, &t);
		ing_table->dataFieldSizeGet(*i, action_id, &size);
		printf("  data field %d: ", *i);
		std::cout << s << " " << static_cast<int>(t) << " " << size << std::endl;

	}
	ids.clear();
	bf_status = ing_table->dataAllocate(action_id, &table_data);
	assert(bf_status == BF_SUCCESS);
	for(tbl_entry=0; tbl_entry < switch_set_ig_queue_length_table_1_entries; tbl_entry++)
	{
		bf_status = table_key->setValue(0x02, switch_set_ig_queue_length_table_1_qid[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = table_key->setValue(0x01, switch_set_ig_queue_length_table_1_portids[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = table_data->setValue(1, switch_set_ig_queue_length_table_1_indices[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = ing_table->tableEntryAdd(*session, dev_tgt, *table_key, *table_data);
		assert(bf_status == BF_SUCCESS);
	}
    std::cout << "set_ig_queue_length_table_1 entries added." << std::endl;
	////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////
    std::cout << "Populating set_ig_m_table_0 entries:" << std::endl;

	bf_status = bfrtInfo->bfrtTableFromNameGet("SwitchIngress.set_ig_m_table_0", &ing_table);
	assert(bf_status == BF_SUCCESS);
	bf_status = ing_table->keyAllocate(&table_key);
	assert(bf_status == BF_SUCCESS);

	ing_table->keyFieldIdListGet(&ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i) {
		bfrt::DataType dt;
		ing_table->keyFieldDataTypeGet(*i, &dt);
		printf("  Key field %d, datatype = %d\n", *i, static_cast<int>(dt));

	}
	ids.clear();
	ing_table->actionIdListGet( &ids);
	action_id = 0;
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		ing_table->actionNameGet(*i, &s);
		printf("  action: %d ", *i);
		std::cout << s << std::endl;
		if(action_id == 0)
			action_id = *i;
			
	}
	ids.clear();
	ing_table->dataFieldIdListGet(action_id, &ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		bfrt::DataType t;
		size_t size;
		ing_table->dataFieldNameGet(*i, action_id, &s);
		ing_table->dataFieldDataTypeGet(*i, action_id, &t);
		ing_table->dataFieldSizeGet(*i, action_id, &size);
		printf("  data field %d: ", *i);
		std::cout << s << " " << static_cast<int>(t) << " " << size << std::endl;

	}
	ids.clear();
	bf_status = ing_table->dataAllocate(action_id, &table_data);
	assert(bf_status == BF_SUCCESS);
	for(tbl_entry=0; tbl_entry < switch_set_ig_m_table_0_entries; tbl_entry++)
	{
		bf_status = table_key->setValue(0x02, switch_set_ig_m_table_0_qid[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = table_key->setValue(0x01, switch_set_ig_m_table_0_portids[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = table_data->setValue(1, switch_set_ig_m_table_0_indices[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = ing_table->tableEntryAdd(*session, dev_tgt, *table_key, *table_data);
		assert(bf_status == BF_SUCCESS);
	}
    std::cout << "set_ig_m_table_0 entries added." << std::endl;
	////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////
    std::cout << "Populating set_ig_m_table_1 entries:" << std::endl;

	bf_status = bfrtInfo->bfrtTableFromNameGet("SwitchIngress.set_ig_m_table_1", &ing_table);
	assert(bf_status == BF_SUCCESS);
	bf_status = ing_table->keyAllocate(&table_key);
	assert(bf_status == BF_SUCCESS);

	ing_table->keyFieldIdListGet(&ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i) {
		bfrt::DataType dt;
		ing_table->keyFieldDataTypeGet(*i, &dt);
		printf("  Key field %d, datatype = %d\n", *i, static_cast<int>(dt));

	}
	ids.clear();
	ing_table->actionIdListGet( &ids);
	action_id = 0;
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		ing_table->actionNameGet(*i, &s);
		printf("  action: %d ", *i);
		std::cout << s << std::endl;
		if(action_id == 0)
			action_id = *i;
	}
	ids.clear();
	ing_table->dataFieldIdListGet(action_id, &ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		bfrt::DataType t;
		size_t size;
		ing_table->dataFieldNameGet(*i, action_id, &s);
		ing_table->dataFieldDataTypeGet(*i, action_id, &t);
		ing_table->dataFieldSizeGet(*i, action_id, &size);
		printf("  data field %d: ", *i);
		std::cout << s << " " << static_cast<int>(t) << " " << size << std::endl;

	}
	ids.clear();
	bf_status = ing_table->dataAllocate(action_id, &table_data);
	assert(bf_status == BF_SUCCESS);
	for(tbl_entry=0; tbl_entry < switch_set_ig_m_table_1_entries; tbl_entry++)
	{
		bf_status = table_key->setValue(0x02, switch_set_ig_m_table_1_qid[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = table_key->setValue(0x01, switch_set_ig_m_table_1_portids[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = table_data->setValue(1, switch_set_ig_m_table_1_indices[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = ing_table->tableEntryAdd(*session, dev_tgt, *table_key, *table_data);
		assert(bf_status == BF_SUCCESS);
	}
    std::cout << "set_ig_m_table_1 entries added." << std::endl;
	////////////////////////////////////////////////////////////////////
	// std::cout << "Populating get_rel_prio_table entries:" << std::endl;

	// bf_status = bfrtInfo->bfrtTableFromNameGet("SwitchIngress.get_rel_prio_table_0", &ing_table);
	// assert(bf_status == BF_SUCCESS);
	// bf_status = ing_table->keyAllocate(&table_key);
	// assert(bf_status == BF_SUCCESS);
	// ing_table->keyFieldIdListGet(&ids);
	// for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i) {
	// 	bfrt::DataType dt;
	// 	ing_table->keyFieldDataTypeGet(*i, &dt);
	// 	printf("  Key field %d, datatype = %d\n", *i, static_cast<int>(dt));
	// 	key_field_id = *i;
	// }
	// ids.clear();
	// ing_table->actionIdListGet(&ids);
	// action_id = 0;
	// for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
	// 	std::string s;
	// 	ing_table->actionNameGet(*i, &s);
	// 	printf("  action: %d ", *i);
	// 	std::cout << s << std::endl;
	// 	action_id = *i;
	// }
	// ids.clear();
	// ing_table->dataFieldIdListGet(&ids);
	// for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
	// 	std::string s;
	// 	bfrt::DataType t;
	// 	size_t size;
	// 	ing_table->dataFieldNameGet(*i, action_id, &s);
	// 	ing_table->dataFieldDataTypeGet(*i, action_id, &t);
	// 	ing_table->dataFieldSizeGet(*i, action_id, &size);
	// 	printf("  data field %d: ", *i);
	// 	std::cout << s << " " << static_cast<int>(t) << " " << size << std::endl;

	// }
	// ids.clear();
	// bf_status = ing_table->dataAllocate(action_id, &table_data);
	// assert(bf_status == BF_SUCCESS);

	// for(tbl_entry=0; tbl_entry < switch_get_rel_prio_table_entries_all; tbl_entry++)
	// {
	// 	bf_status = table_key->setValueRange(1, switch_get_rel_prio_table_m_starts[tbl_entry], switch_get_rel_prio_table_m_ends[tbl_entry]);
	// 	assert(bf_status == BF_SUCCESS);
	// 	bf_status = table_key->setValueRange(2, switch_get_rel_prio_table_rank_starts[tbl_entry], switch_get_rel_prio_table_rank_ends[tbl_entry]);
	// 	assert(bf_status == BF_SUCCESS);
	// 	bf_status = table_data->setValue(1, switch_get_rel_prio_table_rel_prios[tbl_entry]);
	// 	assert(bf_status == BF_SUCCESS);
	// 	bf_status = ing_table->tableEntryAdd(*session, dev_tgt, *table_key, *table_data);
	// 	assert(bf_status == BF_SUCCESS);
	// }
    // std::cout << "get_rel_prio_table entries added." << std::endl;
	///////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////
	// std::cout << "Populating get_rel_prio_table entries:" << std::endl;

	// bf_status = bfrtInfo->bfrtTableFromNameGet("SwitchIngress.get_rel_prio_table_1", &ing_table);
	// assert(bf_status == BF_SUCCESS);
	// bf_status = ing_table->keyAllocate(&table_key);
	// assert(bf_status == BF_SUCCESS);
	// ing_table->keyFieldIdListGet(&ids);
	// for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i) {
	// 	bfrt::DataType dt;
	// 	ing_table->keyFieldDataTypeGet(*i, &dt);
	// 	printf("  Key field %d, datatype = %d\n", *i, static_cast<int>(dt));
	// 	key_field_id = *i;
	// }
	// ids.clear();
	// ing_table->actionIdListGet(&ids);
	// action_id = 0;
	// for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
	// 	std::string s;
	// 	ing_table->actionNameGet(*i, &s);
	// 	printf("  action: %d ", *i);
	// 	std::cout << s << std::endl;
	// 	action_id = *i;
	// }
	// ids.clear();
	// ing_table->dataFieldIdListGet(&ids);
	// for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
	// 	std::string s;
	// 	bfrt::DataType t;
	// 	size_t size;
	// 	ing_table->dataFieldNameGet(*i, action_id, &s);
	// 	ing_table->dataFieldDataTypeGet(*i, action_id, &t);
	// 	ing_table->dataFieldSizeGet(*i, action_id, &size);
	// 	printf("  data field %d: ", *i);
	// 	std::cout << s << " " << static_cast<int>(t) << " " << size << std::endl;

	// }
	// ids.clear();
	// bf_status = ing_table->dataAllocate(action_id, &table_data);
	// assert(bf_status == BF_SUCCESS);

	// for(tbl_entry=0; tbl_entry < switch_get_rel_prio_table_entries_all; tbl_entry++)
	// {
	// 	bf_status = table_key->setValueRange(1, switch_get_rel_prio_table_m_starts[tbl_entry], switch_get_rel_prio_table_m_ends[tbl_entry]);
	// 	assert(bf_status == BF_SUCCESS);
	// 	bf_status = table_key->setValueRange(2, switch_get_rel_prio_table_rank_starts[tbl_entry], switch_get_rel_prio_table_rank_ends[tbl_entry]);
	// 	assert(bf_status == BF_SUCCESS);
	// 	bf_status = table_data->setValue(1, switch_get_rel_prio_table_rel_prios[tbl_entry]);
	// 	assert(bf_status == BF_SUCCESS);
	// 	bf_status = ing_table->tableEntryAdd(*session, dev_tgt, *table_key, *table_data);
	// 	assert(bf_status == BF_SUCCESS);
	// }
    // std::cout << "get_rel_prio_table entries added." << std::endl;
	///////////////////////////////////////////////////////////////////
	// 	////////////////////////////////////////////////////////////////////
	// std::cout << "Populating get_newm_table entries:" << std::endl;

	// bf_status = bfrtInfo->bfrtTableFromNameGet("SwitchEgress.get_newm_table", &ing_table);
	// assert(bf_status == BF_SUCCESS);
	// bf_status = ing_table->keyAllocate(&table_key);
	// assert(bf_status == BF_SUCCESS);
	// ing_table->keyFieldIdListGet(&ids);
	// for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i) {
	// 	bfrt::DataType dt;
	// 	ing_table->keyFieldDataTypeGet(*i, &dt);
	// 	printf("  Key field %d, datatype = %d\n", *i, static_cast<int>(dt));
	// 	key_field_id = *i;
	// }
	// ids.clear();
	// ing_table->actionIdListGet(&ids);
	// action_id = 0;
	// for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
	// 	std::string s;
	// 	ing_table->actionNameGet(*i, &s);
	// 	printf("  action: %d ", *i);
	// 	std::cout << s << std::endl;
	// 	if(action_id == 0)
	// 		action_id = *i;
	// }
	// ids.clear();
	// ing_table->dataFieldIdListGet(&ids);
	// for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
	// 	std::string s;
	// 	bfrt::DataType t;
	// 	size_t size;
	// 	ing_table->dataFieldNameGet(*i, action_id, &s);
	// 	ing_table->dataFieldDataTypeGet(*i, action_id, &t);
	// 	ing_table->dataFieldSizeGet(*i, action_id, &size);
	// 	printf("  data field %d: ", *i);
	// 	std::cout << s << " " << static_cast<int>(t) << " " << size << std::endl;

	// }
	// ids.clear();
	// bf_status = ing_table->dataAllocate(action_id, &table_data);
	// assert(bf_status == BF_SUCCESS);
	// bf_rt_id_t dataFieldId;
	// bf_status = ing_table->dataFieldIdGet("new_m", action_id, &dataFieldId);
	// assert(bf_status == BF_SUCCESS);
	// std::cout << "data field id is " << dataFieldId << std::endl;
	// for(tbl_entry=0; tbl_entry < switch_get_newm_table_entries_all; tbl_entry++)
	// {
	// 	std::cout << "Setting the following values:" << switch_get_newm_table_m_starts[tbl_entry] << " "
	// 		<< switch_get_newm_table_m_ends[tbl_entry] << " " << switch_get_newm_table_rank_starts[tbl_entry] << " " <<
	// 		 switch_get_newm_table_rank_ends[tbl_entry] << " value: " << switch_get_newm_table_newms[tbl_entry] << std::endl;
	// 	bf_status = table_key->setValueRange(1, switch_get_newm_table_m_starts[tbl_entry], switch_get_newm_table_m_ends[tbl_entry]);
	// 	assert(bf_status == BF_SUCCESS);
	// 	bf_status = table_key->setValueRange(2, switch_get_newm_table_rank_starts[tbl_entry], switch_get_newm_table_rank_ends[tbl_entry]);
	// 	assert(bf_status == BF_SUCCESS);
	// 	bf_status = table_data->setValue(dataFieldId, switch_get_newm_table_newms[tbl_entry]);
	// 	assert(bf_status == BF_SUCCESS);
	// 	bf_status = ing_table->tableEntryAdd(*session, dev_tgt, *table_key, *table_data);
	// 	std::cout << bf_status << std::endl;
	// 	assert(bf_status == BF_SUCCESS);	
	// }
    // std::cout << "get_newm_table entries added." << std::endl;
	// ///////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////
    std::cout << "Populating default_fw_table entries:" << std::endl;

	bf_status = bfrtInfo->bfrtTableFromNameGet("SwitchIngress.default_routing.default_fw_table", &ing_table);
	assert(bf_status == BF_SUCCESS);
	bf_status = ing_table->keyAllocate(&table_key);
	assert(bf_status == BF_SUCCESS);
	ing_table->keyFieldIdListGet(&ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i) {
		bfrt::DataType dt;
		ing_table->keyFieldDataTypeGet(*i, &dt);
		printf("  Key field %d, datatype = %d\n", *i, static_cast<int>(dt));
		key_field_id = *i;
	}
	ids.clear();
	ing_table->actionIdListGet(&ids);
	action_id = 0;
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		ing_table->actionNameGet(*i, &s);
		printf("  action: %d ", *i);
		std::cout << s << std::endl;
		action_id = *i;
	}
	ing_table->actionIdGet("SwitchIngress.default_routing.default_fw_action", &action_id);
	printf("  main action id: %d \n", action_id);
	ids.clear();
	ing_table->dataFieldIdListGet(action_id, &ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		bfrt::DataType t;
		size_t size;
		ing_table->dataFieldNameGet(*i, action_id, &s);
		ing_table->dataFieldDataTypeGet(*i, action_id, &t);
		ing_table->dataFieldSizeGet(*i, action_id, &size);
		printf("  data field %d: ", *i);
		data_entry_id = *i;
		std::cout << s << " " << static_cast<int>(t) << " " << size << std::endl;
	}
	ids.clear();
	bf_status = ing_table->dataAllocate(action_id, &table_data);
	assert(bf_status == BF_SUCCESS);
	for(tbl_entry=0; tbl_entry < switch_default_routing_entries; tbl_entry++)
	{
		inet_pton(AF_INET, switch_default_routing_entries_array_keys[tbl_entry].c_str(), &ip_addr);
		printf("Coverted IP addr value = %x\n", ntohl(ip_addr));
		bf_status = table_key->setValue(key_field_id, ntohl(ip_addr));
		assert(bf_status == BF_SUCCESS);
		bf_status = table_data->setValue(data_entry_id, switch_default_routing_destinations[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = ing_table->tableEntryAdd(*session, dev_tgt, *table_key, *table_data);
		assert(bf_status == BF_SUCCESS);
	}
    std::cout << "default_fw_table entries added." << std::endl;
	////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////
    std::cout << "Populating set_eg_queue_length_table entries:" << std::endl;

	bf_status = bfrtInfo->bfrtTableFromNameGet("SwitchEgress.set_eg_queue_length_table", &ing_table);
	assert(bf_status == BF_SUCCESS);
	bf_status = ing_table->keyAllocate(&table_key);
	assert(bf_status == BF_SUCCESS);

	ing_table->keyFieldIdListGet(&ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i) {
		bfrt::DataType dt;
		ing_table->keyFieldDataTypeGet(*i, &dt);
		printf("  Key field %d, datatype = %d\n", *i, static_cast<int>(dt));

	}
	ids.clear();
	ing_table->actionIdListGet( &ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		ing_table->actionNameGet(*i, &s);
		printf("  action: %d ", *i);
		std::cout << s << std::endl;
		action_id = *i;
	}
	ids.clear();
	ing_table->dataFieldIdListGet(action_id, &ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		bfrt::DataType t;
		size_t size;
		ing_table->dataFieldNameGet(*i, action_id, &s);
		ing_table->dataFieldDataTypeGet(*i, action_id, &t);
		ing_table->dataFieldSizeGet(*i, action_id, &size);
		printf("  data field %d: ", *i);
		std::cout << s << " " << static_cast<int>(t) << " " << size << std::endl;

	}
	ids.clear();
	bf_status = ing_table->dataAllocate(action_id, &table_data);
	assert(bf_status == BF_SUCCESS);
	for(tbl_entry=0; tbl_entry < switch_set_eg_queue_length_table_entries; tbl_entry++)
	{
		bf_status = table_key->setValue(0x02, switch_set_eg_queue_length_table_qid[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = table_key->setValue(0x01, switch_set_eg_queue_length_table_ports[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = table_data->setValue(1, switch_set_eg_queue_length_table_indices[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = ing_table->tableEntryAdd(*session, dev_tgt, *table_key, *table_data);
		assert(bf_status == BF_SUCCESS);
	}
    std::cout << "set_eg_queue_length_table entries added." << std::endl;
	////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////
    std::cout << "Populating get_eg_queue_length_table entries:" << std::endl;

	bf_status = bfrtInfo->bfrtTableFromNameGet("SwitchEgress.get_eg_queue_length_table", &ing_table);
	assert(bf_status == BF_SUCCESS);
	bf_status = ing_table->keyAllocate(&table_key);
	assert(bf_status == BF_SUCCESS);

	ing_table->keyFieldIdListGet(&ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i) {
		bfrt::DataType dt;
		ing_table->keyFieldDataTypeGet(*i, &dt);
		printf("  Key field %d, datatype = %d\n", *i, static_cast<int>(dt));

	}
	ids.clear();
	action_id = 0;
	ing_table->actionIdListGet( &ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		ing_table->actionNameGet(*i, &s);
		printf("  action: %d ", *i);
		std::cout << s << std::endl;
		if (action_id == 0)
			action_id = *i;
	}
	ids.clear();
	ing_table->dataFieldIdListGet(action_id, &ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		bfrt::DataType t;
		size_t size;
		ing_table->dataFieldNameGet(*i, action_id, &s);
		ing_table->dataFieldDataTypeGet(*i, action_id, &t);
		ing_table->dataFieldSizeGet(*i, action_id, &size);
		printf("  data field %d: ", *i);
		std::cout << s << " " << static_cast<int>(t) << " " << size << std::endl;

	}
	ids.clear();
	bf_status = ing_table->dataAllocate(action_id, &table_data);
	assert(bf_status == BF_SUCCESS);
	for(tbl_entry=0; tbl_entry < switch_get_eg_queue_length_table_entries; tbl_entry++)
	{
		bf_status = table_key->setValue(0x02, switch_get_eg_queue_length_table_qid[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = table_key->setValue(0x01, switch_get_eg_queue_length_table_ports[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = table_data->setValue(1, switch_get_eg_queue_length_table_indices[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = ing_table->tableEntryAdd(*session, dev_tgt, *table_key, *table_data);
		assert(bf_status == BF_SUCCESS);
	}
    std::cout << "get_eg_queue_length_table entries added." << std::endl;
	////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////
    std::cout << "Populating set_eg_m_table entries:" << std::endl;

	bf_status = bfrtInfo->bfrtTableFromNameGet("SwitchEgress.set_eg_m_table", &ing_table);
	assert(bf_status == BF_SUCCESS);
	bf_status = ing_table->keyAllocate(&table_key);
	assert(bf_status == BF_SUCCESS);

	ing_table->keyFieldIdListGet(&ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i) {
		bfrt::DataType dt;
		ing_table->keyFieldDataTypeGet(*i, &dt);
		printf("  Key field %d, datatype = %d\n", *i, static_cast<int>(dt));

	}
	ids.clear();
	ing_table->actionIdListGet( &ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		ing_table->actionNameGet(*i, &s);
		printf("  action: %d ", *i);
		std::cout << s << std::endl;
		action_id = *i;
	}
	ids.clear();
	ing_table->dataFieldIdListGet(action_id, &ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		bfrt::DataType t;
		size_t size;
		ing_table->dataFieldNameGet(*i, action_id, &s);
		ing_table->dataFieldDataTypeGet(*i, action_id, &t);
		ing_table->dataFieldSizeGet(*i, action_id, &size);
		printf("  data field %d: ", *i);
		std::cout << s << " " << static_cast<int>(t) << " " << size << std::endl;

	}
	ids.clear();
	bf_status = ing_table->dataAllocate(action_id, &table_data);
	assert(bf_status == BF_SUCCESS);
	for(tbl_entry=0; tbl_entry < switch_set_eg_m_table_entries; tbl_entry++)
	{
		bf_status = table_key->setValue(0x02, switch_set_eg_m_table_qid[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = table_key->setValue(0x01, switch_set_eg_m_table_ports[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = table_data->setValue(1, switch_set_eg_m_table_indices[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = ing_table->tableEntryAdd(*session, dev_tgt, *table_key, *table_data);
		assert(bf_status == BF_SUCCESS);
	}
    std::cout << "set_eg_m_table entries added." << std::endl;
	////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////
    std::cout << "Populating get_eg_m_table entries:" << std::endl;

	bf_status = bfrtInfo->bfrtTableFromNameGet("SwitchEgress.get_eg_m_table", &ing_table);
	assert(bf_status == BF_SUCCESS);
	bf_status = ing_table->keyAllocate(&table_key);
	assert(bf_status == BF_SUCCESS);

	ing_table->keyFieldIdListGet(&ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i) {
		bfrt::DataType dt;
		ing_table->keyFieldDataTypeGet(*i, &dt);
		printf("  Key field %d, datatype = %d\n", *i, static_cast<int>(dt));

	}
	ids.clear();
	action_id = 0;
	ing_table->actionIdListGet( &ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		ing_table->actionNameGet(*i, &s);
		printf("  action: %d ", *i);
		std::cout << s << std::endl;
		// if (action_id == 0)
		action_id = *i;
	}
	ids.clear();
	ing_table->dataFieldIdListGet(action_id, &ids);
	for (std::vector<bf_rt_id_t>::const_iterator i = ids.begin(); i != ids.end(); ++i){
		std::string s;
		bfrt::DataType t;
		size_t size;
		ing_table->dataFieldNameGet(*i, action_id, &s);
		ing_table->dataFieldDataTypeGet(*i, action_id, &t);
		ing_table->dataFieldSizeGet(*i, action_id, &size);
		printf("  data field %d: ", *i);
		std::cout << s << " " << static_cast<int>(t) << " " << size << std::endl;

	}
	ids.clear();
	bf_status = ing_table->dataAllocate(action_id, &table_data);
	assert(bf_status == BF_SUCCESS);
	for(tbl_entry=0; tbl_entry < switch_get_eg_m_table_entries; tbl_entry++)
	{
		bf_status = table_key->setValue(0x02, switch_get_eg_m_table_qid[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = table_key->setValue(0x01, switch_get_eg_m_table_ports[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = table_data->setValue(1, switch_get_eg_m_table_indices[tbl_entry]);
		assert(bf_status == BF_SUCCESS);
		bf_status = ing_table->tableEntryAdd(*session, dev_tgt, *table_key, *table_data);
		assert(bf_status == BF_SUCCESS);
	}
    std::cout << "get_eg_m_table entries added." << std::endl;
	////////////////////////////////////////////////////////////////////
	std::cout << "Finished populating all tables." << std::endl;
	std::cout << "Starting bfshell CLI ..." << std::endl;
    system(bfshell);

    return status;
}

int main(int argc, char **argv)
{
    char target_conf_file[100];
    char *progname = (char *) "distpd";
    // char *SDE = (char *) "/home/USER/bf-sde-9.7.0/";
    char *sde_install = (char *) "/home/USER/bf-sde-9.7.0/install";
    bf_status_t             status = 0;
    bf_switchd_context_t    *switchd_ctx;
	struct options opt;

    if(geteuid() != 0) {
        printf("error: Run the control plane as root!\n");
        exit(1);
    }

    if((switchd_ctx = (bf_switchd_context_t *) calloc(1, sizeof(bf_switchd_context_t))) == NULL)
    {
        printf("error: Failed to allocate Switchd context.\n");
        exit(1);
    }
    sprintf(target_conf_file, "%s/share/p4/targets/tofino/%s.conf", sde_install, progname);
    printf("Install: %s\nconf file: %s\n", sde_install, target_conf_file);

    switchd_ctx->install_dir = strdup(sde_install);
    switchd_ctx->conf_file = strdup(target_conf_file);
    // switchd_ctx->running_in_background = false;
    switchd_ctx->is_sw_model = false;
    switchd_ctx->is_asic = true;
    // switchd_ctx->shell_set_ucli = false;
    switchd_ctx->running_in_background = true;
    switchd_ctx->dev_sts_port = INIT_STATUS_TCP_PORT;
    switchd_ctx->dev_sts_thread = true;

    status = bf_switchd_lib_init(switchd_ctx);
    if(status != BF_SUCCESS)
    {
        printf("error: Failed to initialize the device. %s\n", bf_err_str(status));
        exit(1);
    }

	gethostname(opt.hostname, 100);
	printf("Running on %s.\n", opt.hostname);

    status = cp_run(switchd_ctx, &opt);

    if (switchd_ctx) 
        free(switchd_ctx);

    return status;
}