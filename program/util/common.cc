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

int resolve(const char* pathname, struct sockaddr_un *addr, socklen_t *len, int abstract) {
	bzero(addr, sizeof(struct sockaddr_un));
	addr->sun_family = AF_LOCAL;
	if (abstract) {
		strncpy(addr->sun_path + 1, pathname, sizeof(addr->sun_path) - 2);
		addr->sun_path[0] = '@';
		*len = SUN_LEN(addr);
		// abstract socket address
		addr->sun_path[0] = 0;
	}
	else {
		strncpy(addr->sun_path, pathname, sizeof(addr->sun_path) - 1);
		*len = sizeof(struct sockaddr_un);
	}
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

void setBroadcast(int sockfd, int onoff) {
	int ret;
	ret = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &onoff, sizeof(onoff));
	if (ret < 0) {
		ERR_EXIT("setBroadcast");
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

void setPassCred(int sockfd, int onoff) {
	int ret;
	ret = setsockopt(sockfd, SOL_SOCKET, SO_PASSCRED, &onoff, sizeof(onoff));
	if (ret < 0) {
		ERR_EXIT("setPassCred");
	}
}

void setNonblock(int fd, int onoff) {
	int flags, ret;
	flags = fcntl(fd, F_GETFL);
	if (flags < 0) ERR_EXIT("setNonblock:F_GETFL");

	if (onoff) {
		flags |= O_NONBLOCK;
	}
	else {
		flags &= ~O_NONBLOCK;
	}

	ret = fcntl(fd, F_SETFL, flags);
	if (ret < 0) ERR_EXIT("setNonblock:F_SETFL");
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

int recvCred(int sockfd, char *buf, int size, struct ucred *cred) {
	int nr;
  struct msghdr msg;
	struct iovec iov[1];
	char control[CMSG_SPACE(sizeof(struct ucred))]; 
	struct cmsghdr *cmptr;

	/* 
	cmptr = (struct cmsghdr*)control;
	cmptr->cmsg_len = CMSG_LEN(sizeof(struct ucred));
	cmptr->cmsg_level = SOL_SOCKET;
	cmptr->cmsg_type = SCM_CREDENTIALS;
	*/

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	iov[0].iov_base = buf;
	iov[0].iov_len = size;
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	msg.msg_control = control;
	msg.msg_controllen = CMSG_SPACE(sizeof(struct ucred));
	msg.msg_flags = 0;

  nr = recvmsg(sockfd, &msg, 0);
	if (nr < 0) return nr;

	if (cred && (cmptr = CMSG_FIRSTHDR(&msg))) {
    if (cmptr->cmsg_len != CMSG_LEN(sizeof(struct ucred))) {
			ERR_QUIT("control length = %d", cmptr->cmsg_len);
		}
		if (cmptr->cmsg_level != SOL_SOCKET) {
			ERR_QUIT("control level != SOL_SOCKET");
		}
		if (cmptr->cmsg_type != SCM_CREDENTIALS) {
			ERR_QUIT("control type != SCM_CREDENTIALS");
		}
		memcpy(cred, CMSG_DATA(cmptr), sizeof(struct ucred));
	}
	return nr;
}

int sendCred(int sockfd, char *buf, int size) {
	int nw;
  struct msghdr msg;
	struct iovec iov[1];
	char control[CMSG_SPACE(sizeof(struct ucred))]; 
	struct cmsghdr *cmptr = (struct cmsghdr*)control;
	struct ucred *cred = (struct ucred*)CMSG_DATA(cmptr);

	cmptr->cmsg_len = CMSG_LEN(sizeof(struct ucred));
	cmptr->cmsg_level = SOL_SOCKET;
	cmptr->cmsg_type = SCM_CREDENTIALS;
	cred->pid = getpid();
	cred->uid = getuid();
	cred->gid = getgid();

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	iov[0].iov_base = buf;
	iov[0].iov_len = size;
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	msg.msg_control = control;
	msg.msg_controllen = CMSG_SPACE(sizeof(struct ucred));
	msg.msg_flags = 0;

  nw = sendmsg(sockfd, &msg, 0);

	return nw;
}

int nbioConnect(int sockfd, const struct sockaddr *addr, socklen_t addrlen, int nsec) {
	int ret, error;
	socklen_t len;
	struct timeval tv;
	fd_set rfds, wfds;

	setNonblock(sockfd, 1);

	ret = connect(sockfd, addr, addrlen);
	if (ret < 0) {
		if (errno != EINPROGRESS) {
#ifdef DEBUG
			ERR_PRINT("errno != EINPROGRESS\n");
#endif
			close(sockfd);
			return -1;
		}
	}
	else if (ret == 0) {
		setNonblock(sockfd, 0);
#ifdef DEBUG
		LOG("connect successfully immediately!\n");
#endif
		return 0;
	}

	FD_ZERO(&rfds);
	FD_ZERO(&wfds);
	FD_SET(sockfd, &rfds);
	FD_SET(sockfd, &wfds);

	tv.tv_sec = nsec;
	tv.tv_usec = 0;

	ret = select(sockfd + 1, &rfds, &wfds, NULL, nsec ? &tv : NULL);
	if (ret < 0) {
#ifdef DEBUG
		ERR_PRINT("select error\n");
#endif
		close(sockfd);
		return -1;
	}
	else if (ret == 0) {
		errno == ETIMEDOUT;
#ifdef DEBUG
		ERR_PRINT("select timedout\n");
#endif
		close(sockfd);
		return -1;
	}

	// 如果正常，一定可写，不一定可读
	// 如果出错，则一定可读可写
	if (FD_ISSET(sockfd, &rfds) || FD_ISSET(sockfd, &wfds)) {
		len = sizeof(error);
		ret = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len);
		if (ret < 0) {
#ifdef DEBUG
			ERR_PRINT("getsockopt return < 0\n");
#endif
			close(sockfd);
			return -1;
		}
	}

	if (error) {
#ifdef DEBUG
		ERR_PRINT("connect error\n");
#endif
		errno = error;
		close(sockfd);
		return -1;
	}

	setNonblock(sockfd, 0);
#ifdef DEBUG
	ERR_PRINT("connect success\n");
#endif
	return 0;
}

int tcpConnect(const char* hostname, int port) {
	int ret, sockfd;
	struct sockaddr_in servaddr;

	ret = resolve(hostname, port, &servaddr);
	if (ret < 0) return -1;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	ret = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
	if (ret < 0) return -1;

	return sockfd;
}

void printData(const unsigned char* data, int len, int cols) {
	int i;
	for (i = 0; i < len; ++i) {
		if (i % cols == 0) LOG("| %02x ", data[i]);
		else LOG("%02x ", data[i]);
		if ((i + 1) % cols == 0) LOG("|\n");
	}

	// 补齐空白
	if (i == len && i % cols != 0) {
		while(1) {
			if (i % cols != 0)
				LOG("-- ");
			else { 
				LOG("|\n");
				break;
			}
			++i;
		}
	}
}

int printIp(const struct ip *ip, int len) {
	int iphlen, flag, offset;
	unsigned char *data;

	iphlen = ip->ip_hl<< 2;
	data = (unsigned char*)ip + iphlen;

	len -= iphlen;


	ERR_PRINT("------------ ip header ------------\n");
	WARNING("version:          %d\n", ip->ip_v);
	WARNING("header len:       %d\n", iphlen);
	WARNING("tos:              %d\n", ip->ip_tos);
	WARNING("total len:        %d\n", ntohs(ip->ip_len));
	WARNING("id:               %d\n", ntohs(ip->ip_id));
	flag = ntohs(ip->ip_off) & 0xe000;
	WARNING("fragment flag:    [RF:%d, DF:%d, MF:%d]\n"
			, flag & IP_RF ? 1:0, flag & IP_DF ? 1:0, flag & IP_MF ? 1:0);
	WARNING("fragment offset:  %d\n", ntohs(ip->ip_off) & 0x1fff);
	WARNING("ttl:              %d\n", ip->ip_ttl);
	WARNING("protocol:         %d\n", ip->ip_p);
	WARNING("checksum:         0x%04x\n", ntohs(ip->ip_sum));
	WARNING("src ip:           %s\n", inet_ntoa(ip->ip_src));
	WARNING("dst ip:           %s\n", inet_ntoa(ip->ip_dst));
	printData(data, len);
	return 0;
}

void printIcmp(struct icmp* icmp, int len) {
	int hlen;

	ERR_PRINT("------------ icmp header ------------\n");
	WARNING("type:             %d\n", icmp->icmp_type);
	WARNING("code:             %d\n", icmp->icmp_code);
	WARNING("checksum:         %04x\n", icmp->icmp_cksum);
	
	hlen = sizeof(struct icmp);
	printData((unsigned char*)icmp + hlen, len - hlen);
}

unsigned short cksum(unsigned short *addr, int len){
	unsigned int sum = 0;  
	while(len > 1){
		sum += *addr++;
		len -= 2;
	}

	// 处理剩下的一个字节
	if(len == 1){
		sum += *(unsigned char*)addr;
	}

	// 将32位的高16位与低16位相加
	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);

	return (unsigned short) ~sum;
}

int recvFromFlags(int sockfd, char* buf, int len, int *flags, 
		struct sockaddr *addr, socklen_t *addrlen, struct in_pktinfo *pkt) {
	int nr;
	struct msghdr msg;
	struct cmsghdr *cmsg;
	// struct sockaddr_dl *sdl; // 数据链路层地址
	struct iovec iov[1];
	union {
		struct cmsghdr cm;
		char control[CMSG_SPACE(sizeof(struct in_pktinfo))];
	}control_un;

	iov[0].iov_base = buf;
	iov[0].iov_len = len;

	msg.msg_name = addr;
	msg.msg_namelen = *addrlen;
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	msg.msg_control = control_un.control;
	msg.msg_controllen = sizeof(control_un.control);
	msg.msg_flags = 0;

	nr = recvmsg(sockfd, &msg, *flags);
	if (nr < 0) {
#ifdef DEBUG
		ERR_EXIT("recvmsg");
#endif
		return nr;
	}

	// 返回的标志位
	*flags = msg.msg_flags;

	if (msg.msg_controllen < sizeof(struct cmsghdr) // 辅助数据长度不够
			|| (msg.msg_flags & MSG_CTRUNC) // 辅助数据被截断
			|| pkt == NULL) { // 用户并不想知道接口信息
		return nr;
	}

	// 遍历辅助数据
	// Linux 采用 IP_PKTINFO 而不是 IP_RECVDSTADDR 和 IP_RECVIF 
	// 该套接字选项对应结构体 struct in_pktinfo
	// 成员 ipi_ifindex 表示数据包从哪个接口进来的
	// 成员 ipi_spec_dst 表示数据包的本地地址（路由地址）
	// 成员 ipi_addr 表示数据包的目的地址
	for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
		if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_PKTINFO) {
			memcpy(pkt, CMSG_DATA(cmsg), sizeof(struct in_pktinfo));
			break;
		}
		
		ERR_QUIT("unknown ancillary data, len = %d, level = %d, type = %d",
				cmsg->cmsg_len, cmsg->cmsg_level, cmsg->cmsg_type);
	}
	return nr;
}

