#include "common.h"

#define BUF_SIZE 4096
char buf[BUF_SIZE];


int main(int argc, char* argv[]) {
	struct sockaddr_in from;
	struct ip *ip;
	socklen_t len;
	int nr, ret, sockfd, protocol;

	protocol = IPPROTO_ICMP;

	if (argc > 1) {
		// 由命令行传入，表示进程想接收什么协议的数据//
		// 也就是说，希望接收 ip 首部中的协议号为 protocol 的协议
		protocol = atoi(argv[1]);
	}


	sockfd = socket(AF_INET, SOCK_RAW, protocol);
	if (sockfd < 0) ERR_EXIT("socket");

	for (;;) {
		len = sizeof(from);
		nr = recvfrom(sockfd, buf, BUF_SIZE, 0, (struct sockaddr*)&from, &len);
		if (nr < 0) {
			ERR_EXIT("recvfrom");
		}

		WARNING("from: %s\n", inet_ntoa(from.sin_addr));

		// 打印 ip 首部和数据部分
		// buf 是 ip 首部开始第一个字节的地址
	  ip = (struct ip*)buf;	
		printIp(ip, nr);
	}
}


