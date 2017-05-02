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

	doClient(sockfd);

	close(sockfd);
}

void doServer(int sockfd) {
	char buf[4096];
	int nr, nw;
	struct sockaddr_in cliaddr;
	struct msghdr msg;
	struct iovec iov;
	socklen_t len;

  while(1) {
		// fill msghdr
		iov.iov_base = buf;
		iov.iov_len = 4096;
		msg.msg_name = &cliaddr;
		msg.msg_namelen = sizeof(cliaddr);
		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;
		msg.msg_control = NULL;
		msg.msg_controllen = 20;
		msg.msg_flags = 0;
		nr = recvmsg(sockfd, &msg, 0);
		if (nr < 0) {
			if (errno == EINTR) continue;
			ERR_EXIT("recvfrom");
		}
		printf("recvfrom %s:%d\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
		toUpper(buf, nr);

		len = msg.msg_namelen;
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
	char prompt[64];
	char buf[4096];
	struct msghdr msg;
	struct iovec iov[2];
	strcpy(prompt, "send: ");

	ret = resolve(g_option.hostname, g_option.port, &servaddr);
	if (ret < 0) ERR_EXIT("resolve");

  while(1) {
		nr = iread(STDIN_FILENO, buf, 4096);
		if (nr < 0) {
			ERR_EXIT("iread");
		}
		else if (nr == 0) break;
		// fill msghdr struct
		iov[0].iov_base = prompt;
		iov[0].iov_len = strlen(prompt);
		iov[1].iov_base = buf;
		iov[1].iov_len = nr;
		msg.msg_name = &servaddr;
		msg.msg_namelen = sizeof(servaddr);
		msg.msg_iov = iov;
		msg.msg_iovlen = 2;
		msg.msg_control = NULL;
		msg.msg_controllen = 0;
		msg.msg_flags = 0;

		nw = sendmsg(sockfd, &msg, 0);

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

