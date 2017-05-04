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

int resolve(const char* pathname, struct sockaddr_un *addr, int abstract) {
	bzero(addr, sizeof(struct sockaddr_un));
	addr->sun_family = AF_LOCAL;
	strncpy(addr->sun_path, pathname, sizeof(addr->sun_path) - 1);
	if (abstract)
		addr->sun_path[0] = 0;
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


void registSignal(int sig, void (*handler)(int), void(**oldhandler)(int)) {
	struct sigaction sa, old;
	sa.sa_handler = handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (sigaction(sig, &sa, &old) < 0) {
		ERR_EXIT("sigaction");
	}

	if (oldhandler)
		*oldhandler = old.sa_handler;
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
void setRecvTimeout(int sockfd, int nsec) {
	int ret;
	struct timeval tv;
	tv.tv_sec = nsec;
	tv.tv_usec = 0;

	ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	if (ret < 0) {
		ERR_EXIT("setRecvTimeout");
	}
}
void setSendTimeout(int sockfd, int nsec) {
	int ret;
	struct timeval tv;
	tv.tv_sec = nsec;
	tv.tv_usec = 0;

	ret = setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
	if (ret < 0) {
		ERR_EXIT("setRecvTimeout");
	}
}

char* itoa(int n) {
	static char buf[16];
	snprintf(buf, 16, "%d", n);
	return buf;
}

int readfd(int fd, char* buf, int size, int *recvfd) {
	int nr;
	struct msghdr msg;
	struct iovec iov[1];
	struct cmsghdr *cmptr;

	union {
		struct cmsghdr cm;
		char control[CMSG_SPACE(sizeof(int))];
	} control_un;

	msg.msg_control = control_un.control;
	msg.msg_controllen = sizeof(control_un.control);
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	iov[0].iov_base = buf;
	iov[0].iov_len = size;
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;

	nr = recvmsg(fd, &msg, 0);
	if (nr <= 0) return nr;

	if ((cmptr = CMSG_FIRSTHDR(&msg)) != NULL
			&& cmptr->cmsg_len == CMSG_LEN(sizeof(int))) {
		if (cmptr->cmsg_level != SOL_SOCKET) {
			ERR_QUIT("readfd: control level != SOL_SOCKET");
		}
		if (cmptr->cmsg_type != SCM_RIGHTS) {
			ERR_QUIT("readfd: control type != SCM_RIGHTS");
		}
		*recvfd = *((int*)CMSG_DATA(cmptr));
	}
	else {
		*recvfd = -1;
	}
	return nr;
}

int writefd(int fd, char* buf, int size, int sendfd) {
	int nw;
	struct msghdr msg;
	struct iovec iov[1];
	struct cmsghdr *cmptr;

	union {
		struct cmsghdr cm;
		char control[CMSG_SPACE(sizeof(int))];
	} control_un;

	msg.msg_control = control_un.control;
	msg.msg_controllen = sizeof(control_un.control);

	cmptr = CMSG_FIRSTHDR(&msg);
	cmptr->cmsg_len = CMSG_LEN(sizeof(int));
	cmptr->cmsg_level = SOL_SOCKET;
	cmptr->cmsg_type = SCM_RIGHTS;
	*((int*)CMSG_DATA(cmptr)) = sendfd;

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	iov[0].iov_base = buf;
	iov[0].iov_len = size;
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;

	nw = sendmsg(fd, &msg, 0);
	return nw;
}

int myOpen(const char* pathname, int mode) {
  pid_t pid;
	int fd, ret, status, sockfd[2];
	char c;
	
	ret = socketpair(AF_LOCAL, SOCK_STREAM, 0, sockfd);
	if (ret < 0) ERR_EXIT("myOpen:socketpair");

	pid = fork();
	if (pid < 0) ERR_EXIT("myOpen:fork");
	if (pid == 0) {
		// child
		close(sockfd[0]);
		execlp("./openfile", "./openfile", itoa(sockfd[1]), pathname, itoa(mode), NULL);
		ERR_EXIT("myOpen:execlp");
	}

	ret = waitpid(pid, &status, 0);
	if (ret < 0) ERR_EXIT("myOpen:waitpid");
	if (WIFEXITED(status) == 0) ERR_QUIT("myOpen:child terminated abnormally!\n");

	status = WEXITSTATUS(status);
	if (status == 0) {
		readfd(sockfd[0], &c, 1, &fd);
#ifdef DEBUG
		printf("myOpen: c = %c\n", c);
#endif
	}
	else {
		errno = status;
		fd = -1;
	}
	close(sockfd[0]);
	return fd;
}
