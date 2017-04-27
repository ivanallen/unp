#include "common.h"

void doTcpServer(int);
void doUdpServer(int);
void doTcpClient(int);
void doUdpClient(int);
void server_routine();
void client_routine();

struct Options {
	int isServer;
	char hostname[32];
	int port;
	int isUdp;
} g_option;

void handler(int sig) {
	pid_t pid;
	int stat;

	if (sig == SIGCHLD) {
		puts("hello SIGCHLD");
		while(1) {
			pid = waitpid(-1, &stat, WNOHANG);
			if (pid <= 0) break;
			printf("child %d terminated\n", pid);
		}
	}
	if (sig == SIGPIPE) {
		puts("hello SIGPIPE");
		exit(1);
	}
}

int main(int argc, char* argv[]) {
	struct sigaction sa;
	Args args = parsecmdline(argc, argv);
	if (args.empty()){
		printf("Usage:\n  %s [-s] [-u] <-h hostname> [-p port]\n", argv[0]);
		return 1;
	}

	
	registSignal(SIGCHLD, handler);
	registSignal(SIGPIPE, handler);

	SETBOOL(args, g_option.isServer, "s", 0);
	SETBOOL(args, g_option.isUdp, "u", 0);
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
	int ret, listenfd, udpfd, sockfd, maxfd;
	pid_t pid;
	struct sockaddr_in servaddr, cliaddr;
	socklen_t cliaddrlen;
	fd_set rfds, fds;

	// tcp
	ret = resolve(g_option.hostname, g_option.port, &servaddr);
	if (ret < 0) ERR_EXIT("resolve tcp");
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0) ERR_EXIT("socket tcp");
	reuseAddr(listenfd, 1);

	ret = bind(listenfd, (struct sockaddr*)&servaddr, sizeof servaddr);
	if (ret < 0) ERR_EXIT("bind tcp");

	ret = listen(listenfd, 5);
	if (ret < 0) ERR_EXIT("listen");

	// udp
	ret = resolve(g_option.hostname, g_option.port, &servaddr);
	if (ret < 0) ERR_EXIT("resolve udp");

	udpfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (udpfd < 0) ERR_EXIT("socket udp");
	ret = bind(udpfd, (struct sockaddr*)&servaddr, sizeof servaddr);
	if (ret < 0) ERR_EXIT("bind udp");
 
  FD_ZERO(&fds);
	FD_SET(listenfd, &fds);
	FD_SET(udpfd, &fds);
	maxfd = udpfd;

	while(1) {
		rfds = fds;
		ret = select(maxfd + 1, &rfds, NULL, NULL, 0);
		if (ret < 0) {
			if (errno = EINTR) continue;
			ERR_EXIT("select server_routine");
		}
		if (FD_ISSET(udpfd, &rfds)) {
			doUdpServer(udpfd);
		}

		if (FD_ISSET(listenfd, &rfds)) {
			cliaddrlen = sizeof cliaddr;
			sockfd = accept(listenfd, (struct sockaddr*)&cliaddr, &cliaddrlen);
			if (sockfd < 0) {
				if (errno == ECONNABORTED || errno == EINTR) {
					perror("accept");
					continue;
				}
				ERR_EXIT("accept");
			}
			printf("%s:%d come in\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

			pid = fork();
			if (pid == 0) {
				// child
				close(listenfd);
				doTcpServer(sockfd);
				close(sockfd);
				exit(0);
			}
			else if (pid < 0) {
				perror("fork");	
				close(sockfd);
				break;
			}
			close(sockfd);
		}
	}
	close(listenfd);
}

void client_routine() {
	int ret, sockfd;
	struct sockaddr_in servaddr;

	if (g_option.isUdp) {
		sockfd = socket(AF_INET, SOCK_DGRAM, 0);
		if (sockfd < 0) ERR_EXIT("socket");
		doUdpClient(sockfd);
	}
	else {
		ret = resolve(g_option.hostname, g_option.port, &servaddr);
		if (ret < 0) ERR_EXIT("resolve");

		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0) ERR_EXIT("socket");

		ret = connect(sockfd, (struct sockaddr*)&servaddr, sizeof servaddr);
		if (ret < 0) ERR_EXIT("connect");

		doTcpClient(sockfd);
	}

	close(sockfd);
}

void doTcpServer(int sockfd) {
	int nr, nw;
	char buf[4096];

	while(1) {
		nr = readline(sockfd, buf, 4096);
		if (nr == 0) {
			fputs("peer closed", stderr);
			break;
		}
		else if (nr < 0) {
			if (errno = ECONNRESET) {
				perror("readline");
				break;
			}
			ERR_EXIT("readline");
		}

		toUpper(buf, nr);

		nw = writen(sockfd, buf, nr);
		if (nw < nr) {
			perror("short write");
		}
	}
}

void doUdpServer(int sockfd) {
	char buf[4096];
	int nr, nw;
	struct sockaddr_in cliaddr;
	socklen_t len;

	len = sizeof(cliaddr);
	nr = recvfrom(sockfd, buf, 4096, 0, (struct sockaddr*)&cliaddr, &len); 
	if (nr < 0) {
		if (errno == EINTR) return;
		ERR_EXIT("recvfrom");
	}
	toUpper(buf, nr);
	nw = sendto(sockfd, buf, nr, 0, (struct sockaddr*)&cliaddr, len);	
	if (nw < 0) {
		if (errno == EINTR) return;
		ERR_EXIT("sendto");
	}
}

void doTcpClient(int sockfd) {
  fd_set rfds, fds;
	int nr, nw, nready, maxfd, stdinclosed, ret;
	char buf[4096];

	stdinclosed = 0;

	FD_ZERO(&rfds);
	FD_SET(STDIN_FILENO, &rfds);
	FD_SET(sockfd, &rfds);
	maxfd = sockfd;

	while(1) {
		fds = rfds;
		nready = select(maxfd + 1, &fds, NULL, NULL, NULL);
		if (nready < 0) {
			if (nready == EINTR || nready == ECONNRESET) continue;
		}
		else if (nready == 0) continue;

		if (FD_ISSET(STDIN_FILENO, &fds)) {
			// fgets 带有缓冲区，与 select 一起使用太危险，换成 iread，就是对 read 包装了一下。
			nr = iread(STDIN_FILENO, buf, 4096);
			if (nr > 0) {
				nw = writen(sockfd, buf, nr);
				if (nw < nr) {
					perror("short write");
				}
			}
			else if (nr == 0){
				// 不直接 break
				shutdown(sockfd, SHUT_WR);
				FD_CLR(STDIN_FILENO, &rfds);
				stdinclosed = 1;
			}
			else {
				ERR_EXIT("iread");
			}
		}

		if (FD_ISSET(sockfd, &fds)) {
			// 这里的 readline 函数没有缓冲，没关系
			nr = readline(sockfd, buf, 4096);
			if (nr == 0) {
				if (stdinclosed) {
					fputs("peer closed\n", stderr);
				}
				else {
					// 服务器非正常关闭
					fputs("server exception!\n", stderr);
				}
				break;
			}
			else if (nr < 0) ERR_EXIT("readline");
			write(STDOUT_FILENO, buf, nr);
		}
	}
}

void doUdpClient(int sockfd) {
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
			ERR_EXIT("recvfrom");
		}
		nw = iwrite(STDOUT_FILENO, buf, nr);
		if (nw < 0) {
			ERR_EXIT("write");
		}
	}
}

