#include "common.h"

char hostname[32];
int port;
int number;
int isServer;

void server_routine();
void client_routine();

int main(int argc, char* argv[]) {
	Args args = parsecmdline(argc, argv);
	if (args.empty() 
			|| !CONTAINS(args, "n")
			|| !CONTAINS(args, "h")) {
    printf("Usage:\n  %s [-s] <-h hostname> [-p port] <-n conn-number>\n", argv[0]);
		return 1;
	}

	SETINT(args, number, "n", 10);
	SETINT(args, port, "p", 8000);
	SETSTR(args, hostname, "h", "0");
	SETBOOL(args, isServer, "s", 0);

	if (isServer) {
		server_routine();
	}
	else {
		client_routine();
	}
	
	puts("done ...");
	while(1) {
		sleep(10);
	}
	return 0;
}

void server_routine() {
	int ret, listenfd;
	struct sockaddr_in servaddr, cliaddr;
	socklen_t cliaddrlen;

	ret = resolve(hostname, port, &servaddr);
	if (ret < 0) ERR_EXIT("resolve");

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0) ERR_EXIT("socket");

	ret = bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
	if (ret < 0) ERR_EXIT("bind");

	ret = listen(listenfd, 5);
	if (ret < 0) ERR_EXIT("listen");

	while(1) {
		cliaddrlen = sizeof(cliaddr);
		ret = accept(listenfd, (struct sockaddr*)&cliaddr, &cliaddrlen);
		if (ret < 0) {
			perror("accept");
			break;
		}
		printf("%s:%d come in\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
	}
	close(listenfd);
}

void client_routine() {
	int i, ret, sockfd;
	struct sockaddr_in servaddr, localaddr;
	socklen_t localaddrlen;
	
	ret = resolve(hostname, port, &servaddr);
	if (ret < 0) ERR_EXIT("resolve");


	for (i = 0; i < number; ++i) {
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0) ERR_EXIT("socket");

		ret = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
		localaddrlen = sizeof(localaddr);
		getsockname(sockfd, (struct sockaddr*)&localaddr, &localaddrlen);

		if (ret < 0) {
			printf("sockfd = %d, port = %d, client %d connecting failed!\n", sockfd, ntohs(localaddr.sin_port), i);
			perror("connect");
			break;
		}

		if (ret < 0) ERR_EXIT("getsockname");

		else {
			printf("sockfd = %d, port = %d, client %d connecting success!\n", sockfd, ntohs(localaddr.sin_port), i);
		}
	}
}
