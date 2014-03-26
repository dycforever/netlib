#include <ctype.h>
#include <sched.h>
#include <math.h>



#include "common.h"
#include "netlib.h"
#include "ping_common.h"

using namespace dyc;

int main (int argc, char** argv) {
	char hnamebuf[NI_MAXHOST];
    struct ::sockaddr_in whereto;	/* who to ping */
    char* hostname;

    int prob_sock = createDGramSocket();
    int icmp_sock = createICMPSocket();
    Socket socket(prob_sock);
    Socket isocket(icmp_sock);


    int options = 0;

	if (argc > 1) {
		char* target = argv[1];

		memset((char *)&whereto, 0, sizeof(whereto));

		whereto.sin_family = AF_INET;
        // dyc: return 1 means a valid ip string
		if (inet_aton(target, &whereto.sin_addr) == 1) {
            NOTICE("target is a valid ip string: %s", target);
			hostname = target;
			if (argc == 1)
				options |= F_NUMERIC;
		} else {
            // dyc: idn is a hostname string
			char *idn = target;
            NOTICE("target is a hostname string: %s", target);
	        struct hostent *hp = gethostbyname(idn);
			if (!hp) {
				fprintf(stderr, "ping: unknown host %s\n", target);
				exit(2);
			}
			memcpy(&whereto.sin_addr, hp->h_addr, 4);

			strncpy(hnamebuf, hp->h_name, sizeof(hnamebuf) - 1);
			hnamebuf[sizeof(hnamebuf) - 1] = 0;
			hostname = hnamebuf;
		}
	}

    InetAddress addr(hostname, 80);
    socket.connect(addr);

    {
		struct icmp_filter filt;
		filt.data = ~((1<<ICMP_SOURCE_QUENCH)|
			      (1<<ICMP_DEST_UNREACH)|
			      (1<<ICMP_TIME_EXCEEDED)|
			      (1<<ICMP_PARAMETERPROB)|
			      (1<<ICMP_REDIRECT)|
			      (1<<ICMP_ECHOREPLY));
        socket.setopt(level, SOL_RAW, ICMP_FILTER, (char*)&filt, sizeof(filt));
    }


	setsockopt(icmp_sock, SOL_IP, IP_RECVERR, (char *)&hold, sizeof(hold));

// call in setbufs()

	setsockopt(icmp_sock, SOL_SOCKET, SO_SNDBUF, (char *)&sndbuf, sizeof(sndbuf));

	setsockopt(icmp_sock, SOL_SOCKET, SO_RCVBUF, (char *)&hold, sizeof(hold));

// call in setup()

    setsockopt(icmp_sock, SOL_SOCKET, SO_TIMESTAMP, &on, sizeof(on));

	setsockopt(icmp_sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&tv, sizeof(tv));

	setsockopt(icmp_sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv));

    for (;;) {
        struct icmphdr *icp;
        int cc;
        int i;

        icp = (struct icmphdr *)outpack;
        icp->type = ICMP_ECHO;
        icp->code = 0;
        icp->checksum = 0;
        icp->un.echo.sequence = htons(ntransmitted+1);
        icp->un.echo.id = ident;			/* ID */
    }

}
