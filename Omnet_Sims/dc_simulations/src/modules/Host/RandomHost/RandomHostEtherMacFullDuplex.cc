//
// Copyright (C) 2006 Levente Meszaros
// Copyright (C) 2011 Zoltan Bojthe
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

#include "inet/common/Simsignals.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/linklayer/ethernet/EtherEncap.h"
#include "inet/linklayer/ethernet/EtherFrame_m.h"
#include "./RandomHostEtherMacFullDuplex.h"
#include "inet/linklayer/ethernet/EtherPhyFrame_m.h"
#include "inet/networklayer/common/InterfaceEntry.h"

using namespace inet;

// TODO: refactor using a statemachine that is present in a single function
// TODO: this helps understanding what interactions are there and how they affect the state

Define_Module(RandomHostEtherMacFullDuplex);

RandomHostEtherMacFullDuplex::RandomHostEtherMacFullDuplex()
{
}

void RandomHostEtherMacFullDuplex::initialize(int stage)
{
    RandomHostEtherMacBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        if (!par("duplexMode"))
            throw cRuntimeError("Half duplex operation is not supported by RandomHostEtherMacFullDuplex, use the EtherMac module for that! (Please enable csmacdSupport on EthernetInterface)");
    }
    else if (stage == INITSTAGE_LINK_LAYER) {
        beginSendFrames();    //FIXME choose an another stage for it
    }
}

void RandomHostEtherMacFullDuplex::initializeStatistics()
{
    RandomHostEtherMacBase::initializeStatistics();

    // initialize statistics
    totalSuccessfulRxTime = 0.0;
}

void RandomHostEtherMacFullDuplex::initializeFlags()
{
    RandomHostEtherMacBase::initializeFlags();

    duplexMode = true;
    physInGate->setDeliverOnReceptionStart(false);
}

void RandomHostEtherMacFullDuplex::handleMessageWhenUp(cMessage *msg)
{
    if (channelsDiffer)
        readChannelParameters(true);

    if (msg->isSelfMessage())
        handleSelfMessage(msg);
    else if (msg->getArrivalGateId() == upperLayerInGateId)
        handleUpperPacket(check_and_cast<Packet *>(msg));
    else if (msg->getArrivalGate() == physInGate)
        processMsgFromNetwork(check_and_cast<inet::EthernetSignal *>(msg));
    else
        throw cRuntimeError("Message received from unknown gate!");
    processAtHandleMessageFinished();
}

void RandomHostEtherMacFullDuplex::handleSelfMessage(cMessage *msg)
{
    EV_TRACE << "Self-message " << msg << " received\n";

    if (msg == endTxMsg)
        handleEndTxPeriod();
    else if (msg == endIFGMsg)
        handleEndIFGPeriod();
    else if (msg == endPauseMsg)
        handleEndPausePeriod();
    else
        throw cRuntimeError("Unknown self message received!");
}

void RandomHostEtherMacFullDuplex::startFrameTransmission()
{
    ASSERT(currentTxFrame);
    EV_DETAIL << "Transmitting a copy of frame " << currentTxFrame << endl;

    Packet *frame = currentTxFrame->dup();    // note: we need to duplicate the frame because we emit a signal with it in endTxPeriod()
    const auto& hdr = frame->peekAtFront<EthernetMacHeader>();    // note: we need to duplicate the frame because we emit a signal with it in endTxPeriod()
    ASSERT(hdr);
    ASSERT(!hdr->getSrc().isUnspecified());

    // add preamble and SFD (Starting Frame Delimiter), then send out
    encapsulate(frame);

    // send
    EV_INFO << "Transmission of " << frame << " started.\n";
    auto oldPacketProtocolTag = frame->removeTag<PacketProtocolTag>();
    frame->clearTags();
    auto newPacketProtocolTag = frame->addTag<PacketProtocolTag>();
    *newPacketProtocolTag = *oldPacketProtocolTag;
    delete oldPacketProtocolTag;
    auto signal = new inet::EthernetSignal(frame->getName());
    signal->setSrcMacFullDuplex(duplexMode);
    if (sendRawBytes) {
        signal->encapsulate(new Packet(frame->getName(), frame->peekAllAsBytes()));
        delete frame;
    }
    else
        signal->encapsulate(frame);
    send(signal, physOutGate);

    scheduleAt(transmissionChannel->getTransmissionFinishTime(), endTxMsg);
    changeTransmissionState(TRANSMITTING_STATE);
}

