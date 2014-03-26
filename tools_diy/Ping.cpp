#include <ctype.h>
#include <sched.h>
#include <math.h>

#include "common.h"
#include "netlib.h"
#include "ping_common.h"

#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <ifaddrs.h>

#include <linux/filter.h>

using namespace dyc;

struct ::sockaddr_in whereto;	/* who to ping */

#define ICMP_FILTER	1
struct icmp_filter {
	__u32	data;
};

static struct {
    struct ::cmsghdr cm;
    struct ::in_pktinfo ipi;
} cmsg = { {sizeof(struct cmsghdr) + sizeof(struct in_pktinfo), SOL_IP, IP_PKTINFO},
	   {0, }};

u_char outpack[0x10000];
long ntransmitted;

int parse_reply(struct msghdr *msg, int cc, void *addr, struct timeval *tv) {
	struct sockaddr_in *from = addr;
	__u8 *buf = msg->msg_iov->iov_base;
	struct icmphdr *icp;
	struct iphdr *ip;
	int hlen;
	int csfailed;

	/* Check the IP header */
	ip = (struct iphdr *)buf;
	hlen = ip->ihl*4;
	if (cc < hlen + 8 || ip->ihl < 5) {
		if (options & F_VERBOSE)
			fprintf(stderr, "ping: packet too short (%d bytes) from %s\n", cc,
				pr_addr(from->sin_addr.s_addr));
		return 1;
	}

	/* Now the ICMP part */
	cc -= hlen;
	icp = (struct icmphdr *)(buf + hlen);
	csfailed = in_cksum((u_short *)icp, cc, 0);

	if (icp->type == ICMP_ECHOREPLY) {
		if (icp->un.echo.id != ident)
			return 1;			/* 'Twas not our ECHO */
		if (gather_statistics((__u8*)icp, sizeof(*icp), cc,
				      ntohs(icp->un.echo.sequence),
				      ip->ttl, 0, tv, pr_addr(from->sin_addr.s_addr),
				      pr_echo_reply))
			return 0;
	} else {
		/* We fall here when a redirect or source quench arrived.
		 * Also this branch processes icmp errors, when IP_RECVERR
		 * is broken. */

		switch (icp->type) {
		case ICMP_ECHO:
			/* MUST NOT */
			return 1;
		case ICMP_SOURCE_QUENCH:
		case ICMP_REDIRECT:
		case ICMP_DEST_UNREACH:
		case ICMP_TIME_EXCEEDED:
		case ICMP_PARAMETERPROB:
			{
				struct iphdr * iph = (struct  iphdr *)(&icp[1]);
				struct icmphdr *icp1 = (struct icmphdr*)((unsigned char *)iph + iph->ihl*4);
				int error_pkt;
				if (cc < 8+sizeof(struct iphdr)+8 ||
				    cc < 8+iph->ihl*4+8)
					return 1;
				if (icp1->type != ICMP_ECHO ||
				    iph->daddr != whereto.sin_addr.s_addr ||
				    icp1->un.echo.id != ident)
					return 1;
				error_pkt = (icp->type != ICMP_REDIRECT &&
					     icp->type != ICMP_SOURCE_QUENCH);
				if (error_pkt) {
					acknowledge(ntohs(icp1->un.echo.sequence));
					if (working_recverr) {
						return 0;
					} else {
						static int once;
						/* Sigh, IP_RECVERR for raw socket
						 * was broken until 2.4.9. So, we ignore
						 * the first error and warn on the second.
						 */
						if (once++ == 1)
							fprintf(stderr, "\rWARNING: kernel is not very fresh, upgrade is recommended.\n");
						if (once == 1)
							return 0;
					}
				}
				nerrors+=error_pkt;
				if (options&F_QUIET)
					return !error_pkt;
				if (options & F_FLOOD) {
					if (error_pkt)
						write_stdout("\bE", 2);
					return !error_pkt;
				}
				print_timestamp();
				printf("From %s: icmp_seq=%u ",
				       pr_addr(from->sin_addr.s_addr),
				       ntohs(icp1->un.echo.sequence));
				if (csfailed)
					printf("(BAD CHECKSUM)");
				pr_icmph(icp->type, icp->code, ntohl(icp->un.gateway), icp);
				return !error_pkt;
			}
		default:
			/* MUST NOT */
			break;
		}
		if ((options & F_FLOOD) && !(options & (F_VERBOSE|F_QUIET))) {
			if (!csfailed)
				write_stdout("!E", 2);
			else
				write_stdout("!EC", 3);
			return 0;
		}
		if (!(options & F_VERBOSE) || uid)
			return 0;
		if (options & F_PTIMEOFDAY) {
			struct timeval recv_time;
			gettimeofday(&recv_time, NULL);
			printf("%lu.%06lu ", (unsigned long)recv_time.tv_sec, (unsigned long)recv_time.tv_usec);
		}
		printf("From %s: ", pr_addr(from->sin_addr.s_addr));
		if (csfailed) {
			printf("(BAD CHECKSUM)\n");
			return 0;
		}
		pr_icmph(icp->type, icp->code, ntohl(icp->un.gateway), icp);
		return 0;
	}

	if (!(options & F_FLOOD)) {
		pr_options(buf + sizeof(struct iphdr), hlen);

		if (options & F_AUDIBLE)
			putchar('\a');
		putchar('\n');
		fflush(stdout);
	} else {
		putchar('\a');
		fflush(stdout);
	}
	return 0;
}


