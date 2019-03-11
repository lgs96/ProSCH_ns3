/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 * Copyright (c) 2016, University of Padova, Dep. of Information Engineering, SIGNET lab
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
 * Author: Jaume Nin <jnin@cttc.cat>
 *         Nicola Baldo <nbaldo@cttc.cat>
 *
 * Modified by Michele Polese <michele.polese@gmail.com>
 *     (support for RRC_CONNECTED->RRC_IDLE state transition + support for real S1AP link)
 */


#include "epc-enb-application.h"
#include "ns3/log.h"
#include "ns3/mac48-address.h"
#include "ns3/ipv4.h"
#include "ns3/inet-socket-address.h"
#include "ns3/uinteger.h"

#include "ns3/internet-module.h"
#include "epc-gtpu-header.h"
#include "eps-bearer-tag.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EpcEnbApplication");

EpcEnbApplication::EpsFlowId_t::EpsFlowId_t ()
{
}

EpcEnbApplication::EpsFlowId_t::EpsFlowId_t (const uint16_t a, const uint8_t b)
  : m_rnti (a),
    m_bid (b)
{
}

bool
operator == (const EpcEnbApplication::EpsFlowId_t &a, const EpcEnbApplication::EpsFlowId_t &b)
{
  return ( (a.m_rnti == b.m_rnti) && (a.m_bid == b.m_bid) );
}

bool
operator < (const EpcEnbApplication::EpsFlowId_t& a, const EpcEnbApplication::EpsFlowId_t& b)
{
  return ( (a.m_rnti < b.m_rnti) || ( (a.m_rnti == b.m_rnti) && (a.m_bid < b.m_bid) ) );
}


TypeId
EpcEnbApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EpcEnbApplication")
    .SetParent<Object> ()
    .SetGroupName("Lte");
  return tid;
}

void
EpcEnbApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_lteSocket = 0;
  m_s1uSocket = 0;
  delete m_s1SapProvider;
  delete m_s1apSapEnb;
}


EpcEnbApplication::EpcEnbApplication (Ptr<Socket> lteSocket, Ptr<Socket> s1uSocket, Ipv4Address enbS1uAddress, Ipv4Address sgwS1uAddress, uint16_t cellId)
  : m_lteSocket (lteSocket),
    m_s1uSocket (s1uSocket),    
    m_enbS1uAddress (enbS1uAddress),
    m_sgwS1uAddress (sgwS1uAddress),
    m_gtpuUdpPort (2152), // fixed by the standard
    m_s1SapUser (0),
    m_s1apSapEnbProvider (0),
    m_cellId (cellId)
{
  NS_LOG_FUNCTION (this << lteSocket << s1uSocket << sgwS1uAddress);
  m_s1uSocket->SetRecvCallback (MakeCallback (&EpcEnbApplication::RecvFromS1uSocket, this));
  m_lteSocket->SetRecvCallback (MakeCallback (&EpcEnbApplication::RecvFromLteSocket, this));
  m_s1SapProvider = new MemberEpcEnbS1SapProvider<EpcEnbApplication> (this);
  m_s1apSapEnb = new MemberEpcS1apSapEnb<EpcEnbApplication> (this);
}


EpcEnbApplication::~EpcEnbApplication (void)
{
  NS_LOG_FUNCTION (this);
}


void 
EpcEnbApplication::SetS1SapUser (EpcEnbS1SapUser * s)
{
  m_s1SapUser = s;
}


EpcEnbS1SapProvider* 
EpcEnbApplication::GetS1SapProvider ()
{
  return m_s1SapProvider;
}

void 
EpcEnbApplication::SetS1apSapMme (EpcS1apSapEnbProvider * s)
{
  m_s1apSapEnbProvider = s;
}

  
EpcS1apSapEnb* 
EpcEnbApplication::GetS1apSapEnb ()
{
  return m_s1apSapEnb;
}

void 
EpcEnbApplication::DoInitialUeMessage (uint64_t imsi, uint16_t rnti)
{
  NS_LOG_FUNCTION (this);
  // side effect: create entry if not exist
  m_imsiRntiMap[imsi] = rnti;
  m_s1apSapEnbProvider->SendInitialUeMessage (imsi, rnti, imsi, m_cellId); // TODO if more than one MME is used, extend this call
}