void RandomHostEtherMacFullDuplex::handleUpperPacket(Packet *packet)
{
    EV_INFO << "Received " << packet << " from upper layer." << endl;

    numFramesFromHL++;
    emit(packetReceivedFromUpperSignal, packet);

    auto frame = packet->peekAtFront<EthernetMacHeader>();
    if (frame->getDest().equals(getMacAddress())) {
        throw cRuntimeError("logic error: frame %s from higher layer has local MAC address as dest (%s)",
                packet->getFullName(), frame->getDest().str().c_str());
    }

    if (packet->getDataLength() > MAX_ETHERNET_FRAME_BYTES) {    //FIXME two MAX FRAME BYTES in specif...
        throw cRuntimeError("packet from higher layer (%d bytes) exceeds maximum Ethernet frame size (%d)",
                (int)(packet->getByteLength()), B(MAX_ETHERNET_FRAME_BYTES).get());
    }

    if (!connected || disabled) {
        EV_WARN << (!connected ? "Interface is not connected" : "MAC is disabled") << " -- dropping packet " << packet << endl;
        PacketDropDetails details;
        details.setReason(INTERFACE_DOWN);
        emit(packetDroppedSignal, packet, &details);
        numDroppedPkFromHLIfaceDown++;
        delete packet;

        return;
    }

    // fill in src address if not set
    if (frame->getSrc().isUnspecified()) {
        frame = nullptr; // drop shared ptr
        auto newFrame = packet->removeAtFront<EthernetMacHeader>();
        newFrame->setSrc(getMacAddress());
        packet->insertAtFront(newFrame);
        frame = newFrame;
    }

    addPaddingAndSetFcs(packet, MIN_ETHERNET_FRAME_BYTES);  // calculate valid FCS

    // store frame and possibly begin transmitting
    EV_DETAIL << "Frame " << frame << " arrived from higher layers, enqueueing\n";
    txQueue->pushPacket(packet);
    EV << "SEPEHR: TransmitState is " << transmitState << " and TX_IDLE_STATE is " << TX_IDLE_STATE << endl;
    EV << "SEPEHR: transmitState == TX_IDLE_STATE? " << (transmitState == TX_IDLE_STATE) << endl;

    if (transmitState == TX_IDLE_STATE) {
        ASSERT(currentTxFrame == nullptr);
        EV << "SEPEHR: Checking if queue is empty? " << txQueue->isEmpty() << endl;
        if (!txQueue->isEmpty()) {
            popTxQueue();
            startFrameTransmission();
        }
    }
}

