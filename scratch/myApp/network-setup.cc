#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/point-to-point-module.h"
#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-flow-classifier.h"
#include "ns3/mobility-helper.h"
#include "monitorApplication.h"
#include "myJson.h"
#include <utility>
#include <unistd.h>

using namespace ns3;

enum NETWORK_NODE {
    MOBILE = 0,
    LTE_BASESTATION = 1,
    WIFI_AP = 2,
    SERVER_NODE = 3,
    TOAL_NODES = 4
};

AnimationInterface *pAnim = 0;

NS_LOG_COMPONENT_DEFINE ("myApp");

std::pair<Ipv4Address, Ipv4Address> createCsmaNetwork(
    const Parameters &params,
    const Ptr<Node> n1, const Ptr<Node> n2,
    std::string network, std::string mask,
    uint64_t delay, uint64_t error_rate,
    bool enablePcap = false, bool enableTrace = false)
{
    NodeContainer nodes(n1, n2);
    Ptr<CsmaChannel> channel = CreateObjectWithAttributes<CsmaChannel> (
      "DataRate", DataRateValue (DataRate (100*1000*1000)), //bps
      "Delay", TimeValue (MilliSeconds (delay)));

    CsmaHelper csma;
    csma.SetDeviceAttribute ("EncapsulationMode", StringValue ("Llc"));
    NetDeviceContainer devs = csma.Install (nodes, channel);
    
    // Enable trace/pcap
    AsciiTraceHelper *ascii = new AsciiTraceHelper();
    string filename = "csma-" + network;
    csma.EnableAsciiAll (ascii->CreateFileStream (filename + ".tr"));
    csma.EnablePcapAll (filename);

    // Apply a error model to an uniform distribution 
    Ptr<RateErrorModel> em1 =
        CreateObjectWithAttributes<RateErrorModel> (
        "ErrorRate", DoubleValue (error_rate/100.0),
        "ErrorUnit", EnumValue (RateErrorModel::ERROR_UNIT_PACKET)
        );
    devs.Get (0)->SetAttribute ("ReceiveErrorModel", PointerValue (em1));
    devs.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em1));

    Ipv4AddressHelper ip;
    ip.SetBase (Ipv4Address(network.c_str()), Ipv4Mask(mask.c_str()));
    Ipv4InterfaceContainer addresses = ip.Assign (devs);
    return std::make_pair(addresses.GetAddress(0), addresses.GetAddress(1));
}

std::pair<Ipv4Address, Ipv4Address> createWifiNetwork(
    const Parameters &params,
    const Ptr<Node> sta, const Ptr<Node> ap,
    std::string network, std::string mask,
    bool enablePcap = false, bool enableTrace = false)
{
    double frequency = 5.0; // GHz
    uint8_t nStreams = 1;

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
    YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
    phy.SetChannel (channel.Create ());

    // Set MIMO capabilities
    phy.Set ("Antennas", UintegerValue (nStreams));
    phy.Set ("MaxSupportedTxSpatialStreams", UintegerValue (nStreams));
    phy.Set ("MaxSupportedRxSpatialStreams", UintegerValue (nStreams));

    WifiHelper wifi;
    WifiMacHelper mac;

    StringValue ctrlRate;
    if (frequency == 5.0) {
        wifi.SetStandard (WIFI_PHY_STANDARD_80211n_5GHZ);
        ctrlRate = StringValue ("OfdmRate24Mbps");
    } else if (frequency == 2.4) {
        wifi.SetStandard (WIFI_PHY_STANDARD_80211n_2_4GHZ);
        Config::SetDefault ("ns3::LogDistancePropagationLossModel::ReferenceLoss", DoubleValue (40.046));
        ctrlRate = StringValue ("ErpOfdmRate24Mbps");
    }
    else {
        std::cout << "Wrong frequency value!" << std::endl;
        return std::make_pair(nullptr, nullptr);
    }
    wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                        "DataMode", StringValue ("HtMcs7"), // TODO: Can have more support rate?
                        "ControlMode", ctrlRate);

    // BW20? BW40?
    // Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelWidth", UintegerValue (40));
    // Set guard interval //TODO SGI?
    // Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/HtConfiguration/ShortGuardIntervalSupported", BooleanValue (shortGuardInterval));


    Ssid ssid = Ssid ("ns3-80211n");
    mac.SetType ("ns3::StaWifiMac",
                       "Ssid", SsidValue (ssid));
    NetDeviceContainer staDevice = wifi.Install (phy, mac, sta);
    mac.SetType ("ns3::ApWifiMac",
                       "Ssid", SsidValue (ssid));
    NetDeviceContainer apDevice  = wifi.Install (phy, mac, ap);

    // Enable trace/pcap
    AsciiTraceHelper *ascii = new AsciiTraceHelper();
    string filename = "wifi-" + network;
    phy.EnableAsciiAll (ascii->CreateFileStream (filename + ".tr"));
    phy.EnablePcapAll (filename);

    Ipv4AddressHelper ip;
    ip.SetBase (Ipv4Address(network.c_str()), Ipv4Mask(mask.c_str()));
    Ipv4InterfaceContainer ipSta = ip.Assign (staDevice);
    Ipv4InterfaceContainer ipAp = ip.Assign (apDevice);
    return std::make_pair(ipSta.GetAddress(0), ipAp.GetAddress(0));
}

