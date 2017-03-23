/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 CNRS, France
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Anne-Cécile Orgerie <anne-cecile.orgerie@irisa.fr>
 */




/*
    Network setup

    N0---------------N1---------------N2
       1Gbps, 1ns    1Gbps, 1ns

   use the following command to run the script

   ./waf --run "link_energy3  --packetSize=1272 --maxPackets=80000"

*/

#include <fstream>
#include <iostream>
#include <string>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ecofen-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SimpleExample");

double offeredLoad = 0.0;
double throughput = 0.0;
double lastPacketTime = 0.0;

int
main (int argc, char *argv[])
{

  double interval = 0;
  uint32_t maxPackets = 10;
  uint32_t packetSize = 1472;

 CommandLine cmd;

  cmd.AddValue ("interval", "Interval between packets", interval);
  cmd.AddValue ("maxPackets", "Maximum number of packets", maxPackets);
  cmd.AddValue ("packetSize", "Maximum number of packets", packetSize);

 cmd.Parse(argc,argv);
  // preliminary

  interval = 1.0/maxPackets;

  // Log level
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  // Each time there is an interface event, the routing is launched.
  Config::SetDefault ("ns3::Ipv4GlobalRouting::RespondToInterfaceEvents", BooleanValue (true));

  // Create nodes in a container
  NodeContainer nodes;
  nodes.Create (3);

  // Create Internet stack (including loopback interfaces)
  InternetStackHelper stack;
  stack.Install (nodes);

  // Add Energy Parameters to the nodes
  BasicNodeEnergyHelper basicNodeEnergy;
  basicNodeEnergy.Set ("OnConso", DoubleValue (10.242));
  basicNodeEnergy.Set ("OffConso", DoubleValue (0.0));
  basicNodeEnergy.Install (nodes);

  // Create 'relations' between the nodes
  NodeContainer n0n1 = NodeContainer (nodes.Get (0), nodes.Get (1));
  NodeContainer n1n2 = NodeContainer (nodes.Get (1), nodes.Get (2));

  // Create channels
  PointToPointHelper pointtopoint;
  pointtopoint.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
  pointtopoint.SetChannelAttribute ("Delay", StringValue ("1ns"));

  // Create Interfaces
  NetDeviceContainer d0d1 = pointtopoint.Install (n0n1);
  NetDeviceContainer d1d2 = pointtopoint.Install (n1n2);

  // Add energy parameters to the interfaces

  /*
  BasicNetdeviceEnergyHelper basicNetdeviceEnergy;
  basicNetdeviceEnergy.Set ("OnConso", DoubleValue (1.0));
  basicNetdeviceEnergy.Set ("OffConso", DoubleValue (0.0));
  basicNetdeviceEnergy.Install (d0d1);
  basicNetdeviceEnergy.Install (d1d2);
  */
  LinearNetdeviceEnergyHelper linearNetdeviceEnergy;
  linearNetdeviceEnergy.Set ("IdleConso", DoubleValue (0.0));
  linearNetdeviceEnergy.Set ("OffConso", DoubleValue (0.0));
  linearNetdeviceEnergy.Set ("ByteEnergy", DoubleValue (3.212));
  linearNetdeviceEnergy.Install (nodes);

/*
  CompleteNetdeviceEnergyHelper completeNetdeviceEnergy;
  completeNetdeviceEnergy.Set ("IdleConso", DoubleValue(1.0));
  completeNetdeviceEnergy.Set ("OffConso", DoubleValue(0.2));
  completeNetdeviceEnergy.Set ("RecvByteEnergy", DoubleValue (100.0));
  completeNetdeviceEnergy.Set ("SentByteEnergy", DoubleValue (100.0));
  completeNetdeviceEnergy.Set ("RecvPktEnergy", DoubleValue (300.0));
  completeNetdeviceEnergy.Set ("SentPktEnergy", DoubleValue (300.0));
  completeNetdeviceEnergy.Install (nodes);
*/
  ConsumptionLogger conso;
  conso.NodeConso (Seconds (1.0), Seconds (1.05), nodes);


  // Assign Internet addresses
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces0 = address.Assign (d0d1);


  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces1 = address.Assign (d1d2);


  // Create GOD routing and tables
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();


  // Switch off net devices
 /*  Ptr<NetDevice> netdev = d1d2.Get (0);
  OnOffNetdeviceHelper onoff;
  onoff.Set ("SwitchOffEnergy", DoubleValue (2.0));
  onoff.Set ("SwitchOffDuration", DoubleValue (0.5));
  onoff.Set ("SwitchOnEnergy", DoubleValue (6.0));
  onoff.Set ("SwitchOnDuration", DoubleValue (1.0));
  onoff.Install (netdev);
  onoff.NetDeviceSwitchOff (netdev, Seconds (2.5));
  onoff.NetDeviceSwitchOn (netdev, Seconds (3.5));*/

  // Switch off nodes
  /*Ptr<Node> node = nodes.Get (1);
  OnOffNodeHelper onoff;
  onoff.Set ("SwitchOffEnergy", DoubleValue (30.0));
  onoff.Set ("SwitchOffDuration", DoubleValue (2.0));
  onoff.Set ("SwitchOnEnergy", DoubleValue (60.0));
  onoff.Set ("SwitchOnDuration", DoubleValue (3.0));
  onoff.Install (node);
  onoff.NodeSwitchOff (node, Seconds (3.0));
  onoff.NodeSwitchOn (node, Seconds (6.0));*/

  // Create application (scenario)
  UdpEchoServerHelper echoServer0 (9);
  //UdpEchoServerHelper echoServer1 (9);

  ApplicationContainer serverApps0 = echoServer0.Install (nodes.Get (2));
  serverApps0.Start (Seconds (0));
  serverApps0.Stop (Seconds (1.00005));

 // ApplicationContainer serverApps1 = echoServer1.Install (nodes.Get (0));
 // serverApps1.Start (Seconds (0.5));
 // serverApps1.Stop (Seconds (1.5));

  UdpEchoClientHelper echoClient0 (interfaces1.GetAddress (1), 9);
  echoClient0.SetAttribute ("MaxPackets", UintegerValue (maxPackets));
  echoClient0.SetAttribute ("Interval", TimeValue (Seconds (interval)));
  echoClient0.SetAttribute ("PacketSize", UintegerValue (packetSize));
//  UdpEchoClientHelper echoClient1 (interfaces1.GetAddress (1), 9);
  ApplicationContainer clientApps0 = echoClient0.Install (nodes.Get (0));
 // ApplicationContainer clientApps1 = echoClient1.Install (nodes.Get (2));

  clientApps0.Start (Seconds (0));
  clientApps0.Stop (Seconds (1.0));

  //clientApps1.Start (Seconds (0.5));
 // clientApps1.Stop (Seconds (1.5));

  // Enable the traces
  pointtopoint.EnablePcapAll ("Test-simple");

  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());

  Simulator::Stop (Seconds (1.05));
  Simulator::Run ();

  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

  double totalThroughput = 0.0;
  double totalLostPackets = 0.0;

 for (std::map< FlowId, FlowMonitor::FlowStats>::iterator flow=stats.begin(); flow!=stats.end(); flow++)
 {
  Ipv4FlowClassifier::FiveTuple  t = classifier->FindFlow(flow->first);
  NS_LOG_UNCOND( "\nFlowID: " << flow->first << " ("
             << t.sourceAddress << " / " << t.sourcePort << " --> "
             << t.destinationAddress << " / " << t.destinationPort << ")" );
  NS_LOG_UNCOND( "  Tx Packets:   " << flow->second.txPackets );
  NS_LOG_UNCOND( "  Tx Bytes:   " << flow->second.txBytes );

  offeredLoad = flow->second.txBytes * 8.0 / (flow->second.timeLastTxPacket.GetSeconds () - flow->second.timeFirstTxPacket.GetSeconds ()) / 1000000.0 ;
  NS_LOG_UNCOND( "  Offered Load: " << flow->second.txBytes * 8.0 / (flow->second.timeLastTxPacket.GetSeconds () - flow->second.timeFirstTxPacket.GetSeconds ()) / 1000000.0 << " Mbps");

  NS_LOG_UNCOND( "  Rx Packets:   " << flow->second.rxPackets );
  NS_LOG_UNCOND( "  Rx Bytes:   " << flow->second.rxBytes );

  throughput = flow->second.rxBytes * 8.0 / (flow->second.timeLastRxPacket.GetSeconds () - flow->second.timeFirstRxPacket.GetSeconds ()) / 1000000.0;
  NS_LOG_UNCOND( "  Throughput: " << flow->second.rxBytes * 8.0 / (flow->second.timeLastRxPacket.GetSeconds () - flow->second.timeFirstRxPacket.GetSeconds ()) / 1000000.0  << " Mbps");

  NS_LOG_UNCOND( "  Lost Packets: " << flow->second.lostPackets );
  NS_LOG_UNCOND( "  Time last packet Transmited: " << flow->second.timeLastTxPacket.GetSeconds() );
  NS_LOG_UNCOND( "  Time last packet Received: " << flow->second.timeLastRxPacket.GetSeconds() );

  if(t.destinationPort == 9){

    totalThroughput = totalThroughput + throughput;
    totalLostPackets = totalLostPackets + flow->second.lostPackets;

  }
  if(t.destinationPort == 9 && (lastPacketTime < flow->second.timeLastRxPacket.GetSeconds ())){

     lastPacketTime = flow->second.timeLastRxPacket.GetSeconds ();
  }
}
  double averageThroughput = totalThroughput/1;
  double averageLostPackets = totalLostPackets/1;

  NS_LOG_UNCOND( "\nLast Packet Time: " << lastPacketTime);
  NS_LOG_UNCOND( "Average Throughput: " << averageThroughput  << " Mbps");
  NS_LOG_UNCOND( "Average Lost Packet : " << averageLostPackets);

  // Calculate expected value

   double expVal = 0;
  if(maxPackets == 20000){
    expVal = (0.00007102 * packetSize)  + 10.25270095;
  }else if(maxPackets == 40000){
    expVal = (0.00013752 * packetSize) + 10.25818500;
  }else if(maxPackets == 60000){
    expVal = (0.00019202 * packetSize) + 10.27436094;
  }else if(maxPackets == 80000){
    expVal = (0.00028033 * packetSize) + 10.25617084;
  }
  NS_LOG_UNCOND( "Expectd Power for "<< maxPackets<< " is : "<< expVal);

  Simulator::Destroy ();


  return 0;
}