void 
EpcEnbApplication::DoPathSwitchRequest (EpcEnbS1SapProvider::PathSwitchRequestParameters params)
{
  NS_LOG_FUNCTION (this);
  uint16_t enbUeS1Id = params.rnti;  
  uint64_t mmeUeS1Id = params.mmeUeS1Id;
  uint64_t imsi = mmeUeS1Id;
  // side effect: create entry if not exist
  m_imsiRntiMap[imsi] = params.rnti;

  uint16_t gci = params.cellId;
  std::list<EpcS1apSapMme::ErabSwitchedInDownlinkItem> erabToBeSwitchedInDownlinkList;
  for (std::list<EpcEnbS1SapProvider::BearerToBeSwitched>::iterator bit = params.bearersToBeSwitched.begin ();
       bit != params.bearersToBeSwitched.end ();
       ++bit)
    {
      EpsFlowId_t flowId;
      flowId.m_rnti = params.rnti;
      flowId.m_bid = bit->epsBearerId;
      uint32_t teid = bit->teid;
      
      EpsFlowId_t rbid (params.rnti, bit->epsBearerId);
      // side effect: create entries if not exist
      m_rbidTeidMap[params.rnti][bit->epsBearerId] = teid;
      m_teidRbidMap[teid] = rbid;

      EpcS1apSapMme::ErabSwitchedInDownlinkItem erab;
      erab.erabId = bit->epsBearerId;
      erab.enbTransportLayerAddress = m_enbS1uAddress;
      erab.enbTeid = bit->teid;

      erabToBeSwitchedInDownlinkList.push_back (erab);
    }
  m_s1apSapEnbProvider->SendPathSwitchRequest (enbUeS1Id, mmeUeS1Id, gci, erabToBeSwitchedInDownlinkList);
}

void 
EpcEnbApplication::DoUeContextRelease (uint16_t rnti)
{
  NS_LOG_FUNCTION (this << rnti);
  std::map<uint16_t, std::map<uint8_t, uint32_t> >::iterator rntiIt = m_rbidTeidMap.find (rnti);
  if (rntiIt != m_rbidTeidMap.end ())
    {
      for (std::map<uint8_t, uint32_t>::iterator bidIt = rntiIt->second.begin ();
           bidIt != rntiIt->second.end ();
           ++bidIt)
        {
          uint32_t teid = bidIt->second;
          m_teidRbidMap.erase (teid);
        }
      m_rbidTeidMap.erase (rntiIt);
    }
}

void 
EpcEnbApplication::DoInitialContextSetupRequest (uint64_t mmeUeS1Id, uint16_t enbUeS1Id, std::list<EpcS1apSapEnb::ErabToBeSetupItem> erabToBeSetupList)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO("In EnpEnbApplication DoInitialContextSetupRequest size of the erabToBeSetupList is " << erabToBeSetupList.size());

  for (std::list<EpcS1apSapEnb::ErabToBeSetupItem>::iterator erabIt = erabToBeSetupList.begin ();
       erabIt != erabToBeSetupList.end ();
       ++erabIt)
    {
      // request the RRC to setup a radio bearer
      uint64_t imsi = mmeUeS1Id;
      std::map<uint64_t, uint16_t>::iterator imsiIt = m_imsiRntiMap.find (imsi);
      NS_ASSERT_MSG (imsiIt != m_imsiRntiMap.end (), "unknown IMSI");
      uint16_t rnti = imsiIt->second;
      
      struct EpcEnbS1SapUser::DataRadioBearerSetupRequestParameters params;
      params.rnti = rnti;
      params.bearer = erabIt->erabLevelQosParameters;
      params.bearerId = erabIt->erabId;
      params.gtpTeid = erabIt->sgwTeid;
      m_s1SapUser->DataRadioBearerSetupRequest (params);

      EpsFlowId_t rbid (rnti, erabIt->erabId);
      // side effect: create entries if not exist
      m_rbidTeidMap[rnti][erabIt->erabId] = params.gtpTeid;
      m_teidRbidMap[params.gtpTeid] = rbid;

    }
}

void 
EpcEnbApplication::DoPathSwitchRequestAcknowledge (uint64_t enbUeS1Id, uint64_t mmeUeS1Id, uint16_t gci, std::list<EpcS1apSapEnb::ErabSwitchedInUplinkItem> erabToBeSwitchedInUplinkList)
{
  NS_LOG_FUNCTION (this);

  uint64_t imsi = mmeUeS1Id;
  std::map<uint64_t, uint16_t>::iterator imsiIt = m_imsiRntiMap.find (imsi);
  NS_ASSERT_MSG (imsiIt != m_imsiRntiMap.end (), "unknown IMSI");
  uint16_t rnti = imsiIt->second;
  EpcEnbS1SapUser::PathSwitchRequestAcknowledgeParameters params;
  params.rnti = rnti;
  m_s1SapUser->PathSwitchRequestAcknowledge (params);
}

