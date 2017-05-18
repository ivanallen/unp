#include "common.h"

#define BUF_SIZE 4096
char recvbuf[BUF_SIZE];
char sendbuf[BUF_SIZE];
const char *strings[2] = {"hello\n", "I'm icmp echo\n"};

int showiphdr = 0;
int showicmphdr = 0;
char hostname[64];
struct sockaddr_in to;

void recver();
void sender();
void handler(int sig);

int main(int argc, char* argv[]) {
	int ret;
	Args args = parsecmdline(argc, argv);

	if (args.empty() || CONTAINS(args, "help") || !CONTAINS(args, "h")) {
		ERR_QUIT("Usage: %s [--help] [--showip] [--showicmp] <-h hostname>\n", argv[0]);
	}

	SETBOOL(args, showiphdr, "showip", 0);
	SETBOOL(args, showicmphdr, "showicmp", 0);
	SETSTR(args, hostname, "h", "0");

	ret = resolve(hostname, 0, &to);
	if (ret < 0) ERR_EXIT("resolve");

	ERR_PRINT("pid = %d\n", getpid());

	registSignal(SIGALRM, handler);

	alarm(1);

	recver();
}

void handler(int sig) {
	if (sig == SIGALRM) {
		sender();
		alarm(1);
	}
}


void recver() {
	struct sockaddr_in from;
	struct ip *ip;
	struct icmp *icmp;
	struct icmp_echo *icmp_echo;
	socklen_t len;
	int nr, ret, sockfd, protocol, iphlen, icmplen;
	char *data;

	sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sockfd < 0) ERR_EXIT("socket");

	for (;;) {
		len = sizeof(from);
		nr = recvfrom(sockfd, recvbuf, BUF_SIZE, 0, (struct sockaddr*)&from, &len);
		if (nr < 0) {
			if (errno == EINTR) continue;
			ERR_EXIT("recvfrom");
		}

		ERR_PRINT("---------- from: %s ----------\n", inet_ntoa(from.sin_addr));

		// 打印 ip 首部和数据部分
		// buf 是 ip 首部开始第一个字节的地址
	  ip = (struct ip*)recvbuf;	

		if (showiphdr)
			printIp(ip, nr);

		// ip 头部长度
		iphlen = (ip->ip_hl) << 2;

		// 拿到 icmp 首地址
		icmp = (struct icmp*)((char*)ip + iphlen);
		// icmp 报文长度
		icmplen = nr - iphlen;

		if (showicmphdr)
			printIcmp(icmp, icmplen);

		// 判断报文类型
		if (icmp->icmp_type == 0 && icmp->icmp_code == 0) {
			icmp_echo = (struct icmp_echo*)icmp;
			WARNING("id:              %d\n", icmp_echo->icmp_id);
			WARNING("seq:             %d\n", icmp_echo->icmp_seq);
			// 打印到屏幕
			write(STDOUT_FILENO, icmp_echo->icmp_data, icmplen - sizeof(struct icmp_echo)); 
		}
	}
}

void sender() {
	struct icmp_echo *icmp_echo;
	int len, sockfd, ret, icmplen;
  static int i = 0;        

	icmp_echo = (struct icmp_echo*)sendbuf;
  // 计算字符串长度
	len = strlen(strings[i]);
	icmplen = sizeof(struct icmp_echo) + len;

	// 填充 icmp 回显报文
	icmp_echo->icmp_type = 8;
	icmp_echo->icmp_code = 0;
	icmp_echo->icmp_cksum = 0;
	icmp_echo->icmp_id = getpid() & 0xffff;
	icmp_echo->icmp_seq = i;
	strcpy(icmp_echo->icmp_data, strings[i]);

	icmp_echo->icmp_cksum = cksum((unsigned short*)sendbuf, icmplen);
	


	sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sockfd < 0) ERR_EXIT("socket");

	ret = sendto(sockfd, sendbuf, icmplen, 0, (struct sockaddr*)&to, sizeof(to));
	if (ret < 0) ERR_EXIT("sendto");


	i = (i + 1) % 2;
}
