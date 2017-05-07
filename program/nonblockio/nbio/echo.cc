#include "common.h"

void doServer(int);
void doClient(int);
void server_routine();
void client_routine();
void handler(int);

const char *dots[3] = {".", "..", "..."};

struct Options {
	int isServer;
	char hostname[32];
	int port;
	int length;
	int slow;
} g_option;

int main(int argc, char* argv[]) {
	Args args = parsecmdline(argc, argv);
	if (CONTAINS(args, "help")){
		ERR_QUIT("Usage:\n  %s [--help] [-s] [-h hostname] [-p port] [-l length] [--slow]\n", argv[0]);
	}


	SETBOOL(args, g_option.isServer, "s", 0);
	SETBOOL(args, g_option.slow, "slow", 0);
	SETSTR(args, g_option.hostname, "h", "0");
	SETINT(args, g_option.port, "p", 8000);
	SETINT(args, g_option.length, "l", 4096);

	registSignal(SIGINT, handler);

	LOG("\x1b[?25l");
	if (g_option.isServer) {
		server_routine();
	}
	else {
		client_routine();
	}
	LOG("\x1b[?25h");
  return 0;
}

void server_routine() {
	int ret, listenfd, sockfd;
	struct sockaddr_in servaddr, cliaddr;
	socklen_t len;

	ret = resolve(g_option.hostname, g_option.port, &servaddr);
	if (ret < 0) ERR_EXIT("resolve");

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0) ERR_EXIT("socket");
	reuseAddr(listenfd, 1);
	showopts(listenfd, "SO_RCVBUF", stderr);
	showopts(listenfd, "SO_SNDBUF", stderr);

	ret = bind(listenfd, (struct sockaddr*)&servaddr, sizeof servaddr);
	if (ret < 0) ERR_EXIT("bind");

	ret = listen(listenfd, 5);
	if (ret < 0) ERR_EXIT("listen");

	len = sizeof(cliaddr);
	sockfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len);
	if (sockfd < 0) ERR_EXIT("accept");

	WARNING("%s:%d come in\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

	doServer(sockfd);
	close(sockfd);
}

void client_routine() {
	int ret, sockfd;
	struct sockaddr_in servaddr;
	ret = resolve(g_option.hostname, g_option.port, &servaddr);
	if (ret < 0) ERR_EXIT("resolve");

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) ERR_EXIT("socket");
	showopts(sockfd, "SO_RCVBUF", stderr);
	showopts(sockfd, "SO_SNDBUF", stderr);

	ret = connect(sockfd, (struct sockaddr*)&servaddr, sizeof servaddr);
	if (ret < 0) ERR_EXIT("connect");

	// 在 connect 之后再设置非阻塞
	setNonblock(sockfd, 1);
	doClient(sockfd);

	close(sockfd);
}

void doServer(int sockfd) {
	int nr, nw, total, totalsend, i;
	char buf[4096];

	i = 0;
	total = 0;
	totalsend = 0;

	while(1) {
    nr = iread(sockfd, buf, 4096);
		if (nr < 0) ERR_EXIT("iread");
		else if (nr == 0) {
			LOG("\x1b[3B");
			ERR_PRINT("client closed\n");
			break;
		}

		toUpper(buf, nr);
		total += nr;
		LOG("received %d bytes totally%s\n", total, dots[i]);
		LOG("ready to send %d bytes%s\n", nr, dots[i]);
		nw = writen(sockfd, buf, nr);
		totalsend += nw;
		LOG("send %d bytes totally%s\n", totalsend, dots[i]);
		LOG("\x1b[3A");
		i = (i + 1) % 3;
		if (nw < 0) {
			LOG("\x1b[3B");
			ERR_EXIT("writen");
		}
	}
}

