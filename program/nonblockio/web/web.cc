#include "common.h"

#define F_CONNECTING 1 
#define F_READING    2
#define F_DONE       4

#define PORT 80

struct file {
	char f_name[256];
	char f_host[256];
	int f_fd;
	int f_flags;
} file[1000];

void homePage(const char* hostname, const char* filename);
int get(const char*hostname, const char* filename);
int startConnect(struct file* f);
void request(struct file* f);

fd_set rfds, wfds;
int nfiles, nconn, maxnconn, nlefttoread, nlefttoconn, maxfd;

int main(int argc, char* argv[]) {
	int i, ret, n, nr, nw, fd, flags, error;
	socklen_t len;
	char buf[4096];
	fd_set rs, ws;

	homePage(argv[2], argv[3]); // 主页测试

	nfiles = MIN(argc - 4, 1000); // 要下载的文件数
	maxnconn = atoi(argv[1]); // 同时最多允许有多少个 tcp 连接
	nconn = 0; // 当前 tcp 连接数
	nlefttoread = nfiles; // 还有多少文件需要下载
	nlefttoconn = nfiles; // 还有多少文件没有建立 tcp 连接

	WARNING("%d files\n", nfiles);

	// 初始化 struct file 数组
	for (i = 0; i < nfiles; ++i) {
		strncpy(file[i].f_name, argv[i + 4], 256);
		strncpy(file[i].f_host, argv[2], 256);
		file[i].f_fd = -1;
		file[i].f_flags = 0;
	}

	FD_ZERO(&rfds);
	FD_ZERO(&wfds);
	maxfd = -1;

	while(nlefttoread > 0) {
		// 发起连接
		while(nconn < maxnconn && nlefttoconn > 0) {
			for (i = 0; i < nfiles; ++i) {
				if (file[i].f_flags == 0) break;
			}
			if (i == nfiles) {
				ERR_QUIT("nlefttoconn = %d, but nothing found", nlefttoconn);
			}
			startConnect(&file[i]);
			WARNING("initiate connection on %s\n", file[i].f_name);
			++nconn;
			--nlefttoconn;
		}

		rs = rfds;
		ws = wfds;
		n = select(maxfd + 1, &rs, &ws, NULL, NULL);

		for (i = 0; i < nfiles; ++i) {
			flags = file[i].f_flags;
			if (flags == 0 || flags & F_DONE) continue;
			fd = file[i].f_fd;

			if (flags & F_CONNECTING && (FD_ISSET(fd, &rs) || FD_ISSET(fd, &ws))) {
				len = sizeof(error);
				ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len);
				if (ret < 0 || error != 0) {
					ERR_PRINT("nonblocking connect failed for %s", file[i].f_name);
					file[i].f_flags = F_DONE;
				}
				WARNING("connection established for %s\n", file[i].f_name);
				FD_CLR(fd, &wfds);
				request(&file[i]);
			}
			else if (flags & F_READING && FD_ISSET(fd, &rs)) {
				nr = iread(fd, buf, 4096);
				if (nr < 0) {
					if (errno == EWOULDBLOCK) {
						ERR_PRINT("read block on %s\n", file[i].f_name);
					}
					else {
						ERR_EXIT("iread");
					}
				}
				else if (nr == 0) {
					WARNING("end-of-file on %s\n", file[i].f_name);
					close(fd);
					file[i].f_flags = F_DONE;
					FD_CLR(fd, &rfds);
					--nconn;
					--nlefttoread;
				}
				else {
					LOG("read %d bytes from %s\n", nr, file[i].f_name);
				}
			}
		}
	}
}

void homePage(const char* hostname, const char* filename) {
	int ret;
	ret = get(hostname, filename);
	if (ret < 0) ERR_PRINT("get");
}

int get(const char* hostname, const char* filename) {
	int nr, nw, n, sockfd;
	char cmd[4096];
	char buf[4096];

	sockfd = tcpConnect(hostname, PORT);
	if (sockfd < 0) {
		ERR_EXIT("get:tcpConnect");
	}

	n = snprintf(cmd, 4096, "GET %s HTTP/1.1\r\n"
			"Host: %s\r\nConnection: close\r\n\r\n", filename, hostname);
	write(STDOUT_FILENO, cmd, n);

	nw = writen(sockfd, cmd, n);

	while(1) {
    nr = iread(sockfd, buf, 4096);
    if (nr <= 0) {
			break;
		}
		write(STDOUT_FILENO, buf, nr);
	}

	close(sockfd);
	return nr;
}

int startConnect(struct file* f) {
  int fd, ret;
	socklen_t len;
	struct sockaddr_in servaddr;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) return -1; 

	f->f_fd = fd;
	f->f_flags = F_CONNECTING;

	setNonblock(fd, 1);

	ret = resolve(f->f_host, PORT, &servaddr);
	if (ret < 0) return -1;

	ret = connect(fd, (struct sockaddr*)&servaddr, sizeof(servaddr));
	if (ret == 0) {
		return 0;
	}

	if (errno != EINPROGRESS) {
		close(fd);
		return -1;
	}

	FD_SET(fd, &rfds);
	FD_SET(fd, &wfds);
	if (fd > maxfd) maxfd = fd;

	return 0;
}

void request(struct file* f) {
	int n;
	char cmd[4096];

	n = snprintf(cmd, 4096, "GET %s HTTP/1.1\r\n"
			"Host: %s\r\nConnection: close\r\n\r\n", f->f_name, f->f_host);
	writen(f->f_fd, cmd, n);
	f->f_flags = F_READING;
	FD_SET(f->f_fd, &rfds);
	if (f->f_fd > maxfd) maxfd = f->f_fd;
}
