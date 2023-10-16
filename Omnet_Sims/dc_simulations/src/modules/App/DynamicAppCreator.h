//
// Copyright (C) 2004 Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#ifndef __INET_DYNAMICAPPCREATOR_H
#define __INET_DYNAMICAPPCREATOR_H

#include "inet/common/INETDefs.h"

#include "inet/applications/tcpapp/TcpAppBase.h"
#include "inet/common/lifecycle/ILifecycle.h"
#include "inet/common/lifecycle/NodeStatus.h"

using namespace inet;

/**
 * An example request-reply based client application.
 */
class INET_API DynamicAppCreator : public cSimpleModule
{
  protected:
    cMessage *timeoutMsg = nullptr;
    simtime_t startTime;
    double delay_between_bursts;
    double burst_start_time;
    bool use_jitter;
    int num_requests_per_burst;
    bool is_mice_background, is_elephant_background, is_bursty;
    double inter_arrival_time_multiplier, flow_size_multiplier;
    int repetition_num, app_index, parent_index;
    long replyLength;

    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;

    virtual void sendRequest(int server_idx, int reply_length);
    virtual void start_operation();
    virtual void rescheduleOrDeleteTimer(simtime_t d, short int msgKind);
    virtual void handleTimer(cMessage *msg);
    virtual void read_value_from_db(std::string inter_arrival_db_path, std::string flow_size_db_path,
            std::string background_server_db_path, std::string bursty_server_db_path,
            std::string inter_arrival_table_name, std::string flow_size_table_name,
            std::string background_server_table_name, std::string bursty_server_table_name);
    virtual double get_inter_arrival_time();
    virtual int get_flow_size();
    virtual int get_server_idx();
    virtual cGate* get_first_open_in_gate();
    virtual cGate* get_first_open_out_gate();

  public:
    DynamicAppCreator() {}
    virtual ~DynamicAppCreator();
    std::list <int> flow_sizes, background_server_idx, bursty_server_idx;
    std::list <double> inter_arrival_times;

    virtual void count_submodules();
};

#endif

