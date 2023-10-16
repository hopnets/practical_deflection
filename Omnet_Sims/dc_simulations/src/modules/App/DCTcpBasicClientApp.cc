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

#include "./DCTcpBasicClientApp.h"
#include "inet/applications/common/SocketTag_m.h"


#include "inet/applications/tcpapp/GenericAppMsg_m.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/lifecycle/ModuleOperations.h"
#include "inet/common/packet/Packet.h"
#include "inet/common/TimeTag_m.h"
#include <iostream>
#include <fstream>

using namespace inet;

#define MSGKIND_CONNECT    0
#define MSGKIND_SEND       1

Define_Module(DCTcpBasicClientApp);

simsignal_t DCTcpBasicClientApp::flowEndedSignal = registerSignal("flowEnded");
simsignal_t DCTcpBasicClientApp::flowStartedSignal = registerSignal("flowStarted");
simsignal_t DCTcpBasicClientApp::requestSentSignal = registerSignal("requestSent");
simsignal_t DCTcpBasicClientApp::notJitteredRequestSentSignal = registerSignal("notJitteredRequestSent");
simsignal_t DCTcpBasicClientApp::replyLengthsSignal = registerSignal("replyLengths");


DCTcpBasicClientApp::~DCTcpBasicClientApp()
{
    cancelAndDelete(timeoutMsg);
}

void DCTcpBasicClientApp::initialize(int stage)
{
    EV << "SEPEHR: initialize in DCTcpBasicClientApp called with stage " << stage << "!" << endl;
    TcpAppBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        numRequestsToSend = 0;
        earlySend = false;    // TBD make it parameter
        WATCH(numRequestsToSend);
        WATCH(earlySend);

        startTime = par("startTime");
        stopTime = par("stopTime");
        sendTime = par("sendTime");
        use_jitter = par("use_jitter");
        if (stopTime >= SIMTIME_ZERO && stopTime < startTime)
            throw cRuntimeError("Invalid startTime/stopTime parameters");
        timeoutMsg = new cMessage("timer");
    }
}

void DCTcpBasicClientApp::handleStartOperation(LifecycleOperation *operation)
{
    EV << "SEPEHR: handleStartOperation called." << endl;

    simtime_t now = simTime();
    simtime_t start = std::max(startTime, now);
    if (timeoutMsg && ((stopTime < SIMTIME_ZERO) || (start < stopTime) || (start == stopTime && startTime == stopTime))) {
        timeoutMsg->setKind(MSGKIND_CONNECT);
        scheduleAt(start, timeoutMsg);
    }
}

void DCTcpBasicClientApp::handleStopOperation(LifecycleOperation *operation)
{
    cancelEvent(timeoutMsg);
    if (socket.getState() == TcpSocket::CONNECTED || socket.getState() == TcpSocket::CONNECTING || socket.getState() == TcpSocket::PEER_CLOSED)
        close();
}

void DCTcpBasicClientApp::handleCrashOperation(LifecycleOperation *operation)
{
    cancelEvent(timeoutMsg);
    if (operation->getRootModule() != getContainingNode(this))
        socket.destroy();
}


void DCTcpBasicClientApp::socketEstablished(TcpSocket *socket)
{

//        stopTime = SimTime::parse(getEnvir()->getConfig()->getConfigValue("sim-time-limit")).inUnit(SIMTIME_S) + 1;
//            int num_aggs = getAncestorPar("num_aggs").intValue();
//            int num_spines = getAncestorPar("num_spines").intValue();
//
//            cModule *agg_queue_module = getModuleByPath("agg[0].eth[0].mac.queue");
//            int agg_queue_capacity = (agg_queue_module->par("packetCapacity")).intValue();
//            int repetition_num = atoi(getEnvir()->getConfigEx()->getVariable(CFGVAR_REPETITION));

    TcpAppBase::socketEstablished(socket);
    EV << "SEPEHR: socket established!" << endl;
    // determine number of requests in this session
    numRequestsToSend = par("numRequestsPerSession");
    if (numRequestsToSend < 1)
        numRequestsToSend = 1;

    // perform first request if not already done (next one will be sent when reply arrives)
    if (!earlySend) {
        EV << "SEPEHR: SendRequest Called in socketEstablished." << endl;
        timeoutMsg->setKind(MSGKIND_SEND);
        simtime_t d = simTime();
        if(use_jitter) {
            jitter = normal(0.001, 0.0001);
            emit(notJitteredRequestSentSignal, std::max(d, sendTime));
        }
        EV << "SEPEHR: jitter is: " << jitter << endl;
        EV << "SEPEHR: Scheduling a send for " << std::max(d, sendTime) + jitter << endl;
        rescheduleOrDeleteTimer(std::max(d, sendTime) + jitter, MSGKIND_SEND);

    } else {
        numRequestsToSend--;
    }
}

