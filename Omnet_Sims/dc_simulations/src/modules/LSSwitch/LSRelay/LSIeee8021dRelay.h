// Copyright (C) 2013 OpenSim Ltd.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//
// Author: Benjamin Martin Seregi

#ifndef LSIEEE8021DRELAY_H
#define LSIEEE8021DRELAY_H

#include "inet/common/INETDefs.h"
#include "inet/common/LayeredProtocolBase.h"
#include "inet/common/lifecycle/ModuleOperations.h"
#include "inet/common/packet/Packet.h"
#include "inet/networklayer/common/InterfaceTable.h"
#include "inet/linklayer/configurator/Ieee8021dInterfaceData.h"
#include "inet/linklayer/ethernet/EtherFrame_m.h"
#include "../LSMACTable/LSIMacAddressTable.h"

using namespace inet;

//
// This module forward frames (~EtherFrame) based on their destination MAC addresses to appropriate ports.
// See the NED definition for details.
//
class LSIeee8021dRelay : public LayeredProtocolBase
{
  public:
    LSIeee8021dRelay();

    /**
     * Register single MAC address that this switch supports.
     */

    void registerAddress(MacAddress mac);

    /**
     * Register range of MAC addresses that this switch supports.
     */
    void registerAddresses(MacAddress startMac, MacAddress endMac);

  protected:
    MacAddress bridgeAddress;
    IInterfaceTable *ifTable = nullptr;
    LSIMacAddressTable *macTable = nullptr;
    InterfaceEntry *ie = nullptr;
    bool isStpAware = false;

    typedef std::pair<MacAddress, MacAddress> MacAddressPair;


    struct Comp
    {
        bool operator() (const MacAddressPair& first, const MacAddressPair& second) const
        {
            return (first.first < second.first && first.second < second.first);
        }
    };

    bool in_range(const std::set<MacAddressPair, Comp>& ranges, MacAddress value)
    {
        return ranges.find(MacAddressPair(value, value)) != ranges.end();
    }


    std::set<MacAddressPair, Comp> registeredMacAddresses;

    // statistics: see finish() for details.
    int numReceivedNetworkFrames = 0;
    int numDroppedFrames = 0;
    int numReceivedBPDUsFromSTP = 0;
    int numDeliveredBDPUsToSTP = 0;
    int numDispatchedNonBPDUFrames = 0;
    int numDispatchedBDPUFrames = 0;
    bool learn_mac_addresses;
    bool bounce_back_random_port;
    std::list<int> port_idx_connected_to_switch_neioghbors;

  protected:
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }

    /**
     * Updates address table (if the port is in learning state)
     * with source address, determines output port
     * and sends out (or broadcasts) frame on ports
     * (if the ports are in forwarding state).
     * Includes calls to updateTableWithAddress() and getPortForAddress().
     *
     */
    void handleAndDispatchFrame(Packet *packet);

    void handleUpperPacket(Packet *packet) override;
    void handleLowerPacket(Packet *packet) override;

    void dispatch(Packet *packet, InterfaceEntry *ie);
    void learn(MacAddress srcAddr, int arrivalInterfaceId);
    void broadcast(Packet *packet, int arrivalInterfaceId);

    void sendUp(Packet *packet);

    //@{ For ECMP
    void chooseDispatchType(Packet *packet, InterfaceEntry *ie);
    unsigned int Compute_CRC16_Simple(std::list<int> bytes, int bytes_size);
    std::list<int> getInfoFlowByteArray(std::string srcAddr, std::string destAddr, int srcPort, int destPort);
    //@}

    //@{ For lifecycle
    virtual void start();
    virtual void stop();
    virtual void handleStartOperation(LifecycleOperation *operation) override { start(); }
    virtual void handleStopOperation(LifecycleOperation *operation) override { stop(); }
    virtual void handleCrashOperation(LifecycleOperation *operation) override { stop(); }
    virtual bool isUpperMessage(cMessage *message) override { return message->arrivedOn("upperLayerIn"); }
    virtual bool isLowerMessage(cMessage *message) override { return message->arrivedOn("ifIn"); }

    virtual bool isInitializeStage(int stage) override { return stage == INITSTAGE_LINK_LAYER; }
    virtual bool isModuleStartStage(int stage) override { return stage == ModuleStartOperation::STAGE_LINK_LAYER; }
    virtual bool isModuleStopStage(int stage) override { return stage == ModuleStopOperation::STAGE_LINK_LAYER; }
    //@}

    /*
     * Gets port data from the InterfaceTable
     */
    Ieee8021dInterfaceData *getPortInterfaceData(unsigned int portNum);

    bool isForwardingInterface(InterfaceEntry *ie);

    /*
     * Returns the first non-loopback interface.
     */
    virtual InterfaceEntry *chooseInterface();
    virtual void finish() override;
};


#endif // ifndef __INET_LSIEEE8021DRELAY_H

