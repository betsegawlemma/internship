/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */ /*
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

    N0---------------N1---------------N2
       1Gbps, 1ns    1Gbps, 1ns
  
  to run the scritp use the following example
 ./waf --run "1flow_energy  --packetSize=1472 --dataRate=10Mbps --numPackets=849"

*/

#include <fstream>
#include <iostream>
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

NS_LOG_COMPONENT_DEFINE ("EnergyPerFlowExample");

class MyApp : public Application
{
public:
  MyApp ();
  virtual ~MyApp ();

  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
};

MyApp::MyApp ()
  : m_socket (0),
    m_peer (),
    m_packetSize (0),
    m_nPackets (0),
    m_dataRate (0),
    m_sendEvent (),
    m_running (false),
    m_packetsSent (0)
{
}

MyApp::~MyApp ()
{
  m_socket = 0;
}

/* static */
TypeId MyApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("MyApp")
    .SetParent<Application> ()
    .SetGroupName ("EnergyPerFlow")
    .AddConstructor<MyApp> ()
    ;
  return tid;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

void
MyApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  if (InetSocketAddress::IsMatchingType (m_peer))
    {
      m_socket->Bind ();
    }
  else
    {
      m_socket->Bind6 ();
    }
  m_socket->Connect (m_peer);
  SendPacket ();
}

void
MyApp::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
    }
}

void
MyApp::SendPacket (void)
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  m_socket->Send (packet);

  if (++m_packetsSent < m_nPackets)
    {
      ScheduleTx ();
    }
}

void
MyApp::ScheduleTx (void)
{
  if (m_running)
    {
      Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
      m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
    }
}


double offeredLoad = 0.0;
double throughput = 0.0;
double lastPacketTime = 0.0;

int
main (int argc, char *argv[])
{

  uint32_t packetSize = 1472;
  std::string dataRate = "10Mbps";
  uint32_t numPackets = 1;

  CommandLine cmd;

  cmd.AddValue ("dataRate", "Traffic rate", dataRate);
  cmd.AddValue ("packetSize", "Size of packets in Bytes", packetSize);
  cmd.AddValue ("numPackets", "Number of packets ", numPackets);

  cmd.Parse(argc,argv);


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
  basicNodeEnergy.Set ("OnConso", DoubleValue (6.936));
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
  

  LinearNetdeviceEnergyHelper linearNetdeviceEnergy;
  linearNetdeviceEnergy.Set ("IdleConso", DoubleValue (1.102));
  linearNetdeviceEnergy.Set ("OffConso", DoubleValue (0.0));
  linearNetdeviceEnergy.Set ("ByteEnergy", DoubleValue (3.212));
  linearNetdeviceEnergy.Install (nodes);

  ConsumptionLogger conso;
  conso.NodeConso (Seconds (1.0), Seconds (1.01), nodes);


  // Assign Internet addresses
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces0 = address.Assign (d0d1);

  
  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces1 = address.Assign (d1d2);

  
  // Create router nodes, initialize routing database and set up the routing table in the nodes 
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
 
  uint16_t sinkPort1 = 8080;

  Address sinkAddress1, sinkAddress2;
  Address anyAddress1, anyAddress2;
 
  sinkAddress1 = InetSocketAddress (interfaces1.GetAddress (1), sinkPort1);
  anyAddress1 = InetSocketAddress (Ipv4Address::GetAny(), sinkPort1);


  PacketSinkHelper packetSinkHelper1 ("ns3::UdpSocketFactory", anyAddress1);
  ApplicationContainer sinkApps1 = packetSinkHelper1.Install (n1n2.Get (1));
  sinkApps1.Start (Seconds (0.0));
  sinkApps1.Stop (Seconds (1.0));

  PacketSinkHelper packetSinkHelper2 ("ns3::UdpSocketFactory", anyAddress2);
  ApplicationContainer sinkApps2 = packetSinkHelper2.Install (n1n2.Get (1));
  Ptr<Socket> ns3UDPSocket1 = Socket::CreateSocket (n0n1.Get (0), UdpSocketFactory::GetTypeId ());

  Ptr<MyApp> app1 = CreateObject<MyApp> ();
  app1->Setup (ns3UDPSocket1, sinkAddress1, packetSize, numPackets, DataRate (dataRate));
  n0n1.Get (0)->AddApplication (app1);
  app1->SetStartTime (Seconds (0.0));
  app1->SetStopTime (Seconds (1.0));

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
  NS_LOG_UNCOND( "  Offered Load: " << offeredLoad << " Mbps");

  NS_LOG_UNCOND( "  Rx Packets:   " << flow->second.rxPackets );
  NS_LOG_UNCOND( "  Rx Bytes:   " << flow->second.rxBytes );

  throughput = flow->second.rxBytes * 8.0 / (flow->second.timeLastRxPacket.GetSeconds () - flow->second.timeFirstRxPacket.GetSeconds ()) / 1000000.0;
  NS_LOG_UNCOND( "  Throughput: " << throughput << " Mbps");

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

  
  Simulator::Destroy ();

  return 0;
}
