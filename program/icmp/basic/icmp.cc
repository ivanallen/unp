#include "common.h"

#define BUF_SIZE 4096
char buf[BUF_SIZE];

int showiphdr = 0;

int main(int argc, char* argv[]) {
	struct sockaddr_in from;
	struct ip *ip;
	struct icmp *icmp;
	socklen_t len;
	int nr, ret, sockfd, protocol, iphlen, icmplen;

	Args args = parsecmdline(argc, argv);

	SETBOOL(args, showiphdr, "showip", 0);

	sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sockfd < 0) ERR_EXIT("socket");

	for (;;) {
		len = sizeof(from);
		nr = recvfrom(sockfd, buf, BUF_SIZE, 0, (struct sockaddr*)&from, &len);
		if (nr < 0) {
			ERR_EXIT("recvfrom");
		}

		ERR_PRINT("---------- from: %s ----------\n", inet_ntoa(from.sin_addr));

		// 打印 ip 首部和数据部分
		// buf 是 ip 首部开始第一个字节的地址
	  ip = (struct ip*)buf;	

		if (showiphdr)
			printIp(ip, nr);

		// ip 头部长度
		iphlen = (ip->ip_hl) << 2;

		// 拿到 icmp 首地址
		icmp = (struct icmp*)((char*)ip + iphlen);
		// icmp 报文长度
		icmplen = nr - iphlen;
		printIcmp(icmp, icmplen);
	}
}



