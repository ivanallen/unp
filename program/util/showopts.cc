#include "common.h"
#include <netinet/sctp.h>
/*
 * 说明：如果提示缺少 netinet/sctp.h 文件，说明你的系统没有相关的 sctp 开发环境，按照下面的方式安装：
 *
 * ubuntu: sudo apt-get install libsctp-dev
 * centos: sudo yum install lksctp-tools-devel.x86_64
 * 或者: sudo yum install lksctp-tools-devel.i686
 */

union val {
	int i_val; // 标志位，比如 SO_REUSEADDR
	long l_val;
	struct linger linger_val;
	struct timeval timeval_val;
}val;

static char *sock_str_flag(union val *, int);
static char *sock_str_int(union val *, int);
static char *sock_str_linger(union val *, int);
static char *sock_str_timeval(union val *, int);
static void printopt(int fd, struct sock_opts* ptr, const char* prompt, FILE*);
	
struct sock_opts {
	const char *opt_str;
	int opt_level;
	int opt_name;
	char *(*opt_val_str)(union val *, int);
} sock_opts[] = {
	{ "SO_BROADCAST", SOL_SOCKET, SO_BROADCAST, sock_str_flag },
	{ "SO_DEBUG", SOL_SOCKET, SO_DEBUG, sock_str_flag },
	{ "SO_DONTROUTE", SOL_SOCKET, SO_DONTROUTE, sock_str_flag },
	{ "SO_ERROR", SOL_SOCKET, SO_ERROR, sock_str_int},
	{ "SO_KEEPALIVE", SOL_SOCKET, SO_KEEPALIVE, sock_str_flag },
	{ "SO_LINGER", SOL_SOCKET, SO_LINGER, sock_str_linger},
	{ "SO_OOBINLINE", SOL_SOCKET, SO_OOBINLINE, sock_str_flag },
	{ "SO_RCVBUF", SOL_SOCKET, SO_RCVBUF, sock_str_int },
	{ "SO_SNDBUF", SOL_SOCKET, SO_SNDBUF, sock_str_int },
	{ "SO_RCVLOWAT", SOL_SOCKET, SO_RCVLOWAT, sock_str_int },
	{ "SO_SNDLOWAT", SOL_SOCKET, SO_SNDLOWAT, sock_str_int },
	{ "SO_RCVTIMEO", SOL_SOCKET, SO_RCVTIMEO, sock_str_timeval},
	{ "SO_SNDTIMEO", SOL_SOCKET, SO_SNDTIMEO, sock_str_timeval},
	{ "SO_REUSEADDR", SOL_SOCKET, SO_REUSEADDR, sock_str_flag },
#ifdef SO_REUSEPORT
	{ "SO_REUSEPORT", SOL_SOCKET, SO_REUSEPORT, sock_str_flag },
#else
	{ "SO_REUSEPORT", 0, 0, NULL},
#endif
	{ "SO_TYPE", SOL_SOCKET, SO_TYPE, sock_str_int},
#ifdef SO_USELOOPBACK_
	{ "SO_USELOOPBACK", SOL_SOCKET, SO_USELOOPBACK, sock_str_flag },
#endif
	{ "IP_TOS", IPPROTO_IP, IP_TOS, sock_str_flag },
	{ "IP_TTL", IPPROTO_IP, IP_TTL, sock_str_int },
#ifdef SO_DONTFRAG_
	{ "IPV6_DONTFRAG", IPPROTO_IPV6, IPV6_DONTFRAG, sock_str_flag },
#endif
	{ "IPV6_UNICAST_HOPS", IPPROTO_IPV6, IPV6_UNICAST_HOPS, sock_str_int },
	{ "IPV6_V6ONLY", IPPROTO_IPV6, IPV6_V6ONLY, sock_str_flag },
	{ "TCP_MAXSEG", IPPROTO_TCP, TCP_MAXSEG, sock_str_int },
	{ "TCP_NODELAY", IPPROTO_TCP, TCP_NODELAY, sock_str_flag },
	{ "SCTP_AUTOCLOSE", IPPROTO_SCTP, SCTP_AUTOCLOSE, sock_str_int },
#ifdef SCTP_MAXBURST 
	{ "SCTP_MAXBURST", IPPROTO_SCTP, SCTP_MAXBURST, sock_str_int },
#endif
	{ "SCTP_MAXSEG", IPPROTO_SCTP, SCTP_MAXSEG, sock_str_int },
	{ "SCTP_NODELAY", IPPROTO_SCTP, SCTP_NODELAY, sock_str_flag },
};

