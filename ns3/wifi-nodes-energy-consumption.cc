/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/config-store-module.h"
#include "ns3/energy-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"

// Default Network Topology
// use the following command to run the script
// ./waf --run "wifi-nodes-energy-consumption --verbose=false --tracing=false --nWifiSta=10 --interval=0.01 --maxPackets=100" 
// Number of wifi Sta or Edge nodes can be increased up to 250
// 
//                          |
// -------------------------|----------------------------
// Wifi Sta 10.1.3.0
//                 AP
//  *    *    *    *
//  |    |    |    |    10.1.1.0
// n3   n4   n5   n0 -------------- n1   n2
//                   point-to-point  |    |
//                                   *    *
//                                   Ap
//                                       WiFi Edge 10.1.2.0

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("WifiEnergyExample");


/// Trace function for remaining energy at node.
void
RemainingEnergy (double oldValue, double remainingEnergy)
{
  NS_LOG_UNCOND (Simulator::Now ().GetSeconds ()
                 << "s Current remaining energy = " << remainingEnergy << "J");
}

/// Trace function for total energy consumption at node.
void
TotalEnergy (double oldValue, double totalEnergy)
{
  NS_LOG_UNCOND (Simulator::Now ().GetSeconds ()
                 << "\t"  << totalEnergy-oldValue);
}


int 
main (int argc, char *argv[])
{
  bool verbose = false;
  uint32_t nWifiSta = 3;
  uint32_t nWifiEdge = 1;
  bool tracing = false;

  CommandLine cmd;
  cmd.AddValue ("nWifiEdge", "Number of wifi edge nodes/devices", nWifiEdge);
  cmd.AddValue ("nWifiSta", "Number of wifi STA devices", nWifiSta);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  cmd.AddValue ("tracing", "Enable pcap tracing", tracing);
  cmd.Parse (argc,argv);

  // Check for valid number of csma or wifi nodes
  // 250 should be enough, otherwise IP addresses 
  // soon become an issue
  if (nWifiEdge > 250 || nWifiSta > 250)
    {
      std::cout << "Too many wifi station/edge nodes, no more than 250 each." << std::endl;
      return 1;
    }

  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

  NodeContainer p2pNodes;
  p2pNodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("10Gbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("0.01ms"));

  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install (p2pNodes);

  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifiSta);
  NodeContainer staApNode = p2pNodes.Get (0);

  NodeContainer wifiEdgeNodes;
  wifiEdgeNodes.Create (nWifiEdge);
  NodeContainer edgeApNode = p2pNodes.Get (1);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phySta = YansWifiPhyHelper::Default ();
  phySta.SetChannel (channel.Create ());

  YansWifiPhyHelper phyEdge = YansWifiPhyHelper::Default ();
  phyEdge.SetChannel (channel.Create ());

  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  WifiMacHelper macSta;
  Ssid ssid = Ssid ("ns-3-ssid-sta");
  macSta.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phySta, macSta, wifiStaNodes);
  
  macSta.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  NetDeviceContainer staApDevices;
  staApDevices = wifi.Install (phySta, macSta, staApNode);

  WifiMacHelper macEdge;
  ssid = Ssid ("ns-3-ssid-edge");
  macEdge.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer edgeDevices;
  edgeDevices = wifi.Install (phyEdge, macEdge, wifiEdgeNodes);
  
  macEdge.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  NetDeviceContainer edgeApDevices;
  edgeApDevices = wifi.Install (phyEdge, macEdge, edgeApNode);

  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (0.1),
                                 "DeltaY", DoubleValue (0.2),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  mobility.Install (wifiStaNodes);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (staApNode);
  mobility.Install (edgeApNode);
  mobility.Install (wifiEdgeNodes);

  InternetStackHelper stack;
  stack.Install (staApNode);
  stack.Install (edgeApNode);
  stack.Install (wifiStaNodes);
  stack.Install (wifiEdgeNodes);

  Ipv4AddressHelper address;

  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces;
  p2pInterfaces = address.Assign (p2pDevices);

  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer edgeInterfaces;
  edgeInterfaces = address.Assign (edgeDevices);
  address.Assign(edgeApDevices);

  address.SetBase ("10.1.3.0", "255.255.255.0");
  address.Assign (staDevices);
  address.Assign (staApDevices);

  /** Energy Model **/
  /***************************************************************************/
  /* energy source */
  BasicEnergySourceHelper staBasicSourceHelper;
  BasicEnergySourceHelper edgeBasicSourceHelper;
  // configure energy source
  staBasicSourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (100));
  edgeBasicSourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (1000));
  edgeBasicSourceHelper.Set ("BasicEnergySupplyVoltageV", DoubleValue (220));
  // install source
  EnergySourceContainer wifiStaNodesSources = staBasicSourceHelper.Install (wifiStaNodes);
  EnergySourceContainer wifiEdgeNodesSources = edgeBasicSourceHelper.Install (wifiEdgeNodes);

  /* device energy model */
  WifiRadioEnergyModelHelper radioEnergyHelper;
  // configure radio energy model
  radioEnergyHelper.Set ("TxCurrentA", DoubleValue (0.0857));
  radioEnergyHelper.Set ("RxCurrentA", DoubleValue (0.0528));
  radioEnergyHelper.Set ("IdleCurrentA", DoubleValue (0.0188));

  // install device model
  DeviceEnergyModelContainer staDeviceModels = radioEnergyHelper.Install (staDevices, wifiStaNodesSources);
  DeviceEnergyModelContainer edgeDeviceModels = radioEnergyHelper.Install (edgeDevices, wifiEdgeNodesSources);

  /***************************************************************************/

  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (wifiEdgeNodes.Get (0));
  serverApps.Start (Seconds (0));
  serverApps.Stop (Seconds (1.0));

  UdpEchoClientHelper echoClient (edgeInterfaces.GetAddress (0), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (10));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (0.001)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1472));

  ApplicationContainer clientApps = 
  echoClient.Install (wifiStaNodes);
  clientApps.Start (Seconds (0.005));
  clientApps.Stop (Seconds (0.5));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  /** connect trace sources **/
  /***************************************************************************/
  // all sources are connected to node 1
  // energy source

  Ptr<BasicEnergySource> basicSourcePtr0 = DynamicCast<BasicEnergySource> (wifiEdgeNodesSources.Get (0));

