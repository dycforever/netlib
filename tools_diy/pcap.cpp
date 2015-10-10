#include <iostream>
#include <pcap.h>
#include <string>
#include <vector>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#define RTT 3000

const char* gIp = "10.99.20.67";
int gPort = 8800;

class PcapParser
{
public:
    bool Parse(const std::string& dataFile);
    virtual void statistic(struct pcap_pkthdr* packet, const u_char* data) {}
};

class RTTcounter : public PcapParser
{
public:
    RTTcounter() : mCount(1), mSkipCount(0), mLastTime(0), mBeginTime(0) {}
    virtual void statistic(struct pcap_pkthdr* packet, const u_char* data);
    size_t GetCount() const { return mCount; }
    size_t GetSkipCount() const { return mSkipCount; }
    size_t GetLastTime() const { return mLastTime; }
    size_t GetBeginTime() const { return mBeginTime; }

private:
    size_t mCount;
    size_t mSkipCount;
    int64_t mLastTime;
    int64_t mBeginTime;
};

void RTTcounter::statistic(struct pcap_pkthdr* packet, const u_char* data)
{
//    std::cout << hdr.caplen << "  " << hdr.len << std::endl;
    struct timeval time = packet->ts;
    int ethLength = 14;
    struct ethhdr* ethPtr = (struct ethhdr*)data;
    if (ntohs(ethPtr->h_proto) != 0x0800) {
        // not ip packet
        mSkipCount++;
        return;
    }

    struct iphdr* ipPtr=(struct iphdr*)(data + ethLength);

    int ipHeaderLength = ipPtr->ihl * 4;
    if(ipPtr->protocol != 6 || ipPtr->version != 4) {
        // not ipv4 or tcp packet
        mSkipCount++;
        return;
    }
    struct tcphdr* tcpPtr=(struct tcphdr*)(data + ethLength + ipHeaderLength);

    int64_t thisTime = (int64_t)time.tv_sec * 1000000 + time.tv_usec;
    int tcpHeaderLength = tcpPtr->doff * 4;
    int dataLength = ntohs(ipPtr->tot_len) - ipHeaderLength - tcpHeaderLength;
    if (mBeginTime == 0 && !tcpPtr->syn && dataLength != 0) {
        mBeginTime = thisTime;
    }
    const u_char* httpPtr = data + ethLength + ipHeaderLength + tcpHeaderLength;

    struct sockaddr_in targetAddr;
    inet_aton(gIp, &targetAddr.sin_addr);
    if(ipPtr->saddr != targetAddr.sin_addr.s_addr) {
        // not tcp packet
        mSkipCount++;
        return;
    }

    if (ntohs(tcpPtr->source) != gPort || tcpPtr->syn || tcpPtr->fin) {
        mSkipCount++;
        return;
    }


    if (mLastTime != 0 && thisTime - mLastTime > RTT) {
        mCount++;
    }
    mLastTime = thisTime;
}


bool PcapParser::Parse(const std::string& dataFile)
{
    char errbuf[PCAP_ERRBUF_SIZE]={0};
    pcap_t* handler = NULL;
    const u_char* data;
    struct pcap_pkthdr hdr;
    
    handler = pcap_open_offline(dataFile.c_str(), errbuf);
    if (handler == NULL) {
        return false;
    }
    while ((data = pcap_next(handler, &hdr)) != NULL) {            
        statistic(&hdr, data);
    }
    return true;
}

int main(int argc, char* argv[])
{
    if (argc != 4) {
        printf("usage: %s file_name ip(10.99.20.67) port(8800)\n", argv[0]);
        return 0;
    }

    gIp = argv[2];
    gPort = atoi(argv[3]);

    const char* pcapFile = argv[1];
    RTTcounter counter;
    counter.Parse(pcapFile);
    std::cout << counter.GetCount() << "\t" << counter.GetLastTime() - counter.GetBeginTime() << std::endl;
    
    return 0;
}

