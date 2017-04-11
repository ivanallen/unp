// tcp_server.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <winsock2.h>
#include <stdio.h>
#pragma comment(lib,"WS2_32.lib")




int main(int argc, char* argv[])
{
	if (argc < 3) {
		printf("Usage: %s <ip> <port> [SO_LINGER]\n", argv[0]);
		return 1;
	}

	WSADATA wsd;
	SOCKET listenfd, sockClient;
	SOCKADDR_IN addrSrv, addrCli;
	int addrlen,n;
	char buf[64];


	if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0)
	{
		printf("start up failed!\n");
		return 1;
	}
	listenfd = socket(AF_INET, SOCK_STREAM, 0); //创建socket

	addrSrv.sin_addr.S_un.S_addr = inet_addr(argv[1]);
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(atoi(argv[2]));

	bind(listenfd, (sockaddr*)&addrSrv, sizeof(addrSrv));

	listen(listenfd, 5);

	addrlen = sizeof(addrCli);
	sockClient = accept(listenfd, (sockaddr*)&addrCli, &addrlen);

	printf("%s:%d come in\n", inet_ntoa(addrCli.sin_addr), ntohs(addrCli.sin_port));

	while (1) {
		n = recv(sockClient, buf, 64, 0); //接收客户端数据
		if (n == 0) break;
		if (n < 0) {
			puts("read error");
			break;
		}

		send(sockClient, buf, n, 0); // 发送回去
	}
	closesocket(sockClient); //关闭连接
	closesocket(listenfd);
	WSACleanup();
	return 0;
}

