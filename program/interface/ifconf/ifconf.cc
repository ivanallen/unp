#include "common.h"

int main() {
	int count, i;
	struct ifreq *ifr;	

	count = getIfConf(&ifr);
	if (count < 0) {
		ERR_PRINT("no device found!\n");
		return 1;
	}

	for (i = 0; i < count; ++i) {
    printf("name: %s, addr: %s\n",
				ifr[i].ifr_name,
				inet_ntoa(((struct sockaddr_in*)&ifr[i].ifr_addr)->sin_addr));
	}

	freeIfConf(ifr);
	return 0;
}