void RandomHostEtherMacFullDuplex::processMsgFromNetwork(inet::EthernetSignal *signal)
{
    EV_INFO << signal << " received." << endl;

    if (!connected || disabled) {
        EV_WARN << (!connected ? "Interface is not connected" : "MAC is disabled") << " -- dropping msg " << signal << endl;
        if (typeid(*signal) == typeid(EthernetSignal)) {    // do not count JAM and IFG packets
            auto packet = check_and_cast<Packet *>(signal->decapsulate());
            delete signal;
            decapsulate(packet);
            PacketDropDetails details;
            details.setReason(INTERFACE_DOWN);
            emit(packetDroppedSignal, packet, &details);
            delete packet;
            numDroppedIfaceDown++;
        }
        else
            delete signal;

        return;
    }

    if (signal->getSrcMacFullDuplex() != duplexMode)
        throw cRuntimeError("Ethernet misconfiguration: MACs on the same link must be all in full duplex mode, or all in half-duplex mode");

    if (dynamic_cast<EthernetFilledIfgSignal *>(signal))
        throw cRuntimeError("There is no burst mode in full-duplex operation: EtherFilledIfg is unexpected");
    bool hasBitError = signal->hasBitError();
    auto packet = check_and_cast<Packet *>(signal->decapsulate());
    delete signal;
    totalSuccessfulRxTime += packet->getDuration();
    decapsulate(packet);
    emit(packetReceivedFromLowerSignal, packet);

    if (hasBitError || !verifyCrcAndLength(packet)) {
        numDroppedBitError++;
        PacketDropDetails details;
        details.setReason(INCORRECTLY_RECEIVED);
        emit(packetDroppedSignal, packet, &details);
        delete packet;
        return;
    }

    const auto& frame = packet->peekAtFront<EthernetMacHeader>();
    if (dropFrameNotForUs(packet, frame))
        return;

    if (frame->getTypeOrLength() == ETHERTYPE_FLOW_CONTROL) {
        const auto& controlFrame = currentTxFrame->peekDataAt<EthernetControlFrame>(frame->getChunkLength(), b(-1));
        if (controlFrame->getOpCode() == ETHERNET_CONTROL_PAUSE) {
            auto pauseFrame = check_and_cast<const EthernetPauseFrame *>(controlFrame.get());
            int pauseUnits = pauseFrame->getPauseTime();
            delete packet;
            numPauseFramesRcvd++;
            emit(rxPausePkUnitsSignal, pauseUnits);
            processPauseCommand(pauseUnits);
        }
        else {
            EV_INFO << "Received unknown ethernet flow control frame" << frame << " dropped." << endl;
            delete packet;
        }
    }
    else {
        EV_INFO << "Reception of " << frame << " successfully completed." << endl;
        processReceivedDataFrame(packet, frame);
    }
}

void RandomHostEtherMacFullDuplex::handleEndIFGPeriod()
{
    ASSERT(nullptr == currentTxFrame);
    if (transmitState != WAIT_IFG_STATE)
        throw cRuntimeError("Not in WAIT_IFG_STATE at the end of IFG period");

    // End of IFG period, okay to transmit
    EV_DETAIL << "SEPEHR: IFG elapsed in RandomHostEtherMacFullDuplex" << endl;

    if (!txQueue->isEmpty())
        popTxQueue();
    beginSendFrames();
}

void RandomHostEtherMacFullDuplex::handleEndTxPeriod()
{
    // we only get here if transmission has finished successfully
    if (transmitState != TRANSMITTING_STATE)
        throw cRuntimeError("Model error: End of transmission, and incorrect state detected");

    if (nullptr == currentTxFrame)
        throw cRuntimeError("Model error: Frame under transmission cannot be found");

    numFramesSent++;
    numBytesSent += currentTxFrame->getByteLength();
    emit(packetSentToLowerSignal, currentTxFrame);    //consider: emit with start time of frame

    const auto& header = currentTxFrame->peekAtFront<EthernetMacHeader>();
    if (header->getTypeOrLength() == ETHERTYPE_FLOW_CONTROL) {
        const auto& controlFrame = currentTxFrame->peekDataAt<EthernetControlFrame>(header->getChunkLength(), b(-1));
        if (controlFrame->getOpCode() == ETHERNET_CONTROL_PAUSE) {
            const auto& pauseFrame = CHK(dynamicPtrCast<const EthernetPauseFrame>(controlFrame));
            numPauseFramesSent++;
            emit(txPausePkUnitsSignal, pauseFrame->getPauseTime());
        }
    }

    EV_INFO << "Transmission of " << currentTxFrame << " successfully completed.\n";
    deleteCurrentTxFrame();
    lastTxFinishTime = simTime();

    if (pauseUnitsRequested > 0) {
        // if we received a PAUSE frame recently, go into PAUSE state
        EV_DETAIL << "Going to PAUSE mode for " << pauseUnitsRequested << " time units\n";

        scheduleEndPausePeriod(pauseUnitsRequested);
        pauseUnitsRequested = 0;
    }
    else {
        EV_DETAIL << "Start IFG period\n";
        scheduleEndIFGPeriod();
    }
}