int getIfConf(struct ifreq** ifr) {
	int len, lastlen, sockfd, ret;
	char *buf;
	struct ifconf ifc;

	if (ifr == NULL) return -1;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) ERR_EXIT("socket");
	len = sizeof(struct ifreq);

	// 如果缓冲区大小不够，ioctl 也会成功返回 
	// 所以这里只能采用倍增法试探合适的大小
	while(1) {
		buf = (char*)malloc(len);
		ifc.ifc_buf = buf;
		ifc.ifc_len = len; // 值-结果参数

		// 试探性请求
		ret = ioctl(sockfd, SIOCGIFCONF, &ifc);
		if (ret < 0) {
#ifdef DEBUG
			ERR_EXIT("ioctl");
#endif
			return -1;
		}
		else {
			if (ifc.ifc_len < len) {
				// 说明缓冲区够了 
#ifdef DEBUG
				LOG("buf insufficient, buflen = %d, ifc_len = %d\n", 
						len, ifc.ifc_len);
#endif
				break;
			}
		}

#ifdef DEBUG
		LOG("buf insufficient, buflen = %d, ifc_len = %d\n", 
				len, ifc.ifc_len);
#endif

		// 没有成功，说明内存不够，将长度翻倍
		len <<= 1;
		free(buf);
	}


	*ifr = (struct ifreq*)buf;
	return ifc.ifc_len/sizeof(struct ifreq);
}

