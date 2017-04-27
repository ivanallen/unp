#include "common.h"

void doServer(int);
void doClient(int);
void server_routine();
void client_routine();
int recvfrom_timeo(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen, int nsec);

struct Options {
	int isServer;
	char hostname[32];
	int port;
	int timeout;
	int showopts;
} g_option;

int main(int argc, char* argv[]) {
	Args args = parsecmdline(argc, argv);
	if (CONTAINS(args, "help")) {
		printf("Usage:\n  %s [--help] [-s] [-h hostname] [-p port] [--timeo nsec] [--showopts]\n", argv[0]);
		return 1;
	}


	SETBOOL(args, g_option.isServer, "s", 0);
	SETSTR(args, g_option.hostname, "h", "0");
	SETINT(args, g_option.port, "p", 8000);
	SETINT(args, g_option.timeout, "timeo", 0);
	SETBOOL(args, g_option.showopts, "showopts", 0);

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

	ret = resolve(g_option.hostname, g_option.port, &servaddr);
	if (ret < 0) ERR_EXIT("resolve");

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) ERR_EXIT("socket");

	ret = bind(sockfd, (struct sockaddr*)&servaddr, sizeof servaddr);
	if (ret < 0) ERR_EXIT("bind");

	doServer(sockfd);

	close(sockfd);
}

void client_routine() {
	int sockfd;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) ERR_EXIT("socket");
	setRecvTimeout(sockfd, g_option.timeout);
	if (g_option.showopts) showopts(sockfd, "SO_RCVTIMEO");

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
		toUpper(buf, nr);
	  nw = sendto(sockfd, buf, nr, 0, (struct sockaddr*)&cliaddr, len);	
		if (nw < 0) {
			if (errno == EINTR) continue;
			ERR_EXIT("sendto");
		}
	}
}

void doClient(int sockfd) {
	int ret, nr, nw;
	struct sockaddr_in servaddr, replyaddr;
	socklen_t len;
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
		len = sizeof(replyaddr);
		nr = recvfrom(sockfd, buf, 4096, 0, (struct sockaddr*)&replyaddr, &len); 
		if (nr < 0) {
			if (errno == EINTR) continue;
			if (errno == EWOULDBLOCK) {
				puts("timed out");
				continue;
			}
			ERR_EXIT("recvfrom");
		}
		printf("%s:%d reply: ", inet_ntoa(replyaddr.sin_addr), ntohs(replyaddr.sin_port));
		fflush(stdout);
		nw = iwrite(STDOUT_FILENO, buf, nr);
		if (nw < 0) {
			ERR_EXIT("write");
		}
	}
}

