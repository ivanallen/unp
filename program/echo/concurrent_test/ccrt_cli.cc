#include "common.h"

char hostname[32];
int port;
int number;

void client_routine();

int main(int argc, char* argv[]) {
	Args args = parsecmdline(argc, argv);
	if (args.empty() 
			|| !CONTAINS(args, "n")
			|| !CONTAINS(args, "h")) {
    printf("Usage:\n  %s <-h hostname> [-p port] <-n conn-number>\n", argv[0]);
		return 1;
	}

	SETINT(args, number, "n", 10);
	SETINT(args, port, "p", 8000);
	SETSTR(args, hostname, "h", "0");

	client_routine();
	puts("done ...");
	while(1) {
		sleep(10);
	}
	return 0;
}

void client_routine() {
	int i, ret, sockfd;

	struct sockaddr_in servaddr;
	ret = resolve(hostname, port, &servaddr);
	if (ret < 0) ERR_EXIT("resolve");


	for (i = 0; i < number; ++i) {
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0) ERR_EXIT("socket");

		ret = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
		if (ret < 0) {
			perror("connect");
			break;
		}
		else {
			printf("sockfd = %d, client %d connecting success!\n", sockfd, i);
		}
	}
}
