#include "common.h"

void doServer(int);
void doClient(int);
void server_routine();
void client_routine();

struct Options {
	int isServer;
	char hostname[32];
	int port;
} g_option;

int main(int argc, char* argv[]) {
	Args args = parsecmdline(argc, argv);
	if (args.empty()){
		printf("Usage:\n  %s [-s] [-h hostname] [-p port]\n", argv[0]);
		return 1;
	}


	SETBOOL(args, g_option.isServer, "s", 0);
	SETSTR(args, g_option.hostname, "h", "0");
	SETINT(args, g_option.port, "p", 8000);

	if (g_option.isServer) {
		server_routine();
	}
	else {
		client_routine();
	}

  return 0;
}

void server_routine() {
	int ret, sockfd, on;
	struct sockaddr_in servaddr, cliaddr;
	socklen_t cliaddrlen;

	ret = resolve(g_option.hostname, g_option.port, &servaddr);
	if (ret < 0) ERR_EXIT("resolve");

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) ERR_EXIT("socket");

	on = 1;
	ret = setsockopt(sockfd, IPPROTO_IP, IP_PKTINFO, &on, sizeof(on));
	if (ret < 0) ERR_EXIT("setsockopt");

	ret = bind(sockfd, (struct sockaddr*)&servaddr, sizeof servaddr);
	if (ret < 0) ERR_EXIT("bind");

	doServer(sockfd);

	close(sockfd);
}

void client_routine() {
	int sockfd;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) ERR_EXIT("socket");

	doClient(sockfd);

	close(sockfd);
}

void doServer(int sockfd) {
	char buf[4096];
	char ifname[IF_NAMESIZE];
	int nr, nw, flags;
	struct sockaddr_in cliaddr;
	socklen_t len;
	struct in_pktinfo pkt;

  while(1) {
		len = sizeof(cliaddr);
		flags = 0;
		nr = recvFromFlags(sockfd, buf, 20, &flags, (struct sockaddr*)&cliaddr, &len, &pkt);
		if (nr < 0) {
			if (errno == EINTR) continue;
			ERR_EXIT("recvFromFlags");
		}
		puts("...");
		printf("%d byte datagram from %s", nr, inet_ntoa(cliaddr.sin_addr));
		if (pkt.ipi_addr.s_addr != 0) {
			printf(", to %s", inet_ntoa(pkt.ipi_addr));
		}
		if (pkt.ipi_ifindex > 0) {
			printf(", recv i/f = %s", if_indextoname(pkt.ipi_ifindex, ifname));
		}

		if (flags & MSG_TRUNC) {
			printf(" (datagram truncated)");
		}
		if (flags & MSG_CTRUNC) {
			printf(" (control info truncated)");
		}
#ifdef MSG_BCAST
		if (flags & MSG_BCAST) {
			printf(" (broadcast)");
		}
#endif
#ifdef MSG_MCAST
		if (flags & MSG_MCAST) {
			printf(" (multicast)");
		}
#endif

		printf("\n");

	  nw = sendto(sockfd, buf, nr, 0, (struct sockaddr*)&cliaddr, len);	
		if (nw < 0) {
			if (errno == EINTR) continue;
			ERR_EXIT("sentdo");
		}
	}
}

void doClient(int sockfd) {
	int ret, len, nr, nw;
	struct sockaddr_in servaddr;
	char buf[4096];

	ret = resolve(g_option.hostname, g_option.port, &servaddr);
	if (ret < 0) ERR_EXIT("resolve");

  while(1) {
		nr = iread(STDIN_FILENO, buf, 4096);
		if (nr < 0) {
			ERR_EXIT("iread");
		}
		else if (nr == 0) break;
	  nw = sendto(sockfd, buf, nr, 0, (struct sockaddr*)&servaddr, sizeof(servaddr));	
		if (nw < 0) {
			if (errno == EINTR) continue;
			ERR_EXIT("sendto");
		}
		nr = recvfrom(sockfd, buf, 4096, 0, NULL, NULL); 
		if (nr < 0) {
			if (errno == EINTR) continue;
			ERR_EXIT("recvfrom");
		}
		nw = iwrite(STDOUT_FILENO, buf, nr);
		if (nr < 0) {
			ERR_EXIT("iwrite");
		}
	}
}

