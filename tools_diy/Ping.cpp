#include "common.h"

int main () {
    int prob_sock = createDGramSocket();
    int icmp_sock = createICMPSocket();
    Socket socket(prob_sock);
    Socket isocket(icmp_sock);

    socket.connect("");

	if (argc > 1) {
		char* target = argv[1];

		memset((char *)&whereto, 0, sizeof(whereto));

        struct sockaddr_in whereto;	/* who to ping */

		whereto.sin_family = AF_INET;
        // dyc: return 1 means a valid ip string
		if (inet_aton(target, &whereto.sin_addr) == 1) {
			hostname = target;
			if (argc == 1)
				options |= F_NUMERIC;
		} else {
            // dyc: may be a hostname
			char *idn;

            // dyc: idn is a hostname string
			idn = target;
            // TODO
			hp = gethostbyname(idn);
			if (!hp) {
				fprintf(stderr, "ping: unknown host %s\n", target);
				exit(2);
			}
			memcpy(&whereto.sin_addr, hp->h_addr, 4);

			strncpy(hnamebuf, hp->h_name, sizeof(hnamebuf) - 1);
			hnamebuf[sizeof(hnamebuf) - 1] = 0;
			hostname = hnamebuf;
		}
		if (argc > 1)
			route[nroute++] = whereto.sin_addr.s_addr;
		argc--;
		argv++;
	}

}