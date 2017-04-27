#include "common.h"

void doServer(int);
void doClient(int);
void server_routine();
void client_routine();

struct Options {
	char hostname[32];
	int port;
} g_option;

int main(int argc, char* argv[]) {
	Args args = parsecmdline(argc, argv);
	if (args.empty()){
		printf("Usage:\n  %s [-h hostname] [-p port]\n", argv[0]);
		return 1;
	}


	SETSTR(args, g_option.hostname, "h", "0");
	SETINT(args, g_option.port, "p", 8000);

	client_routine();

  return 0;
}


void client_routine() {
	int sockfd, ret;
	struct sockaddr_in servaddr;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) ERR_EXIT("socket");

	ret = resolve(g_option.hostname, g_option.port, &servaddr);
	if (ret < 0) ERR_EXIT("resolve");

	ret = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
	if (ret < 0) ERR_EXIT("connect");

	doClient(sockfd);

	close(sockfd);
}


void doClient(int sockfd) {
	int ret;
	struct sockaddr_in sockname;
	socklen_t len;

	len = sizeof(sockname);
	ret = getsockname(sockfd, (struct sockaddr*)&sockname, &len);
	if (ret < 0) {
		ERR_EXIT("getsockname");
	}

	printf("local address %s:%d\n", inet_ntoa(sockname.sin_addr), ntohs(sockname.sin_port));
}

