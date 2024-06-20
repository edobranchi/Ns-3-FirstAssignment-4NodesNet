#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor.h"
#include "ns3/ipv4-flow-classifier.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SimpleNetwork");

int main (int argc, char *argv[])
{
  CommandLine cmd(__FILE__);
  cmd.Parse(argc, argv);

  Time::SetResolution(Time::NS);
  LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

  // Creazione dei nodi
  NodeContainer nodes;
  nodes.Create (4);

  // Creazione dei canali punto a punto
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("50Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("1ms"));

  // Installazione dei dispositivi sui nodi
  NetDeviceContainer devices01, devices23;
  devices01 = pointToPoint.Install (nodes.Get(0), nodes.Get(1));
  devices23 = pointToPoint.Install (nodes.Get(2), nodes.Get(3));

  // Installazione dello stack internet sui nodi
  InternetStackHelper stack;
  stack.Install (nodes);

  // Assegnazione degli indirizzi IP
  Ipv4AddressHelper address;
  Ipv4InterfaceContainer interfaces01, interfaces23;

  address.SetBase ("10.1.1.0", "255.255.255.0");
  interfaces01 = address.Assign (devices01);

  address.SetBase ("10.1.2.0", "255.255.255.0");
  interfaces23 = address.Assign (devices23);

  UdpEchoServerHelper echoServer(9);

  ApplicationContainer serverApps01 = echoServer.Install(nodes.Get(1));
  ApplicationContainer serverApps23 = echoServer.Install(nodes.Get(3));

  serverApps01.Start(Seconds(1.0));
  serverApps01.Stop(Seconds(20.0));
  serverApps23.Start(Seconds(1.0));
  serverApps23.Stop(Seconds(20.0));

  UdpEchoClientHelper echoClient01(interfaces01.GetAddress(1), 9);
  UdpEchoClientHelper echoClient23(interfaces23.GetAddress(1), 9);

  echoClient01.SetAttribute("MaxPackets", UintegerValue(0));
  echoClient01.SetAttribute("Interval", TimeValue(Seconds(0.01)));
  echoClient01.SetAttribute("PacketSize", UintegerValue(1024));
  
  echoClient23.SetAttribute("MaxPackets", UintegerValue(0));
  echoClient23.SetAttribute("Interval", TimeValue(Seconds(0.01)));
  echoClient23.SetAttribute("PacketSize", UintegerValue(1024));

  ApplicationContainer clientApps01 = echoClient01.Install(nodes.Get(0));
  ApplicationContainer clientApps23 = echoClient23.Install(nodes.Get(2));

  clientApps01.Start(Seconds(2.0));
  clientApps01.Stop(Seconds(20.0));
  
  clientApps23.Start(Seconds(2.0));
  clientApps23.Stop(Seconds(20.0));

  FlowMonitorHelper flowHelper;
  Ptr<FlowMonitor> flowMonitor = flowHelper.InstallAll();

 Simulator::Stop(Seconds(21));
  // Avvio della simulazione
  Simulator::Run ();

  // Output dei dati raccolti
  flowMonitor->CheckForLostPackets();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
  std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats();

  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); ++i)
    {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
      NS_LOG_UNCOND("Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")");
      NS_LOG_UNCOND("  Throughput: " << i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds()) / 1024 << " Kbps");
      NS_LOG_UNCOND("  Mean RTT: " << i->second.delaySum.GetSeconds() / i->second.rxPackets);
    }

  Simulator::Destroy();
  return 0;
}