int recv_reply(Socket& isocket) {
    struct timeval recv_time;
    struct timeval *recv_timep = NULL;
    int not_ours = 0; /* Raw socket can receive messages
                       * destined to other running pings. */

    struct msghdr msg;
    int datalen = 56;
#define MAXIPLEN 60
#define MAXICMPLEN 76
    int packlen = datalen + MAXIPLEN + MAXICMPLEN;

    struct iovec iov;
    char* packet = (char*)malloc(packlen);
    iov.iov_base = packet;
    iov.iov_len = packlen;

    memset(&msg, 0, sizeof(msg));
    char* addrbuf = (char*) malloc(128);
    msg.msg_name = addrbuf;
    msg.msg_namelen = 128;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    char* ans_data = (char*)malloc(4096);
    msg.msg_control = ans_data;
    msg.msg_controllen = 4096;

    int cc = isocket.recvmsg(&msg, 0);
//    polling = MSG_DONTWAIT;

    if (cc < 0) {
        perror("ping: recvmsg");
    } else {
        not_ours = parse_reply(&msg, cc, addrbuf, recv_timep);
    }
    return 0;
}

int send_probe(Socket& isocket) {
    struct icmphdr *icp;
    int cc;
    int i;

    icp = (struct icmphdr *)outpack;
    icp->type = ICMP_ECHO;
    icp->code = 0;
    icp->checksum = 0;
    icp->un.echo.sequence = htons(ntransmitted+1);
    icp->un.echo.id = getpid();			/* ID */
    int datalen = 56;
    cc = datalen + 8;			/* skips ICMP portion */

    /* compute ICMP checksum here */
    icp->checksum = 0x1234;

    // dyc: timing = 1 if (datalen >= sizeof(struct timeval))
//    if (timing && !(options&F_LATENCY)) {
//        struct timeval tmp_tv;
//        gettimeofday(&tmp_tv, NULL);
//        memcpy(icp+1, &tmp_tv, sizeof(tmp_tv));
//        icp->checksum = 0x5678;
//    }

    static struct iovec iov = {outpack, 0};
    static struct msghdr m = { &whereto, sizeof(whereto),
        &iov, 1, &cmsg, 0, 0 };

    int cmsg_len = 0;
    m.msg_controllen = cmsg_len;
    iov.iov_len = cc;

    int confirm = 0;
    i = isocket.sendmsg(&m, confirm);

    return (cc == i ? 0 : i);
}

int main (int argc, char** argv) {
	char hnamebuf[NI_MAXHOST];
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
        socket.setopt(SOL_RAW, ICMP_FILTER, (char*)&filt, sizeof(filt));
    }


//	setsockopt(icmp_sock, SOL_IP, IP_RECVERR, (char *)&hold, sizeof(hold));

// call in setbufs()
//	setsockopt(icmp_sock, SOL_SOCKET, SO_SNDBUF, (char *)&sndbuf, sizeof(sndbuf));
//	setsockopt(icmp_sock, SOL_SOCKET, SO_RCVBUF, (char *)&hold, sizeof(hold));

// call in setup()
//  setsockopt(icmp_sock, SOL_SOCKET, SO_TIMESTAMP, &on, sizeof(on));
//	setsockopt(icmp_sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&tv, sizeof(tv));
//	setsockopt(icmp_sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv));

    for (;;) {
        send_probe(isocket);
        recv_reply(isocket);
        break;
    }


}
