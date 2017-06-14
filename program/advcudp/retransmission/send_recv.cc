#include "common.h"
#include "send_recv.h"

struct echo_head sendhdr, recvhdr;

void handler(int sig);

/*
 * sockfd: 套接字描述符
 * outbuf: 发送缓冲区
 * outlen: 发送缓冲区大小
 * inbuf: 接收缓冲区
 * inlen: 接收缓冲区大小
 * dstaddr: 目标套接字地址
 * addrlen: 套接字地址长度
 * return: 返回 0 成功，小于 0 失败
 */
int send_recv(int sockfd, char* outbuf, int outlen, 
		char* inbuf, int inlen, struct sockaddr* dstaddr, socklen_t addrlen) {

	int nr, nw, maxfd, ret, rtt_count;
	int64_t base;
	struct msghdr msgsend, msgrecv;
	struct iovec iovsend[2];
	struct iovec iovrecv[2];
	fd_set rfds;
	sigset_t sigmask, emptyset;

	
	FD_ZERO(&rfds);
	maxfd = sockfd;

	sigemptyset(&sigmask);
	sigemptyset(&emptyset);
	sigaddset(&sigmask, SIGALRM);
	// 阻塞 SIGALRM 信号
	ret = sigprocmask(SIG_BLOCK, &sigmask, NULL);
	if (ret < 0) ERR_EXIT("sigprocmask");

	registSignal(SIGALRM, handler);

	iovsend[0].iov_base = &sendhdr;
	iovsend[0].iov_len = sizeof(struct echo_head);
	iovsend[1].iov_base = outbuf;
	iovsend[1].iov_len = outlen;

	msgsend.msg_name = dstaddr;
	msgsend.msg_namelen = addrlen;
	msgsend.msg_iov = iovsend;
	msgsend.msg_iovlen = 2;
	msgsend.msg_control = 0;
	msgsend.msg_controllen = 0;
	msgsend.msg_flags = 0;

	iovrecv[0].iov_base = &recvhdr;
	iovrecv[0].iov_len = sizeof(struct echo_head);
	iovrecv[1].iov_base = inbuf;
	iovrecv[1].iov_len = inlen;

	msgrecv.msg_name = NULL;
	msgrecv.msg_namelen = 0;
	msgrecv.msg_iov = iovrecv;
	msgrecv.msg_iovlen = 2;
	msgrecv.msg_control = 0;
	msgrecv.msg_controllen = 0;
	msgrecv.msg_flags = 0;

	// 用于统计重传次数
	rtt_count = 0;
	++sendhdr.seq;
	base = now();
sendagain:
	sendhdr.ts = (now() - base)/ 1000;
	LOG("send, seq = %d, ts = %d\n", sendhdr.seq, sendhdr.ts);
	nw = sendmsg(sockfd, &msgsend, 0);
	if (nw < 0) ERR_EXIT("sendmsg");

	alarm(1); // 设置超时时间
	do {
		FD_SET(sockfd, &rfds);
		ret = pselect(maxfd + 1, &rfds, NULL, NULL, NULL, &emptyset);
		if (ret < 0) {
			// 超时重传
			if (errno == EINTR) {
				++rtt_count;
				// 重传三次不成功宣告失败。
				if (rtt_count < 5) {
					WARNING("send again: ");
					goto sendagain;
				}
				else {
					errno = EHOSTUNREACH;
					alarm(0); // 清理定时器 
					return -1;
				}
			}
			ERR_EXIT("pselect");
		}
		nr = recvmsg(sockfd, &msgrecv, 0); 
		if (nr < 0) ERR_EXIT("recvmsg");

		// 接收到的数据长度不够 or 接收的报文乱序，则重新接收
	} while(nr < sizeof(struct echo_head) || sendhdr.seq != recvhdr.seq);

	// 清理定时器
	alarm(0);

	return nr - sizeof(struct echo_head);
}


void handler(int sig) {
}
