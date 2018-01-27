// client.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"


#include <winsock2.h>
#include <stdio.h>
#pragma comment(lib,"WS2_32.lib")

int so_linger = 0;

void setopt(int sockfd) {
	int ret;
	if (so_linger) {
		struct linger slinger;
		slinger.l_onoff = 1; // 关闭连接时发送 RST 段
		slinger.l_linger = 0;// 不等待
		ret = setsockopt(sockfd, SOL_SOCKET, SO_LINGER, (const char*)&slinger, sizeof(slinger));
		if (ret < 0) {
			printf("set SO_LINGER failed\n");
			return;
		}
	}
}


int main(int argc, char* argv[])
{
	if (argc < 3) {
		printf("Usage: %s <ip> <port> [SO_LINGER]\n", argv[0]);
		return 1;
	}
	if (argc >= 4) {
		if (!strcmp("SO_LINGER", argv[3])) {
			so_linger = 1;
		}
	}

	WSADATA wsd;
	SOCKET sockClient;  //客户端socket
	SOCKADDR_IN addrSrv;
	int n;
	char recvBuf[64];
	char sendBuf[64];
	if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0)
	{
		printf("start up failed!\n");
		return 1;
	}
	sockClient = socket(AF_INET, SOCK_STREAM, 0); //创建socket
	setopt(sockClient);

	addrSrv.sin_addr.S_un.S_addr = inet_addr(argv[1]);
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(atoi(argv[2]));
	if (connect(sockClient, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR)) != 0) { //连接服务器端
		printf("connect error");
		return 1;
	}

	puts("connect success");

	while (1) {
		scanf("%s", sendBuf);
		if (sendBuf[0] == 'q') break;
		send(sockClient, sendBuf, strlen(sendBuf) + 1, 0); //向服务器端发送数据
		n = recv(sockClient, recvBuf, 64, 0); //接收服务器端数据
		if (n < 0) {
			printf("read error, %d\n", WSAGetLastError());
			return 1;
		}
		printf("%s\n", recvBuf);
	}
	closesocket(sockClient); //关闭连接
	WSACleanup();
	return 0;
}
