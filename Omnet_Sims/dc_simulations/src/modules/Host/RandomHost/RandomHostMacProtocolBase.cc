//
// (C) 2013 Opensim Ltd.
//
// This library is free software, you can redistribute it
// and/or modify
// it under  the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation;
// either version 2 of the License, or any later version.
// The library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU Lesser General Public License for more details.
//
// Author: Andras Varga (andras@omnetpp.org)
//

#include "inet/common/IInterfaceRegistrationListener.h"
#include "inet/common/ModuleAccess.h"
#include "./RandomHostMacProtocolBase.h"

using namespace inet;

RandomHostMacProtocolBase::RandomHostMacProtocolBase()
{
}

RandomHostMacProtocolBase::~RandomHostMacProtocolBase()
{
    delete currentTxFrame;
    if (hostModule)
        hostModule->unsubscribe(interfaceDeletedSignal, this);
}

MacAddress RandomHostMacProtocolBase::parseMacAddressParameter(const char *addrstr)
{
    MacAddress address;

    if (!strcmp(addrstr, "auto"))
        // assign automatic address
        address = MacAddress::generateAutoAddress();
    else
        address.setAddress(addrstr);

    return address;
}

void RandomHostMacProtocolBase::initialize(int stage)
{
    LayeredProtocolBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        upperLayerInGateId = findGate("upperLayerIn");
        upperLayerOutGateId = findGate("upperLayerOut");
        lowerLayerInGateId = findGate("lowerLayerIn");
        lowerLayerOutGateId = findGate("lowerLayerOut");
        hostModule = findContainingNode(this);
        if (hostModule)
            hostModule->subscribe(interfaceDeletedSignal, this);
    }
    else if (stage == INITSTAGE_NETWORK_INTERFACE_CONFIGURATION)
        registerInterface();
}

void RandomHostMacProtocolBase::registerInterface()
{
    ASSERT(interfaceEntry == nullptr);
    interfaceEntry = getContainingNicModule(this);
    configureInterfaceEntry();
}

void RandomHostMacProtocolBase::sendUp(cMessage *message)
{
    if (message->isPacket())
        emit(packetSentToUpperSignal, message);
    send(message, upperLayerOutGateId);
}

void RandomHostMacProtocolBase::sendDown(cMessage *message)
{
    if (message->isPacket())
        emit(packetSentToLowerSignal, message);
    send(message, lowerLayerOutGateId);
}

bool RandomHostMacProtocolBase::isUpperMessage(cMessage *message)
{
    return message->getArrivalGateId() == upperLayerInGateId;
}

bool RandomHostMacProtocolBase::isLowerMessage(cMessage *message)
{
    return message->getArrivalGateId() == lowerLayerInGateId;
}

void RandomHostMacProtocolBase::deleteCurrentTxFrame()
{
    delete currentTxFrame;
    currentTxFrame = nullptr;
}

void RandomHostMacProtocolBase::dropCurrentTxFrame(PacketDropDetails& details)
{
    emit(packetDroppedSignal, currentTxFrame, &details);
    delete currentTxFrame;
    currentTxFrame = nullptr;
}

void RandomHostMacProtocolBase::popTxQueue()
{
    if (currentTxFrame != nullptr)
        throw cRuntimeError("Model error: incomplete transmission exists");
    ASSERT(txQueue != nullptr);
    auto first_packet = txQueue->getPacket(0);
    auto packet = txQueue->getPacket(0);
    std::string protocol = first_packet->getName();
    EV << "SEPEHR: First Packet's name is: " << protocol << endl;
    EV << "SEPEHR: There are " << txQueue->getNumPackets() << " packets in the queue." << endl;
    if (!(protocol.find("tcpseg") == std::string::npos)){
        int shouldBeRemovedFromQueueIdx = uniform(0, txQueue->getNumPackets());
        while(shouldBeRemovedFromQueueIdx >= 0) {
            packet = txQueue->getPacket(shouldBeRemovedFromQueueIdx);
            protocol = packet->getName();
            if (!(protocol.find("tcpseg") == std::string::npos))
                break;
            shouldBeRemovedFromQueueIdx--;
        }
        EV << "SEPEHR: Randomly removing packet number " << shouldBeRemovedFromQueueIdx << " of the queue." << endl;
        if(shouldBeRemovedFromQueueIdx != 0) {
            if (shouldBeRemovedFromQueueIdx - 1 != 0) {
                auto packet_after = txQueue->getPacket(shouldBeRemovedFromQueueIdx-1);
                currentTxFrame = packet;
                txQueue->removePacket(packet);
                txQueue->pushPacketAfter(packet_after, txQueue->popPacket());
            }
            else {
                currentTxFrame = packet;
                txQueue->removePacket(packet);
            }
        }
        else {
            currentTxFrame = txQueue->popPacket();
        }

    }
    else {
        EV << "SEPEHR: Popping packet regularly from the front." << endl;
        currentTxFrame = txQueue->popPacket();
    }
    take(currentTxFrame);
}

void RandomHostMacProtocolBase::flushQueue(PacketDropDetails& details)
{
    // code would look slightly nicer with a pop() function that returns nullptr if empty
    if (txQueue)
        while (!txQueue->isEmpty()) {
            auto packet = txQueue->popPacket();
            emit(packetDroppedSignal, packet, &details); //FIXME this signal lumps together packets from the network and packets from higher layers! separate them
            delete packet;
        }
}

void RandomHostMacProtocolBase::clearQueue()
{
    if (txQueue)
        while (!txQueue->isEmpty())
            delete txQueue->popPacket();
}

void RandomHostMacProtocolBase::handleMessageWhenDown(cMessage *msg)
{
    if (!msg->isSelfMessage() && msg->getArrivalGateId() == lowerLayerInGateId) {
        EV << "Interface is turned off, dropping packet\n";
        delete msg;
    }
    else
        LayeredProtocolBase::handleMessageWhenDown(msg);
}

void RandomHostMacProtocolBase::handleStartOperation(LifecycleOperation *operation)
{
    interfaceEntry->setState(InterfaceEntry::State::UP);
    interfaceEntry->setCarrier(true);
}

void RandomHostMacProtocolBase::handleStopOperation(LifecycleOperation *operation)
{
    PacketDropDetails details;
    details.setReason(INTERFACE_DOWN);
    if (currentTxFrame)
        dropCurrentTxFrame(details);
    flushQueue(details);
    interfaceEntry->setCarrier(false);
    interfaceEntry->setState(InterfaceEntry::State::DOWN);
}

void RandomHostMacProtocolBase::handleCrashOperation(LifecycleOperation *operation)
{
    deleteCurrentTxFrame();
    clearQueue();
    interfaceEntry->setCarrier(false);
    interfaceEntry->setState(InterfaceEntry::State::DOWN);
}

void RandomHostMacProtocolBase::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details)
{
    if (signalID == interfaceDeletedSignal) {
        if (interfaceEntry == check_and_cast<const InterfaceEntry *>(obj))
            interfaceEntry = nullptr;
    }
}

