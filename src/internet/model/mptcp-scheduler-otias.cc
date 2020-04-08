#include "ns3/mptcp-scheduler-otias.h"
#include "ns3/mptcp-subflow.h"
#include "ns3/mptcp-socket-base.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE("MpTcpSchedulerOTIAS");

namespace ns3
{

static inline
SequenceNumber64 SEQ32TO64(SequenceNumber32 seq)
{
  return SequenceNumber64( seq.GetValue());
}

TypeId
MpTcpSchedulerOTIAS::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MpTcpSchedulerOTIAS")
    .SetParent<MpTcpScheduler> ()
    .AddConstructor<MpTcpSchedulerOTIAS> ()
  ;
  return tid;
}

MpTcpSchedulerOTIAS::MpTcpSchedulerOTIAS() :
  MpTcpScheduler(),
  m_metaSock(0)
{
  NS_LOG_FUNCTION(this);
}

MpTcpSchedulerOTIAS::~MpTcpSchedulerOTIAS (void)
{
  NS_LOG_FUNCTION(this);
}

void
MpTcpSchedulerOTIAS::SetMeta(Ptr<MpTcpSocketBase> metaSock)
{
  NS_ASSERT(metaSock);
  NS_ASSERT_MSG(m_metaSock == 0, "SetMeta already called");
  m_metaSock = metaSock;
}

Ptr<MpTcpSubflow>
MpTcpSchedulerOTIAS::GetSubflowToUseForEmptyPacket()
{
  NS_ASSERT(m_metaSock->GetNActiveSubflows() > 0 );
  return  m_metaSock->GetSubflow(0);
}

/* We assume scheduler can't send data on subflows, so it can just generate mappings */
bool
MpTcpSchedulerOTIAS::GenerateMapping(int& activeSubflowArrayId, SequenceNumber64& dsn, uint16_t& length)
{
  NS_LOG_FUNCTION(this);
  NS_ASSERT(m_metaSock);
  int nbOfSubflows = m_metaSock->GetNActiveSubflows();
  int attempt = 0;
  uint32_t amountOfDataToSend = 0;

  //! Tx data not sent to subflows yet
  SequenceNumber32 metaNextTxSeq = m_metaSock->Getvalue_nextTxSeq(); 
  amountOfDataToSend = m_metaSock->m_txBuffer->SizeFromSequence( metaNextTxSeq );
  uint32_t metaWindow = m_metaSock->AvailableWindow();

  if(amountOfDataToSend <= 0)
   {
     NS_LOG_DEBUG("Nothing to send from meta");
     return false;
   }

  if(metaWindow <= 0)
   {
     NS_LOG_DEBUG("No meta window available (TODO should be in persist state ?)");
     return false;
   }


  //algo for OTIAS
  //naming of variables follows original paper
   double min_T = 0xFFFFFFFF;
   int selected_subflow = 0;

   for(int i = 0; i<(int)(m_metaSock->GetNActiveSubflows()); i++)
   {
      Ptr<MpTcpSubflow> subflow = m_metaSock->GetSubflow(i);
       //printf("[+] Subflow %d\n", i);
      
      double not_yet_send = amountOfDataToSend;
      double num_can_send = subflow->m_tcb->m_cWnd - subflow->UnAckDataCount();
      double num_RTT_wait = (not_yet_send - num_can_send) / subflow->m_tcb->m_cWnd;
      double T = (num_RTT_wait + 0.5)  * subflow->m_rtt->GetEstimate().GetMilliSeconds();

      if(T < min_T)
      {
        min_T = T;
        selected_subflow = i;
      }
   }


  printf("[+] Subflow %d\n", selected_subflow);

  while(attempt < nbOfSubflows)
     {
       attempt++;
       int id = selected_subflow;
       Ptr<MpTcpSubflow> subflow = m_metaSock->GetSubflow(id);
       uint32_t subflowWindow = subflow->AvailableWindow();
       uint32_t canSend = std::min( subflowWindow, metaWindow);

       //! Can't send more than SegSize
       //metaWindow en fait on s'en fout du SegSize ?
       if(canSend > 0)
        {
          activeSubflowArrayId = id;
          dsn = SEQ32TO64(metaNextTxSeq);
          canSend = std::min(canSend, amountOfDataToSend);
          // For now we limit ourselves to a per packet basis
          length = std::min(canSend, subflow->GetSegSize());
          return true;
        }
     }
  return false;
}

} // end of 'ns3'
