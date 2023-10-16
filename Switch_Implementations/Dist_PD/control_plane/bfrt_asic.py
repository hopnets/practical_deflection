# ############### User-defined config ##########################################
def main():
    import math
    p4 = bfrt.distpd.pipe

    ########## INGRESS ##########
    p4_ingress = p4.SwitchIngress

    ########## EGRESS ##########
    p4_egress = p4.SwitchEgress

    data = []
    C = 1024
    alpha = 0.9
    MULTIPLIER = 8
    for i in range(10):
        data.append([])
        for j in range(10):
            mid1 = (5*i) + 2
            mid2 = (5*j) + 2
            rel_prio = MULTIPLIER * math.floor(C*alpha*(1-math.exp(-(mid1/mid2))))
            print(mid1, mid2, rel_prio)
            data[i].append(rel_prio)
    for i in range(10):
        data.append([])
        for j in range(10):
            s1 = (5*i)
            e1 = (5*(i+1)) - 1
            s2 = (5*j)
            e2 = (5*(j+1)) - 1
            print(s1,e1,s2,e2, data[i][j])  # multiply start and end to 1000
            p4_ingress.get_rel_prio_table_0.add_with_get_rel_prio_action_0(rank_19_0__start=s1, rank_19_0__end=e1, m_19_0__start=s2, m_19_0__end=e2, rel_prio=data[i][j])
            p4_ingress.get_rel_prio_table_1.add_with_get_rel_prio_action_1(rank_19_0__start=s1, rank_19_0__end=e1, deflect_m_19_0__start=s2, deflect_m_19_0__end=e2, rel_prio=data[i][j])

    data2 = []
    N = 50

    for i in range(10):
        data2.append([])
        for j in range(50):
            mid1 = (5*i) + 2
            mid2 = j #(5*j) + 2
            newM =  math.ceil((((N - 1)*mid2 + mid1) * 1.0) / N)
            print(mid1, mid2, newM)
            data2[i].append(newM)
    for i in range(10):
        data2.append([])
        for j in range(50):
            s1 = (5*i)
            e1 = (5*(i+1)) - 1
            s2 = j #(5*j)
            e2 = j #(5*(j+1)) - 1
            print(s1,e1,s2,e2, data2[i][j])
            p4_egress.get_newm_table.add_with_get_newm_action(rank_19_0__start=s1, rank_19_0__end=e1, M_19_0__start=s2, M_19_0__end=e2, new_m=data2[i][j])


main()