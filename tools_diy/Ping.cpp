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

// dyc: useless now
int  pipesize = -1;
__u16 acked;
volatile int in_pr_addr = 0;	/* pr_addr() is executing */
volatile int exiting;
long nerrors;			/* icmp errors */
int working_recverr;

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

char *pr_addr(__u32 addr);
int options;

void print_timestamp(void)
{
	if (options & F_PTIMEOFDAY) {
		struct timeval tv;
		gettimeofday(&tv, NULL);
		printf("[%lu.%06lu] ",
		       (unsigned long)tv.tv_sec, (unsigned long)tv.tv_usec);
	}
}

void pr_options(unsigned char * cp, int hlen)
{
	int i, j;
	int optlen, totlen;
	unsigned char * optptr;
	static int old_rrlen;
	static char old_rr[MAX_IPOPTLEN];

	totlen = hlen-sizeof(struct iphdr);
	optptr = cp;

	while (totlen > 0) {
		if (*optptr == IPOPT_EOL)
			break;
		if (*optptr == IPOPT_NOP) {
			totlen--;
			optptr++;
			printf("\nNOP");
			continue;
		}
		cp = optptr;
		optlen = optptr[1];
		if (optlen < 2 || optlen > totlen)
			break;

		switch (*cp) {
		case IPOPT_SSRR:
		case IPOPT_LSRR:
			printf("\n%cSRR: ", *cp==IPOPT_SSRR ? 'S' : 'L');
			j = *++cp;
			i = *++cp;
			i -= 4;
			cp++;
			if (j > IPOPT_MINOFF) {
				for (;;) {
					__u32 address;
					memcpy(&address, cp, 4);
					cp += 4;
					if (address == 0)
						printf("\t0.0.0.0");
					else
						printf("\t%s", pr_addr(address));
					j -= 4;
					putchar('\n');
					if (j <= IPOPT_MINOFF)
						break;
				}
			}
			break;
		case IPOPT_RR:
			j = *++cp;		/* get length */
			i = *++cp;		/* and pointer */
			if (i > j)
				i = j;
			i -= IPOPT_MINOFF;
			if (i <= 0)
				break;
			if (i == old_rrlen
			    && !memcmp(cp, old_rr, i)
			    && !(options & F_FLOOD)) {
				printf("\t(same route)");
				i = ((i + 3) / 4) * 4;
				cp += i;
				break;
			}
			old_rrlen = i;
			memcpy(old_rr, (char *)cp, i);
			printf("\nRR: ");
			cp++;
			for (;;) {
				__u32 address;
				memcpy(&address, cp, 4);
				cp += 4;
				if (address == 0)
					printf("\t0.0.0.0");
				else
					printf("\t%s", pr_addr(address));
				i -= 4;
				putchar('\n');
				if (i <= 0)
					break;
			}
			break;
		case IPOPT_TS:
		{
			int stdtime = 0, nonstdtime = 0;
			__u8 flags;
			j = *++cp;		/* get length */
			i = *++cp;		/* and pointer */
			if (i > j)
				i = j;
			i -= 5;
			if (i <= 0)
				break;
			flags = *++cp;
			printf("\nTS: ");
			cp++;
			for (;;) {
				long l;

				if ((flags&0xF) != IPOPT_TS_TSONLY) {
					__u32 address;
					memcpy(&address, cp, 4);
					cp += 4;
					if (address == 0)
						printf("\t0.0.0.0");
					else
						printf("\t%s", pr_addr(address));
					i -= 4;
					if (i <= 0)
						break;
				}
				l = *cp++;
				l = (l<<8) + *cp++;
				l = (l<<8) + *cp++;
				l = (l<<8) + *cp++;

				if  (l & 0x80000000) {
					if (nonstdtime==0)
						printf("\t%ld absolute not-standard", l&0x7fffffff);
					else
						printf("\t%ld not-standard", (l&0x7fffffff) - nonstdtime);
					nonstdtime = l&0x7fffffff;
				} else {
					if (stdtime==0)
						printf("\t%ld absolute", l);
					else
						printf("\t%ld", l - stdtime);
					stdtime = l;
				}
				i -= 4;
				putchar('\n');
				if (i <= 0)
					break;
			}
			if (flags>>4)
				printf("Unrecorded hops: %d\n", flags>>4);
			break;
		}
		default:
			printf("\nunknown option %x", *cp);
			break;
		}
		totlen -= optlen;
		optptr += optlen;
	}
}