int main(int argc, char** argv) {

    Parameters params = parseArgsToParams(argc, argv);

    if (params.UseMPTCP) {
        Config::SetDefault ("ns3::TcpL4Protocol::UseMpTcpSocketBase", UintegerValue(params.UseMPTCP));
        Config::SetDefault ("ns3::TcpSocketBase::EnableMpTcp", BooleanValue (true));
        // TODO: Below settings does not work.
        // Config::SetDefault ("ns3::MpTcpSocketBase::PathManagerMode", EnumValue (MpTcpSocketBase::FullMesh));
        // Config::SetDefault ("ns3::MpTcpSocketBase::Scheduler", TypeIdValue(MpTcpSchedulerRoundRobin::GetTypeId()));
    }

    // Set end time of the simulation
    Simulator::Stop (Seconds (params.endTime + 5));

    // Here, we will create TOTAL_NODES for network topology
    NS_LOG_INFO ("Create nodes.");
    NodeContainer allNodes;
    allNodes.Create(NETWORK_NODE::TOAL_NODES);
    NodeContainer mobileLte = NodeContainer(
        allNodes.Get(NETWORK_NODE::MOBILE), allNodes.Get(NETWORK_NODE::LTE_BASESTATION));
    NodeContainer mobileWifi = NodeContainer(
        allNodes.Get(NETWORK_NODE::MOBILE), allNodes.Get(NETWORK_NODE::WIFI_AP));

    // Set position of nodes
    // mobility.
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
    positionAlloc->Add (Vector (40.0, 50.0, 0.0));  // MOBILE
    positionAlloc->Add (Vector (50.0, 60.0, 0.0)); // WIFI_AP
    positionAlloc->Add (Vector (50.0, 40.0, 0.0));    // LTE_BASESTATION
    positionAlloc->Add (Vector (70.0, 50.0, 0.0));  // SERVER_NODE
    mobility.SetPositionAllocator (positionAlloc);

    mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Mode", StringValue ("Time"),
                             "Time", StringValue ("1s"),
                             "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.5]"),
                             "Bounds", RectangleValue (Rectangle (30.0, 70.0, 30.0, 70.0)));
    mobility.Install (allNodes.Get(NETWORK_NODE::MOBILE));

    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (allNodes.Get(NETWORK_NODE::WIFI_AP));

    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (allNodes.Get(NETWORK_NODE::LTE_BASESTATION));

    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (allNodes.Get(NETWORK_NODE::SERVER_NODE));

    // Install network stacks on the nodes
    InternetStackHelper internet;
    internet.Install (allNodes);

    // Create Network
    NS_LOG_INFO ("Create CSMA network between Wi-Fi AP and server.");
    std::pair<Ipv4Address, Ipv4Address> ipsWiFiServer = createCsmaNetwork(
        params,
        allNodes.Get(NETWORK_NODE::WIFI_AP), allNodes.Get(NETWORK_NODE::SERVER_NODE),
        "10.2.1.0", "255.255.255.0",
        params.delayWifiServer, params.errateWifiServer,
        true /* Enable Pcap*/,
        true /* Enable Trace*/);

    NS_LOG_INFO ("Create CSMA network between LTE and server.");
    std::pair<Ipv4Address, Ipv4Address> ipsLteServer = createCsmaNetwork(
        params,
        allNodes.Get(NETWORK_NODE::LTE_BASESTATION), allNodes.Get(NETWORK_NODE::SERVER_NODE),
        "10.3.1.0", "255.255.255.0",
        params.delayLteServer, params.errateLteServer);
    NS_UNUSED(ipsLteServer);

    NS_LOG_INFO ("Create Wi-Fi network between mobile and Wi-Fi AP.");
    std::pair<Ipv4Address, Ipv4Address> ipsMobileWifi = createWifiNetwork(
        params,
        allNodes.Get(NETWORK_NODE::MOBILE), allNodes.Get(NETWORK_NODE::WIFI_AP),
        "10.1.2.0", "255.255.255.0",
        true /* Enable Pcap*/,
        true /* Enable Trace*/);
    NS_UNUSED(ipsMobileWifi);

    NS_LOG_INFO ("Create point-2-point network between mobile and LTE.");
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
    NetDeviceContainer devMobileLte = p2p.Install (mobileLte);

    // Later, we add IP addresses.
    NS_LOG_INFO ("Assign IP Addresses.");
    Ipv4AddressHelper ipv4;

    ipv4.SetBase ("10.1.3.0", "255.255.255.0");
    ipv4.Assign (devMobileLte);

    //Turn on global static routing
    NS_LOG_INFO ("Enable global static routing.");
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    // Install sender and receiver applications
    NS_LOG_INFO ("Create traffic source & sink.");

    // Install sender to MOBILE node
    Ptr<Node> appSource = allNodes.Get(MOBILE);
    Ptr<Sender> sender = CreateObject<Sender>();
    appSource->AddApplication (sender);
    sender->SetStartTime (Seconds (1));
    sender->SetStopTime(Seconds (params.endTime));
    sender->SetAttribute("Destination",  Ipv4AddressValue(ipsWiFiServer.second));
    sender->SetAttribute("UseTCP",  UintegerValue(params.UseTCP));
    sender->SetAttribute("PacketSize",  UintegerValue(params.packetSize));
    sender->SetAttribute("PacketNum",  UintegerValue(params.burstPktNum));
    std::stringstream ss;
    ss << "ns3::ConstantRandomVariable[Constant=" << params.burstItvSec << "]"; 
    sender->SetAttribute("Interval", StringValue(ss.str()));

    // Install receiver to SERVER_NODE
    Ptr<Node> appSink = allNodes.Get(SERVER_NODE);
    Ptr<Receiver> receiver = CreateObject<Receiver>();
    appSink->AddApplication (receiver);
    receiver->SetStartTime (Seconds (1));
    receiver->SetStopTime(Seconds (params.endTime));
    receiver->SetAttribute("UseTCP",  UintegerValue(params.UseTCP));

    // Install FlowMonitor on all nodes
    FlowMonitorHelper flowmonHelper;
    Ptr<FlowMonitor> monitor = flowmonHelper.InstallAll ();

    // Trace routing tables
    // Ipv4GlobalRoutingHelper g;
    // Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("myApp.routes", std::ios::out);
    // g.PrintRoutingTableAllAt (Seconds (0), routingStream);

    // Create the animation object and configure for specified output
    // Make sure the AnimationInterface is created as closer to Simulator::Run (); as possible
    pAnim = new AnimationInterface ("myApp.xml");
    pAnim->UpdateNodeDescription(allNodes.Get(NETWORK_NODE::MOBILE), "Mobile Device");
    pAnim->UpdateNodeDescription(allNodes.Get(NETWORK_NODE::WIFI_AP), "Wi-Fi AP");
    pAnim->UpdateNodeDescription(allNodes.Get(NETWORK_NODE::LTE_BASESTATION), "LTE Base Station");
    pAnim->UpdateNodeDescription(allNodes.Get(NETWORK_NODE::SERVER_NODE), "Server");
    pAnim->EnablePacketMetadata (true); // Optional
    pAnim->EnableIpv4L3ProtocolCounters (Seconds (0), Seconds (params.endTime + 5)); // Optional

    // Provide the absolute path to the resource
    std::string pic_path = string(getcwd(NULL, 0)) + "/scratch/myApp/pic/"; // Ignore potential memory leak...

    // No function?
    // uint32_t mobilePic = pAnim->AddResource (pic_path + "wifi.jpg");
    // pAnim->UpdateNodeImage(allNodes.Get(NETWORK_NODE::MOBILE)->GetId(), mobilePic);

    // Use backgroud for only the mobile pic
    pAnim->SetBackgroundImage (pic_path + "mobile.jpg", 0, 0, 0.01, 0.01, 0.5);

    Simulator::Run ();
    std::cout << "Animation Trace file created:" << "myApp.xml" << std::endl;

    // Save FlowMonitor results to file
    std::cout << "Statistical results file created:" << "myApp.flowmon.xml" << std::endl;
    flowmonHelper.SerializeToXmlFile ("myApp.flowmon.xml", true, true);
    

#if 1 // Not necessary
    // Print per flow statistics
    monitor->CheckForLostPackets ();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmonHelper.GetClassifier ());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
        std::cout << "*IP* Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
        std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
        std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
        std::cout << "  TxOffered:  " << i->second.txBytes * 1.0 / 1000 / 1000  << " Mbps\n";
        std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
        std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
        std::cout << "  Throughput: " << i->second.rxBytes * 1.0 / 1000 / 1000  << " Mbps\n";
    }
#endif

    std::cout << "Ooutput simulation results to myApp.appStats.json" << std::endl;
    std::ofstream os ("myApp.appStats.json", std::ios::out|std::ios::binary);
    jsonObjStart(os);
    jsonObjAdd(os, "params", parmsToJSONObj());
    jsonObjAdd(os, "stats", monStatsToJSONObj());
    jsonObjEnd(os);
    os.close();

    Simulator::Destroy ();
    delete pAnim;
    return 0;
}