void RandomHostEtherMacFullDuplex::finish()
{
    RandomHostEtherMacBase::finish();

    simtime_t t = simTime();
    simtime_t totalRxChannelIdleTime = t - totalSuccessfulRxTime;
    recordScalar("rx channel idle (%)", 100 * (totalRxChannelIdleTime / t));
    recordScalar("rx channel utilization (%)", 100 * (totalSuccessfulRxTime / t));
}

void RandomHostEtherMacFullDuplex::handleEndPausePeriod()
{
    ASSERT(nullptr == currentTxFrame);
    if (transmitState != PAUSE_STATE)
        throw cRuntimeError("End of PAUSE event occurred when not in PAUSE_STATE!");

    EV_DETAIL << "Pause finished, resuming transmissions\n";
    if (!currentTxFrame && !txQueue->isEmpty())
        popTxQueue();
    beginSendFrames();
}

void RandomHostEtherMacFullDuplex::processReceivedDataFrame(Packet *packet, const Ptr<const EthernetMacHeader>& frame)
{
    // statistics
    unsigned long curBytes = packet->getByteLength();
    numFramesReceivedOK++;
    numBytesReceivedOK += curBytes;
    emit(rxPkOkSignal, packet);

    packet->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ethernetMac);
    packet->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::ethernetMac);
    if (interfaceEntry)
        packet->addTagIfAbsent<InterfaceInd>()->setInterfaceId(interfaceEntry->getInterfaceId());

    numFramesPassedToHL++;
    emit(packetSentToUpperSignal, packet);
    // pass up to upper layer
    EV_INFO << "Sending " << packet << " to upper layer.\n";
    send(packet, upperLayerOutGateId);
}

void RandomHostEtherMacFullDuplex::processPauseCommand(int pauseUnits)
{
    if (transmitState == TX_IDLE_STATE) {
        EV_DETAIL << "PAUSE frame received, pausing for " << pauseUnitsRequested << " time units\n";
        if (pauseUnits > 0)
            scheduleEndPausePeriod(pauseUnits);
    }
    else if (transmitState == PAUSE_STATE) {
        EV_DETAIL << "PAUSE frame received, pausing for " << pauseUnitsRequested
                  << " more time units from now\n";
        cancelEvent(endPauseMsg);

        // Terminate PAUSE if pauseUnits == 0; Extend PAUSE if pauseUnits > 0
        scheduleEndPausePeriod(pauseUnits);
    }
    else {
        // transmitter busy -- wait until it finishes with current frame (endTx)
        // and then it'll go to PAUSE state
        EV_DETAIL << "PAUSE frame received, storing pause request\n";
        pauseUnitsRequested = pauseUnits;
    }
}

void RandomHostEtherMacFullDuplex::scheduleEndIFGPeriod()
{
    ASSERT(nullptr == currentTxFrame);
    changeTransmissionState(WAIT_IFG_STATE);
    simtime_t endIFGTime = simTime() + (b(INTERFRAME_GAP_BITS).get() / curEtherDescr->txrate);
    scheduleAt(endIFGTime, endIFGMsg);
}

void RandomHostEtherMacFullDuplex::scheduleEndPausePeriod(int pauseUnits)
{
    ASSERT(nullptr == currentTxFrame);
    // length is interpreted as 512-bit-time units
    simtime_t pausePeriod = ((pauseUnits * PAUSE_UNIT_BITS) / curEtherDescr->txrate);
    scheduleAt(simTime() + pausePeriod, endPauseMsg);
    changeTransmissionState(PAUSE_STATE);
}

void RandomHostEtherMacFullDuplex::beginSendFrames()
{
    if (currentTxFrame) {
        // Other frames are queued, transmit next frame
        EV_DETAIL << "Transmit next frame in output queue\n";
        startFrameTransmission();
    }
    else {
        // No more frames set transmitter to idle
        changeTransmissionState(TX_IDLE_STATE);
        EV_DETAIL << "No more frames to send, transmitter set to idle\n";
    }
}

