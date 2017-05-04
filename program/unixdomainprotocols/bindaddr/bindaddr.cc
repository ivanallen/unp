#include "common.h"

int main(int argc, char* argv[]) {
  int sockfd, ret, abstract;
	char pathname[PATH_MAX];
	struct sockaddr_un addr1, addr2;
	socklen_t len;

	Args args = parsecmdline(argc, argv);
	if (CONTAINS(args, "h")) {
		ERR_QUIT("usage: %s <--path pathname> [--abstract]\n");
	}

	SETSTR(args, pathname, "path", "dog");
	SETBOOL(args, abstract, "abstract", 0);

	sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);
	if (sockfd < 0) ERR_EXIT("socket");

	// 初学的时候可以无视 abstract 这些东西，就当作 abstract == 0
	if (!abstract)
		unlink(pathname);

	resolve(pathname, &addr1, &len, abstract);

  printf("size of sockaddr_un = %d\n", len);
	
	ret = bind(sockfd, (struct sockaddr*)&addr1, len);
	if (ret < 0) ERR_EXIT("bind");

	len = sizeof(addr2);
	ret = getsockname(sockfd, (struct sockaddr*)&addr2, &len);

	if (abstract) addr2.sun_path[0] = '@';
	if (ret < 0) ERR_EXIT("getsockname");

	printf("bound name = %s, returned len = %d\n", addr2.sun_path, len);

	return 0;
}