void showopts(int fd, const char* opt, FILE* fp) {
	int i;
	int size = sizeof(sock_opts)/sizeof(struct sock_opts);
	
	for (i = 0; i < size; ++i) {
		if (opt != NULL) {
			if (!strcmp(opt, sock_opts[i].opt_str)) {
				printopt(fd, &sock_opts[i], "current value", fp);
				break;
			}
			continue;
		}

		printopt(fd, &sock_opts[i], "current value", fp);
	}

	if (i == size && opt) {
		LOG("no such options: %s\n", opt);
	}
}
void showopts(const char* opt, FILE* fp) {
	int i, fd;
	int size = sizeof(sock_opts)/sizeof(struct sock_opts);
	struct sock_opts* ptr;
	
	for (i = 0; i < size; ++i) {
		ptr = &sock_opts[i];
		switch(ptr->opt_level) {
			case SOL_SOCKET:
			case IPPROTO_IP:
			case IPPROTO_TCP:
				fd = socket(AF_INET, SOCK_STREAM, 0);
				break;
			case IPPROTO_IPV6:
				fd = socket(AF_INET6, SOCK_STREAM, 0);
				break;
			case IPPROTO_SCTP:
				fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
				break;
			default:
				printf("Can't create fd for level %d, opt : %s\n", ptr->opt_level, ptr->opt_str);
		}
		if (opt != NULL) {
			if (!strcmp(opt, sock_opts[i].opt_str)) {
				printopt(fd, &sock_opts[i], "default", fp);
				close(fd);
				break;
			}
			close(fd);
			continue;
		}
		printopt(fd, &sock_opts[i], "default", fp);
		close(fd);
	}

	if (i == size && opt) {
		LOG("no such options: %s\n", opt);
	}
}

static void printopt(int fd, struct sock_opts* ptr, const char* prompt, FILE* fp) {
	int ret;
	socklen_t len;
	fprintf(fp, "%s:\t", ptr->opt_str);

	len = sizeof(val);
	ret = getsockopt(fd, ptr->opt_level, ptr->opt_name, &val, &len);
	if (ret < 0) {
		LOG("Operation not supported!\n");
	}
	else {
		fprintf(fp, "%s = %s\n", prompt, (*ptr->opt_val_str)(&val, len));
	}
}

// 标志, on-off 
static char *sock_str_flag(union val *ptr, int len) {
	static char res[128];
	if (len != sizeof(int)) {
		snprintf(res, sizeof(res), "size (%d) not sizeof(int)", len);
	}
	else {
		snprintf(res, sizeof(res), "%s", (ptr->i_val == 0) ? "off" : "on");
	}
	return res;
}
// 打印值大小
static char *sock_str_int(union val *ptr, int len) {
  static char res[128];
	if (len != sizeof(int) && len != sizeof(long)) {
		snprintf(res, sizeof(res), "size (%d) not sizeof(int) or sizeof(long)", len);
	}
	else if (len == sizeof(long)){
		snprintf(res, sizeof(res), "%ld", ptr->l_val);
	}
	else if (len == sizeof(int)){
		snprintf(res, sizeof(res), "%d", (int)(ptr->l_val));
	}
	return res;
}
// 打印 linger
static char *sock_str_linger(union val *ptr, int len) {
  static char res[128];
	if (len != sizeof(struct linger)) {
		snprintf(res, sizeof(res), "size (%d) not sizeof(struct linger)", len);
	}
	else {
		snprintf(res, sizeof(res), "{ l_onoff = %d, l_linger = %d }", ptr->linger_val.l_onoff, ptr->linger_val.l_linger);
	}
	return res;
}
// 打印时间
static char *sock_str_timeval(union val *ptr, int len) {
  static char res[128];
	if (len != sizeof(struct timeval)) {
		snprintf(res, sizeof(res), "size (%d) not sizeof(struct timeval)", len);
	}
	else {
		snprintf(res, sizeof(res), "{ %ld sec, %ld usec }", ptr->timeval_val.tv_sec, ptr->timeval_val.tv_usec);
	}
	return res;
}
