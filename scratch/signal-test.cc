#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/olsr-helper.h"
#include "ns3/ipv4-header.h"
#include "ns3/tcp-header.h"
#include <stdio.h>
#include <stdlib.h>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SimulationScript");


int main(int argc, char *argv[]) {

    // Parameters
    uint16_t num_nodes = 2;
    double begin_time = 60.0;
    double sim_time = 2.0;
    uint32_t payloadSize = 1472;
	double distance = 10.0;

    // Process command line arguments
    CommandLine cmd;
	cmd.AddValue("distance", "Distance between two nodes", distance);
    cmd.Parse(argc, argv);

    // Create nodes
    NodeContainer wifiNode;
    wifiNode.Create(num_nodes);

    // Channel + PHY
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
    phy.SetChannel(channel.Create());
    phy.Set("ShortGuardEnabled", BooleanValue(1));
	phy.Set("TxPowerStart", DoubleValue(20.0));
	phy.Set("TxPowerEnd", DoubleValue(20.0));
    //phy.Set("CcaMode1Threshold", DoubleValue(-82.0));
    //phy.Set("EnergyDetectionThreshold", DoubleValue(-62.0));

    // MAC
    WifiHelper wifi;
    wifi.SetStandard(WIFI_PHY_STANDARD_80211a);
    WifiMacHelper mac;

    std::string phyMode("OfdmRate54Mbps");
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode", StringValue(phyMode), "ControlMode", StringValue(phyMode));

    mac.SetType("ns3::AdhocWifiMac");
    NetDeviceContainer devices = wifi.Install(phy, mac, wifiNode);

    Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelWidth", UintegerValue(20));

    // Node positions
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(distance, 0.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiNode);

    // NETWORK
    InternetStackHelper internet;
    internet.Install(wifiNode);
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i = ipv4.Assign(devices);

    // Traffic
    UdpServerHelper myServerA(9);
    ApplicationContainer serverAppA = myServerA.Install(wifiNode.Get(0));
    serverAppA.Start(Seconds(begin_time));
    serverAppA.Stop(Seconds(begin_time+sim_time));
    UdpClientHelper myClientA(i.GetAddress(0), 9);
    myClientA.SetAttribute("MaxPackets", UintegerValue(4294967295u));
    myClientA.SetAttribute("Interval", TimeValue(Seconds(0.0001)));
    myClientA.SetAttribute("PacketSize", UintegerValue(payloadSize));
    ApplicationContainer clientAppA = myClientA.Install(wifiNode.Get(1));
    clientAppA.Start(Seconds(begin_time + 0.001));
    clientAppA.Stop(Seconds(begin_time + sim_time));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Run simulation
    Simulator::Stop(Seconds(begin_time+sim_time+0.1));
    Simulator::Run();
    Simulator::Destroy();

    // Throughput calculation
    double throughputA = 0;
    uint32_t totalPacketsThroughA = DynamicCast<UdpServer>(serverAppA.Get(0))->GetReceived();
    throughputA = totalPacketsThroughA * payloadSize * 8 / (sim_time * 1000000.0);  // Mbps

    NS_LOG_UNCOND("throughputA: " << throughputA << " Mbps");
    return 0;
}


