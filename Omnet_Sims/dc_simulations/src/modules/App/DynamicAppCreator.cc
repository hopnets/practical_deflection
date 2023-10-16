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

#include "DynamicAppCreator.h"

#include "inet/applications/common/SocketTag_m.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/lifecycle/ModuleOperations.h"
#include "inet/common/packet/Packet.h"
#include "inet/common/TimeTag_m.h"
#include <iostream>
#include <fstream>
#include <sqlite3.h>
#include <list>

using namespace inet;

#define MSGKIND_SEND_MICE_REQUEST       0
#define MSGKIND_SEND_BURSTY_REQUEST     1
#define MSGKIND_START_OPERATION         2


#define REQKIND_BURSTY      1
#define REQKIND_MICE        2

Define_Module(DynamicAppCreator);

int local_port_padding = 0;


DynamicAppCreator::~DynamicAppCreator()
{
    cancelAndDelete(timeoutMsg);
}

void DynamicAppCreator::initialize(int stage)
{
    EV << "SEPEHR: initialize in DynamicAppCreator called with stage " << stage << "!" << endl;
    if (stage == INITSTAGE_LOCAL) {
        timeoutMsg = new cMessage("timer");
        startTime = par("startTime");
        delay_between_bursts = par("delay_between_bursts");
        is_bursty = par("is_bursty");
        is_mice_background = par("is_mice_background") && (!is_bursty);
        is_elephant_background = par("is_elephant_background") && (!is_bursty) && (!is_mice_background);
        use_jitter = par("use_jitter");
        inter_arrival_time_multiplier = par("inter_arrival_time_multiplier");
        flow_size_multiplier = par("flow_size_multiplier");
        burst_start_time = par("burst_start_time");
        num_requests_per_burst = par("num_requests_per_burst");
        replyLength = par("replyLength");
        repetition_num = atoi(getEnvir()->getConfigEx()->getVariable(CFGVAR_REPETITION));
        app_index = getIndex();
        parent_index = getParentModule()->getIndex();
//        local_port_padding = 0;
    }
    simtime_t now = simTime();
    count_submodules();
    rescheduleOrDeleteTimer(std::max(startTime, now), MSGKIND_START_OPERATION);
}

void DynamicAppCreator::start_operation()
{
    EV << "start_operation called for server[" << parent_index << "].app[" << app_index << "]" << endl;
    std::string flow_multiplier = std::to_string(flow_size_multiplier);
    std::string inter_multiplier = std::to_string(inter_arrival_time_multiplier);
    std::string rep_num_string = std::to_string(repetition_num);
    std::string application_category = par("application_category");
    std::string inter_arrival_db_name = "distributions/" + application_category + "_inter_arrival_time_db_" + flow_multiplier + "_flowmult_" + inter_multiplier + "_intermult_" + rep_num_string + ".db";
    std::string flow_size_db_name = "distributions/" + application_category + "_flow_size_db_" + flow_multiplier + "_flowmult_" + inter_multiplier + "_intermult_" + rep_num_string + ".db";
    std::string background_server_db_name = "distributions/" + application_category + "_background_server_idx_db_" + flow_multiplier + "_flowmult_" + inter_multiplier + "_intermult_" + rep_num_string + ".db";
    std::string bursty_server_db_name = "distributions/" + application_category + "_bursty_server_idx_db_" + flow_multiplier + "_flowmult_" + inter_multiplier + "_intermult_" + rep_num_string + ".db";
    read_value_from_db(inter_arrival_db_name, flow_size_db_name, background_server_db_name,
            bursty_server_db_name, "inter_arrival", "flow_size", "background_server_idx", "bursty_server_idx");

    if (is_bursty) {
        rescheduleOrDeleteTimer(burst_start_time, MSGKIND_SEND_BURSTY_REQUEST);
    } else {
        rescheduleOrDeleteTimer(simTime() + get_inter_arrival_time(), MSGKIND_SEND_MICE_REQUEST);
    }
}

void DynamicAppCreator::sendRequest(int server_idx, int reply_length)
{
    EV << "SEPEHR: sendRequest called in DynamicAppCreator." << endl;

    std::string module_name_str = "server" + std::to_string(parent_index) + "app" + std::to_string(app_index) + "node" + std::to_string(local_port_padding);
    const char* module_name = module_name_str.c_str();
    cModuleType *moduleType = cModuleType::get("dc_simulations.modules.App.DCTcpBasicClientApp");
    cModule *module = moduleType->create(module_name, this);

//    std::cout << "our name is " << module_name << " and module name is " << module->getName() << endl;

    module->par("localAddress") = "";
    module->par("localPort") = 81 + local_port_padding;
    module->par("connectAddress") = "server[" + std::to_string(server_idx) + "]";
    module->par("connectPort") = 80;
    module->par("numRequestsPerSession") = 1;
    module->par("replyLength") = reply_length;
    module->par("requestLength") = par("requestLength");
    module->par("idleInterval") = 50.0;
    module->par("reconnectInterval") = 500.0;
    module->par("use_jitter") = par("use_jitter");

    cGate* open_in_gate = get_first_open_in_gate();
    cGate* open_out_gate = get_first_open_out_gate();

    module->gate("socketOut")->connectTo(open_in_gate);
    open_out_gate->connectTo(module->gate("socketIn"));

    module->finalizeParameters();
//    module->buildInside();
    for(int i=0; i< NUM_INIT_STAGES; i++) {
        module->callInitialize(i);
    }
    local_port_padding++;
}

