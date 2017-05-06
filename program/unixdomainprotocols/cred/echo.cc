#include "common.h"

void doServer(int);
void doClient(int);
void server_routine();
void client_routine();
void printCred(struct ucred *cred);

struct Options {
	int isServer;
	int abstract;
	char path[PATH_MAX];
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
	if (CONTAINS(args, "help")){
		printf("Usage:\n  %s [-s] [--path pathname] [--abstract]\n", argv[0]);
		return 1;
	}


	registSignal(SIGCHLD, handler);
	registSignal(SIGPIPE, handler);

	SETBOOL(args, g_option.isServer, "s", 0);
	SETBOOL(args, g_option.abstract, "abstract", 0);
	SETSTR(args, g_option.path, "path", "");

	if (g_option.isServer) {
		server_routine();
	}
	else {
		client_routine();
	}

  return 0;
}

void server_routine() {
	int ret, listenfd, sockfd;
	pid_t pid;
	struct sockaddr_un servaddr, cliaddr;
	socklen_t len;

	if (!g_option.abstract)
		unlink(g_option.path);

	resolve(g_option.path, &servaddr, &len, g_option.abstract);

	listenfd = socket(AF_LOCAL, SOCK_STREAM, 0);
	if (listenfd < 0) ERR_EXIT("socket");

	ret = bind(listenfd, (struct sockaddr*)&servaddr, len);
	if (ret < 0) ERR_EXIT("bind");

	ret = listen(listenfd, 5);
	if (ret < 0) ERR_EXIT("listen");

	while(1) {
		len = sizeof(cliaddr);
		sockfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len);
		if (sockfd < 0) {
			if (errno == ECONNABORTED || errno == EINTR) {
				perror("accept");
				continue;
			}
			ERR_EXIT("accept");
		}
		printf("%s come in\n", cliaddr.sun_path);
		// 开启接收凭证功能
		setPassCred(sockfd, 1);

		pid = fork();
		if (pid == 0) {
			// child
			close(listenfd);
			doServer(sockfd);
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
	close(listenfd);
}

void client_routine() {
	int ret, sockfd;
	struct sockaddr_un servaddr, cliaddr;
	socklen_t len;

	resolve(g_option.path, &servaddr, &len, g_option.abstract);
	//resolve(tmpnam(NULL), &cliaddr);

	sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);
	if (sockfd < 0) ERR_EXIT("socket");

	//ret = bind(sockfd, (struct sockaddr*)&cliaddr, sizeof cliaddr);
	//if (ret < 0) ERR_EXIT("bind");


	ret = connect(sockfd, (struct sockaddr*)&servaddr, len);
	if (ret < 0) ERR_EXIT("connect");

	doClient(sockfd);

	close(sockfd);
}

void doServer(int sockfd) {
	int nr, nw;
	char buf[4096];
	struct ucred cred;

	while(1) {
		//nr = readline(sockfd, buf, 4096);
		nr = recvCred(sockfd, buf, 4096, &cred);
		if (nr == 0) {
			fputs("peer closed\n", stderr);
			break;
		}
		else if (nr < 0) {
			if (errno = ECONNRESET) {
				perror("readline");
				break;
			}
			ERR_EXIT("readline");
		}

		printCred(&cred);

		toUpper(buf, nr);

		nw = writen(sockfd, buf, nr);
		if (nw < nr) {
			perror("short write");
		}
	}
}

void doClient(int sockfd) {
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
				nw = sendCred(sockfd, buf, nr);
				// nw = writen(sockfd, buf, nr);
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

void printCred(struct ucred *cred) {
	printf("pid = %d\n", cred->pid);
	printf("uid = %d\n", cred->uid);
	printf("gid = %d\n", cred->gid);
}
