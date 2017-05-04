#include "common.h"

int main(int argc, char* argv[]) {
	int sockfd, mode, fd, ret;
	char c;
	char *pathname;

	if (argc < 4) {
		ERR_QUIT("usage: <sockfd> <pathname> <mode>\n");
	}

	sockfd = atoi(argv[1]);
	pathname = argv[2];
	mode = atoi(argv[3]);

	fd = open(pathname, mode);
	if (fd < 0) ERR_EXIT("open");

	printf("openfile: fd = %d\n", fd);

	c = 'x';
	ret = writefd(sockfd, &c, 1, fd);
	if (ret < 0) {
		exit((errno > 0) ? errno : 255);
	}

	return 0;
}