void DynamicAppCreator::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        EV << "SEPEHR: handleMessage is called in DynamicAppCreator! msg is: " << msg->str() << endl;
        handleTimer(msg);
    }
}

void DynamicAppCreator::handleTimer(cMessage *msg)
{
    if (msg->getKind() == MSGKIND_START_OPERATION){
        start_operation();
    }
    else if (msg->getKind() == MSGKIND_SEND_MICE_REQUEST ||
            msg->getKind() == MSGKIND_SEND_BURSTY_REQUEST) {
        int run_num = 1;
        if (msg->getKind() == MSGKIND_SEND_BURSTY_REQUEST) {
            run_num = num_requests_per_burst;
        }
        for(int i = 0; i < run_num; i++) {
            sendRequest(get_server_idx(), get_flow_size());
        }
        count_submodules();
        rescheduleOrDeleteTimer(simTime() + get_inter_arrival_time(), msg->getKind());
    } else {
        throw cRuntimeError("Invalid timer msg: kind=%d", msg->getKind());
    }
}

void DynamicAppCreator::rescheduleOrDeleteTimer(simtime_t d, short int msgKind)
{
    EV << "SEPEHR: rescheduleOrDeleteTimer called for " << d << " in DynamicAppCreator." << endl;
    cancelEvent(timeoutMsg);
    timeoutMsg->setKind(msgKind);
    scheduleAt(d, timeoutMsg);
}


static int callback_inter_arrival(void *data, int argc, char **argv, char **azColName){
   int i;
   DynamicAppCreator *app_object = (DynamicAppCreator*) data;

   for(i = 0; i<argc; i++){
      if (std::strcmp(argv[i], "") != 0) {
          app_object->inter_arrival_times.push_back(std::stod(argv[i]));
      }
   }

   return 0;
}

static int callback_flow_size(void *data, int argc, char **argv, char **azColName){
   int i;
   DynamicAppCreator *app_object = (DynamicAppCreator*) data;

   for(i = 0; i<argc; i++){
      if (std::strcmp(argv[i], "") != 0) {
          app_object->flow_sizes.push_back(std::stoi(argv[i]));
      }
   }
   return 0;
}

static int callback_bursty_server(void *data, int argc, char **argv, char **azColName){
   int i;
   DynamicAppCreator *app_object = (DynamicAppCreator*) data;

   for(i = 0; i<argc; i++){
      if (std::strcmp(argv[i], "") != 0) {
          app_object->bursty_server_idx.push_back(std::stoi(argv[i]));
      }
   }
   return 0;
}

static int callback_background_server(void *data, int argc, char **argv, char **azColName){
   int i;
   DynamicAppCreator *app_object = (DynamicAppCreator*) data;

   for(i = 0; i<argc; i++){
      if (std::strcmp(argv[i], "") != 0) {
          app_object->background_server_idx.push_back(std::stoi(argv[i]));
      }
   }
   return 0;
}

void DynamicAppCreator::read_value_from_db(std::string inter_arrival_db_path, std::string flow_size_db_path,
        std::string background_server_db_path, std::string bursty_server_db_path,
        std::string inter_arrival_table_name, std::string flow_size_table_name,
        std::string background_server_table_name, std::string bursty_server_table_name)
{
    std::string column_name = "server" + std::to_string(parent_index) + "app" + std::to_string(app_index);

    sqlite3* DB;
    char *zErrMsg = 0;
    int rc;
    std::string sql;
    int exit = 0;


    if (is_bursty) {

        exit = sqlite3_open(bursty_server_db_path.c_str(), &DB);
        if (exit) {
            throw cRuntimeError(sqlite3_errmsg(DB));
            return;
        }
        sql = "SELECT " + column_name + " from " + bursty_server_table_name;
        rc = sqlite3_exec(DB, sql.c_str(), callback_bursty_server, (void*)this, &zErrMsg);

        if( rc != SQLITE_OK ) {
          sqlite3_free(zErrMsg);
          throw cRuntimeError("SQL error: %s\n", zErrMsg);
        }
        sqlite3_close(DB);

    }
    else {

        exit = sqlite3_open(inter_arrival_db_path.c_str(), &DB);
        if (exit) {
            throw cRuntimeError(sqlite3_errmsg(DB));
            return;
        }
        sql = "SELECT " + column_name + " from " + inter_arrival_table_name;
        rc = sqlite3_exec(DB, sql.c_str(), callback_inter_arrival, (void*)this, &zErrMsg);
        if( rc != SQLITE_OK ) {
          sqlite3_free(zErrMsg);
          throw cRuntimeError("SQL error: %s\n", zErrMsg);
        }
        sqlite3_close(DB);



        exit = sqlite3_open(flow_size_db_path.c_str(), &DB);
        if (exit) {
            throw cRuntimeError(sqlite3_errmsg(DB));
            return;
        }
        sql = "SELECT " + column_name + " from " + flow_size_table_name;
        rc = sqlite3_exec(DB, sql.c_str(), callback_flow_size, (void*)this, &zErrMsg);

        if( rc != SQLITE_OK ) {
          sqlite3_free(zErrMsg);
          throw cRuntimeError("SQL error: %s\n", zErrMsg);
        }
        sqlite3_close(DB);



        exit = sqlite3_open(background_server_db_path.c_str(), &DB);
        if (exit) {
            throw cRuntimeError(sqlite3_errmsg(DB));
            return;
        }
        sql = "SELECT " + column_name + " from " + background_server_table_name;
        rc = sqlite3_exec(DB, sql.c_str(), callback_background_server, (void*)this, &zErrMsg);

        if( rc != SQLITE_OK ) {
          sqlite3_free(zErrMsg);
          throw cRuntimeError("SQL error: %s\n", zErrMsg);
        }
        sqlite3_close(DB);

    }
}