void DCTcpBasicClientApp::sendRequest()
{
    long requestLength = par("requestLength");
    long replyLength = par("replyLength");
    if (requestLength < 1)
        requestLength = 1;
    if (replyLength < 1)
        replyLength = 1;

    const auto& payload = makeShared<GenericAppMsg>();
    Packet *packet = new Packet("data");
    payload->setChunkLength(B(requestLength));
    payload->setExpectedReplyLength(B(replyLength));
    payload->setServerClose(false);
    payload->addTag<CreationTimeTag>()->setCreationTime(simTime());
    payload->setRequesterID(packet->getId());
    packet->insertAtBack(payload);

    EV_INFO << "sending request with " << requestLength << " bytes, expected reply length " << replyLength << " bytes,"
            << "remaining " << numRequestsToSend - 1 << " request\n";
    EV << "SEPEHR: sending request with request ID: " << payload->getRequesterID() << endl;
    emit(DCTcpBasicClientApp::requestSentSignal, payload->getRequesterID());
    emit(DCTcpBasicClientApp::replyLengthsSignal, replyLength);
    sendPacket(packet);
}

void DCTcpBasicClientApp::handleTimer(cMessage *msg)
{
    switch (msg->getKind()) {
        case MSGKIND_CONNECT:
            connect();    // active OPEN

            // significance of earlySend: if true, data will be sent already
            // in the ACK of SYN, otherwise only in a separate packet (but still
            // immediately)
            if (earlySend) {
                EV << "SEPEHR: SendRequest Called in earlySend." << endl;
                sendRequest();
            }
            break;

        case MSGKIND_SEND:
            EV << "SEPEHR: SendRequest Called." << endl;
            sendRequest();
            numRequestsToSend--;

            if (numRequestsToSend > 0) {
                throw cRuntimeError("numRequestsToSend larger than 1? Are you sure?");
            }
            // no scheduleAt(): next request will be sent when reply to this one
            // arrives (see socketDataArrived())
            break;

        default:
            throw cRuntimeError("Invalid timer msg: kind=%d", msg->getKind());
    }
}


void DCTcpBasicClientApp::rescheduleOrDeleteTimer(simtime_t d, short int msgKind)
{
    cancelEvent(timeoutMsg);

    if (stopTime < SIMTIME_ZERO || d < stopTime) {
        timeoutMsg->setKind(msgKind);
        EV << "SEPEHR: rescheduleOrDeleteTimer is called to schedule to send a packet at " << d << "s" << endl;
        scheduleAt(d, timeoutMsg);
    }
    else {
        delete timeoutMsg;
        timeoutMsg = nullptr;
    }
}

void DCTcpBasicClientApp::socketDataArrived(TcpSocket *socket, Packet *msg, bool urgent)
{
    EV << "SEPEHR: Message rcved: " << msg << endl;
    auto msg_dup = msg->dup();
    auto chunk = msg_dup->removeAtFront<SliceChunk>();
    bool was_the_last_packet = false;
    while (true) {
        auto chunk_length = chunk->getLength();
        auto chunk_offset = chunk->getOffset();
        auto main_chunk = chunk->getChunk();
        auto total_length = main_chunk->getChunkLength();
        if (chunk_offset == b(0)) {
            Packet* temp = new Packet();
            temp->insertAtBack(main_chunk);
            auto payload = temp->popAtFront<GenericAppMsg>();
            emit(DCTcpBasicClientApp::flowStartedSignal, payload->getRequesterID());
            delete temp;
        }
        if (chunk_length + chunk_offset == total_length) {
            Packet* temp = new Packet();
            temp->insertAtBack(main_chunk);
            auto payload = temp->popAtFront<GenericAppMsg>();
            emit(DCTcpBasicClientApp::flowEndedSignal, payload->getRequesterID());
            was_the_last_packet = true;
            delete temp;
        }
        if (msg_dup->getByteLength() == 0)
            break;
        chunk = msg_dup->removeAtFront<SliceChunk>();
    }

    delete msg_dup;
    TcpAppBase::socketDataArrived(socket, msg, urgent);


    if (numRequestsToSend > 0) {
        EV_INFO << "reply arrived\n";
    }
    else if (was_the_last_packet) {
        EV_INFO << "reply to last request arrived, closing session\n";
        close();
    }
    else if (simTime() >= stopTime && socket->getState() != TcpSocket::LOCALLY_CLOSED) {
        close();
    }
}

void DCTcpBasicClientApp::close()
{
    EV << "SEPEHR: DCTcpBasicClientApp::close called!" << endl;
    TcpAppBase::close();
    cancelEvent(timeoutMsg);
}

void DCTcpBasicClientApp::socketClosed(TcpSocket *socket)
{
    EV << "SEPEHR: DCTcpBasicClientApp::socketClosed called!" << endl;
    TcpAppBase::socketClosed(socket);
    gate("socketOut")->disconnect();
    gate("socketIn")->disconnect();
    finish();
    deleteModule();

    // start another session after a delay
//    if (timeoutMsg) {
//        simtime_t d = simTime() + par("idleInterval");
//        rescheduleOrDeleteTimer(d, MSGKIND_CONNECT);
//    }
}

void DCTcpBasicClientApp::socketFailure(TcpSocket *socket, int code)
{
    EV << "SEPEHR: DCTcpBasicClientApp::socketFailure called!" << endl;
    TcpAppBase::socketFailure(socket, code);

    // reconnect after a delay
    if (timeoutMsg) {
        simtime_t d = simTime() + par("reconnectInterval");
        rescheduleOrDeleteTimer(d, MSGKIND_CONNECT);
    }
}