void doClient(int sockfd) {
	int n, nr, nw, i, length, maxfd, ret, cliclosed, servclosed, totalsend, actualsend;
  char *to, *from, *tostart, *toend, *fromstart, *fromend;
	fd_set rfds, wfds, fds;

	i = 0;
	totalsend = 0;
	actualsend = 0;
	cliclosed = 0;
	servclosed = 0;
	length = g_option.length;
	// 发送缓冲区 
	to = (char*)malloc(length);
	assert(to);
	// 接收缓冲区
	from = (char*)malloc(length);
	assert(from);

	tostart = toend = to;
	fromstart = fromend = from;
	/*
    to: |oooo|xxxxxxxxxxxxx|-----------------|
              ^           ^
              tostart     toend 
		x 表示尚未发送， o 表示已经发送
	 */

	maxfd = sockfd;

	while(1) {
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		// 发送缓冲区有空闲，监听标准输入
		if (cliclosed == 0 && toend < to + length) 
			FD_SET(STDIN_FILENO, &rfds);
		// 接收缓冲区有空闲，监听 sockfd
		if (servclosed == 0 && fromend < from + length)
			FD_SET(sockfd, &rfds);
		// 有尚未发送的数据，监听 sockfd 可写事件
		if (tostart < toend)
			FD_SET(sockfd, &wfds);
		// 接收缓冲区有尚未处理的数据，监听 stdout 可写事件
		if (fromstart < fromend)
			FD_SET(STDOUT_FILENO, &wfds);

		ret = select(maxfd + 1, &rfds, &wfds, NULL, NULL);
		if (ret < 0) {
			if (errno == EINTR) continue;
			ERR_EXIT("select");
		}

		if (FD_ISSET(STDIN_FILENO, &rfds)) {
			// 从标准输入读取数据到发送缓冲区空闲区
			nr = read(STDIN_FILENO, toend, to + length - toend );
			if (nr < 0) {
				// 这种情况不太可能
				if (errno != EWOULDBLOCK)
					ERR_EXIT("read from stdin");
			}
			else if (nr == 0) {
				// client no data to send. 
				cliclosed = 1;
				
				LOG("stdin closed!\n");
				// 发送缓冲区没有数据要发送了, 但是可能还有数据没有接收完，不能退出
				if (tostart == toend) {
					LOG("no data to send\n");
					shutdown(sockfd, SHUT_WR);
				}
			}
			else {
				// 更新 toend
				toend += nr;
				// 通知发送缓冲区有数据可发送
				FD_SET(sockfd, &wfds);
			}
		}

		if (FD_ISSET(sockfd, &rfds)) {
			// 从套接字读取数据到接收缓冲区
			nr = read(sockfd, fromend, from + length - fromend);
			if (nr < 0) {
				// 这似乎也不太可能
				if (errno != EWOULDBLOCK)
					ERR_EXIT("read from sockfd");
			}
			else if (nr == 0) {
				// server no data to send.
				if (cliclosed) {
					LOG("server closed!\n");
				}
				else {
					ERR_PRINT("server exception!\n");
				}

				// 服务器已经关闭
				servclosed = 1;

				// 接收缓冲区数据已经处理完毕，有可能未处理完毕（假定写入到 stdout 无限慢）
				if (fromstart == fromend) {
					LOG("1:finished!\n");
					break;
				}
			}
			else {
				fromend += nr;
				// 通知接收缓冲区有数据要打印到标准输出
				FD_SET(STDOUT_FILENO, &wfds);
			}
		}

		// 接收缓冲区有数据未处理
		if (FD_ISSET(STDOUT_FILENO, &wfds) && ((n = fromend - fromstart) > 0)) {
			nw = write(STDOUT_FILENO, fromstart, g_option.slow ? 1 : n);
			if (nw < 0) {
				if (errno != EWOULDBLOCK)
					ERR_EXIT("write to stdout");
			}
			else {
				fromstart += nw;
				if (fromstart == fromend) {
					fromstart = fromend = from;
					if (servclosed) {
						LOG("2:finished!\n");
						break;
					}
				}
			}
		}

		// 发送缓冲区有数据可发送
		if (FD_ISSET(sockfd, &wfds) && ((n = toend - tostart) > 0)) {
			nw = write(sockfd, tostart, n);
			if (nw < 0) {
				if (errno != EWOULDBLOCK)
					ERR_EXIT("write to sockfd");
			}
			else {
				tostart += nw;
				if (tostart == toend) {
					tostart = toend = to;
					// 数据已经发送完毕，客户端已经没有数据要发送了
					if (cliclosed) {
						shutdown(sockfd, SHUT_WR);
					}
				}
			}
		}
	}
	free(to);
	free(from);
}

void handler(int sig) {
	if (sig == SIGINT) {
		ERR_PRINT("exited!\n");
		fprintf(stderr, "\x1b[0m\x1b[?25h");
		exit(0);
	}
}
