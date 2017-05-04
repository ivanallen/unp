#include "common.h"

int main(int argc, char* argv[]) {
  int sockfd, ret;
	char *pathname;
	struct sockaddr_un addr1, addr2;
	socklen_t len;

	pathname = NULL;
	if (argc >= 2) {
		pathname = argv[1];
	}

	sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);
	if (sockfd < 0) ERR_EXIT("socket");

	unlink(pathname);

	resolve(pathname, &addr1);

  printf("size of sockaddr_un = %d\n", sizeof addr1);
	
	ret = bind(sockfd, (struct sockaddr*)&addr1, sizeof(addr1));
	if (ret < 0) ERR_EXIT("bind");

	len = sizeof(addr2);
	ret = getsockname(sockfd, (struct sockaddr*)&addr2, &len);

	if (ret < 0) ERR_EXIT("getsockname");

	printf("bound name = %s, returned len = %d\n", addr2.sun_path, len);

	return 0;
}
