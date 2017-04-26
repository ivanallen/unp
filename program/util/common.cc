#include "common.h"

extern int h_errno;

int64_t now() {
	int ret;
	struct timeval now;
	ret = gettimeofday(&now, NULL);
	if (ret < 0) ERR_EXIT("gettimeofday");
	return now.tv_sec * 1000000 + now.tv_usec;
}

int resolve(const char* hostname, int port, struct sockaddr_in *addr) {
	int ret;
	struct hostent *he;

	bzero(addr, sizeof(struct sockaddr_in));

	ret = inet_aton(hostname, &addr->sin_addr);
	if (ret == 0) {
		he = gethostbyname(hostname);
		errno = h_errno;
		if (!he) return -1; 
	  addr->sin_addr= *(struct in_addr*)(he->h_addr);
	}

	addr->sin_family = AF_INET;
	addr->sin_port = htons((short)port);

	return 0;
}

int readn(int fd, char* buf, int n) {
	int len, nread;

	nread = 0;// 已经读取的字节数
	while(nread < n) {
		len = read(fd, buf + nread, n - nread); 
		if (len < 0) {
			if (errno == EINTR) continue;
			return -1;
		}
		if (len == 0) break;
		nread += len;
	}
	return nread;
}

int writen(int fd, const char* buf, int n) {
	int len, nwrite;
	nwrite = 0;
	while(nwrite < n) {
		len = write(fd, buf + nwrite, n - nwrite);
		if (len < 0) {
			if (errno == EINTR) continue;
			return -1;
		}
		else if (len == 0) {
			// short write
			puts("write return 0");
			break;
		}

		nwrite += len;
	}
	return nwrite;
}

// ignore interrupted by signal
int iread(int fd, char* buf, int n) {
	int ret;
  while(1) {
		ret = read(fd, buf, n);
		if (ret < 0) {
			if (errno == EINTR) continue;
			return -1;
		}
		break;
	}
	return ret;
}

int iwrite(int fd, const char* buf, int n) {
	int ret;
	while(1) {
		ret = write(fd, buf, n);
		if (ret < 0) {
			if (errno == EINTR) continue;
			return -1;
		}
		break;
	}
	return ret;
}

std::map<std::string, std::string> parsecmdline(int argc, char* argv[]) {
  int i;
	char* cur;
	std::map<std::string, std::string> res;

	i = 1;
	while(i < argc) {
		cur = argv[i];
		if (strlen(cur) < 2 || cur[0] != '-') {
			printf("未识别的参数: %s\n", cur);
			exit(1);
		}

		++cur;
		if (cur[0] == '-') {
			// 长参数
			++cur;
			if (cur[0] == '\0') {
				printf("未识别的参数: %s\n", cur);
				exit(1);
			}
		}

		// 选项带参数
		if (i < argc - 1 && argv[i+1][0] != '-') {
			res[std::string(cur)] = std::string(argv[i+1]);
			i += 2;
		}
		else {
			res[std::string(cur)] = std::string("true");
			i++;
		}
	}

	return res;
}

int readline(int fd, char* buf, int n) {
	int i, nr, len;
	char c;

	i = 0;
	nr = 0;
  while(1) {	
		len = read(fd, &c, 1);
		if (len == 0) return nr;
	  else if (len < 0) {
			if (errno == EINTR) continue;
			return -1;
		}	

		nr += len;
		buf[i++] = c;

		if (c == '\n') {
      break;
		}
	}
	return nr;
}

void toUpper(char* str, int n) {
	int i = 0;

  while(i < n && str[i]) {
		str[i] = toupper(str[i]);
		++i;
	}
}


void registSignal(int sig, void (*handler)(int)) {
	struct sigaction sa;
	sa.sa_handler = handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (sigaction(sig, &sa, NULL) < 0) {
		ERR_EXIT("sigaction");
	}
}

void ignoreSignal(int sig) {
	void (*ret)(int);

	ret = signal(sig, SIG_IGN);
	if (ret == SIG_ERR) {
		ERR_EXIT("ignoreSignal");
	}
}

void reuseAddr(int sockfd, int onoff) {
	int ret;
  ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &onoff, sizeof(onoff));
	if (ret < 0) {
		ERR_EXIT("reuseAddr");
	}
}

void setLinger(int sockfd, int onoff, int seconds) {
	int ret;
	struct linger lgr;

	lgr.l_onoff = onoff;
	lgr.l_linger = seconds;

	ret = setsockopt(sockfd, SOL_SOCKET, SO_LINGER, &lgr, sizeof(lgr));
	if (ret < 0) {
		ERR_EXIT("setLinger");
	}
}

void setSendBufSize(int sockfd, int size) {
	int ret;
	ret = setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
	if (ret < 0) {
		ERR_EXIT("setSendBufSize");
	}
}

void setRecvBufSize(int sockfd, int size) {
	int ret;
	ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
	if (ret < 0) {
		ERR_EXIT("setRecvBufSize");
	}
}

void setMaxSegSize(int sockfd, int size) {
	int ret;
	ret = setsockopt(sockfd, IPPROTO_TCP, TCP_MAXSEG, &size, sizeof(size));
	if (ret < 0) {
		ERR_EXIT("setMaxSegSize");
	}
}

void setNoDelay(int sockfd, int onoff) {
	int ret;
	ret = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &onoff, sizeof(onoff));
	if (ret < 0) {
		ERR_EXIT("setNoDelay");
	}
}

void setCork(int sockfd, int onoff) {
	int ret;
	ret = setsockopt(sockfd, IPPROTO_TCP, TCP_CORK, &onoff, sizeof(onoff));
	if (ret < 0) {
		ERR_EXIT("setCork");
	}
}