void pr_iph(struct iphdr *ip)
{
	int hlen;
	u_char *cp;

	hlen = ip->ihl << 2;
	cp = (u_char *)ip + 20;		/* point to options */

	printf("Vr HL TOS  Len   ID Flg  off TTL Pro  cks      Src      Dst Data\n");
	printf(" %1x  %1x  %02x %04x %04x",
	       ip->version, ip->ihl, ip->tos, ip->tot_len, ip->id);
	printf("   %1x %04x", ((ip->frag_off) & 0xe000) >> 13,
	       (ip->frag_off) & 0x1fff);
	printf("  %02x  %02x %04x", ip->ttl, ip->protocol, ip->check);
	printf(" %s ", inet_ntoa(*(struct in_addr *)&ip->saddr));
	printf(" %s ", inet_ntoa(*(struct in_addr *)&ip->daddr));
	printf("\n");
	pr_options(cp, hlen);
}

void pr_icmph(__u8 type, __u8 code, __u32 info, struct icmphdr *icp)
{
	switch(type) {
	case ICMP_ECHOREPLY:
		printf("Echo Reply\n");
		/* XXX ID + Seq + Data */
		break;
	case ICMP_DEST_UNREACH:
		switch(code) {
		case ICMP_NET_UNREACH:
			printf("Destination Net Unreachable\n");
			break;
		case ICMP_HOST_UNREACH:
			printf("Destination Host Unreachable\n");
			break;
		case ICMP_PROT_UNREACH:
			printf("Destination Protocol Unreachable\n");
			break;
		case ICMP_PORT_UNREACH:
			printf("Destination Port Unreachable\n");
			break;
		case ICMP_FRAG_NEEDED:
			printf("Frag needed and DF set (mtu = %u)\n", info);
			break;
		case ICMP_SR_FAILED:
			printf("Source Route Failed\n");
			break;
		case ICMP_NET_UNKNOWN:
			printf("Destination Net Unknown\n");
			break;
		case ICMP_HOST_UNKNOWN:
			printf("Destination Host Unknown\n");
			break;
		case ICMP_HOST_ISOLATED:
			printf("Source Host Isolated\n");
			break;
		case ICMP_NET_ANO:
			printf("Destination Net Prohibited\n");
			break;
		case ICMP_HOST_ANO:
			printf("Destination Host Prohibited\n");
			break;
		case ICMP_NET_UNR_TOS:
			printf("Destination Net Unreachable for Type of Service\n");
			break;
		case ICMP_HOST_UNR_TOS:
			printf("Destination Host Unreachable for Type of Service\n");
			break;
		case ICMP_PKT_FILTERED:
			printf("Packet filtered\n");
			break;
		case ICMP_PREC_VIOLATION:
			printf("Precedence Violation\n");
			break;
		case ICMP_PREC_CUTOFF:
			printf("Precedence Cutoff\n");
			break;
		default:
			printf("Dest Unreachable, Bad Code: %d\n", code);
			break;
		}
		if (icp && (options & F_VERBOSE))
			pr_iph((struct iphdr*)(icp + 1));
		break;
	case ICMP_SOURCE_QUENCH:
		printf("Source Quench\n");
		if (icp && (options & F_VERBOSE))
			pr_iph((struct iphdr*)(icp + 1));
		break;
	case ICMP_REDIRECT:
		switch(code) {
		case ICMP_REDIR_NET:
			printf("Redirect Network");
			break;
		case ICMP_REDIR_HOST:
			printf("Redirect Host");
			break;
		case ICMP_REDIR_NETTOS:
			printf("Redirect Type of Service and Network");
			break;
		case ICMP_REDIR_HOSTTOS:
			printf("Redirect Type of Service and Host");
			break;
		default:
			printf("Redirect, Bad Code: %d", code);
			break;
		}
		if (icp)
			printf("(New nexthop: %s)\n", pr_addr(icp->un.gateway));
		if (icp && (options & F_VERBOSE))
			pr_iph((struct iphdr*)(icp + 1));
		break;
	case ICMP_ECHO:
		printf("Echo Request\n");
		/* XXX ID + Seq + Data */
		break;
	case ICMP_TIME_EXCEEDED:
		switch(code) {
		case ICMP_EXC_TTL:
			printf("Time to live exceeded\n");
			break;
		case ICMP_EXC_FRAGTIME:
			printf("Frag reassembly time exceeded\n");
			break;
		default:
			printf("Time exceeded, Bad Code: %d\n", code);
			break;
		}
		if (icp && (options & F_VERBOSE))
			pr_iph((struct iphdr*)(icp + 1));
		break;
	case ICMP_PARAMETERPROB:
		printf("Parameter problem: pointer = %u\n", icp ? (ntohl(icp->un.gateway)>>24) : info);
		if (icp && (options & F_VERBOSE))
			pr_iph((struct iphdr*)(icp + 1));
		break;
	case ICMP_TIMESTAMP:
		printf("Timestamp\n");
		/* XXX ID + Seq + 3 timestamps */
		break;
	case ICMP_TIMESTAMPREPLY:
		printf("Timestamp Reply\n");
		/* XXX ID + Seq + 3 timestamps */
		break;
	case ICMP_INFO_REQUEST:
		printf("Information Request\n");
		/* XXX ID + Seq */
		break;
	case ICMP_INFO_REPLY:
		printf("Information Reply\n");
		/* XXX ID + Seq */
		break;
#ifdef ICMP_MASKREQ
	case ICMP_MASKREQ:
		printf("Address Mask Request\n");
		break;
#endif
#ifdef ICMP_MASKREPLY
	case ICMP_MASKREPLY:
		printf("Address Mask Reply\n");
		break;
#endif
	default:
		printf("Bad ICMP type: %d\n", type);
	}
}

