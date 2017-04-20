// psh_client.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"


#include <winsock2.h>
#include <stdio.h>
#include <conio.h>
#pragma comment(lib,"WS2_32.lib")

int MAX_SIZE;
int count = 1;
char *data = data;
int blocksize = 1024;

int main(int argc, char* argv[])
{
	if (argc < 3) {
		printf("Usage: %s <ip> <port> [count] [block size (< 8192)]\n", argv[0]);
		return 1;
	}
	if (argc >= 4) {
		count = atoi(argv[3]);
	}
	if (argc >= 5) {
		blocksize = atoi(argv[4]);
		MAX_SIZE = blocksize;
		data = (char*)malloc(MAX_SIZE);
	}


	WSADATA wsd;
	SOCKET sockClient;  //客户端socket
	SOCKADDR_IN addrSrv;
	int n, i, sendbufsize, ret;

	for (i = 0; i < MAX_SIZE; ++i) {
		data[i] = 'a' + (i % 26);
	}



	if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0)
	{
		printf("start up failed!\n");
		return 1;
	}
	sockClient = socket(AF_INET, SOCK_STREAM, 0); //创建socket

	n = sizeof(sendbufsize);
	ret = getsockopt(sockClient, SOL_SOCKET, SO_SNDBUF, (char*)&sendbufsize, &n);
	if (ret < 0) {
		puts("getsockopt failed");
		return 1;
	}
	printf("actual sendbufsize: %d\n", sendbufsize);



	addrSrv.sin_addr.S_un.S_addr = inet_addr(argv[1]);
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(atoi(argv[2]));
	if (connect(sockClient, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR)) != 0) { //连接服务器端
		printf("connect error: %d", WSAGetLastError());
		return 1;
	}

	puts("connect success");

	while (count--) {
		send(sockClient, data, blocksize, 0); //向服务器端发送数据，连续发送 count 次。
		Sleep(10); // 10ms
	}
	closesocket(sockClient); //关闭连接
	WSACleanup();
	free(data);
	return 0;
}

