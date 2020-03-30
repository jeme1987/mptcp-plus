from __future__ import division
import sys
import os
try:
    from xml.etree import cElementTree as ElementTree
except ImportError:
    from xml.etree import ElementTree

def parse_time_ns(tm):
    if tm.endswith('ns'):
        return int(tm[:-4])
    raise ValueError(tm)



## FiveTuple
class FiveTuple(object):
    ## class variables
    ## @var sourceAddress 
    #  source address
    ## @var destinationAddress 
    #  destination address
    ## @var protocol 
    #  network protocol
    ## @var sourcePort 
    #  source port
    ## @var destinationPort 
    #  destination port
    ## @var __slots__ 
    #  class variable list
    __slots__ = ['sourceAddress', 'destinationAddress', 'protocol', 'sourcePort', 'destinationPort']
    def __init__(self, el):
        '''The initializer.
        @param self The object pointer.
        @param el The element.
        '''
        self.sourceAddress = el.get('sourceAddress')
        self.destinationAddress = el.get('destinationAddress')
        self.sourcePort = int(el.get('sourcePort'))
        self.destinationPort = int(el.get('destinationPort'))
        self.protocol = int(el.get('protocol'))
        
## Histogram
class Histogram(object):
    ## class variables
    ## @var bins
    #  histogram bins
    ## @var nbins
    #  number of bins
    ## @var number_of_flows
    #  number of flows
    ## @var __slots__
    #  class variable list
    __slots__ = 'bins', 'nbins', 'width', 'start_base'
    def __init__(self, el=None):
        ''' The initializer.
        @param self The object pointer.
        @param el The element.
        '''
        self.bins = []
        if el is not None:
            self.nbins = int(el.get('nBins'))
            self.bins = dict()
            for bin in el.findall('bin'):
                self.bins[int(bin.get('index'))] = int(bin.get('count'))
                # Assume there is no misformed data in the input file
                self.width = float(bin.get('width'))
                self.start_base = float(bin.get('start'))/int(bin.get('index'))

    '''
    for i in range(flow.packetSizeHistogram.GetNBins () ):
                print (" ",i,"(", flow.packetSizeHistogram.GetBinStart (i), "-", \
                    flow.packetSizeHistogram.GetBinEnd (i), "): ", flow.packetSizeHistogram.GetBinCount (i))
    '''
    def print(self):
        for idx in range(self.nbins):
            if idx in self.bins.keys():
                print("({:.4f}-{:.4f}): {}".format(
                    idx * self.start_base,
                    idx * self.start_base + self.width,
                    self.bins[idx]))


## Flow
class Flow(object):
    ## class variables
    ## @var flowId
    #  delay ID
    ## @var delayMean
    #  mean delay
    ## @var packetLossRatio
    #  packet loss ratio
    ## @var rxBitrate
    #  receive bit rate
    ## @var txBitrate
    #  transmit bit rate
    ## @var fiveTuple
    #  five tuple
    ## @var packetSizeMean
    #  packet size mean
    ## @var probe_stats_unsorted
    #  unsirted probe stats
    ## @var hopCount
    #  hop count
    ## @var flowInterruptionsHistogram
    #  flow histogram
    ## @var rx_duration
    #  receive duration
    ## @var __slots__
    #  class variable list
    __slots__ = ['flowId', 'delayMean', 'packetLossRatio', 'rxBitrate', 'txBitrate',
                 'fiveTuple', 'packetSizeMean', 'probe_stats_unsorted',
                 'hopCount', 'delayHistogram', 'jitterHistogram', 'packetSizeHistogram', 'rx_duration']
    def __init__(self, flow_el):
        ''' The initializer.
        @param self The object pointer.
        @param flow_el The element.
        '''
        self.flowId = int(flow_el.get('flowId'))
        rxPackets = int(flow_el.get('rxPackets'))
        txPackets = int(flow_el.get('txPackets'))
        tx_duration = float(int(flow_el.get('timeLastTxPacket')[:-4]) - int(flow_el.get('timeFirstTxPacket')[:-4]))*1e-9
        rx_duration = float(int(flow_el.get('timeLastRxPacket')[:-4]) - int(flow_el.get('timeFirstRxPacket')[:-4]))*1e-9
        self.rx_duration = rx_duration
        self.probe_stats_unsorted = []
        if rxPackets:
            self.hopCount = float(flow_el.get('timesForwarded')) / rxPackets + 1
        else:
            self.hopCount = -1000
        if rxPackets:
            self.delayMean = float(flow_el.get('delaySum')[:-4]) / rxPackets * 1e-9
            self.packetSizeMean = float(flow_el.get('rxBytes')) / rxPackets
        else:
            self.delayMean = None
            self.packetSizeMean = None
        if rx_duration > 0:
            self.rxBitrate = int(flow_el.get('rxBytes'))*8 / rx_duration
        else:
            self.rxBitrate = None
        if tx_duration > 0:
            self.txBitrate = int(flow_el.get('txBytes'))*8 / tx_duration
        else:
            self.txBitrate = None
        lost = float(flow_el.get('lostPackets'))
        #print "rxBytes: %s; txPackets: %s; rxPackets: %s; lostPackets: %s" % (flow_el.get('rxBytes'), txPackets, rxPackets, lost)
        if rxPackets == 0:
            self.packetLossRatio = None
        else:
            self.packetLossRatio = (lost / (rxPackets + lost))

        delayHistogram = flow_el.find("delayHistogram")
        if delayHistogram is None:
            self.delayHistogram = None
        else:
            self.delayHistogram = Histogram(delayHistogram)

        jitterHistogram = flow_el.find("jitterHistogram")
        if jitterHistogram is None:
            self.jitterHistogram = None
        else:
            self.jitterHistogram = Histogram(jitterHistogram)

        packetSizeHistogram = flow_el.find("packetSizeHistogram")
        if packetSizeHistogram is None:
            self.packetSizeHistogram = None
        else:
            self.packetSizeHistogram = Histogram(packetSizeHistogram)
            