//  basicSourcePtr->TraceConnectWithoutContext ("RemainingEnergy", MakeCallback (&RemainingEnergy));
  // device energy model
  Ptr<DeviceEnergyModel> basicRadioModelPtr0 =
  basicSourcePtr0->FindDeviceEnergyModels ("ns3::WifiRadioEnergyModel").Get (0);

  NS_ASSERT (basicRadioModelPtr0 != NULL);
  basicRadioModelPtr0->TraceConnectWithoutContext ("TotalEnergyConsumption", MakeCallback (&TotalEnergy));

/*  Ptr<BasicEnergySource> basicSourcePtr1 = DynamicCast<BasicEnergySource> (wifiStaNodesSources.Get (0));

  Ptr<DeviceEnergyModel> basicRadioModelPtr1 =
  basicSourcePtr1->FindDeviceEnergyModels ("ns3::WifiRadioEnergyModel").Get (0);

  NS_ASSERT (basicRadioModelPtr1 != NULL);
  basicRadioModelPtr1->TraceConnectWithoutContext ("TotalEnergyConsumption", MakeCallback (&TotalEnergy));
*/

  Simulator::Stop (Seconds (10.0));
  if (tracing == true)
    {
      //AsciiTraceHelper ascii;
      //pointToPoint.EnableAsciiAll(ascii.CreateFileStream ("energy-wifi.tr"));
      pointToPoint.EnablePcapAll ("wifiEnergy");
     // phySta.EnablePcapAll ("wifiEnergy");
     // phyEdge.EnablePcapAll ("wifiEnergy");

    }


  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
