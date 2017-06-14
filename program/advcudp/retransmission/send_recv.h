#ifndef __SEND_RECV_H__
#define __SEND_RECV_H__

struct echo_head {
	int seq; // 序号
	int ts; // 时间戳
};

int send_recv(int sockfd, char* outbuf, int outlen, 
		char* inbuf, int inlen, struct sockaddr* dstaddr, socklen_t addrlen);

#endif //__SEND_RECV_H__

