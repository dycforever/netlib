#include <iostream>
#include <pcap.h>
#include <string>
#include <vector>

class PCap
{
public:
    bool Parse(const std::string& dataFile, std::vector<std::string>& tcpPkgse);
};

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <iostream>

bool PCap::Parse(const std::string& dataFile, std::vector<std::string>& tcpPkgs)
{
    char errbuf[PCAP_ERRBUF_SIZE]={0};
    pcap_t* handler = NULL;
    const u_char* data;
    struct pcap_pkthdr hdr;
    
    handler = pcap_open_offline(dataFile.c_str(), errbuf);
    if (handler == NULL) {
        return false;
    }
    while ((data = pcap_next(handler, &hdr)) != NULL)
    {            
        //std::cout << hdr.caplen << "  " << hdr.len << std::endl;
        struct ethhdr* ethPtr;
        ethPtr = (struct ethhdr*)data;
        int ethLength = 14+2;
        // depends on if the ether header is available
//        if (ntohs(ethPtr->h_proto) == 0x0800) {
            struct iphdr *ipPtr;
            ipPtr=(struct iphdr*)(data + ethLength);
            int ipHeaderLength = ipPtr->ihl * 4;
            if(ipPtr->protocol == 6) {
                struct tcphdr* tcpPtr=(struct tcphdr*)(data + ethLength + ipHeaderLength);
                if (ntohs(tcpPtr->dest) == 80 || ntohs(tcpPtr->source) == 80) {
                    int tcpHeaderLength = tcpPtr->doff * 4;
                    const u_char* httpPtr = data + ethLength + ipHeaderLength + tcpHeaderLength;
                    int httpLength = ntohs(ipPtr->tot_len) - ipHeaderLength - tcpHeaderLength;
                    if (httpLength > 0) {
                        tcpPkgs.push_back(std::string((char*)httpPtr, httpLength));
                    }
                }
            }
//        }
    }
    return true;
}

int main(int argc, char* argv[])
{
    if (argc != 2) {
        return 0;
    }

    const char* pcapFile = argv[1];
    PCap pcap;
    std::vector<std::string> tcpPkgs;
    pcap.Parse(pcapFile, tcpPkgs);
    
    std::cout << tcpPkgs.size() << std::endl;
    for (size_t i = 0; i < tcpPkgs.size(); i++) {
        std::cout << tcpPkgs[i] << std::endl;
    }
    return 0;
}