void freeIfConf(struct ifreq *ifr) {
	free(ifr);
}

int getIfiInfo(struct ifi_info **ifi) {

	int count, ret, sockfd, i, k, myflags, flags;
	struct ifi_info *_ifi;
	struct ifreq *ifr, ifrcopy;
	struct sockaddr_in *sa;

	if (ifi == NULL) return -1;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) ERR_EXIT("socket");

	count = getIfConf(&ifr);
	if (count < 0) ERR_EXIT("getIfConf");

	_ifi = (struct ifi_info*)malloc(count*sizeof(struct ifi_info));
	bzero(_ifi, count*sizeof(struct ifi_info));

	k = 0;
	for(i = 0; i < count; ++i) {
		// 复制一份 ifr
		ifrcopy = ifr[i];

		// 1. 填充 ifi_name
		myflags = 0;
		memcpy(_ifi[k].ifi_name, ifr[i].ifr_name, IFI_NAMESIZE);

		// 2. 填充 ifi_index
		_ifi[k].ifi_index = if_nametoindex(ifr[i].ifr_name);

		// 3. 填充 mtu
		ret = ioctl(sockfd, SIOCGIFMTU, &ifrcopy);
		if (ret < 0) ERR_EXIT("ioctl");
		_ifi[k].ifi_mtu = ifrcopy.ifr_mtu;

		// 4. 填充 ifi_haddr, ifi_hlen
		ret = ioctl(sockfd, SIOCGIFHWADDR, &ifrcopy);
		if (ret < 0) ERR_EXIT("ioctl");
		memcpy(_ifi[k].ifi_haddr, ((struct sockaddr*)&ifrcopy.ifr_hwaddr)->sa_data, 6);
		_ifi[k].ifi_hlen = 6;

		// 5. 填充 ifi_flags
		ret = ioctl(sockfd, SIOCGIFFLAGS, &ifrcopy);
		if (ret < 0) ERR_EXIT("ioctl");
		flags = ifrcopy.ifr_flags;
		_ifi[k].ifi_flags = flags;

		// 6. 填充 ifi_myflags 
		// 暂时不管
		// _ifi[k].ifi_myflags = 0;
		
		// 7. 填充 ifi_addr 
		sa = (struct sockaddr_in*)&ifr[i].ifr_addr;
		_ifi[k].ifi_addr = (struct sockaddr*)malloc(sizeof(sockaddr_in));
		memcpy(_ifi[k].ifi_addr, sa, sizeof(sockaddr_in));

		// 8. 填充 ifi_netmask
		ret = ioctl(sockfd, SIOCGIFNETMASK, &ifrcopy);
		if (ret < 0) ERR_EXIT("ioctl");
		sa = (struct sockaddr_in*)&ifrcopy.ifr_netmask;
		_ifi[k].ifi_netmask = (struct sockaddr*)malloc(sizeof(sockaddr_in));
		memcpy(_ifi[k].ifi_netmask, sa, sizeof(sockaddr_in));

		// 9. 填充 ifi_brdaddr, 只有支持 broadcast 的接口才有
		if (flags & IFF_BROADCAST) {
			ret = ioctl(sockfd, SIOCGIFBRDADDR, &ifrcopy);
			if (ret < 0) ERR_EXIT("ioctl");
			sa = (struct sockaddr_in*)&ifrcopy.ifr_addr;
			_ifi[k].ifi_brdaddr = (struct sockaddr*)malloc(sizeof(sockaddr_in));
			memcpy(_ifi[k].ifi_brdaddr, sa, sizeof(sockaddr_in));
		}

		// 10. 填充 ifi_dstaddr, 只有是 P2P 的时候才有效
		if (flags & IFF_POINTOPOINT) {
			ret = ioctl(sockfd, SIOCGIFDSTADDR, &ifrcopy);
			if (ret < 0) ERR_EXIT("ioctl");
			sa = (struct sockaddr_in*)&ifrcopy.ifr_dstaddr;
			_ifi[k].ifi_dstaddr = (struct sockaddr*)malloc(sizeof(sockaddr_in));
			memcpy(_ifi[k].ifi_dstaddr, sa, sizeof(sockaddr_in));
		}

		++k;
	}

	freeIfConf(ifr);

	*ifi = _ifi;
	return k;
}

void freeIfiInfo(struct ifi_info* ifi, int n) {
	int i;
	for (i = 0; i < n; ++i) {
		free(ifi[i].ifi_addr);
		free(ifi[i].ifi_netmask);
		free(ifi[i].ifi_brdaddr);
		free(ifi[i].ifi_dstaddr);
	}
	free(ifi);
}