int DynamicAppCreator::get_server_idx() {
    int server_idx;
    if (is_bursty) {
        if (bursty_server_idx.empty()) {
            throw cRuntimeError("You're trying to read from bursty_server_idx while it's empty");
        }
        server_idx = bursty_server_idx.front();
        bursty_server_idx.pop_front();
    } else {
        if (background_server_idx.empty()) {
            throw cRuntimeError("You're trying to read from background_server_idx while it's empty");
        }
        server_idx = background_server_idx.front();
        background_server_idx.pop_front();
    }
    return server_idx;
}

double DynamicAppCreator::get_inter_arrival_time() {
    if (is_bursty) {
        return delay_between_bursts;
    }
    else {
        if (inter_arrival_times.empty()) {
            throw cRuntimeError("You're trying to read from inter_arrival_times while it's empty");
        }
        double inter_arrival_time = inter_arrival_times.front();
        inter_arrival_times.pop_front();
        return inter_arrival_time;
    }
}

int DynamicAppCreator::get_flow_size() {
    if (is_bursty) {
        return replyLength;
    }
    else {
        if (flow_sizes.empty()) {
            throw cRuntimeError("You're trying to read from flow_sizes while it's empty");
        }
        int flow_size = flow_sizes.front();
        flow_sizes.pop_front();
        return flow_size;
    }
}

void DynamicAppCreator::count_submodules() {
    if (true) {
        int num_submodules = 0;
        for (cModule::SubmoduleIterator it(this); !it.end(); it++)
        {
            num_submodules++;
        }
        std::cout << "info for server[" << parent_index << "].app[" << app_index << "]" << endl;
        std::cout << "SEPEHR: Number of submodules is " << num_submodules << endl;
        auto dispatcher = check_and_cast<cSimpleModule *>(gate("socketIn")->getPathStartGate()->getOwnerModule());
        std::cout << "SEPEHR: Dispatcher gate sizes are: in -> " << dispatcher->gateSize("in")
                << ", out -> " << dispatcher->gateSize("out") << endl;
        std::cout << "SEPEHR: Sizes are: flow_sizes -> " << flow_sizes.size() << ", inter_arrival_times -> " << inter_arrival_times.size() <<
                ", background_server_idx -> " << background_server_idx.size() << ", burst_server_idx -> " << bursty_server_idx.size() << endl;
        std::cout << "***********************************************************" << endl;

    }
}

cGate* DynamicAppCreator::get_first_open_in_gate() {
    auto dispatcher = check_and_cast<cSimpleModule *>(gate("socketIn")->getPathStartGate()->getOwnerModule());
    for (int i = 0; i < dispatcher->gateSize("in"); i++) {
        if (!dispatcher->gate("in", i)->isConnected()) {
            return dispatcher->gate("in", i);
        }
    }

    dispatcher->setGateSize("in", dispatcher->gateSize("in") + 1);
    return dispatcher->gate("in", dispatcher->gateSize("in") - 1);
}

cGate* DynamicAppCreator::get_first_open_out_gate() {
    auto dispatcher = check_and_cast<cSimpleModule *>(gate("socketIn")->getPathStartGate()->getOwnerModule());
    for (int i = 0; i < dispatcher->gateSize("out"); i++) {
        if (!dispatcher->gate("out", i)->isConnected()) {
            return dispatcher->gate("out", i);
        }
    }

    dispatcher->setGateSize("out", dispatcher->gateSize("out") + 1);
    return dispatcher->gate("out", dispatcher->gateSize("out") - 1);
}