void 
EpcEnbApplication::RecvFromLteSocket (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this);  
  NS_ASSERT (socket == m_lteSocket);
  Ptr<Packet> packet = socket->Recv ();

  /*Process5 Packet Info
  std::cout<<"Received from lte"<<std::endl;
  packet->Print(std::cout);
  */
  /// \internal
  /// Workaround for \bugid{231}
  //SocketAddressTag satag;
  //packet->RemovePacketTag (satag);

  EpsBearerTag tag;
  bool found = packet->RemovePacketTag (tag);
  NS_ASSERT (found);
  uint16_t rnti = tag.GetRnti ();
  uint8_t bid = tag.GetBid ();
  NS_LOG_LOGIC ("received packet with RNTI=" << (uint32_t) rnti << ", BID=" << (uint32_t)  bid);
  std::map<uint16_t, std::map<uint8_t, uint32_t> >::iterator rntiIt = m_rbidTeidMap.find (rnti);
  if (rntiIt == m_rbidTeidMap.end ())
    {
      NS_LOG_WARN ("UE context not found, discarding packet when receiving from lteSocket");
    }
  else
    {
	  //Process5: to test early ack operation
	  Ipv4Header tempIpv4Header;
	  TcpHeader tempTcpHeader;

	  packet->RemoveHeader(tempIpv4Header);
	  uint32_t bytesRemoved = packet->RemoveHeader(tempTcpHeader);

	  m_toServerIpv4Header = tempIpv4Header;
	  m_toServerTcpHeader = tempTcpHeader;

	  packet->AddHeader(tempTcpHeader);
	  packet->AddHeader(tempIpv4Header);

      std::map<uint8_t, uint32_t>::iterator bidIt = rntiIt->second.find (bid);
      NS_ASSERT (bidIt != rntiIt->second.end ());
      uint32_t teid = bidIt->second;

      if(tempTcpHeader.GetFlags()==(TcpHeader::SYN|TcpHeader::ACK))
      {
    	NS_LOG_LOGIC("Packet from lte, flag is "<<TcpHeader::FlagsToString(tempTcpHeader.GetFlags()));
        SendToS1uSocket (packet, teid);
      }
      else if(bytesRemoved==0)
      {
    	NS_LOG_LOGIC("Not a tcp packet");
    	SendToS1uSocket (packet, teid);
      }
    }
}

//Modified for Process5, for early ack operation
void 
EpcEnbApplication::RecvFromS1uSocket (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);  
  NS_ASSERT (socket == m_s1uSocket);

  Ptr<Packet> packet = socket->Recv ();

  //Process5 set client path
  GtpuHeader tempGtpuHeader;
  Ipv4Header tempIpv4Header;
  TcpHeader tempTcpHeader;

  packet->RemoveHeader(tempGtpuHeader);
  packet->RemoveHeader(tempIpv4Header);
  uint32_t bytesRemoved = packet->RemoveHeader(tempTcpHeader);

  NS_LOG_LOGIC("Packet from s1u, flag is "<<TcpHeader::FlagsToString(tempTcpHeader.GetFlags())<<" Header is "<<tempTcpHeader<< ""
		  " Packet Size is "<<packet->GetSize());

  if(!(bytesRemoved == 0 || bytesRemoved > 60))
  {
	  //Original operation, now operate only when handshaking phase
	  if((tempTcpHeader.GetFlags()==(TcpHeader::SYN)||(TcpHeader::ACK))&& packet->GetSize()==0)
	  {
		packet->AddHeader(tempTcpHeader);
		packet->AddHeader(tempIpv4Header);
		packet->AddHeader(tempGtpuHeader);

		m_toClientTcpHeader = tempTcpHeader;
		m_toClientIpv4Header = tempIpv4Header;

		//Process5: original code
		GtpuHeader gtpu;
		packet->RemoveHeader (gtpu);
		uint32_t teid = gtpu.GetTeid ();

		std::map<uint32_t, EpsFlowId_t>::iterator it = m_teidRbidMap.find (teid);
		if (it != m_teidRbidMap.end ())
		{
		  SendToLteSocket (packet, it->second.m_rnti, it->second.m_bid);
		}
		else
		{
		  packet = 0;
		  NS_LOG_DEBUG("UE context not found, discarding packet when receiving from s1uSocket");
		}
	  }
	  else
	  {
		Ptr<Packet> tempP = packet->Copy();
		SendEarlyAck(tempP,tempTcpHeader);
	  }
  }
  else if(bytesRemoved ==0)
  {
	packet->AddHeader(tempTcpHeader);
	packet->AddHeader(tempIpv4Header);
	packet->AddHeader(tempGtpuHeader);

	m_toClientTcpHeader = tempTcpHeader;
	m_toClientIpv4Header = tempIpv4Header;

	//Process5: original code
	GtpuHeader gtpu;
	packet->RemoveHeader (gtpu);
	uint32_t teid = gtpu.GetTeid ();

	std::map<uint32_t, EpsFlowId_t>::iterator it = m_teidRbidMap.find (teid);
	if (it != m_teidRbidMap.end ())
	{
	  SendToLteSocket (packet, it->second.m_rnti, it->second.m_bid);
	}
	else
	{
	  packet = 0;
	  NS_LOG_DEBUG("UE context not found, discarding packet when receiving from s1uSocket");
	}
  }


  //Process5
  /*
  TcpHeader tempHeader;
  if(tempP->PeekHeader(tempHeader)!=0)
  {
    SendEarlyAck (tempP);
  }*/

  /// \internal
  /// Workaround for \bugid{231}
  //SocketAddressTag tag;
  //packet->RemovePacketTag (tag);
}

