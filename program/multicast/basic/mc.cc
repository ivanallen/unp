#include "common.h"

void doServer(int);
void doClient(int);
void server_routine();
void client_routine();

struct Options {
	int isServer;
	char hostname[32];
	char multiAddr[32];
	int port;
} g_option;

int main(int argc, char* argv[]) {
	Args args = parsecmdline(argc, argv);
	if (args.empty()){
		printf("Usage:\n  %s [-s] [-h hostname] [-g group] [-p port]\n", argv[0]);
		return 1;
	}


	SETBOOL(args, g_option.isServer, "s", 0);
	SETSTR(args, g_option.hostname, "h", "0");
	SETSTR(args, g_option.multiAddr, "g", "230.2.2.2");
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
	int ret, sockfd;
	struct sockaddr_in servaddr, cliaddr;
	socklen_t cliaddrlen;
	struct ip_mreq mreq;

	ret = resolve(g_option.hostname, g_option.port, &servaddr);
	if (ret < 0) ERR_EXIT("resolve");

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) ERR_EXIT("socket");

	ret = bind(sockfd, (struct sockaddr*)&servaddr, sizeof servaddr);
	if (ret < 0) ERR_EXIT("bind");

	mreq.imr_multiaddr.s_addr = inet_addr(g_option.multiAddr);
	mreq.imr_interface.s_addr = servaddr.sin_addr.s_addr;
	ret = setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
	if (ret < 0) ERR_EXIT("setsockopt:IP_ADD_MEMBERSHIP");

	LOG("multicast group: %s\n", g_option.multiAddr);


	doServer(sockfd);

	ret = setsockopt(sockfd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
	if (ret < 0) ERR_EXIT("setsockopt:IP_DROP_MEMBERSHIP");

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
	int nr, nw;
	struct sockaddr_in cliaddr;
	socklen_t len;

  while(1) {
		len = sizeof(cliaddr);
		nr = recvfrom(sockfd, buf, 4096, 0, (struct sockaddr*)&cliaddr, &len); 
		if (nr < 0) {
			if (errno == EINTR) continue;
			ERR_EXIT("recvfrom");
		}
		puts("...");
	  nw = sendto(sockfd, buf, nr, 0, (struct sockaddr*)&cliaddr, len);	
		if (nw < 0) {
			if (errno == EINTR) continue;
			ERR_EXIT("sentdo");
		}
	}
}

void doClient(int sockfd) {
	int ret, len, nr, nw;
	struct sockaddr_in servaddr, cliaddr;
	socklen_t addrlen;
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
		addrlen = sizeof(cliaddr);
		nr = recvfrom(sockfd, buf, 4096, 0, (struct sockaddr*)&cliaddr, &addrlen); 
		if (nr < 0) {
			if (errno == EINTR) continue;
			ERR_EXIT("recvfrom");
		}
		LOG("from %s:%d ", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
		nw = iwrite(STDOUT_FILENO, buf, nr);
		if (nr < 0) {
			ERR_EXIT("iwrite");
		}
	}
}

