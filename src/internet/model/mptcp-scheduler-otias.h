#ifndef MPTCP_SCHEDULER_OTIAS_H
#define MPTCP_SCHEDULER_OTIAS_H

#include "ns3/mptcp-scheduler.h"
#include "ns3/object.h"
#include "ns3/ptr.h"
#include "ns3/mptcp-scheduler-otias.h"
#include <vector>
#include <list>

namespace ns3
{

class MpTcpSocketBase;
class MpTcpSubflow;

class MpTcpSchedulerOTIAS : public MpTcpScheduler
{

public:
  static TypeId
  GetTypeId (void);

  MpTcpSchedulerOTIAS();
  virtual ~MpTcpSchedulerOTIAS ();
  void SetMeta(Ptr<MpTcpSocketBase> metaSock);

  /**
   * \brief This function is responsible for generating a list of packets to send
   * and to specify on which subflow to send.
   *
   * These *mappings* will be passed on to the meta socket that will send them without altering the
   * mappings.
   * It is of utmost importance to generate a perfect mapping !!! Any deviation
   * from the foreseen mapping will trigger an error and crash the simulator
   *
   *  \warn This function MUST NOT fiddle with metasockInternal
   * subflowId: pair(start,size)
   */
  virtual bool GenerateMapping(int& activeSubflowArrayId, SequenceNumber64& dsn, uint16_t& length);

  /**
   * \brief chooseSubflowForRetransmit
   * \return Index of subflow to use
   */
  virtual Ptr<MpTcpSubflow> GetSubflowToUseForEmptyPacket();

protected:
  Ptr<MpTcpSocketBase> m_metaSock;  //!<
};

} // end of 'ns3'

#endif /* MPTCP_SCHEDULER_OTIAS_H */
