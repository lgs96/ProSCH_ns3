/*s program is free software; you can redistribute it and/or modify
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
 * Author: Michele Polese <michele.polese@gmail.com>
 */

#include "ns3/mmwave-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include "ns3/mmwave-point-to-point-epc-helper.h"
//#include "ns3/gtk-config-store.h"
#include <ns3/buildings-helper.h>
#include <ns3/buildings-module.h>
#include <ns3/random-variable-stream.h>
#include <ns3/lte-ue-net-device.h>

#include <iostream>
#include <ctime>
#include <stdlib.h>
#include <list>


using namespace ns3;
using namespace std;
/**
 * Sample simulation script for MC device. It instantiates a LTE and a MmWave eNodeB,
 * attaches one MC UE to both and starts a flow for the UE to  and from a remote host.
 */

NS_LOG_COMPONENT_DEFINE ("McFirstExample");

class MyAppTag : public Tag
{
	public:
		MyAppTag ()
		{
		}

		MyAppTag (Time sendTs) : m_sendTs (sendTs)
	{
	}

		static TypeId GetTypeId (void)
		{
			static TypeId tid = TypeId ("ns3::MyAppTag")
				.SetParent<Tag> ()
				.AddConstructor<MyAppTag> ();
			return tid;
		}

		virtual TypeId  GetInstanceTypeId (void) const
		{
			return GetTypeId ();
		}

		virtual void  Serialize (TagBuffer i) const
		{
			i.WriteU64 (m_sendTs.GetNanoSeconds());
		}

		virtual void  Deserialize (TagBuffer i)
		{
			m_sendTs = NanoSeconds (i.ReadU64 ());
		}

		virtual uint32_t  GetSerializedSize () const
		{
			return sizeof (m_sendTs);
		}

		virtual void Print (std::ostream &os) const
		{
			std::cout << m_sendTs;
		}

		Time m_sendTs;
};


class MyApp : public Application
{
	public:

		MyApp ();
		virtual ~MyApp();
		void ChangeDataRate (DataRate rate);
		void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate, bool isRandom);



	private:
		virtual void StartApplication (void);
		virtual void StopApplication (void);

		void ScheduleTx (void);
		void SendPacket (void);
		void RandomPacket (void);//defined by gsoul 180910 for generate random packet 

		Ptr<Socket>     m_socket;
		Address         m_peer;
		uint32_t        m_packetSize;
		uint32_t        m_nPackets;
		DataRate        m_dataRate;
		EventId         m_sendEvent;
		bool            m_running;
		uint32_t        m_packetsSent;
		bool		m_isRandom;
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

MyApp::~MyApp()
{
	m_socket = 0;
}

	void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate,bool isRandom)
{
	m_socket = socket;
	m_peer = address;
	m_packetSize = packetSize;
	m_nPackets = nPackets;
	m_dataRate = dataRate;
	m_isRandom = isRandom;
}

	void
MyApp::ChangeDataRate (DataRate rate)
{
	m_dataRate = rate;
}

	void
MyApp::StartApplication (void)
{
	m_running = true;
	m_packetsSent = 0;
	m_socket->Bind ();
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
//Modified by gsoul 180910. This function supports both random traffic and constant traffic.
	void
MyApp::SendPacket (void)
{
	Ptr<Packet> packet = Create<Packet> (m_packetSize);
	MyAppTag tag (Simulator::Now ());

	m_socket->Send (packet);
	if (++m_packetsSent < m_nPackets)
	{
		if(!m_isRandom)
		{
			ScheduleTx ();
		}
		else
		{
			RandomPacket ();
		}
	}
}



//ScheduleTx send defined size of packet every time that divided by data rate with bits
	void
MyApp::ScheduleTx (void)
{
	if (m_running)
	{
		Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
		m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
	}
}

//Defined by gsoul 180910. This function sends packet with random interval generated by exponential distribution.
	void 
MyApp::RandomPacket(void)
{
	if(m_running)
	{
		double mean = m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ());
		double bound = 0.0;

		Ptr<ExponentialRandomVariable> x = CreateObject<ExponentialRandomVariable>();
		x->SetAttribute("Mean", DoubleValue(mean));
		x->SetAttribute("Bound", DoubleValue(bound));

		Time tNext (Seconds (x->GetValue()));
		m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket,this);	
	}	
}
/*
   static void
   CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
   {
 *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldCwnd << "\t" << newCwnd << std::endl;
 }


 static void
 RttChange (Ptr<OutputStreamWrapper> stream, Time oldRtt, Time newRtt)
 {
 *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldRtt.GetSeconds () << "\t" << newRtt.GetSeconds () << std::endl;
 }
 */



double instantPacketSize[100], packetRxTime[100], lastPacketRxTime[100];
double sumPacketSize[100];

static void
Rx (Ptr<OutputStreamWrapper> stream, uint16_t i, Ptr<const Packet> packet, const Address &from){
	packetRxTime[i] = Simulator::Now().GetSeconds();
	if (lastPacketRxTime[i] == packetRxTime[i]){
		instantPacketSize[i] += packet->GetSize();
		return;
	}
	else{
		sumPacketSize[i] += instantPacketSize[i];
		*stream->GetStream () << lastPacketRxTime[i] << "\t" << instantPacketSize[i] << "\t" << sumPacketSize[i]
			<< std::endl;
		lastPacketRxTime[i] =  packetRxTime[i];
		instantPacketSize[i] = packet->GetSize();
	}
}

uint64_t lastTotalRx[100];
uint16_t c[10];
double t_total[10];
	void
CalculateThroughput (Ptr<OutputStreamWrapper> stream, Ptr<PacketSink> sink, uint16_t i)
{
	Time now = Simulator::Now ();                                         /* Return the simulator's virtual time. */
	double cur = (sink->GetTotalRx () - lastTotalRx[i]) * (double) 8 / 1e5;     /* Convert Application RX Packets to MBits. */
	c[i]++;
	t_total[i]+=cur;
	*stream->GetStream()  << now.GetSeconds () << "\t" << cur <<"\t"<<(double)(t_total[i]/c[i])<<std::endl;
	lastTotalRx[i] = sink->GetTotalRx ();
	Simulator::Schedule (MilliSeconds (100), &CalculateThroughput,stream,sink,i);
}



	static void 
