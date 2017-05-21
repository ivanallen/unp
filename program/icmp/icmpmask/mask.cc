#include "common.h"

#define BUF_SIZE 4096
#define ICMP_SIZE (sizeof(struct icmp_mask)) 

char *hostname; // 主机名或 ip
char recvbuf[BUF_SIZE]; // 接收缓冲区
char sendbuf[ICMP_SIZE]; // 发送缓冲区

void run(); // ping 主程序
void handler(int sig); // 信号处理函数

int main(int argc, char* argv[]) {
	if (argc < 2) {
		ERR_QUIT("Usage: %s <hostname or ip>\n", argv[0]);
	}
	hostname = argv[1];
	registSignal(SIGALRM, handler);
	run();
}

void run() {
	struct ip *ip; // ip 首部
	struct icmp_mask *icmp_mask, *icmp_mask_reply; // 请求与应答
	struct sockaddr_in to; // 目标主机地址
	int i, len, sockfd, ret, nr, onoff;

	// 构造套接字地址，端口并没有什么用，随便填
	ret = resolve(hostname, 0, &to);
	if (ret < 0) ERR_EXIT("resolve");

	// 创建原始套接字, 只接收承载 ICMP 协议的数据报
	sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sockfd < 0) ERR_EXIT("socket");

	onoff = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &onoff, sizeof(onoff));

	icmp_mask = (struct icmp_mask*)sendbuf;
	ip = (struct ip*)recvbuf;

	// 填充 icmp 回显请求报文, 这三个字段以后都不用再改，放在循环外面
	icmp_mask->icmp_type = 17;
	icmp_mask->icmp_code = 0;
	icmp_mask->icmp_id = getpid() & 0xffff;
	icmp_mask->icmp_cksum = 0;
	icmp_mask->icmp_seq = 0; // 序号, 起始序号为 1
	icmp_mask->icmp_mask.s_addr = 0;
	icmp_mask->icmp_cksum = cksum((unsigned short*)sendbuf, ICMP_SIZE); // 指定大小为 64

	// 发送数据
again:
	ret = sendto(sockfd, sendbuf, ICMP_SIZE, 0, (struct sockaddr*)&to, sizeof(to));
	if (ret < 0) {
		if (errno == EINTR) goto again;
		ERR_EXIT("sendto");
	}

	while(1) {
		// 接收 ip 数据报，超时时间设置为 5 秒，5 秒还不收不到基本上可以认为收不到了。
		alarm(5);
		nr = recvfrom(sockfd, recvbuf, BUF_SIZE, 0, NULL, NULL);
		if (nr < 0) {
			if (errno == EINTR) {
				ERR_PRINT("no reply!\n");
				break;
			}
			ERR_EXIT("recvfrom");
		}
		// 从 ip 数据报里拿到 icmp 报文
		icmp_mask_reply = (struct icmp_mask*)((char*)ip + (ip->ip_hl << 2));
		if (icmp_mask_reply->icmp_type != 18 || icmp_mask_reply->icmp_code != 0
				|| icmp_mask_reply->icmp_id != (getpid() & 0xffff)) {
			// 收到的不是我们想要的包，重来继续收
			continue;
		}
		// 执行到这里说明收到了自己的包，将接收个数递增
		LOG("received mask = %08x, from %s\n", icmp_mask_reply->icmp_mask.s_addr, inet_ntoa(ip->ip_src));
	}
}

void handler(int sig) {
}
