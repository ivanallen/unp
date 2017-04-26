#include "common.h"

void doServer(int);
void doClient(int);
void server_routine();
void client_routine();
void setopt(int);
void handler(int sig);
void usage(const char* prog_name);
void help(const char* prog_name);

struct Options {
	int isServer;
	char hostname[32];
	int port;
	int reuse;
	int linger;
	int slowread;
	int useclose;
	int showopts;
	int sndbufsize;
	int rcvbufsize;
  int nodelay;
  int mss;	
	int cork;
	int writesize;
} g_option;

int main(int argc, char* argv[]) {
	struct sigaction sa;
	Args args = parsecmdline(argc, argv);
	if (args.empty()){
		usage(argv[0]);
		return 1;
	}
	if (CONTAINS(args, "help")) {
		help(argv[0]);
		return 1;
	}

	
	registSignal(SIGCHLD, handler);
	registSignal(SIGPIPE, handler);
	registSignal(SIGQUIT, handler);

	SETBOOL(args, g_option.isServer, "s", 0);
	SETSTR(args, g_option.hostname, "h", "0");
	SETINT(args, g_option.port, "p", 8000);
	SETBOOL(args, g_option.reuse, "reuse", 0);
	SETINT(args, g_option.slowread, "slowread", -1);
	SETINT(args, g_option.linger, "linger", -1);
	SETBOOL(args, g_option.useclose, "useclose", 0);
	SETBOOL(args, g_option.showopts, "showopts", 0);
	SETINT(args, g_option.sndbufsize, "sendbuf", -1);
	SETINT(args, g_option.rcvbufsize, "recvbuf", -1);
	SETBOOL(args, g_option.nodelay, "nodelay", 0);
	SETBOOL(args, g_option.cork, "cork", 0);
	SETINT(args, g_option.mss, "mss", -1);
	SETINT(args, g_option.writesize, "writesize", -1);

	if (g_option.slowread > 4096) g_option.slowread = 4096;
	if (g_option.writesize > 4096) g_option.writesize = 4096;

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
	struct sockaddr_in servaddr, cliaddr;
	socklen_t cliaddrlen;

	ret = resolve(g_option.hostname, g_option.port, &servaddr);
	if (ret < 0) ERR_EXIT("resolve");

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0) ERR_EXIT("socket");

	// 设置各种选项
	setopt(listenfd);

	ret = bind(listenfd, (struct sockaddr*)&servaddr, sizeof servaddr);
	if (ret < 0) ERR_EXIT("bind");

	ret = listen(listenfd, 5);
	if (ret < 0) ERR_EXIT("listen");

	while(1) {
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
	struct sockaddr_in servaddr;

	ret = resolve(g_option.hostname, g_option.port, &servaddr);
	if (ret < 0) ERR_EXIT("resolve");

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) ERR_EXIT("socket");
	setopt(sockfd);

	ret = connect(sockfd, (struct sockaddr*)&servaddr, sizeof servaddr);
	if (ret < 0) ERR_EXIT("connect");

	doClient(sockfd);

	close(sockfd);
}

void doServer(int sockfd) {
	int nr, nw, total;
	char buf[4096];


	total = 0;
	while(g_option.slowread != -1) {
		sleep(2);
		if (g_option.slowread > 0) {
			nr = iread(sockfd, buf, g_option.slowread);
			if (nr < 0) {
				printf("%d bytes received\n", total);
				ERR_EXIT("iread");
			}
			if (nr == 0) {
				printf("%d bytes received\n", total);
				fputs("peer closed\n", stderr);
				return;
			}
			total += nr;
			printf("read %d bytes...\n", nr);
		}
	}

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
			nr = iread(STDIN_FILENO, buf, g_option.writesize);
			if (nr > 0) {
				nw = writen(sockfd, buf, nr);
				if (nw < nr) {
					perror("short write");
				}
			}
			else if (nr == 0){
				// 不直接 break
				if (g_option.linger >= 0 || g_option.useclose) {
					ret = close(sockfd);
					printf("close return %d\n", ret);
					break;
				}
				else {
					shutdown(sockfd, SHUT_WR);
					FD_CLR(STDIN_FILENO, &rfds);
					stdinclosed = 1;
				}
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

void setopt(int sockfd) {
	if (g_option.reuse) {
		puts("set reuse");
		reuseAddr(sockfd, 1);
	}
	if (g_option.linger >= 0) {
		printf("set linger: {on %ds}\n", g_option.linger);
		setLinger(sockfd, 1, g_option.linger);
	}
	if (g_option.sndbufsize > 0) {
		printf("set sndbuf: %d\n", g_option.sndbufsize);
		setSendBufSize(sockfd, g_option.sndbufsize);
	}
	if (g_option.rcvbufsize > 0) {
		printf("set rcvbuf: %d\n", g_option.rcvbufsize);
		setRecvBufSize(sockfd, g_option.rcvbufsize);
	}
	if (g_option.mss > 0) {
		printf("set mss: %d\n", g_option.mss);
		setMaxSegSize(sockfd, g_option.mss);
	}
	if (g_option.nodelay) {
		printf("set nodelay\n");
		setNoDelay(sockfd, 1);
	}
	if (g_option.cork) {
		printf("set cork\n");
		setCork(sockfd, 1);
	}
	if (g_option.showopts) showopts(sockfd, NULL);
}

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
	if (sig == SIGQUIT) {
		puts("hello SIGUQIT, now go on...");
		g_option.slowread= -1;
	}
}

void usage(const char* prog_name) {
	const char *prompt = 
    "[--help] [-s] [-h hostname] [-p port]\n"
    "[--linger seconds] [--slowread size] [--reuse]\n"
    "[--useclose] [--writesize]\n"
		"[--sendbuf size] [--recvbuf size]\n"
		"[--nodelay] [--cork] [--mss size]\n"
	  "[--showopts]\n";
  fprintf(stderr, "usage:\n %s %s\n", prog_name, prompt); 
}

void help(const char* prog_name) {
	const char* s = 
		"\t-s                  以服务器方式启动\n"
		"\t-h hostname         指定主机名或者 ip 地址，默认为通用地址\n"
		"\t-p port             指定端口号，默认是 8000\n"
		"\t--reuse             打开 SO_REUSEADDR\n"
		"\t--linger seconds    打开 SO_LINGER\n"
		"\t--slowread size     服务器使用，慢速读取数据，size 指定一次读取的字节数\n"
	  "\t--useclose          开启开选项，关闭服务器时使用 close 而不是 shutdown\n"
		"\t--nodelay           关闭 Nagle 算法\n"
		"\t--cork              打开 TCP_CORK\n"
		"\t--sendbuf size      设置发送缓冲区大小\n"
		"\t--recvbuf size      设置接收缓冲区大小\n"
		"\t--mss size          设置 MSS 大小\n"
		"\t--writesize size    设置客户端一次 write 多少字节\n"
	  "\t--showopts          打印套接字选项\n";

	usage(prog_name);
	puts("");
	printf("%s", s);
}