## ProbeFlowStats
class ProbeFlowStats(object):
    ## class variables
    ## @var probeId
    #  probe ID
    ## @var packets
    #  network packets
    ## @var bytes
    #  bytes
    ## @var delayFromFirstProbe
    #  delay from first probe
    ## @var __slots__
    #  class variable list
    __slots__ = ['probeId', 'packets', 'bytes', 'delayFromFirstProbe']

## Simulation
class Simulation(object):
    ## class variables
    ## @var flows
    #  list of flows
    def __init__(self, simulation_el):
        ''' The initializer.
        @param self The object pointer.
        @param simulation_el The element.
        '''
        self.flows = []
        FlowClassifier_el, = simulation_el.findall("Ipv4FlowClassifier")
        flow_map = {}
        for flow_el in simulation_el.findall("FlowStats/Flow"):
            flow = Flow(flow_el)
            flow_map[flow.flowId] = flow
            self.flows.append(flow)
        for flow_cls in FlowClassifier_el.findall("Flow"):
            flowId = int(flow_cls.get('flowId'))
            flow_map[flowId].fiveTuple = FiveTuple(flow_cls)

        for probe_elem in simulation_el.findall("FlowProbes/FlowProbe"):
            probeId = int(probe_elem.get('index'))
            for stats in probe_elem.findall("FlowStats"):
                flowId = int(stats.get('flowId'))
                s = ProbeFlowStats()
                s.packets = int(stats.get('packets'))
                s.bytes = int(stats.get('bytes'))
                s.probeId = probeId
                if s.packets > 0:
                    s.delayFromFirstProbe =  parse_time_ns(stats.get('delayFromFirstProbeSum')) / float(s.packets)
                else:
                    s.delayFromFirstProbe = 0
                flow_map[flowId].probe_stats_unsorted.append(s)


def main(argv):
    file_obj = open(argv[1])
    print ("Reading XML file ",)
 
    sys.stdout.flush()        
    level = 0
    sim_list = []
    for event, elem in ElementTree.iterparse(file_obj, events=("start", "end")):
        if event == "start":
            level += 1
        if event == "end":
            level -= 1
            if level == 0 and elem.tag == 'FlowMonitor':
                sim = Simulation(elem)
                sim_list.append(sim)
                elem.clear() # won't need this any more
                sys.stdout.write(".")
                sys.stdout.flush()
    print (" done.")


    for sim in sim_list:
        for flow in sim.flows:
            t = flow.fiveTuple
            proto = {6: 'TCP', 17: 'UDP'} [t.protocol]
            print ("FlowID: %i (%s %s/%s --> %s/%i)" % \
                (flow.flowId, proto, t.sourceAddress, t.sourcePort, t.destinationAddress, t.destinationPort))
            if flow.txBitrate is None:
                print ("\tTX bitrate: None")
            else:
                print ("\tTX bitrate: %.2f kbit/s" % (flow.txBitrate*1e-3,))
            if flow.rxBitrate is None:
                print ("\tRX bitrate: None")
            else:
                print ("\tRX bitrate: %.2f kbit/s" % (flow.rxBitrate*1e-3,))
            if flow.delayMean is None:
                print ("\tMean Delay: None")
            else:
                print ("\tMean Delay: %.2f ms" % (flow.delayMean*1e3,))
            if flow.packetLossRatio is None:
                print ("\tPacket Loss Ratio: None")
            else:
                print ("\tPacket Loss Ratio: %.2f %%" % (flow.packetLossRatio*100))

            print ("\nDelay Histogram")
            flow.delayHistogram.print()
            
            print("\nJitter Histogram")
            flow.jitterHistogram.print()

            print ("\nPacketSize Histogram")
            flow.packetSizeHistogram.print()


if __name__ == '__main__':
    main(sys.argv)