Ssthresh (Ptr<OutputStreamWrapper> stream, uint32_t oldSsthresh, uint32_t newSsthresh)
{
	*stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldSsthresh << "\t" << newSsthresh << std::endl;
}

	void
ChangeSpeed(Ptr<Node>  n, Vector speed)
{
	n->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (speed);
}
	static void
CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
	*stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldCwnd << "\t" << newCwnd << std::endl;
}

	static void
RttChange (Ptr<OutputStreamWrapper> stream, Time oldRtt, Time newRtt)
{
	*stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldRtt.GetSeconds () << "\t" << newRtt.GetSeconds () << std::endl;
}

double ack_throughput[100];

	static void
Reset_ack(Ptr<OutputStreamWrapper> stream, uint16_t i)
{
	std::cout<<"Simulator time: "<<Simulator::Now().GetSeconds()<<" for "<<i+1<<std::endl;
	*stream->GetStream () << Simulator::Now().GetSeconds()<<"\t"<<ack_throughput[i]/1e5<<std::endl;	
	ack_throughput[i] = 0;
	Simulator::Schedule (MilliSeconds(100), &Reset_ack,stream,i);
}

	static void
RxChange (Ptr<OutputStreamWrapper> stream, uint16_t i, const Ptr<const Packet> packet, const TcpHeader &header, const Ptr<const TcpSocketBase> socket)
{
	ack_throughput[i] += 54*8;
}

	static void
RTOChange (Ptr <OutputStreamWrapper> stream, Time oldrto, Time newrto)
{
	*stream->GetStream () <<Simulator::Now().GetSeconds() << "\t" <<oldrto.GetSeconds()<<"\t"<<newrto.GetSeconds()<<std::endl;
}

	static void
Traces(uint16_t nodeNum,uint16_t ExNum)
{
	AsciiTraceHelper asciiTraceHelper;

	std::ostringstream pathCW;
	pathCW<<"/NodeList/"<<nodeNum+2<<"/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow";

	std::ostringstream fileCW;
	fileCW<<ExNum<<"_tcp_cwnd_ue"<<nodeNum+1<<".txt";

	std::ostringstream pathRTT;
	pathRTT<<"/NodeList/"<<nodeNum+2<<"/$ns3::TcpL4Protocol/SocketList/0/RTT";

	std::ostringstream fileRTT;
	fileRTT<<ExNum<<"_tcp_rtt_ue"<<nodeNum+1<<".txt";

	std::ostringstream pathSST;
	pathSST<<"/NodeList/"<<nodeNum+2<<"/$ns3::TcpL4Protocol/SocketList/0/SlowStartThreshold";

	std::ostringstream fileSST;
	fileSST<<ExNum<<"_tcp_sst_ue"<<nodeNum+1<<".txt";

	std::ostringstream pathRx;
	pathRx<<"/NodeList/"<<nodeNum+2<<"/$ns3::TcpL4Protocol/SocketList/0/Rx";

	std::ostringstream fileRx;
	fileRx<<ExNum<<"_tcp_ack_ue"<<nodeNum+1<<".txt";

	std::ostringstream pathRTO;
	pathRTO<<"/NodeList/"<<nodeNum+2<<"/$ns3::TcpL4Protocol/SocketList/0/RTO";

	std::ostringstream fileRTO;
	fileRTO<<ExNum<<"_tcp_rto_ue"<<nodeNum+1<<".txt";

	Ptr<OutputStreamWrapper> stream1 = asciiTraceHelper.CreateFileStream (fileCW.str ().c_str ());
	Config::ConnectWithoutContext (pathCW.str ().c_str (), MakeBoundCallback(&CwndChange, stream1));

	Ptr<OutputStreamWrapper> stream2 = asciiTraceHelper.CreateFileStream (fileRTT.str ().c_str ());
	Config::ConnectWithoutContext (pathRTT.str ().c_str (), MakeBoundCallback(&RttChange, stream2));

	Ptr<OutputStreamWrapper> stream4 = asciiTraceHelper.CreateFileStream (fileSST.str ().c_str ());
	Config::ConnectWithoutContext (pathSST.str ().c_str (), MakeBoundCallback(&Ssthresh, stream4));

	Ptr<OutputStreamWrapper> stream5 = asciiTraceHelper.CreateFileStream (fileRx.str ().c_str ());
	Config::ConnectWithoutContext (pathRx.str ().c_str (), MakeBoundCallback(&RxChange, stream5, nodeNum));

	Ptr<OutputStreamWrapper> stream6 = asciiTraceHelper.CreateFileStream (fileRTO.str ().c_str ());
	Config::ConnectWithoutContext (pathRTO.str ().c_str (), MakeBoundCallback(&RTOChange, stream6));
}

	int
