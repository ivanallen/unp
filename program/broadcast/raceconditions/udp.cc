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

void handler(int sig);

int main(int argc, char* argv[]) {
	Args args = parsecmdline(argc, argv);
	if (args.empty()){
		printf("Usage:\n  %s [-s] [-h hostname] [-p port]\n", argv[0]);
		return 1;
	}


	SETBOOL(args, g_option.isServer, "s", 0);
	SETSTR(args, g_option.hostname, "h", "0");
	SETINT(args, g_option.port, "p", 8000);

	registSignal(SIGALRM, handler);

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
	int onoff = 1;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) ERR_EXIT("socket");

	// setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &onoff, sizeof(onoff));
	// 相当于上面那一句，这里作了封装。
	setBroadcast(sockfd, onoff);

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
	int ret, len, nr, nw, maxfd;
	struct sockaddr_in servaddr, cliaddr;
	char buf[4096];
	socklen_t addrlen;
	sigset_t stalarm, stempty;
	fd_set rfds;

	sigemptyset(&stalarm);
	sigemptyset(&stempty);
	sigaddset(&stalarm, SIGALRM);

	FD_ZERO(&rfds);
	maxfd = sockfd;


	ret = resolve(g_option.hostname, g_option.port, &servaddr);
	if (ret < 0) ERR_EXIT("resolve");

	sigprocmask(SIG_BLOCK, &stalarm, NULL);

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
		alarm(2);

		for(;;) {
			addrlen = sizeof(cliaddr);
			FD_SET(sockfd, &rfds);
			sleep(3);
			ret = pselect(maxfd + 1, &rfds, NULL, NULL, NULL, &stempty);
			if (ret < 0) {
				if (errno == EINTR) break;
				ERR_EXIT("pselect");
			}
			nr = recvfrom(sockfd, buf, 4096, 0, (struct sockaddr*)&cliaddr, &addrlen); 

			LOG("from %s:%d ", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
			nw = iwrite(STDOUT_FILENO, buf, nr);
			if (nr < 0) {
				ERR_EXIT("iwrite");
			}
		}
	}
}

void handler(int sig) {
}
