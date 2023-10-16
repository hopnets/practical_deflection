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

#include "LongRunningMultiSocketTcpAppBase.h"

#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/transportlayer/contract/tcp/TcpSocket.h"
#include "inet/applications/common/SocketTag_m.h"

using namespace inet;

simsignal_t LongRunningMultiSocketTcpAppBase::connectSignal = registerSignal("connect");

LongRunningMultiSocketTcpAppBase::~LongRunningMultiSocketTcpAppBase() {
    for (int i = 0; i < socket_map_array_size; i++)
        for (auto socket_pair: socket_map_array[i].getMap())
            delete socket_pair.second;
}

void LongRunningMultiSocketTcpAppBase::initialize(int stage)
{
    EV << "SEPEHR: initialize called in LongRunningMultiSocketTcpAppBase." << endl;
    ApplicationBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        numSessions = numBroken = packetsSent = packetsRcvd = bytesSent = bytesRcvd = 0;

        WATCH(numSessions);
        WATCH(numBroken);
        WATCH(packetsSent);
        WATCH(packetsRcvd);
        WATCH(bytesSent);
        WATCH(bytesRcvd);


    }
    else if (stage == INITSTAGE_APPLICATION_LAYER) {
        // parameters
        int num_aggs = getAncestorPar("num_aggs").intValue();
        int num_servers_under_each_agg = getAncestorPar("num_servers").intValue();
        socket_map_array_size = num_aggs*num_servers_under_each_agg;
        socket_map_array = new SocketMap[socket_map_array_size];
    }
}

void LongRunningMultiSocketTcpAppBase::print_socket_map_info() {
    int socket_number = 0;
    for (int i = 0; i < socket_map_array_size; i++) {
        socket_number += socket_map_array[i].size();
    }
    EV << "Server[" << getParentModule()->getIndex() << "].app[" <<
            getIndex() << "] has " << socket_number << " sockets." << endl;
}

void LongRunningMultiSocketTcpAppBase::connect(int local_port, int dest_server_idx, int connect_port)
{
    EV << "SEPEHR: connect called in LongRunningMultiSocketTcpAppBase" << endl;
    TcpSocket* socket;
    socket = new TcpSocket();

    const char *localAddress = par("localAddress");
    socket->bind(*localAddress ? L3AddressResolver().resolve(localAddress) : L3Address(), local_port);
    socket->setCallback(this);
    socket->setOutputGate(gate("socketOut"));

    std::string connect_address_str = "server[" + std::to_string(dest_server_idx) + "]";
    const char* connect_address = connect_address_str.c_str();
    int timeToLive = par("timeToLive");
    if (timeToLive != -1)
        socket->setTimeToLive(timeToLive);

    int dscp = par("dscp");
    if (dscp != -1)
        socket->setDscp(dscp);

    // connect
    L3Address destination;
    L3AddressResolver().tryResolve(connect_address, destination);
    if (destination.isUnspecified()) {
        throw cRuntimeError("Connecting to %a port= %s: cannot resolve destination address\n", connect_address, connect_port);
    }
    else {
        bool is_server0 = getParentModule()->getIndex() == 0;
        EV << "Connecting to " << connect_address << "(" << destination << ") port=" << connect_port << endl;
        socket->set_in_use(true);
        socket_map_array[dest_server_idx].addSocket(socket);
        if (is_server0) {
            EV << "Connecting to " << connect_address << "(" << destination << ") port=" << connect_port << endl;
        }
        print_socket_map_info();
        socket->connect(destination, connect_port);
        numSessions++;
    }
}

void LongRunningMultiSocketTcpAppBase::handleMessageWhenUp(cMessage *msg)
{
    EV << "SEPEHR: Handling Message LongRunningMultiSocketTcpAppBaseTcpAppBase! msg is: " << msg->str() << endl;
    if (msg->isSelfMessage()) {
        EV << "SEPEHR: Calling handleTimer" << endl;
        handleTimer(msg);
    }

    else {
        EV << "SEPEHR: Calling socket.processMessage" << endl;

        TcpSocket *socket;

        for (int i = 0; i < socket_map_array_size; i++) {
            socket = check_and_cast_nullable<TcpSocket*>(socket_map_array[i].findSocketFor(msg));
            if (socket) {
                socket->processMessage(msg);
                return;
            }
        }

        throw cRuntimeError("message %s(%s) arrived for unknown socket 1\n", msg->getFullName(), msg->getClassName());
        delete msg;
    }
}

