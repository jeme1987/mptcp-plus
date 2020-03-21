
#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include "ns3/flow-monitor-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/ipv4-nix-vector-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/packet-sink.h"
#include "ns3/simulator.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/random-variable-stream.h"
#include "ns3/traffic-control-module.h"
#include "ns3/mptcp-socket-base.h"

#include "ns3/enum.h"
#include "ns3/config-store-module.h"


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("MpTcpTestingExample");


int 
main (int argc, char *argv[])
{
  
  Config::SetDefault ("ns3::Ipv4GlobalRouting::RandomEcmpRouting", BooleanValue(true));
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpNewReno"));
  // 42 = headers size
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1000));
 
  //Enable MPTCP 
  //Config::SetDefault ("ns3::TcpSocketBase::EnableMpTcp", BooleanValue (true));
  Config::SetDefault("ns3::MpTcpSocketBase::PathManagerMode", EnumValue (MpTcpSocketBase::FullMesh));
  //Config::SetDefault ("ns::MpTcpNdiffPorts::MaxSubflows", UintegerValue (4));
  
 
// Variables Declaration
   uint16_t port = 999;
   uint32_t maxBytes = 121400;
 
// Initialize Internet Stack and Routing Protocols
		
	InternetStackHelper internet;
	Ipv4AddressHelper ipv4;

// creating routers, source and destination. Installing internet stack
  NodeContainer host;				// NodeContainer for source and destination	
		host.Create (2);
		internet.Install (host);
  NodeContainer router;				// NodeContainer for router	
		router.Create (4);
		internet.Install (router);	

/////////////////////creating toplogy////////////////

// connecting routers and hosts and assign ip addresses

/*

 H0-----R0------R2-----H1
  \                   /   
   \                 /   
    \               /   
     \             /   
      \           /   
       \         / 
        R1------R3
*/

  NodeContainer h0r0 = NodeContainer (host.Get (0), router.Get (0));
  NodeContainer h0r1 = NodeContainer (host.Get (0), router.Get (1));
  
  NodeContainer r0r2 = NodeContainer (router.Get (0), router.Get (2));
  //NodeContainer r0r3 = NodeContainer (router.Get (0), router.Get (3));
  //NodeContainer r1r2 = NodeContainer (router.Get (1), router.Get (2));
  NodeContainer r1r3 = NodeContainer (router.Get (1), router.Get (3));

  NodeContainer h1r2 = NodeContainer (host.Get (1), router.Get (2));
  NodeContainer h1r3 = NodeContainer (host.Get (1), router.Get (3));

  //NodeContainer r0r1 = NodeContainer (router.Get (0), router.Get (1));
// We create the channels first without any IP addressing information
  NS_LOG_INFO ("Create channels.");
  
  PointToPointHelper p2p_host;
 
  p2p_host.SetDeviceAttribute ("DataRate", StringValue ("2Mbps"));
  p2p_host.SetChannelAttribute ("Delay", StringValue ("1ms"));
  NetDeviceContainer l0h0r0 = p2p_host.Install (h0r0);
  NetDeviceContainer l1h0r1 = p2p_host.Install (h0r1);

  p2p_host.EnablePcapAll ("host.pcap");

  PointToPointHelper p2p_backbone;
  p2p_backbone.SetDeviceAttribute ("DataRate", StringValue ("2.4Gbps"));
  p2p_backbone.SetChannelAttribute ("Delay", StringValue ("1ms"));

  NetDeviceContainer l2r0r2 = p2p_backbone.Install (r0r2);
  //NetDeviceContainer l3r0r3 = p2p.Install (r0r3);
  //NetDeviceContainer l4r1r2 = p2p.Install (r1r2);
  NetDeviceContainer l5r1r3 = p2p_backbone.Install (r1r3);
 
  NetDeviceContainer l6h1r2 = p2p_backbone.Install (h1r2);
  NetDeviceContainer l7h1r3 = p2p_backbone.Install (h1r3);
  
