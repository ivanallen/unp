#include "common.h"

#define BUF_SIZE 4096
#define ICMP_SIZE 64

char *hostname; // 主机名或 ip
char recvbuf[BUF_SIZE]; // 接收缓冲区
char sendbuf[ICMP_SIZE]; // 发送缓冲区

void run(); // ping 主程序
void handler(int sig); // 信号处理函数
int nsend, nrecv; // 用于统计
int64_t start;

int main(int argc, char* argv[]) {
	if (argc < 2) {
		ERR_QUIT("Usage: %s <hostname or ip>\n", argv[0]);
	}
	hostname = argv[1];
	registSignal(SIGALRM, handler);
	registSignal(SIGINT, handler);
	run();
}

void run() {
	struct ip *ip; // ip 首部
	struct icmp_echo *icmp_echo, *icmp_echo_reply; // 请求与应答
	struct sockaddr_in to; // 目标主机地址
	int i, len, sockfd, ret, nr;
	int64_t timerecv, timesend;
	double rtt;

	nsend = 0; // 已经发送包的个数
	nrecv = 0; // 已经接收包的个数

	// 随机初始化
	for (i = 0; i < ICMP_SIZE; ++i)
		sendbuf[i] = "abcdefghijklmnopqrstuvwxyz"[i%26];

	// 构造套接字地址，端口并没有什么用，随便填
	ret = resolve(hostname, 0, &to);
	if (ret < 0) ERR_EXIT("resolve");


	// 创建原始套接字, 只接收承载 ICMP 协议的数据报
	sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sockfd < 0) ERR_EXIT("socket");

	// 打印信息
	WARNING("PING %s (%s) %d bytes of data.\n", hostname, inet_ntoa(to.sin_addr), ICMP_SIZE);

	icmp_echo = (struct icmp_echo*)sendbuf;
	ip = (struct ip*)recvbuf;

	// 用于时间统计
	start = now();

	// 填充 icmp 回显请求报文, 这三个字段以后都不用再改，放在循环外面
	icmp_echo->icmp_type = 8;
	icmp_echo->icmp_code = 0;
	icmp_echo->icmp_id = getpid() & 0xffff;

	while(1) {
		icmp_echo->icmp_cksum = 0;
		icmp_echo->icmp_seq = nsend + 1; // 序号, 起始序号为 1
		*((int64_t*)icmp_echo->icmp_data) = now(); // 当前时间，微秒
		icmp_echo->icmp_cksum = cksum((unsigned short*)sendbuf, ICMP_SIZE); // 指定大小为 64

		// 发送数据
		ret = sendto(sockfd, sendbuf, ICMP_SIZE, 0, (struct sockaddr*)&to, sizeof(to));
		if (ret < 0) {
			if (errno == EINTR) continue;
			ERR_EXIT("sendto");
		}
		++nsend; // 序号加 1

again:
		// 接收 ip 数据报，超时时间设置为 5 秒。
		alarm(5);
		nr = recvfrom(sockfd, recvbuf, BUF_SIZE, 0, NULL, NULL);
		if (nr < 0) {
			if (errno == EINTR) {
				ERR_PRINT("TIMEDOUT.\n");
				continue; // 超时，丢包
			}
			ERR_EXIT("recvfrom");
		}

		// 从 ip 数据报里拿到 icmp 报文
		icmp_echo_reply = (struct icmp_echo*)((char*)ip + (ip->ip_hl << 2));
		if (icmp_echo_reply->icmp_type != 0 || icmp_echo_reply->icmp_code != 0
				|| icmp_echo_reply->icmp_id != (getpid() & 0xffff)) {
			// 收到的不是我们想要的包，重来继续收，不能使用 continue 是因为我们不能判定包就丢失了
			goto again;
		}
		// 执行到这里说明收到了自己的包，将接收个数递增
		++nrecv;
		// 计算往返时间
	  timerecv = now();	
		timesend = *((int64_t*)icmp_echo_reply->icmp_data);
		rtt = (timerecv - timesend) / 1000.0;
		LOG("%d bytes from %s (%s): icmp_seq=%d ttl=%d time=%.1f ms\n", 
				ICMP_SIZE, hostname, inet_ntoa(to.sin_addr), icmp_echo_reply->icmp_seq, ip->ip_ttl, rtt);
		sleep(1);
	}
}

void handler(int sig) {
	int64_t end; 
	if (sig == SIGINT) {
		end = now();
		WARNING("\n--- %s ping statistics ---\n", hostname);
		WARNING("%d packets transmitted, %d received, %d%% packet loss, time %dms\n",
				nsend, nrecv, (nsend - nrecv) * 100 / nsend, (int)(end - start) / 1000);
		exit(0);
	}
}
