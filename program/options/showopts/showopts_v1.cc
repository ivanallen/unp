#include "common.h"

int main(int argc, char* argv[]) {
	int i, sockfd;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (argc > 1) {
		for (i = 1; i < argc; ++i) {
			showopts(sockfd, argv[i]);
		}
	}
	else {
		showopts(sockfd, NULL);
	}
	close(sockfd);
  return 0;
}