void LongRunningMultiSocketTcpAppBase::socketEstablished(TcpSocket *)
{
    // *redefine* to perform or schedule first sending
    EV_INFO << "connected\n";
}

void LongRunningMultiSocketTcpAppBase::close(TcpSocket* socket)
{
    EV_INFO << "issuing CLOSE command\n";

    socket->close();
}

TcpSocket* LongRunningMultiSocketTcpAppBase::is_any_socket_available(int dest_server_idx) {
    TcpSocket* socket;
    bool is_server0 = getParentModule()->getIndex() == 0;
    for (auto socket_pair: socket_map_array[dest_server_idx].getMap()) {
        socket = check_and_cast_nullable<TcpSocket*>(socket_pair.second);
        if (!socket->isOpen()) {
            throw cRuntimeError("A socket that is not open is included in the list!");
        }
        if (!socket->is_in_use()) {
            if (is_server0)
                EV << "Found a socket available for server[" << dest_server_idx <<
                        "] with socket ID " << socket->getSocketId() << endl;
            return socket;
        }
    }
    if (is_server0)
        EV << "Found no socket available for server[" << dest_server_idx << "]" << endl;
    return nullptr;
}

void LongRunningMultiSocketTcpAppBase::sendPacket(Packet *msg, TcpSocket* socket)
{
    if (socket) {
        delete msg->removeTagIfPresent<SocketInd>();
        EV << "SEPEHR: the packet is related to a socket with id " << socket->getSocketId() << endl;
        int numBytes = msg->getByteLength();
        emit(packetSentSignal, msg);
        socket->send(msg);

        packetsSent++;
        bytesSent += numBytes;
        return;
    }

    throw cRuntimeError("message %s(%s) arrived for unknown socket 2\n", msg->getFullName(), msg->getClassName());
    delete msg;

}

void LongRunningMultiSocketTcpAppBase::refreshDisplay() const
{
    ApplicationBase::refreshDisplay();
//    getDisplayString().setTagArg("t", 0, TcpSocket::stateName(socket.getState()));
}

void LongRunningMultiSocketTcpAppBase::socketDataArrived(TcpSocket *, Packet *msg, bool)
{
    // *redefine* to perform or schedule next sending
    packetsRcvd++;
    bytesRcvd += msg->getByteLength();\
    EV << "Emitting packetReceivedSignal with msg: " << msg << endl;
    emit(packetReceivedSignal, msg);
    delete msg;
}

void LongRunningMultiSocketTcpAppBase::socketPeerClosed(TcpSocket *socket_)
{
//    ASSERT(socket_ == &socket);
    // close the connection (if not already closed)
//    if (socket.getState() == TcpSocket::PEER_CLOSED) {
//        EV_INFO << "remote TCP closed, closing here as well\n";
//        close();
//    }
}

void LongRunningMultiSocketTcpAppBase::socketClosed(TcpSocket *socket)
{
    // *redefine* to start another session etc.
    EV_INFO << "connection closed\n";
    TcpSocket *found_socket;

    for (int i = 0; i < socket_map_array_size; i++) {
        found_socket = check_and_cast_nullable<TcpSocket*>(socket_map_array[i].removeSocket(socket));
        if (found_socket) {
            EV << "Socket found and removed from the list" << endl;
            return;
        }
    }

    throw cRuntimeError("Couldn't find socket anywhere!");
}

void LongRunningMultiSocketTcpAppBase::socketFailure(TcpSocket *, int code)
{
    // subclasses may override this function, and add code try to reconnect after a delay.
    EV_WARN << "connection broken\n";
    numBroken++;
}

void LongRunningMultiSocketTcpAppBase::finish()
{
    std::string modulePath = getFullPath();

    EV_INFO << modulePath << ": opened " << numSessions << " sessions\n";
    EV_INFO << modulePath << ": sent " << bytesSent << " bytes in " << packetsSent << " packets\n";
    EV_INFO << modulePath << ": received " << bytesRcvd << " bytes in " << packetsRcvd << " packets\n";
}