void 
EpcEnbApplication::SendToLteSocket (Ptr<Packet> packet, uint16_t rnti, uint8_t bid)
{
  NS_LOG_FUNCTION (this << packet << rnti << (uint16_t) bid << packet->GetSize ());  
  EpsBearerTag tag (rnti, bid);
  packet->AddPacketTag (tag);
  int sentBytes = m_lteSocket->Send (packet);
  NS_ASSERT (sentBytes > 0);
}


void 
EpcEnbApplication::SendToS1uSocket (Ptr<Packet> packet, uint32_t teid)
{
  NS_LOG_FUNCTION (this << packet << teid <<  packet->GetSize ());  
  GtpuHeader gtpu;
  gtpu.SetTeid (teid);
  // From 3GPP TS 29.281 v10.0.0 Section 5.1
  // Length of the payload + the non obligatory GTP-U header
  gtpu.SetLength (packet->GetSize () + gtpu.GetSerializedSize () - 8);  
  packet->AddHeader (gtpu);

  //Process5
  m_toServerGtpuHeader = gtpu;

  uint32_t flags = 0;
  m_s1uSocket->SendTo (packet, flags, InetSocketAddress (m_sgwS1uAddress, m_gtpuUdpPort));
}

void
EpcEnbApplication::DoReleaseIndication (uint64_t imsi, uint16_t rnti, uint8_t bearerId)
{
  NS_LOG_FUNCTION (this << bearerId );
  std::list<EpcS1apSapMme::ErabToBeReleasedIndication> erabToBeReleaseIndication;
  EpcS1apSapMme::ErabToBeReleasedIndication erab;
  erab.erabId = bearerId;
  erabToBeReleaseIndication.push_back (erab);
  //From 3GPP TS 23401-950 Section 5.4.4.2, enB sends EPS bearer Identity in Bearer Release Indication message to MME
  m_s1apSapEnbProvider->SendErabReleaseIndication (imsi, rnti, erabToBeReleaseIndication);
}

///////////////////////////developing from 190308~ Process5: Proxy tcp in Epc-Enb-App//////////////////////////
//190308
void
EpcEnbApplication::SendEarlyAck (Ptr<Packet> packet, TcpHeader originTcpHeader)//no h function
{
  NS_LOG_FUNCTION (this<< packet);

  //Set headers sniffed from user
  TcpHeader newTcpHeader = m_toServerTcpHeader;
  Ipv4Header newIpv4Header = m_toServerIpv4Header;
  GtpuHeader newGtpuHeader = m_toServerGtpuHeader;

  //Set Ealry Ack sequence number
  SequenceNumber32 dataSeqNum = originTcpHeader.GetSequenceNumber();
  uint32_t dataSize = packet->GetSize();
  SequenceNumber32 AckNum = dataSeqNum + dataSize;
  newTcpHeader.SetAckNumber(AckNum);
  newTcpHeader.SetFlags(TcpHeader::ACK);

  //Send empty packet that contains ack sequence
  Ptr<Packet> tempP = new Packet();
  tempP->AddHeader(newTcpHeader);
  tempP->AddHeader(newIpv4Header);
  tempP->AddHeader(newGtpuHeader);

  uint32_t flags = 0;
  m_s1uSocket->SendTo (tempP, flags, InetSocketAddress (m_sgwS1uAddress, m_gtpuUdpPort));
}
}
// namespace ns3



