#include "common.h"

void doServer(int);
void doClient(int);
void server_routine();
void client_routine();

struct Options {
	int isServer;
	char path[PATH_MAX];
} g_option;

int main(int argc, char* argv[]) {
	Args args = parsecmdline(argc, argv);
	if (args.empty()){
		printf("Usage:\n  %s [-s] [--path pathname]\n", argv[0]);
		return 1;
	}


	SETBOOL(args, g_option.isServer, "s", 0);
	SETSTR(args, g_option.path, "path", "./dog");

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
	struct sockaddr_un servaddr, cliaddr;
	socklen_t len;

	resolve(g_option.path, &servaddr, &len);

	sockfd = socket(AF_LOCAL, SOCK_DGRAM, 0);
	if (sockfd < 0) ERR_EXIT("socket");

	unlink(g_option.path);
	ret = bind(sockfd, (struct sockaddr*)&servaddr, len);
	if (ret < 0) ERR_EXIT("bind");

	doServer(sockfd);

	close(sockfd);
}

void client_routine() {
	int sockfd, ret;
	struct sockaddr_un cliaddr;
	socklen_t len;

	resolve(tmpnam(NULL), &cliaddr, &len);

	sockfd = socket(AF_LOCAL, SOCK_DGRAM, 0);
	if (sockfd < 0) ERR_EXIT("socket");

	ret = bind(sockfd, (struct sockaddr*)&cliaddr, len);
	if (ret < 0) ERR_EXIT("client_routine: bind");

	doClient(sockfd);

	close(sockfd);
}

void doServer(int sockfd) {
	char buf[4096];
	int nr, nw;
	struct sockaddr_un cliaddr;
	socklen_t len;

	while(1) {
		len = sizeof(cliaddr);
		nr = recvfrom(sockfd, buf, 4096, 0, (struct sockaddr*)&cliaddr, &len);
		if (nr < 0) {
			if (errno == EINTR) continue;
			ERR_EXIT("recvfrom");
		}
		toUpper(buf, nr);
		printf("%s come in\n", cliaddr.sun_path);
		nw = sendto(sockfd, buf, nr, 0, (struct sockaddr*)&cliaddr, len);
		if (nw < 0) {
			if (errno == EINTR) continue;
			ERR_EXIT("sendto");
		}
	}
}

void doClient(int sockfd) {
	int ret, nr, nw;
	struct sockaddr_un servaddr, replyaddr;
	socklen_t len;
	char buf[4096];

	resolve(g_option.path, &servaddr, &len);

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
			ERR_EXIT("recvfrom");
		}
		printf("%s reply: ", replyaddr.sun_path);
		fflush(stdout);
		nw = iwrite(STDOUT_FILENO, buf, nr);
		if (nw < 0) {
			ERR_EXIT("write");
		}
	}
}

