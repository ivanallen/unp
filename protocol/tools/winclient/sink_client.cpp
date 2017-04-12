// sink_client.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <winsock2.h>
#include <stdio.h>
#include <conio.h>
#pragma comment(lib,"WS2_32.lib")

#define MAX_SIZE 1024
int count = 1;
char data[MAX_SIZE];

int main(int argc, char* argv[])
{
	if (argc < 3) {
		printf("Usage: %s <ip> <port> [count]\n", argv[0]);
		return 1;
	}
	if (argc >= 4) {
		count = atoi(argv[3]);
	}


	WSADATA wsd;
	SOCKET sockClient;  //客户端socket
	SOCKADDR_IN addrSrv;
	int i;

	for (i = 0; i < MAX_SIZE; ++i) {
		data[i] = 'a' + (i % 26);
	}



	if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0)
	{
		printf("start up failed!\n");
		return 1;
	}
	sockClient = socket(AF_INET, SOCK_STREAM, 0); //创建socket



	addrSrv.sin_addr.S_un.S_addr = inet_addr(argv[1]);
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(atoi(argv[2]));
	if (connect(sockClient, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR)) != 0) { //连接服务器端
		printf("connect error: %d", WSAGetLastError());
		return 1;
	}

	puts("connect success");

	while (count -- ) {
		send(sockClient, data, MAX_SIZE, 0); //向服务器端发送数据，连续发送 count 次。		
	}
	closesocket(sockClient); //关闭连接
	WSACleanup();
	return 0;
}




