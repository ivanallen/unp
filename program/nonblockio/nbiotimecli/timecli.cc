#include "common.h"

char hostname[64];
int port;
int timeout;

void client_routine();
void doClient(int sockfd);

int main(int argc, char* argv[]) {
	WARNING("Usage: %s [-h hostname] [-p port] [-t timeout]\n", argv[0]);
	Args args = parsecmdline(argc, argv);

	SETSTR(args, hostname, "h", "nisttime.carsoncity.k12.mi.us");
	SETINT(args, port, "p", 13);
	SETINT(args, timeout, "t", 5);

	LOG("hostname : %s\n", hostname);
	LOG("port     : %d\n", port);
	LOG("timeout  : %d\n", timeout);

	client_routine();
}

void client_routine() {
	int ret, sockfd;
	struct sockaddr_in servaddr;

	ret = resolve(hostname, port, &servaddr);
	if (ret < 0) ERR_EXIT("resolve");

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) ERR_EXIT("socket");

	ret = nbioConnect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr), timeout);
	if (ret < 0) ERR_EXIT("nbioConnect");

	doClient(sockfd);
	close(sockfd);
}

void doClient(int sockfd) {
	char buf[4096];
	int nr;

	while(1) {
		nr = iread(sockfd, buf, 4096);
		if (nr < 0) ERR_EXIT("iread");
		else if (nr == 0) {
			break;
		}
		else {
			writen(STDOUT_FILENO, buf, nr);
		}
	}
}

