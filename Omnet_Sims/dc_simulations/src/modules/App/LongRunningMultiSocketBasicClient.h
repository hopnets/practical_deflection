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

#ifndef __INET_LONGRUNNINGMULTISOCKETBASICCLIENT_H
#define __INET_LONGRUNNINGMULTISOCKETBASICCLIENT_H

#include "inet/common/INETDefs.h"
#include "unordered_map"

#include "inet/common/lifecycle/ILifecycle.h"
#include "inet/common/lifecycle/NodeStatus.h"
#include "base/LongRunningMultiSocketTcpAppBase.h"

using namespace inet;

/**
 * An example request-reply based client application.
 */
class INET_API LongRunningMultiSocketBasicClient : public LongRunningMultiSocketTcpAppBase
{
  protected:
//    cMessage *timeoutMsg = nullptr;
    bool earlySend = false;    // if true, don't wait with sendRequest() until established()
    int numRequestsToSend;    // requests to send in this session
    simtime_t startTime;
    simtime_t stopTime;
    simtime_t sendTime;
    bool use_jitter;
    simtime_t jitter = 0;
    int num_requests_per_burst;
    bool is_mice_background;
    bool is_elephant_background;
    double background_inter_arrival_time_multiplier, background_flow_size_multiplier;
    double bursty_inter_arrival_time_multiplier, bursty_flow_size_multiplier;
    int repetition_num, app_index, parent_index;
    long replyLength, requestLength;
    std::unordered_map<long, b> chunk_length_keeper;
    std::unordered_map<long, b> total_length_keeper;

    virtual void sendRequest(TcpSocket* socket);
    virtual void rescheduleOrDeleteTimer(simtime_t d, short int msgKind);

    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleTimer(cMessage *msg) override;

    virtual void socketEstablished(TcpSocket *socket) override;
    virtual void socketDataArrived(TcpSocket *socket, Packet *msg, bool urgent) override;
    virtual void socketClosed(TcpSocket *socket) override;
    virtual void socketFailure(TcpSocket *socket, int code) override;

    virtual void handleStartOperation(LifecycleOperation *operation) override;
    virtual void handleStopOperation(LifecycleOperation *operation) override;
    virtual void handleCrashOperation(LifecycleOperation *operation) override;

    virtual void close(TcpSocket* socket) override;
    virtual void finish() override;

    virtual void read_value_from_db(std::string inter_arrival_db_path, std::string flow_size_db_path,
                    std::string background_server_db_path, std::string bursty_server_db_path,
                    std::string background_flow_ids_db_path, std::string bursty_flow_ids_db_path,
                    std::string inter_arrival_table_name, std::string flow_size_table_name,
                    std::string background_server_table_name, std::string bursty_server_table_name,
                    std::string background_flow_ids_table_name, std::string bursty_flow_ids_table_name);
    virtual double get_inter_arrival_time();
    virtual int get_flow_size();
    virtual int get_server_idx();
    virtual unsigned long get_flow_id();
    virtual void connect_or_send_for_bursty_request();
    virtual void connect_or_send_for_background_request();
    virtual int get_local_port();

  public:
    LongRunningMultiSocketBasicClient() {}
    virtual ~LongRunningMultiSocketBasicClient();
    static simsignal_t flowEndedSignal;
    static simsignal_t flowStartedSignal;
    static simsignal_t actualFlowStartedTimeSignal;
    static simsignal_t requestSentSignal;
    static simsignal_t notJitteredRequestSentSignal;
    static simsignal_t replyLengthsSignal;
    static simsignal_t chunksReceivedLengthSignal;
    static simsignal_t chunksReceivedTotalLengthSignal;
    std::list <int> flow_sizes, background_server_idx, bursty_server_idx;
    std::list <double> inter_arrival_times;
    std::list <unsigned long> background_flow_ids, bursty_flow_ids;
    bool is_bursty;
};

#endif

