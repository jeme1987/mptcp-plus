#ifndef NETWORK_PARAMS_H
#define NETWORK_PARAMS_H

#include "ns3/core-module.h"

class Parameters {
public:
    double endTime;

    //Traffic
    uint16_t UseTCP;
    uint16_t UseMPTCP;
    uint64_t packetSize;
    uint64_t burstPktNum;
    double burstItvSec;

    // CSMA 
    uint64_t delayLteServer;
    uint64_t delayWifiServer;
    uint64_t errateLteServer;  // uniform distribution
    uint64_t errateWifiServer; // uniform distribution

    Parameters() {
        UseTCP = 0;
        UseMPTCP = 0;
        delayLteServer = 0;
        delayWifiServer = 0;
        errateLteServer = 0;
        errateWifiServer = 0;
        packetSize = 1024;   // Don't change, MPTCP work abnormally with larger packet size.  (packet segmentation) 
        burstPktNum = 1;    // Don't change, MPTCP work abnormally when buffer size is full. (packet segmentation)
        burstItvSec = 0.1;  // Set different interval to control the traffic
        endTime = 30.0;     // The MPTCP original FastRtt scheduler suffers SIGSEGV with long execution time ...
    }
};


Parameters parseArgsToParams(int argc, char** argv);
std::string parmsToJSONObj();

#endif