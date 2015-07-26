/*
 * mmwave-apectrum-phy.h
 *
 *  Created on: Nov 4, 2014
 *      Author: sourjya
 */

#ifndef SRC_MMWAVE_MODEL_MMWAVE_SPECTRUM_PHY_H_
#define SRC_MMWAVE_MODEL_MMWAVE_SPECTRUM_PHY_H_


#include <ns3/object-factory.h>
#include <ns3/event-id.h>
#include <ns3/spectrum-value.h>
#include <ns3/mobility-model.h>
#include <ns3/packet.h>
#include <ns3/nstime.h>
#include <ns3/net-device.h>
#include <ns3/spectrum-phy.h>
#include <ns3/spectrum-channel.h>
#include <ns3/spectrum-interference.h>
#include <ns3/data-rate.h>
#include <ns3/generic-phy.h>
#include <ns3/packet-burst.h>
#include "mmwave-spectrum-signal-parameters.h"
#include "ns3/random-variable-stream.h"
#include "ns3/mmwave-beamforming.h"
#include "mmwave-interference.h"
#include "mmwave-control-messages.h"
<<<<<<< HEAD
#include "mmwave-harq-phy.h"
=======
#include <ns3/mmwave-harq-phy.h>

>>>>>>> 0089c2076d4cc0764f29aa538c0a9c63b8d9113a

namespace ns3{

struct tbInfo_t
{
  uint16_t size;
  uint8_t m_mcs;
  std::vector<int> rbBitmap;
  bool downlink;
  bool corrupt;
};

typedef std::map<uint16_t, tbInfo_t> expectedTbs_t;

typedef Callback< void, Ptr<Packet> > MmWavePhyRxDataEndOkCallback;
typedef Callback< void, std::list<Ptr<MmWaveControlMessage> > > MmWavePhyRxCtrlEndOkCallback;


class MmWaveSpectrumPhy : public SpectrumPhy
{
public:
	MmWaveSpectrumPhy();
	virtual ~MmWaveSpectrumPhy();

	enum State
	  {
	    IDLE = 0,
		TX,
		RX_DATA,
		RX_CTRL
	  };

	static TypeId GetTypeId(void);
	virtual void DoDispose();

	void SetDevice(Ptr<NetDevice> d);
	Ptr<NetDevice> GetDevice();
	void SetMobility (Ptr<MobilityModel> m);
	Ptr<MobilityModel> GetMobility ();
	void SetChannel (Ptr<SpectrumChannel> c);
	Ptr<const SpectrumModel> GetRxSpectrumModel () const;

	Ptr<AntennaModel> GetRxAntenna ();
	void SetAntenna (Ptr<AntennaModel> a);

	void SetState (State newState);

	void SetNoisePowerSpectralDensity (Ptr<const SpectrumValue> noisePsd);
	void SetTxPowerSpectralDensity (Ptr<SpectrumValue> TxPsd);
	void StartRx (Ptr<SpectrumSignalParameters> params);
	void StartRxData (Ptr<MmwaveSpectrumSignalParametersDataFrame> params);
	void StartRxCtrl (Ptr<SpectrumSignalParameters> params);
	Ptr<SpectrumChannel> GetSpectrumChannel();
	void SetCellId (uint16_t cellId);

	bool StartTxDataFrames (Ptr<PacketBurst> pb, std::list<Ptr<MmWaveControlMessage> > ctrlMsgList, Time duration, uint8_t slotInd);

	bool StartTxDlControlFrames (std::list<Ptr<MmWaveControlMessage> > ctrlMsgList, Time duration); // control frames from enb to ue
	bool StartTxUlControlFrames (void); // control frames from ue to enb

	void SetPhyRxDataEndOkCallback (MmWavePhyRxDataEndOkCallback c);
	void SetPhyRxCtrlEndOkCallback (MmWavePhyRxCtrlEndOkCallback c);

	void AddDataPowerChunkProcessor (Ptr<mmWaveChunkProcessor> p);
	void AddDataSinrChunkProcessor (Ptr<mmWaveChunkProcessor> p);

	void UpdateSinrPerceived (const SpectrumValue& sinr);

	void AddExpectedTb (uint16_t rnti, uint16_t size, uint8_t m_mcs, std::vector<int> map, bool downlink);

	void SetHarqPhyModule (Ptr<MmWaveHarqPhy> harq);


private:
	void ChangeState (State newState);
	void EndTx ();
	void EndRxData ();
	void EndRxCtrl ();

	Ptr<mmWaveInterference> m_interferenceData;
	Ptr<MobilityModel> m_mobility;
	Ptr<NetDevice> m_device;
	Ptr<SpectrumChannel> m_channel;
	Ptr<const SpectrumModel> m_rxSpectrumModel;
	Ptr<SpectrumValue> m_txPsd;
	//Ptr<PacketBurst> m_txPacketBurst;
	std::list<Ptr<PacketBurst> > m_rxPacketBurstList;
	std::list<Ptr<MmWaveControlMessage> > m_rxControlMessageList;

	Time m_firstRxStart;
	Time m_firstRxDuration;

	Ptr<AntennaModel> m_anetnna;

	uint16_t m_cellId;

	State m_state;

	MmWavePhyRxCtrlEndOkCallback    m_phyRxCtrlEndOkCallback;
	MmWavePhyRxDataEndOkCallback m_phyRxDataEndOkCallback;

	TracedCallback<EnbPhyPacketCountParameter> m_reportEnbPacketCount;
	TracedCallback<UePhyPacketCountParameter> m_reportUePacketCount;

	SpectrumValue m_sinrPerceived;

	expectedTbs_t m_expectedTbs;

	Ptr<UniformRandomVariable> m_random;

	Ptr<MmWaveHarqPhy> m_harqPhyModule;



};

}


#endif /* SRC_MMWAVE_MODEL_MMWAVE_SPECTRUM_PHY_H_ */