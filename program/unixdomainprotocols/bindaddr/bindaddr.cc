#include "common.h"

int main(int argc, char* argv[]) {
  int sockfd, ret;
	struct sockaddr_un addr1, addr2;
	socklen_t len;

	if (argc < 2) {
		printf("usage: %s <pathname>\n", argv[0]);
		return 1;
	}

	sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);
	if (sockfd < 0) ERR_EXIT("socket");

	unlink(argv[1]);

	bzero(&addr1, sizeof(addr1));
	addr1.sun_family = AF_LOCAL;
	strncpy(addr1.sun_path, argv[1], sizeof(addr1.sun_path) - 1);
	
	ret = bind(sockfd, (struct sockaddr*)&addr1, sizeof(addr1));
	if (ret < 0) ERR_EXIT("bind");

	len = sizeof(addr2);
	ret = getsockname(sockfd, (struct sockaddr*)&addr2, &len);

	if (ret < 0) ERR_EXIT("getsockname");

	printf("bound name = %s, returned len = %d\n", addr2.sun_path, len);

	return 0;
}
