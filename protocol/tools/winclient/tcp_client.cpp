//#include "stdafx.h"

#include <winsock2.h>
#include <stdio.h>
#pragma comment(lib,"WS2_32.lib")
int main(int argc, char* argv[])
{
	if (argc < 3) return 1;

	WSADATA wsd;
	SOCKET sockClient; 
	SOCKADDR_IN addrSrv;
	char recvBuf[64];
	char sendBuf[64];
	if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0)
	{
		printf("start up failed!\n");
		return 1;
	}
	sockClient = socket(AF_INET, SOCK_STREAM, 0);
	addrSrv.sin_addr.S_un.S_addr = inet_addr(argv[1]);
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(atoi(argv[2]));
	connect(sockClient, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));

	while (1) {
		scanf("%s", sendBuf);
		if (sendBuf[0] == 'q') break;
		send(sockClient, sendBuf, strlen(sendBuf) + 1, 0);
		recv(sockClient, recvBuf, 64, 0);
		printf("%s\n", recvBuf);
	}
	closesocket(sockClient);
	WSACleanup();
	return 0;
}