// Later, we add IP addresses.
  NS_LOG_INFO ("Assign IP Addresses.");

  // h0 uses 10.1.1.0 and 10.1.2.0
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i0h0r0 = ipv4.Assign (l0h0r0);

  ipv4.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer i1h0r1 = ipv4.Assign (l1h0r1);

  
  ipv4.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer i2r0r2 = ipv4.Assign (l2r0r2);

  ipv4.SetBase ("10.1.4.0", "255.255.255.0");
  //Ipv4InterfaceContainer i3r0r3 = ipv4.Assign (l3r0r3);
  
  ipv4.SetBase ("10.1.5.0", "255.255.255.0");
  //Ipv4InterfaceContainer i4r1r2 = ipv4.Assign (l4r1r2);

  ipv4.SetBase ("10.1.6.0", "255.255.255.0");
  Ipv4InterfaceContainer i5r1r3 = ipv4.Assign (l5r1r3);


  //h1 uses 10.1.7.0 and 10.1.8.0
  ipv4.SetBase ("10.1.7.0", "255.255.255.0");
  Ipv4InterfaceContainer i6h1r2 = ipv4.Assign (l6h1r2);
  
  ipv4.SetBase ("10.1.8.0", "255.255.255.0");
  Ipv4InterfaceContainer i7h1r3 = ipv4.Assign (l7h1r3);



// Create router nodes, initialize routing database and set up the routing
  // tables in the nodes.
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  NS_LOG_INFO ("Create applications.");
   

  // Create OnOff Application

 // Create BuldSend Application to send Tcp packets  //

  OnOffHelper oo = OnOffHelper ("ns3::TcpSocketFactory",
                         InetSocketAddress (i6h1r2.GetAddress (0), port)); //link from router1 to host1
  // Set the amount of data to send in bytes.  Zero is unlimited.
  oo.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
  oo.SetAttribute ("PacketSize", UintegerValue (1000));
  oo.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.02]"));
  oo.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.01]"));
  oo.SetAttribute ("DataRate", DataRateValue (DataRate ("500kb/s")));

  ApplicationContainer SourceApp = oo.Install (host.Get (0));
  SourceApp.Start (Seconds (0.0));
  SourceApp.Stop (Seconds (10.0));
  
  // Create a packet sink to receive packets.
  // 
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory",
                        		 InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer SinkApp = packetSinkHelper.Install (host.Get(1));
	
  SinkApp.Start (Seconds (0.0));
  SinkApp.Stop (Seconds (6.0));   
                                        
//=========== Start the simulation ===========//

	std::cout << "Start Simulation.. "<<"\n";

       // Output config store to txt format
  Config::SetDefault ("ns3::ConfigStore::Filename", StringValue ("output-attributes.txt"));
  Config::SetDefault ("ns3::ConfigStore::FileFormat", StringValue ("RawText"));
  Config::SetDefault ("ns3::ConfigStore::Mode", StringValue ("Save"));
  ConfigStore outputConfig2;
  outputConfig2.ConfigureDefaults ();
  outputConfig2.ConfigureAttributes ();


        NS_LOG_INFO ("Run Simulation.");
  	Simulator::Stop (Seconds(30.0));
   
////////////////////////// Use of NetAnim model/////////////////////////////////////////
 
		AnimationInterface::SetConstantPosition(host.Get(0), 0.0, 20.0);
		AnimationInterface::SetConstantPosition(router.Get(0), 20.0, 0.0);
		AnimationInterface::SetConstantPosition(router.Get(1), 20.0, 40.0);
		AnimationInterface::SetConstantPosition(router.Get(2), 60.0, 0.0);
		AnimationInterface::SetConstantPosition(router.Get(3), 60.0, 40.0);
		AnimationInterface::SetConstantPosition(host.Get(1), 80.0 , 20.0);  
    AnimationInterface anim ("mptcp-example.xml");

    anim.UpdateNodeDescription(host.Get(0), "Host 0");
    anim.UpdateNodeDescription(host.Get(1), "Host 1");
    anim.UpdateNodeDescription(router.Get(0), "R0");
    anim.UpdateNodeDescription(router.Get(1), "R1");
    anim.UpdateNodeDescription(router.Get(2), "R2");
    anim.UpdateNodeDescription(router.Get(3), "R3");

    //anim.EnablePacketMetadata (true);
	  	
/////////////////////////////////////////////////////////////////////////////////////////////
  	
    Simulator::Run ();
        NS_LOG_INFO ("Done.");
      
	std::cout << "Simulation finished "<<"\n";
       
  	Simulator::Destroy ();
  	

  Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (SinkApp.Get (0));
  std::cout << "Total Bytes Received: " << sink1->GetTotalRx () << std::endl;
	return 0;
}

