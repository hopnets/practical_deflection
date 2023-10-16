
############### User-defined config ##########################################
def main():
    def devport(pipe, port):
        return ((pipe & 3) << 7) | (port & 0x7F)

    def pipeport(dp):
        return ((dp & 0x180) >> 7), (dp & 0x7F)

    def mcport(pipe, port):
        return pipe * 72 + port

    hw_devports_enabled = [128,144,160,176]
    PORT_NUM = 10
    RANDOM_NUM_MAX = 2**8 - 1
    dst_addrs = ["10.2.2.3", "10.2.2.5", "10.2.2.6", "10.2.2.101"]
    import ipaddress
    dest_addr_hex = [hex(int(ipaddress.ip_address(dst_addr))) for dst_addr in dst_addrs]

    p4 = bfrt.quantilepd.pipe

    ########## PM  ##############
    print("Setting up ports")
    for dp in hw_devports_enabled:
        bfrt.port.port.add(DEV_PORT=dp, AUTO_NEGOTIATION="PM_AN_FORCE_DISABLE", SPEED="BF_SPEED_40G", FEC="BF_FEC_TYP_NONE", PORT_ENABLE=True)
    print("================================")

    ################ BCAST ########################
    bfrt.pre.node.entry(MULTICAST_NODE_ID=1, MULTICAST_RID=0xFFFF,
                        MULTICAST_LAG_ID=[], DEV_PORT=hw_devports_enabled).push()
    bfrt.pre.mgid.entry(MGID=1,
                        MULTICAST_NODE_ID=[1],
                        MULTICAST_NODE_L1_XID_VALID=[False],
                        MULTICAST_NODE_L1_XID=[0]).push()
    print("Multicast domain 1 configured")
    for dp in hw_devports_enabled:
        print("Configuring port {} metadata".format(dp))
        l2_xid = mcport(*pipeport(dp))
        p4.SwitchIngressParser.PORT_METADATA.entry(ingress_port=dp, port_pcp=0, port_vid=0, l2_xid=l2_xid).push()
        print("Configuring port {} for broadcast pruning".format(dp))
        bfrt.pre.prune.entry(MULTICAST_L2_XID=l2_xid, DEV_PORT=[dp]).push()
    print("================================")

    ########## INGRESS ##########
    p4_ingress = p4.SwitchIngress
    p4_ingress.default_routing.default_fw_table.add_with_default_fw_action(dstAddr=dest_addr_hex[0], idx=128)
    p4_ingress.default_routing.default_fw_table.add_with_default_fw_action(dstAddr=dest_addr_hex[1], idx=144)
    p4_ingress.default_routing.default_fw_table.add_with_default_fw_action(dstAddr=dest_addr_hex[2], idx=160)
    p4_ingress.default_routing.default_fw_table.add_with_default_fw_action(dstAddr=dest_addr_hex[3], idx=176)
    
    
    p4_ingress.read_prefix_id_table.add_with_read_prefix_id_action(dstAddr=dest_addr_hex[0], prefix_id=0)
    p4_ingress.read_prefix_id_table.add_with_read_prefix_id_action(dstAddr=dest_addr_hex[1], prefix_id=1)
    p4_ingress.read_prefix_id_table.add_with_read_prefix_id_action(dstAddr=dest_addr_hex[2], prefix_id=2)
    p4_ingress.read_prefix_id_table.add_with_read_prefix_id_action(dstAddr=dest_addr_hex[3], prefix_id=3)
    p4_ingress.deflect_read_prefix_id_table.add_with_deflect_read_prefix_id_action(dstAddr=dest_addr_hex[0], deflect_prefix_id=1)
    p4_ingress.deflect_read_prefix_id_table.add_with_deflect_read_prefix_id_action(dstAddr=dest_addr_hex[1], deflect_prefix_id=1)
    p4_ingress.deflect_read_prefix_id_table.add_with_deflect_read_prefix_id_action(dstAddr=dest_addr_hex[2], deflect_prefix_id=1)
    p4_ingress.deflect_read_prefix_id_table.add_with_deflect_read_prefix_id_action(dstAddr=dest_addr_hex[3], deflect_prefix_id=1)

    p4_ingress.routing_quantilepd.random1_profile.add_with_get_port_0_idx_action(ACTION_MEMBER_ID=0, idx=128)
    p4_ingress.routing_quantilepd.random1_profile.add_with_get_port_0_idx_action(ACTION_MEMBER_ID=1, idx=144)
    p4_ingress.routing_quantilepd.random1_profile.add_with_get_port_0_idx_action(ACTION_MEMBER_ID=2, idx=160)
    p4_ingress.routing_quantilepd.random1_profile.add_with_get_port_0_idx_action(ACTION_MEMBER_ID=3, idx=176)
    # p4_ingress.routing_quantilepd.random1_action_selector.entry(SELECTOR_GROUP_ID=0, ACTION_MEMBER_ID=[0, 1, 2, 3], ACTION_MEMBER_STATUS=[True, True, True, True], MAX_GROUP_SIZE=8).push()
    p4_ingress.routing_quantilepd.random1_action_selector.entry(SELECTOR_GROUP_ID=0, ACTION_MEMBER_ID=[0], ACTION_MEMBER_STATUS=[True], MAX_GROUP_SIZE=8).push()
    p4_ingress.routing_quantilepd.random1_action_selector.entry(SELECTOR_GROUP_ID=1, ACTION_MEMBER_ID=[1], ACTION_MEMBER_STATUS=[True], MAX_GROUP_SIZE=8).push()
    p4_ingress.routing_quantilepd.random1_action_selector.entry(SELECTOR_GROUP_ID=2, ACTION_MEMBER_ID=[2], ACTION_MEMBER_STATUS=[True], MAX_GROUP_SIZE=8).push()
    p4_ingress.routing_quantilepd.random1_action_selector.entry(SELECTOR_GROUP_ID=3, ACTION_MEMBER_ID=[3], ACTION_MEMBER_STATUS=[True], MAX_GROUP_SIZE=8).push()

    p4_ingress.routing_quantilepd.get_port_0_idx_table.entry(prefix_id=0, SELECTOR_GROUP_ID=0).push()
    p4_ingress.routing_quantilepd.get_port_0_idx_table.entry(prefix_id=1, SELECTOR_GROUP_ID=1).push()
    p4_ingress.routing_quantilepd.get_port_0_idx_table.entry(prefix_id=2, SELECTOR_GROUP_ID=2).push()
    p4_ingress.routing_quantilepd.get_port_0_idx_table.entry(prefix_id=3, SELECTOR_GROUP_ID=3).push()

    p4_ingress.routing_quantilepd.random2_profile.add_with_get_port_1_idx_action(ACTION_MEMBER_ID=0, idx=128)
    p4_ingress.routing_quantilepd.random2_profile.add_with_get_port_1_idx_action(ACTION_MEMBER_ID=1, idx=144)
    p4_ingress.routing_quantilepd.random2_profile.add_with_get_port_1_idx_action(ACTION_MEMBER_ID=2, idx=160)
    p4_ingress.routing_quantilepd.random2_profile.add_with_get_port_1_idx_action(ACTION_MEMBER_ID=3, idx=176)
    # p4_ingress.routing_quantilepd.random2_action_selector.entry(SELECTOR_GROUP_ID=0, ACTION_MEMBER_ID=[0, 1, 2, 3], ACTION_MEMBER_STATUS=[True, True, True, True], MAX_GROUP_SIZE=8).push()
    p4_ingress.routing_quantilepd.random2_action_selector.entry(SELECTOR_GROUP_ID=0, ACTION_MEMBER_ID=[0], ACTION_MEMBER_STATUS=[True], MAX_GROUP_SIZE=8).push()
    p4_ingress.routing_quantilepd.random2_action_selector.entry(SELECTOR_GROUP_ID=1, ACTION_MEMBER_ID=[1], ACTION_MEMBER_STATUS=[True], MAX_GROUP_SIZE=8).push()
    p4_ingress.routing_quantilepd.random2_action_selector.entry(SELECTOR_GROUP_ID=2, ACTION_MEMBER_ID=[2], ACTION_MEMBER_STATUS=[True], MAX_GROUP_SIZE=8).push()
    p4_ingress.routing_quantilepd.random2_action_selector.entry(SELECTOR_GROUP_ID=3, ACTION_MEMBER_ID=[3], ACTION_MEMBER_STATUS=[True], MAX_GROUP_SIZE=8).push()
    p4_ingress.routing_quantilepd.get_port_1_idx_table.entry(prefix_id=0, SELECTOR_GROUP_ID=0).push()
    p4_ingress.routing_quantilepd.get_port_1_idx_table.entry(prefix_id=1, SELECTOR_GROUP_ID=1).push()
    p4_ingress.routing_quantilepd.get_port_1_idx_table.entry(prefix_id=2, SELECTOR_GROUP_ID=2).push()
    p4_ingress.routing_quantilepd.get_port_1_idx_table.entry(prefix_id=3, SELECTOR_GROUP_ID=3).push()

    p4_ingress.deflection_routing_quantilepd.deflect_random1_profile.add_with_deflect_get_port_0_idx_action(ACTION_MEMBER_ID=4, idx=140)
    p4_ingress.deflection_routing_quantilepd.deflect_random1_action_selector.entry(SELECTOR_GROUP_ID=1, ACTION_MEMBER_ID=[4], ACTION_MEMBER_STATUS=[True], MAX_GROUP_SIZE=8).push()
    p4_ingress.deflection_routing_quantilepd.deflect_get_port_0_idx_table.entry(deflect_prefix_id=1, SELECTOR_GROUP_ID=1).push()

    p4_ingress.deflection_routing_quantilepd.deflect_random2_profile.add_with_deflect_get_port_1_idx_action(ACTION_MEMBER_ID=4, idx=140)
    p4_ingress.deflection_routing_quantilepd.deflect_random2_action_selector.entry(SELECTOR_GROUP_ID=1, ACTION_MEMBER_ID=[4], ACTION_MEMBER_STATUS=[True], MAX_GROUP_SIZE=8).push()
    p4_ingress.deflection_routing_quantilepd.deflect_get_port_1_idx_table.entry(deflect_prefix_id=1, SELECTOR_GROUP_ID=1).push()

    p4_ingress.get_ig_queue_length_table_0.add_with_get_ig_queue_length_action_0(rand_port_0_idx=128, qid=0, index=0)
    p4_ingress.get_ig_queue_length_table_0.add_with_get_ig_queue_length_action_0(rand_port_0_idx=144, qid=0, index=1)
    p4_ingress.get_ig_queue_length_table_0.add_with_get_ig_queue_length_action_0(rand_port_0_idx=160, qid=0, index=2)
    p4_ingress.get_ig_queue_length_table_0.add_with_get_ig_queue_length_action_0(rand_port_0_idx=176, qid=0, index=3)

    p4_ingress.get_ig_queue_length_table_1.add_with_get_ig_queue_length_action_1(rand_port_1_idx=128, qid=0, index=0)
    p4_ingress.get_ig_queue_length_table_1.add_with_get_ig_queue_length_action_1(rand_port_1_idx=144, qid=0, index=1)
    p4_ingress.get_ig_queue_length_table_1.add_with_get_ig_queue_length_action_1(rand_port_1_idx=160, qid=0, index=2)
    p4_ingress.get_ig_queue_length_table_1.add_with_get_ig_queue_length_action_1(rand_port_1_idx=176, qid=0, index=3)

    p4_ingress.deflect_get_ig_queue_length_table_0.add_with_get_ig_queue_length_action_2(deflect_rand_port_0_idx=140, qid=0, index=4)

    p4_ingress.deflect_get_ig_queue_length_table_1.add_with_get_ig_queue_length_action_3(deflect_rand_port_1_idx=140, qid=0, index=4)

    p4_ingress.ig_mem_queue_len_reg.entry(REGISTER_INDEX=0, f1=0xffff).push()
    p4_ingress.ig_mem_queue_len_reg.entry(REGISTER_INDEX=1, f1=0xffff).push()
    p4_ingress.ig_mem_queue_len_reg.entry(REGISTER_INDEX=2, f1=0xffff).push()
    p4_ingress.ig_mem_queue_len_reg.entry(REGISTER_INDEX=3, f1=0xffff).push()
    p4_ingress.ig_mem_queue_len_reg.entry(REGISTER_INDEX=4, f1=0xffff).push()


    ########## EGRESS ##########
    p4_egress = p4.SwitchEgress

    p4_egress.set_eg_queue_length_table.add_with_set_eg_queue_length_action(egress_port=144, egress_qid=0, index=0)
    p4_egress.set_eg_queue_length_table.add_with_set_eg_queue_length_action(egress_port=128, egress_qid=0, index=1)
    p4_egress.set_eg_queue_length_table.add_with_set_eg_queue_length_action(egress_port=160, egress_qid=0, index=2)
    p4_egress.set_eg_queue_length_table.add_with_set_eg_queue_length_action(egress_port=186, egress_qid=0, index=3)
    p4_egress.set_eg_queue_length_table.add_with_set_eg_queue_length_action(egress_port=140, egress_qid=0, index=4)


main()