main (int argc, char *argv[])
{
	//LogComponentEnable ("LteUeRrc", LOG_FUNCTION);
	//LogComponentEnable ("LteEnbRrc", LOG_LEVEL_LOGIC);
	//LogComponentEnable("EpcUeNas", LOG_FUNCTION);
	//  LogComponentEnable ("LteEnbRrc", LOG_LEVEL_INFO);
	//  LogComponentEnable ("LteRlcTm", LOG_FUNCTION);
	// LogComponentEnable("MmWavePointToPointEpcHelper",LOG_FUNCTION);
	//  LogComponentEnable("EpcUeNas",LOG_FUNCTION);
	// LogComponentEnable("LtePdcp", LOG_FUNCTION);
	// LogComponentEnable ("MmWaveSpectrumPhy", LOG_FUNCTION);
	// LogComponentEnable ("MmWaveUeMac", LOG_FUNCTION);
	//LogComponentEnable ("MmWaveEnbMac", LOG_FUNCTION);
	//LogComponentEnable ("LteUeMac", LOG_FUNCTION);
	// LogComponentEnable ("LteEnbMac", LOG_FUNCTION);
	//  LogComponentEnable ("LteEnbMac", LOG_INFO);
	////  LogComponentEnable ("MmWaveEnbPhy", LOG_FUNCTION);
	//LogComponentEnable ("MmWaveUePhy", LOG_FUNCTION);
	// LogComponentEnable ("MmWaveEnbMac", LOG_FUNCTION);
	//LogComponentEnable ("UdpServer", LOG_FUNCTION);
	//LogComponentEnable ("PacketSink", LOG_FUNCTION);
	//LogComponentEnable("MmWavePropagationLossModel",LOG_LEVEL_ALL);
	//  LogComponentEnable("LteRrcProtocolReal", LOG_FUNCTION);
	//LogComponentEnable ("EpcMme", LOG_FUNCTION);
	// LogComponentEnable ("mmWavePhyRxTrace", LOG_FUNCTION);
	//LogComponentEnable ("MmWaveRrMacScheduler", LOG_FUNCTION);
	// LogComponentEnable("McUeNetDevice", LOG_FUNCTION);
	// LogComponentEnable("EpcSgwPgwApplication", LOG_FUNCTION);
	// LogComponentEnable("EpcEnbApplication", LOG_FUNCTION);
	//LogComponentEnable("MmWaveEnbMac", LOG_LOGIC);
	// LogComponentEnable ("LteEnbMac", LOG_FUNCTION);
	//  LogComponentEnable ("LteEnbMac", LOG_INFO);
	////  LogComponentEnable ("MmWaveEnbPhy", LOG_FUNCTION);
	//LogComponentEnable ("MmWaveUePhy", LOG_FUNCTION);
	// LogComponentEnable ("MmWaveEnbMac", LOG_FUNCTION);
	//LogComponentEnable ("UdpServer", LOG_FUNCTION);
	//LogComponentEnable ("PacketSink", LOG_FUNCTION);
	//LogComponentEnable("MmWavePropagationLossModel",LOG_LEVEL_ALL);
	//  LogComponentEnable("LteRrcProtocolReal", LOG_FUNCTION);
	//LogComponentEnable ("EpcMme", LOG_FUNCTION);
	// LogComponentEnable ("mmWavePhyRxTrace", LOG_FUNCTION);
	//LogComponentEnable ("MmWaveRrMacScheduler", LOG_FUNCTION);
	// LogComponentEnable("McUeNetDevice", LOG_FUNCTION);
	// LogComponentEnable("EpcSgwPgwApplication", LOG_FUNCTION);
	// LogComponentEnable("EpcEnbApplication", LOG_FUNCTION);
	//LogComponentEnable("MmWaveEnbMac", LOG_LOGIC);
	// LogComponentEnable ("LteEnbMac", LOG_FUNCTION);
	//  LogComponentEnable ("LteEnbMac", LOG_INFO);
	////  LogComponentEnable ("MmWaveEnbPhy", LOG_FUNCTION);
	//LogComponentEnable ("MmWaveUePhy", LOG_FUNCTION);
	// LogComponentEnable ("MmWaveEnbMac", LOG_FUNCTION);
	//LogComponentEnable ("UdpServer", LOG_FUNCTION);
	//LogComponentEnable ("PacketSink", LOG_FUNCTION);
	//LogComponentEnable("MmWavePropagationLossModel",LOG_LEVEL_ALL);
	//  LogComponentEnable("LteRrcProtocolReal", LOG_FUNCTION);
	//LogComponentEnable ("EpcMme", LOG_FUNCTION);
	// LogComponentEnable ("mmWavePhyRxTrace", LOG_FUNCTION);
	//LogComponentEnable ("MmWaveRrMacScheduler", LOG_FUNCTION);
	// LogComponentEnable("McUeNetDevice", LOG_FUNCTION);
	// LogComponentEnable("EpcSgwPgwApplication", LOG_FUNCTION);
	// LogComponentEnable("EpcEnbApplication", LOG_FUNCTION);
	//LogComponentEnable("MmWaveEnbMac", LOG_LOGIC);
	// LogComponentEnable ("LteEnbMac", LOG_FUNCTION);
	//  LogComponentEnable ("LteEnbMac", LOG_INFO);
	////  LogComponentEnable ("MmWaveEnbPhy", LOG_FUNCTION);
	//LogComponentEnable ("MmWaveUePhy", LOG_FUNCTION);
	// LogComponentEnable ("MmWaveEnbMac", LOG_FUNCTION);
	//LogComponentEnable ("UdpServer", LOG_FUNCTION);
	//LogComponentEnable ("PacketSink", LOG_FUNCTION);
	//LogComponentEnable("MmWavePropagationLossModel",LOG_LEVEL_ALL);
	//  LogComponentEnable("LteRrcProtocolReal", LOG_FUNCTION);
	//LogComponentEnable ("EpcMme", LOG_FUNCTION);
	// LogComponentEnable ("mmWavePhyRxTrace", LOG_FUNCTION);
	//LogComponentEnable ("MmWaveRrMacScheduler", LOG_FUNCTION);
	// LogComponentEnable("McUeNetDevice", LOG_FUNCTION);
	// LogComponentEnable("EpcSgwPgwApplication", LOG_FUNCTION);
	// LogComponentEnable("EpcEnbApplication", LOG_FUNCTION);
	//LogComponentEnable("MmWaveEnbMac", LOG_LOGIC);
	// LogComponentEnable ("MmWaveSpectrumPhy", LOG_FUNCTION);
	// LogComponentEnable ("MmWaveUeMac", LOG_FUNCTION);
	//LogComponentEnable ("MmWaveEnbMac", LOG_FUNCTION);
	//LogComponentEnable ("LteUeMac", LOG_FUNCTION);
	// LogComponentEnable ("LteEnbMac", LOG_FUNCTION);
	//  LogComponentEnable ("LteEnbMac", LOG_INFO);
	////  LogComponentEnable ("MmWaveEnbPhy", LOG_FUNCTION);
	//LogComponentEnable ("MmWaveUePhy", LOG_FUNCTION);
	// LogComponentEnable ("MmWaveEnbMac", LOG_FUNCTION);
	//LogComponentEnable ("UdpServer", LOG_FUNCTION);
	//LogComponentEnable ("PacketSink", LOG_FUNCTION);
	//LogComponentEnable("MmWavePropagationLossModel",LOG_LEVEL_ALL);
	//  LogComponentEnable("LteRrcProtocolReal", LOG_FUNCTION);
	//LogComponentEnable ("EpcMme", LOG_FUNCTION);
	// LogComponentEnable ("mmWavePhyRxTrace", LOG_FUNCTION);
	//LogComponentEnable ("MmWaveRrMacScheduler", LOG_FUNCTION);
	// LogComponentEnable("McUeNetDevice", LOG_FUNCTION);
	// LogComponentEnable("EpcSgwPgwApplication", LOG_FUNCTION);
	// LogComponentEnable("EpcEnbApplication", LOG_FUNCTION);
	//LogComponentEnable("MmWaveEnbMac", LOG_LOGIC);
	// LogComponentEnable ("LteEnbMac", LOG_FUNCTION);
	//  LogComponentEnable ("LteEnbMac", LOG_INFO);
	////  LogComponentEnable ("MmWaveEnbPhy", LOG_FUNCTION);
	//LogComponentEnable ("MmWaveUePhy", LOG_FUNCTION);
	// LogComponentEnable ("MmWaveEnbMac", LOG_FUNCTION);
	//LogComponentEnable ("UdpServer", LOG_FUNCTION);
	//LogComponentEnable ("PacketSink", LOG_FUNCTION);
	//LogComponentEnable("MmWavePropagationLossModel",LOG_LEVEL_ALL);
	//  LogComponentEnable("LteRrcProtocolReal", LOG_FUNCTION);
	//LogComponentEnable ("EpcMme", LOG_FUNCTION);
	// LogComponentEnable ("mmWavePhyRxTrace", LOG_FUNCTION);
	//LogComponentEnable ("MmWaveRrMacScheduler", LOG_FUNCTION);
	// LogComponentEnable("McUeNetDevice", LOG_FUNCTION);
	// LogComponentEnable("EpcSgwPgwApplication", LOG_FUNCTION);
	// LogComponentEnable("EpcEnbApplication", LOG_FUNCTION);
	//LogComponentEnable("MmWaveEnbMac", LOG_LOGIC);
	// LogComponentEnable("MmWaveEnbPhy", LOG_FUNCTION);
	//LogComponentEnable("LteEnbMac", LOG_FUNCTION);
	// LogComponentEnable("LteUePhy", LOG_FUNCTION);
	//LogComponentEnable ("LteEnbPhy", LOG_FUNCTION);
	//  LogComponentEnable("MmWavePointToPointEpcHelper", LOG_FUNCTION);
	//  LogComponentEnable("MmWaveHelper",LOG_FUNCTION);
	//LogComponentEnable("EpcX2",LOG_LEVEL_LOGIC);
	// LogComponentEnable("EpcX2",LOG_LOGIC);
	// LogComponentEnable ("mmWaveRrcProtocolIdeal", LOG_FUNCTION);
	//LogComponentEnable ("MmWaveLteRrcProtocolReal", LOG_FUNCTION);
	//LogComponentEnable("EpcX2Header", LOG_FUNCTION);
        //LogComponentEnable("McEnbPdcp",LOG_LEVEL_INFO);
	//	LogComponentEnable("McUePdcp",LOG_FUNCTION);
	//	LogComponentEnable ("McUePdcp", LOG_LOGIC);
	LogComponentEnable("LteRlcAm", LOG_LEVEL_LOGIC);
	//  LogComponentEnable("LteRlcUmLowLat", LOG_FUNCTION);
	//  LogComponentEnable("EpcS1ap", LOG_FUNCTION);
	// LogComponentEnable("EpcMmeApplication", LOG_FUNCTION);
	// LogComponentEnable("EpcMme", LOG_FUNCTION);
	// LogComponentEnable("LteRrcProtocolIdeal", LOG_LEVEL_ALL);
	//LogComponentEnable("MmWaveFlexTtiMacScheduler", LOG_FUNCTION);
	//  LogComponentEnable("AntennaArrayModel", LOG_FUNCTION);
	// LogComponentEnable("UdpServer", LOG_LEVEL_INFO);
	//LogComponentEnable("UdpClient", LOG_LEVEL_INFO);
	//  LogComponentEnable ("MmWavePointToPointEpcHelper",LOG_FUNCTION);
	//  LogComponentEnable ("Socket",LOG_LEVEL_ALL);
	// LogComponentEnable("UdpSocketImpl", LOG_LEVEL_ALL);
	// LogComponentEnable("UdpL4Protocol", LOG_LEVEL_ALL);
	//  LogComponentEnable("IpL4Protocol", LOG_LEVEL_ALL);
	// LogComponentEnable("Ipv4EndPoint", LOG_LEVEL_ALL);

	//LogComponentEnable("MmWaveSpectrumPhy", LOG_LEVEL_FUNCTION);
	// LogComponentEnable("MmWaveHelper", LOG_INFO);
	// LogComponentEnable("MmWaveHelper", LOG_FUNCTION);

	// LogComponentEnable ("mmWaveInterference", LOG_LEVEL_FUNCTION);
	// LogComponentEnable("LteSpectrumPhy", LOG_LEVEL_ALL);
	//	 LogComponentEnable("TcpCongestionOps", LOG_LEVEL_DEBUG);
	//LogComponentEnable("TcpSocketBase", LOG_LEVEL_INFO);
	//LogComponentEnable("MmWave3gppChannel",LOG_FUNCTION);
	//LogComponentEnable("MmWaveChannelRaytracing",LOG_FUNCTION);
	//LogComponentEnable("MmWaveBeamforming",LOG_FUNCTION);
	//LogComponentEnable("MmWaveChannelMatrix",LOG_LEVEL_ALL);
	//LogComponentEnable("MmWaveBearerStatsCalculator", LOG_LEVEL_FUNCTION);
	//LogComponentEnable("MmWaveBearerStatsConnector", LOG_LEVEL_FUNCTION);
	//LogComponentEnable("MultiModelSpectrumChannel", LOG_FUNCTION);
	//LogComponentEnable ("EventImpl",LOG_FUNCTION);
	//LogComponentEnable("LteRlcUmLowLat", LOG_FUNCTION);
	//LogComponentEnable("LteRlcUm",LOG_FUNCTION);
	//LogComponentEnable ("MmWavePhyMacCommon",LOG_LEVEL_DEBUG);

	std::cout << "splitting number : " << std::endl;
	std::cout << "0,1 -> single path" << std::endl;//0:73GHz, 1:28GHz
	std::cout << "2 -> alternative splitting " << std::endl;
	std::cout << "3 -> p-Splitting" <<std::endl;
	std::cout << "4 -> SDF " << std::endl;
	std::cout << "5 -> SQF " << std::endl;
	// uint16_t numberOfNodes = 1;
	// uint16_t nodeNum=1;

	uint16_t ExperimentNum = 2;	

	double simTime = 20;
	double interPacketInterval = 20;  // 500 microseconds
	bool harqEnabled = true;
	bool rlcAmEnabled = true;
	bool fixedTti = false;
	unsigned symPerSf = 24;
	double sfPeriod = 100.0;
	bool tcp = true, dl= true, ul=false;
	double x2Latency = 10, mmeLatency=15.0;
	//	bool isEnablePdcpReordering = true;
	//	bool isEnableLteMmwave = false;
	double TxPower = 15;
	uint16_t typeOfSplitting = 1; // 3 : p-split
	//	bool isDuplication = false; //gsoul 180905
	uint16_t Velocity = 10;
	std::string scheduler ="MmWaveFlexTtiMacScheduler";
	std::string pathLossModel = "BuildingsObstaclePropagationLossModel";
	std::string X2dataRate = "100Gb/s";
	uint32_t nPacket = 0xffffffff;
	bool isRandom = true; //gsoul 180910 for random traffic generate
	bool ReadBuilding = true;
	int BuildingNum = 40;
	int x2LinkDelay = 10;
	// Command line arguments
	CommandLine cmd;
	// cmd.AddValue("numberOfNodes", "Number of eNodeBs + UE pairs", nodeNum);
	cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
	cmd.AddValue("interPacketInterval", "Inter packet interval [ms])", interPacketInterval);
	cmd.AddValue ("velocity" , "UE's velocity", Velocity);
	cmd.AddValue ("isTcp", "TCP or UDP", tcp);
	cmd.AddValue("x2LinkDataRate", "X2 link data rate " , X2dataRate);
	cmd.AddValue("x2LinkDelay" , "X2 link delay", x2LinkDelay);
	cmd.AddValue("pathLossModel", "path loss modles", pathLossModel);
	cmd.AddValue ("scheduler", "lte scheduler", scheduler);
	cmd.AddValue("rlcAmEnabled", "lte rlc avilability",rlcAmEnabled);
	cmd.AddValue("harqEnabled", "harq enable or not", harqEnabled);
	cmd.AddValue("typeOfSplitting", "splitting algorithm type",typeOfSplitting);
	cmd.AddValue("nPacket", "number of packets" , nPacket);

	cmd.Parse(argc, argv);
	// Config::SetDefault ("ns3::LteEnbRrc::EpsBearerToRlcMapping", EnumValue (ns3::LteEnbRrc::RLC_AM_ALWAYS));
	Config::SetDefault("ns3::LteEnbRrc::SecondaryCellHandoverMode", EnumValue(2));
	//	Config::SetDefault("ns3::McUePdcp::EnableReordering", BooleanValue(isEnablePdcpReordering));
	//	Config::SetDefault("ns3::McEnbPdcp::EnableDuplication", BooleanValue(isDuplication));
	Config::SetDefault ("ns3::MmWaveHelper::RlcAmEnabled", BooleanValue(rlcAmEnabled));
	Config::SetDefault ("ns3::MmWaveHelper::HarqEnabled", BooleanValue(harqEnabled));
	Config::SetDefault ("ns3::MmWaveEnbPhy::TxPower",DoubleValue(TxPower));
	//	Config::SetDefault ("ns3::MmWaveSpectrumPhy::DisableInterference",BooleanValue(true));
	Config::SetDefault ("ns3::MmWaveFlexTtiMacScheduler::HarqEnabled", BooleanValue(harqEnabled));
	Config::SetDefault ("ns3::MmWaveFlexTtiMaxWeightMacScheduler::HarqEnabled", BooleanValue(harqEnabled));
	Config::SetDefault ("ns3::MmWaveFlexTtiMaxWeightMacScheduler::FixedTti", BooleanValue(fixedTti));
	Config::SetDefault ("ns3::MmWaveFlexTtiMaxWeightMacScheduler::SymPerSlot", UintegerValue(6));
	Config::SetDefault ("ns3::MmWavePhyMacCommon::ResourceBlockNum", UintegerValue(1));
	//Config::SetDefault ("ns3::MmWavePhyMacCommon::ChunkPerRB", UintegerValue(72));
	Config::SetDefault ("ns3::MmWavePhyMacCommon::SymbolsPerSubframe", UintegerValue(symPerSf));
	Config::SetDefault ("ns3::MmWavePhyMacCommon::SubframePeriod", DoubleValue(sfPeriod));
	Config::SetDefault ("ns3::MmWavePhyMacCommon::TbDecodeLatency", UintegerValue(200.0));
	Config::SetDefault ("ns3::MmWavePhyMacCommon::NumHarqProcess", UintegerValue((uint32_t)100));
	Config::SetDefault ("ns3::MmWaveBeamforming::LongTermUpdatePeriod", TimeValue (MilliSeconds (100.0)));
	Config::SetDefault ("ns3::MmWavePhyMacCommon::ChunkWidth",DoubleValue(13.889e6));//1000MHz bandwidth
	Config::SetDefault ("ns3::LteEnbRrc::SystemInformationPeriodicity", TimeValue (MilliSeconds (5.0)));
	// Config::SetDefault ("ns3::MmWavePropagationLossModel::ChannelStates", StringValue ("n"));
	Config::SetDefault ("ns3::LteEnbNetDevice::UlBandwidth",UintegerValue(100));//20MHz bandwidth
	Config::SetDefault ("ns3::LteRlcAm::ReportBufferStatusTimer", TimeValue(MicroSeconds(100.0)));
	Config::SetDefault ("ns3::LteRlcUmLowLat::ReportBufferStatusTimer", TimeValue(MicroSeconds(100.0)));
	Config::SetDefault ("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue (320));
	Config::SetDefault ("ns3::LteEnbRrc::FirstSibTime", UintegerValue (2));
	Config::SetDefault ("ns3::MmWavePointToPointEpcHelper::X2LinkDelay", TimeValue (MilliSeconds(x2Latency)));
	Config::SetDefault ("ns3::MmWavePointToPointEpcHelper::X2LinkDataRate", DataRateValue(DataRate ("1000Gb/s")));
	Config::SetDefault ("ns3::MmWavePointToPointEpcHelper::X2LinkMtu",  UintegerValue(10000));
	Config::SetDefault ("ns3::MmWavePointToPointEpcHelper::S1uLinkDelay", TimeValue (MicroSeconds(1000)));
	Config::SetDefault ("ns3::MmWavePointToPointEpcHelper::S1apLinkDelay", TimeValue (MicroSeconds(mmeLatency)));
	//	Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpNewReno::GetTypeId ()));
	Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (1024*1024*100));
	Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (1024*1024*100));
	Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1400));	

	Config::SetDefault ("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue (100 * 1024 * 1024));
	Config::SetDefault ("ns3::LteRlcUmLowLat::MaxTxBufferSize", UintegerValue (100 * 1024 * 1024));
	Config::SetDefault ("ns3::LteRlcAm::StatusProhibitTimer", TimeValue(MilliSeconds(1.0)));
	Config::SetDefault ("ns3::LteRlcAm::MaxTxBufferSize", UintegerValue (64 *1024 * 1024));

	Config::SetDefault ("ns3::PointToPointEpcHelper::X2LinkDelay", TimeValue (MilliSeconds(x2LinkDelay)));
	Config::SetDefault ("ns3::PointToPointEpcHelper::X2LinkDataRate", DataRateValue (DataRate(X2dataRate)));

	//	Config::SetDefault("ns3::McEnbPdcp::numberOfAlgorithm",UintegerValue(typeOfSplitting));
	//	Config::SetDefault("ns3::McEnbPdcp::enableLteMmWaveDC", BooleanValue(isEnableLteMmwave));
	Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpNewReno::GetTypeId ()));

	Ptr<MmWaveHelper> mmwaveHelper = CreateObject<MmWaveHelper> ();
	mmwaveHelper->SetSchedulerType ("ns3::"+scheduler);

	Ptr<MmWavePointToPointEpcHelper> epcHelper = CreateObject<MmWavePointToPointEpcHelper> ();
	mmwaveHelper->SetEpcHelper (epcHelper);
	mmwaveHelper->SetHarqEnabled (harqEnabled);

	//mmwaveHelper->SetAttribute ("PathlossModel", StringValue ("ns3::"+pathLossModel));
	mmwaveHelper->SetAttribute ("PathlossModel", StringValue ("ns3::MmWave3gppBuildingsPropagationLossModel"));
	mmwaveHelper->SetAttribute("ChannelModel", StringValue("ns3::MmWave3gppChannel"));
	mmwaveHelper->Initialize();
	cmd.Parse(argc, argv);

	uint16_t nodeNum = 1;

	Ptr<Node> pgw = epcHelper->GetPgwNode ();
	NodeContainer remoteHostContainer;
	remoteHostContainer.Create (nodeNum);
	InternetStackHelper internet;
	internet.Install (remoteHostContainer);
	Ipv4Address remoteHostAddr[nodeNum];
	Ipv4StaticRoutingHelper ipv4RoutingHelper;
	Ptr<Node> remoteHost ;
	for (uint16_t i=0 ; i<nodeNum; i++)
	{
		// Create the Internet by connecting remoteHost to pgw. Setup routing too
		remoteHost = remoteHostContainer.Get (i);
		PointToPointHelper p2ph;
		p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
		p2ph.SetDeviceAttribute ("Mtu", UintegerValue (2500));
		p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
		NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
		//		p2ph.EnablePcapAll("Tcp_highspeed");		

		Ipv4AddressHelper ipv4h;
		std::ostringstream subnet;
		subnet<<i+1<<".1.0.0";
		ipv4h.SetBase (subnet.str ().c_str (), "255.255.0.0");
		Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
		// interface 0 is localhost, 1 is the p2p device

		remoteHostAddr[i] = internetIpIfaces.GetAddress (1);
		Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
		remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.255.0.0"), 1);
	}
	// create LTE, mmWave eNB nodes and UE node
	NodeContainer ueNodes;
	NodeContainer mmWaveEnbNodes_28G;
	//	NodeContainer mmWaveEnbNodes_73G;
	NodeContainer lteEnbNodes;
	NodeContainer allEnbNodes;

	//	mmWaveEnbNodes_73G.Create(3);
	mmWaveEnbNodes_28G.Create(6);
	lteEnbNodes.Create(1);
	ueNodes.Create(nodeNum);

	allEnbNodes.Add(lteEnbNodes);
	//	allEnbNodes.Add(mmWaveEnbNodes_73G);
	allEnbNodes.Add(mmWaveEnbNodes_28G);
	//	std::ofstream f ("enb_topology.txt");

	Vector mmw1Position = Vector(0.0,0.0, 35);  ///28Ghz //path 0
	Vector mmw2Position = Vector(0.0, 50.0, 35); //28Ghz // path 0
	Vector mmw3Position = Vector(0.0, 100.0, 35); //28Ghz // path 0

	Vector mmw4Position = Vector(100.0, 0.0, 35); //28Ghz // path 1
	Vector mmw5Position = Vector(100.0,50.0, 35);  ///28Ghz //path 1
	Vector mmw6Position = Vector(100.0, 100, 35); //28Ghz // path 1
	//Vector mmw7Position = Vector (100.0, 60,25);
	//Vector mmw8Position = Vector(100.0, 90, 25); //73Ghz // path 1
	// Vector mmw7Position = Vector(0.0, 40, 12); //73Ghz // path 1
	//uint16_t t= 1;

	// Install Mobility Model
	Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
	enbPositionAlloc->Add (Vector ((double)30.0, -5, 35));
	enbPositionAlloc->Add (mmw1Position);
	enbPositionAlloc->Add (mmw3Position);
	enbPositionAlloc->Add (mmw5Position);
	enbPositionAlloc->Add (mmw2Position);
	enbPositionAlloc->Add (mmw4Position);
	enbPositionAlloc->Add (mmw6Position);
	//	enbPositionAlloc->Add (mmw7Position);
	//enbPositionAlloc->Add (mmw8Position);

	MobilityHelper enbmobility;
	enbmobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	enbmobility.SetPositionAllocator(enbPositionAlloc);
	enbmobility.Install (allEnbNodes);

	MobilityHelper uemobility;

	Ptr<ListPositionAllocator> uePositionAlloc = CreateObject<ListPositionAllocator> ();
	//for(uint16_t i =0 ; i<ueNodes.GetN(); i++){
	uePositionAlloc->Add(Vector(50 ,0,1.5));
	//uePositionAlloc->Add(Vector(50 ,51,1.5));

	//	uePositionAlloc->Add(Vector(52 ,100,1.5));
	//	uePositionAlloc->Add(Vector(48 ,100,1.5));
	//	uePositionAlloc->Add(Vector(50 ,110,1.5));
	//	uePositionAlloc->Add(Vector(52 ,50,1.5));
	//	uePositionAlloc->Add(Vector(48 ,50,1.5));

	uemobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
	uemobility.SetPositionAllocator(uePositionAlloc);
	uemobility.Install (ueNodes);
	uemobility.AssignStreams(ueNodes,0);

	Simulator::Schedule(Seconds(0.5),&ChangeSpeed, ueNodes.Get(0),Vector(0,Velocity,0));
	Simulator::Schedule(Seconds(10.5),&ChangeSpeed, ueNodes.Get(0),Vector(0,-Velocity,0));
	Simulator::Schedule(Seconds(20.5),&ChangeSpeed, ueNodes.Get(0),Vector(0,Velocity,0));


	if(!ReadBuilding)
	{
		int Building_xlim_low = 5;
		int Building_xlim_high = 95;
		int Building_ylim_low = 5;
		int Building_ylim_high = 95;

		Ptr<Building> building1;
		//	ofstream file("building_topology.txt");
		srand((unsigned int) time (NULL));

		for (int i = 0; i<BuildingNum; i++){

			double xcoordinate = (double)((int)rand()%(Building_xlim_high-Building_xlim_low)+Building_xlim_low);
			double ycoordinate = (double)((int)rand()%(Building_ylim_high-Building_ylim_low)+Building_ylim_low);
			double xlength = rand()%6+1;
			double ylength = rand()%6+1;

			building1 = Create<Building>();
			building1->SetBoundaries(Box(xcoordinate, xcoordinate + xlength, ycoordinate, ycoordinate + ylength,0,35));

			//		file<<xcoordinate<<"\t"<<xcoordinate + xlength<<"\t"<<ycoordinate<<"\t"<<ycoordinate + ylength<<std::endl;
		}
	}

	else if(ReadBuilding)
	{
		string buildingTopology = "2_BuildingPosition.txt";

		Ptr<Building> building ;
		//      double d = 0.5;
		std::ifstream inFile;
		inFile.open(buildingTopology.c_str());
		if (inFile.fail())
			cout<< "NO file" <<endl;
		char inputString[100];
		while(!inFile.eof()){
			inFile.getline(inputString, 100);
			string a(inputString);
			string b = a.substr(a.find(':')+1);
			string c = b.substr(b.find(':')+1);
			string d = c.substr(c.find(':')+1);
			//std::cout <<   << std::endl;
			uint16_t x = atoi(a.c_str());
			uint16_t x_d = atoi(b.c_str());
			uint16_t y= atoi(c.c_str());
			uint16_t y_d = atoi(d.c_str());
			building = Create<Building>();
			if (x!=0)
				building ->SetBoundaries(Box(x,x_d, y, y_d, 0, 35));
		}

		inFile.close();
	}
	//	BuildingsHelper::Install (mmWaveEnbNodes_73G);
	BuildingsHelper::Install(mmWaveEnbNodes_28G);
	BuildingsHelper::Install(lteEnbNodes);
	BuildingsHelper::Install (ueNodes);
	BuildingsHelper::MakeMobilityModelConsistent();

	//Text file to print node animation
	std::ostringstream uefile;
	uefile <<ExperimentNum<< "_UePosition.txt";
	AsciiTraceHelper asciiTraceHelper_ue;
	Ptr<OutputStreamWrapper> ue_stream = asciiTraceHelper_ue.CreateFileStream(uefile.str().c_str());

	std::ostringstream enbfile;
	enbfile <<ExperimentNum<< "_EnbPosition.txt";
	AsciiTraceHelper asciiTraceHelper_enb;
	Ptr<OutputStreamWrapper> enb_stream = asciiTraceHelper_enb.CreateFileStream(enbfile.str().c_str());

	std::ostringstream buildingfile;
	buildingfile <<ExperimentNum<< "_BuildingPosition.txt";
	AsciiTraceHelper asciiTraceHelper_build;
	Ptr<OutputStreamWrapper> build_stream = asciiTraceHelper_build.CreateFileStream(buildingfile.str().c_str());


	for (uint16_t i = 0; i<uePositionAlloc->GetSize(); i++) {
		*ue_stream->GetStream() << uePositionAlloc->GetNext() << std::endl;
	}

	for (uint16_t i = 0; i<enbPositionAlloc->GetSize(); i++) {
		*enb_stream->GetStream() << enbPositionAlloc->GetNext() << std::endl;
	}


	for (BuildingList::Iterator it = BuildingList::Begin(); it != BuildingList::End(); ++it) {
		Box box = (*it)->GetBoundaries();
		*build_stream->GetStream() << box.xMin << ":" << box.xMax << ":" << box.yMin << ":" << box.yMax << std::endl;
	}

	// Install mmWave, lte, mc Devices to the nodes
	NetDeviceContainer lteEnbDevs = mmwaveHelper->InstallLteEnbDevice (lteEnbNodes);
	NetDeviceContainer mmWaveEnbDevs_28GHZ = mmwaveHelper->InstallEnbDevice(mmWaveEnbNodes_28G);
	//	NetDeviceContainer mmWaveEnbDevs_73GHZ = mmwaveHelper->InstallEnbDevice_73GHZ(mmWaveEnbNodes_73G);

	NetDeviceContainer mcUeDevs;

	mcUeDevs = mmwaveHelper->InstallMcUeDevice (ueNodes);
	//device = mmwaveHelper->InstallUeDevice(ueNodes.Get(1));
	//mcUeDevs.Add(device);


	// Install the IP stack on the UEs
	internet.Install (ueNodes);
	Ipv4InterfaceContainer ueIpIface;
	ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (mcUeDevs));

	// Assign IP address to UEs, and install applications
	for (uint32_t u = 0; u < ueNodes.GetN ();u++)
	{
		Ptr<Node> ueNode = ueNodes.Get (u);
		// Set the default gateway for the UE
		Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
		ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
	}

	// Add X2 interfaces
	mmwaveHelper->AddX2Interface (lteEnbNodes, mmWaveEnbNodes_28G);
	mmwaveHelper->AttachToClosestEnb (mcUeDevs, mmWaveEnbDevs_28GHZ,lteEnbDevs);
	//NetDeviceContainer mcUeDevs1 = mcUeDevs.Get(1);
	//mmwaveHelper->AttachToClosestEnb(mcUeDevs1, mmWaveEnbDevs_28GHZ);
	uint16_t dlPort = 1234;
	uint16_t ulPort = 2000;
	ApplicationContainer clientApps;
	ApplicationContainer serverApps;

	if (tcp){

		for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
		{
			if (dl)
			{
				//NS_LOG_LOGIC ("installing TCP DL app for UE " << u);
				PacketSinkHelper dlPacketSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
				serverApps.Add (dlPacketSinkHelper.Install (ueNodes.Get(u)));
				Ptr<MyApp> app = CreateObject<MyApp> ();
				Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (remoteHostContainer.Get (u), TcpSocketFactory::GetTypeId ());
				Address sinkAddress (InetSocketAddress (ueIpIface.GetAddress (u), dlPort));

				app->Setup (ns3TcpSocket, sinkAddress, 1400, 0xffffffff, DataRate ("1500Mbps"),isRandom);

				remoteHostContainer.Get (u)->AddApplication (app);

				double ueStartTime = 0.5;
				double ueGapTime = 0.1;

				std::ostringstream fileName;
				fileName<<ExperimentNum<<"_tcp_data_ue_gsoul"<<u+1<<".txt";
				AsciiTraceHelper asciiTraceHelper;
				Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream (fileName.str ().c_str ());
				serverApps.Get(u)->TraceConnectWithoutContext("Rx",MakeBoundCallback (&Rx, stream,u));

				std::ostringstream fileName_2;
				fileName_2<<ExperimentNum<<"_tcp_throughput_ue_gsoul" << u+1 <<".txt";
				AsciiTraceHelper asciiTraceHelper_2;
				Ptr<OutputStreamWrapper> stream_2 = asciiTraceHelper_2.CreateFileStream(fileName_2.str().c_str());
				Simulator::Schedule (Seconds (ueStartTime+u*ueGapTime), &CalculateThroughput,stream_2,serverApps.Get(u)->GetObject<PacketSink>(),u);

				std::ostringstream fileName_3;
				fileName_3<<ExperimentNum<<"_tcp_ack_throughput_ue_gsoul" << u+1 <<".txt";
				AsciiTraceHelper asciiTraceHelper_3;
				Ptr<OutputStreamWrapper> stream_3 = asciiTraceHelper_3.CreateFileStream(fileName_3.str().c_str());
				Simulator::Schedule (Seconds (ueStartTime+u*ueGapTime),&Reset_ack,stream_3,u);

				Simulator::Schedule (Seconds (ueStartTime+0.001+u*ueGapTime), &Traces, u,ExperimentNum);

				app->SetStartTime (Seconds (ueStartTime+(u)*ueGapTime));
				app->SetStopTime (Seconds (simTime+0.1));
				dlPort ++;
			}
			if (ul)
			{
				//NS_LOG_LOGIC ("installing TCP UL app for UE " << u);
				OnOffHelper ulClientHelper ("ns3::TcpSocketFactory",
						InetSocketAddress (remoteHostAddr[u], ulPort));
				ulClientHelper.SetConstantRate(DataRate ("100Mb/s"), 1500);
				clientApps.Add (ulClientHelper.Install (ueNodes.Get(u)));
				PacketSinkHelper ulPacketSinkHelper ("ns3::TcpSocketFactory",
						InetSocketAddress (Ipv4Address::GetAny (), ulPort));
				serverApps.Add (ulPacketSinkHelper.Install (remoteHost));
			}
		}

	}
	else{
		for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
		{
			if(dl)
			{
				PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
				serverApps.Add (dlPacketSinkHelper.Install (ueNodes.Get(u)));
				Ptr<MyApp> app = CreateObject<MyApp> ();
				Ptr<Socket> ns3UdpSocket = Socket::CreateSocket (remoteHostContainer.Get (u), UdpSocketFactory::GetTypeId ());
				Address sinkAddress (InetSocketAddress (ueIpIface.GetAddress (u), dlPort));
				app->Setup (ns3UdpSocket, sinkAddress, 1400, nPacket, DataRate ("500Mbps"),isRandom);
				remoteHostContainer.Get (u)->AddApplication (app);

				std::ostringstream fileName;
				fileName<<"udp_data_ue_gsoul_no_dupl"<<u+1<<".txt";
				AsciiTraceHelper asciiTraceHelper;
				Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream (fileName.str ().c_str ());
				serverApps.Get(u)->TraceConnectWithoutContext("Rx",MakeBoundCallback (&Rx, stream,u));

				std::ostringstream fileName_2;
				fileName_2<<"udp_throughput_ue_gsoul" << u+1 <<".txt";
				AsciiTraceHelper asciiTraceHelper_2;
				Ptr<OutputStreamWrapper> stream_2 = asciiTraceHelper_2.CreateFileStream(fileName_2.str().c_str());
				Simulator::Schedule (Seconds (0.1), &CalculateThroughput,stream_2,serverApps.Get(u)->GetObject<PacketSink>(),u);

				app->SetStartTime (Seconds (0.3));
				app->SetStopTime (Seconds (simTime+0.1));
			}
			if(ul)
			{
				++ulPort;
				PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
				serverApps.Add (ulPacketSinkHelper.Install (remoteHost));
				UdpClientHelper ulClient (remoteHostAddr[u], ulPort);
				ulClient.SetAttribute ("Interval", TimeValue (MicroSeconds(interPacketInterval)));
				ulClient.SetAttribute ("MaxPackets", UintegerValue(1000000));
				clientApps.Add (ulClient.Install (ueNodes.Get(u)));
			}
		}
	}

	mmwaveHelper -> EnableTraces();
	// Start applications
	Config::Set ("/NodeList/*/DeviceList/*/TxQueue/MaxPackets", UintegerValue (1000*1000));
	serverApps.Start (Seconds (0.001));


	Simulator::Stop(Seconds(simTime));
	Simulator::Run();
	Simulator::Destroy();
	return 0;
}




