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

int writen(int fd, char* buf, int n) {
	int len, nwrite;
	nwrite = 0;
	while(nwrite < n) {
		len = write(fd, buf + nwrite, n - nwrite);
		if (len <= 0) {
			if (errno == EINTR) continue;
			return -1;
		}

		nwrite += len;
	}
	return nwrite;
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