jmp_buf pr_addr_jmp;
char *
pr_addr(__u32 addr)
{
	struct hostent *hp;
	static char buf[4096];

	in_pr_addr = !setjmp(pr_addr_jmp);

	if (exiting || (options & F_NUMERIC) ||
	    !(hp = gethostbyaddr((char *)&addr, 4, AF_INET)))
		sprintf(buf, "%s", inet_ntoa(*(struct in_addr *)&addr));
	else {
		char *s;
		s = NULL;
		snprintf(buf, sizeof(buf), "%s (%s)", s ? s : hp->h_name,
			 inet_ntoa(*(struct in_addr *)&addr));
	}

	in_pr_addr = 0;

	return(buf);
}

int parse_reply(struct msghdr *msg, int cc, void *addr, struct timeval *tv) {
	struct sockaddr_in *from = (struct sockaddr_in *) addr;
	__u8 *buf = (__u8 *)msg->msg_iov->iov_base;
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
//	csfailed = in_cksum((u_short *)icp, cc, 0);
	csfailed = 0x90ab;

	if (icp->type == ICMP_ECHOREPLY) {
        printf("received a reply");
		if (icp->un.echo.id != getpid()) {
            printf("reply id error");
			return 1;			/* 'Twas not our ECHO */
        }
//		if (gather_statistics((__u8*)icp, sizeof(*icp), cc,
//				      ntohs(icp->un.echo.sequence),
//				      ip->ttl, 0, tv, pr_addr(from->sin_addr.s_addr),
//				      pr_echo_reply))
//			return 0;
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
				if (cc < (int)(8+sizeof(struct iphdr)+8) ||
				    cc < (int)(8+iph->ihl*4+8))
					return 1;
				if (icp1->type != ICMP_ECHO ||
				    iph->daddr != whereto.sin_addr.s_addr ||
				    icp1->un.echo.id != getpid())
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
		if (!(options & F_VERBOSE) || getuid())
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
//    struct timeval recv_time;
    struct timeval *recv_timep = NULL;

// Raw socket can receive messages
// destined to other running pings.
    int not_ours = 0; 

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

    NOTICE("recv %d bytes", packlen);
    int cc = isocket.recvmsg(&msg, 0);
//    polling = MSG_DONTWAIT;

    if (cc < 0) {
        perror("ping: recvmsg");
    } else {
        not_ours = parse_reply(&msg, cc, addrbuf, recv_timep);
    }

    if (not_ours)
        FATAL("parse reply failed");
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
    NOTICE("send msg %d bytes", i);

    return (cc == i ? 0 : i);
}

int main (int argc, char** argv) {
	char hnamebuf[NI_MAXHOST];
    char* hostname;

//    int prob_sock = createDGramSocket();
    int icmp_sock = createICMPSocket();
//    Socket socket(prob_sock);
    Socket isocket(icmp_sock);

    NOTICE("run diy ping with version: %s, argc: %d", SNAPSHOT, argc);

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
	} else {
        FATAL("too few argments, %d", argc);
        return -1;
    }

    InetAddress addr(hostname, 80);
//    socket.connect(addr);

    {
        NOTICE("set opt");
        struct icmp_filter filt;
		filt.data = ~((1<<ICMP_SOURCE_QUENCH)|
			      (1<<ICMP_DEST_UNREACH)|
			      (1<<ICMP_TIME_EXCEEDED)|
			      (1<<ICMP_PARAMETERPROB)|
			      (1<<ICMP_REDIRECT)|
			      (1<<ICMP_ECHOREPLY));
        if (isocket.setopt(SOL_RAW, ICMP_FILTER, (char*)&filt, sizeof(filt)) ) {
            FATAL("set opt failed");
            return -1;
        }
    }

	int hold1 = 1;
	CHECK_ERROR(-1, isocket.setopt(SOL_IP, IP_RECVERR, (char *)&hold1, sizeof(hold1)) == 0, "set opt failed");

// call in setbufs()
    int datalen = 56;
    int optlen= 0;
	int hold = datalen + 8;
	hold += ((hold+511)/512)*(optlen + 20 + 16 + 64 + 160);
	int sndbuf = alloc;
	CHECK_ERROR(-1, isocket.setopt(SOL_SOCKET, SO_SNDBUF, (char *)&sndbuf, sizeof(sndbuf)) == 0, "set opt failed");
	CHECK_ERROR(-1, isocket.setopt(SOL_SOCKET, SO_RCVBUF, (char *)&hold, sizeof(hold)) == 0, "set opt failed");

// call in setup()
	int on = 1;
    CHECK_ERROR(-1, isocket.setopt(SOL_SOCKET, SO_TIMESTAMP, &on, sizeof(on)) == 0, "set opt failed");

	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	CHECK_ERROR(-1, isocket.setopt(SOL_SOCKET, SO_SNDTIMEO, (char*)&tv, sizeof(tv)) == 0, "set opt failed");

	tv.tv_sec = 1;
	tv.tv_usec = 0;
	CHECK_ERROR(-1, isocket.setopt(SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv)) == 0, "set opt failed");

    for (;;) {
        NOTICE("send_probe");
        int ret = send_probe(isocket);
        if (ret != 0) {
            FATAL("send probe failed");
            return -1;
        }
        NOTICE("recv_reply");
        recv_reply(isocket);
        break;
    }

    return 0;
}
