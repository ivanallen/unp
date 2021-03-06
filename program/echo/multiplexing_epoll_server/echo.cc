#include "common.h"
// 现代的 linux 已经将这个宏废弃了，所以自己定义一个
#define MAX_EVENTS 5

struct Option{
	char hostname[32];
	int port;
	int isServer;
}g_option;

void server_routine();
void client_routine();
int doAccept(int listenfd, int epfd);
int doServer(int sockfd);
void doClient(int sockfd);
int makefdset(int fds[], int n, fd_set *fdst);

int main(int argc, char* argv[]) {
	Args args = parsecmdline(argc, argv);
	if (args.empty() || !CONTAINS(args, "h")) {
		printf("Usage:\n  %s [-s] <-h hostname> [-p port]\n", argv[0]);
	}

	SETSTR(args, g_option.hostname, "h", "0");
	SETINT(args, g_option.port, "p", 8000);
	SETBOOL(args, g_option.isServer, "s", 0);

	if (g_option.isServer) {
		server_routine();
	}
	else {
		client_routine();
	}
	return 0;
}

void server_routine() {
	int i, ret, nready, epfd, listenfd, sockfd;
	struct sockaddr_in servaddr;
	struct epoll_event evts[MAX_EVENTS];
	struct epoll_event evt;

	ret = resolve(g_option.hostname, g_option.port, &servaddr);
	if (ret < 0) ERR_EXIT("resolve");

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0) ERR_EXIT("socket");

	ret = bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
	if (ret < 0) ERR_EXIT("bind");

	ret = listen(listenfd, 5);
	if (ret < 0) ERR_EXIT("listen");

	// 参数大于 0 即可，它原本表示想添加多少描述符到内核
	epfd = epoll_create(1);
	if (epfd < 0) ERR_EXIT("epoll_create");
	
	// add listenfd
  evt.data.fd = listenfd;
	evt.events = EPOLLIN;
	ret = epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &evt);
	if (ret < 0) ERR_EXIT("epoll_ctl add listenfd");


	while(1) {
    nready = epoll_wait(epfd, evts, MAX_EVENTS, 0);
		if (nready < 0) {
			if (errno == EINTR) continue;
			ERR_EXIT("epoll");
		}
		if (nready == 0) continue;

		// 处理服务器收到的数据
		for (i = 0; i < nready; ++i) {
			if (evts[i].data.fd == listenfd && evts[i].events & EPOLLIN) {
				ret = doAccept(listenfd, epfd);
				if (ret < 0) {
					puts("Too many open files!");
				}
			}
			else if (evts[i].events & EPOLLIN) {
				ret = doServer(evts[i].data.fd);
				// ret == 0 表示对端关闭, -1 表示连接被重置
				if (ret <= 0) {
					ret = epoll_ctl(epfd, EPOLL_CTL_DEL, evts[i].data.fd, NULL);
					if (ret < 0) ERR_EXIT("epoll_ctl delete sockfd");
					close(evts[i].data.fd);
				}
			}
		}
	}
}

int doAccept(int listenfd, int epfd) {
	int sockfd, i, ret;
	struct sockaddr_in cliaddr;
	socklen_t cliaddrlen;
	struct epoll_event evt;

	cliaddrlen = sizeof(cliaddr);
	sockfd = accept(listenfd, (struct sockaddr*)&cliaddr, &cliaddrlen);
	if (sockfd == -1) {
		if (errno != EINTR && errno != ECONNABORTED) {
			ERR_EXIT("doAccept");
		}
	}

	printf("%s:%d come in\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
	
  evt.data.fd = sockfd;
	evt.events = EPOLLIN;
	ret = epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &evt);
	if (ret < 0) ERR_EXIT("epoll_ctl add connected sockfd");
	return 0;
}

void client_routine() {
	int ret, sockfd;
	struct sockaddr_in servaddr;

	ret = resolve(g_option.hostname, g_option.port, &servaddr);
	if (ret < 0) ERR_EXIT("resolve");

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) ERR_EXIT("socket");

	ret = connect(sockfd, (struct sockaddr*)&servaddr, sizeof servaddr);
	if (ret < 0) ERR_EXIT("connect");

	doClient(sockfd);

	// close(sockfd);
}

// return maxfd
int makefdset(int fds[], int n, fd_set *fdst) {
	int i, maxfd;

	maxfd = 0;
	FD_ZERO(fdst);

	for (i = 0; i < n; ++i) {
		if (fds[i] != -1) {
      FD_SET(fds[i], fdst);
			if (fds[i] > maxfd) maxfd = fds[i];
		}
	}

	return maxfd;
}

// return 0: peer close;
int doServer(int sockfd) {
	int nr, nw;
	char buf[4096];

	nr = readline(sockfd, buf, 4096);
	if (nr == 0) {
		puts("peer closed");
		return 0;
	}
	else if (nr < 0) {
		if (errno = ECONNRESET) {
			perror("readline");
			return -1;
		}
		ERR_EXIT("readline");
	}

	toUpper(buf, nr);

	nw = writen(sockfd, buf, nr);
	if (nw < nr) {
		puts("short write");
	}
	return 1;
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
