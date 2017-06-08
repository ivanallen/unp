#include <unistd.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void ifname_routine(const char* name);
void ifindex_routine(int ifindex);

int main(int argc, char* argv[]) {
	if (argc < 1) {
		fprintf(stderr, "Usage: %s {[-i <ifindex>] | ifname}\n", argv[0]);
		return 1;
	}

	if (!strcmp(argv[1], "-i")) {
		ifindex_routine(atoi(argv[2]));
	}
	else {
		ifname_routine(argv[1]);
	}
}

void ifname_routine(const char* name) {
	struct ifreq ifr;
	int ret, sockfd, i;

	strcpy(ifr.ifr_name, name);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("socket");
		return;
	}
	ret = ioctl(sockfd, SIOCGIFADDR, &ifr);
	if (ret < 0) {
		perror("ioctl");
		return;
	}
	printf("ifr_addr: %s\n", inet_ntoa(((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr));
	ret = ioctl(sockfd, SIOCGIFDSTADDR, &ifr);
	if (ret < 0) {
		perror("ioctl");
		return;
	}
	printf("ifr_dstaddr: %s\n", inet_ntoa(((struct sockaddr_in*)&ifr.ifr_dstaddr)->sin_addr));
	ret = ioctl(sockfd, SIOCGIFBRDADDR, &ifr);
	if (ret < 0) {
		perror("ioctl");
		return;
	}
	printf("ifr_broadaddr: %s\n", inet_ntoa(((struct sockaddr_in*)&ifr.ifr_broadaddr)->sin_addr));
	ret = ioctl(sockfd, SIOCGIFNETMASK, &ifr);
	if (ret < 0) {
		perror("ioctl");
		return;
	}
	printf("ifr_netmask: %s\n", inet_ntoa(((struct sockaddr_in*)&ifr.ifr_netmask)->sin_addr));
	ret = ioctl(sockfd, SIOCGIFHWADDR, &ifr);
	if (ret < 0) {
		perror("ioctl");
		return;
	}
	printf("ifr_hwaddr: ");
	for (i = 0; i < 6; ++i) {
		if (i != 5)
			printf("%02x:", (unsigned char)ifr.ifr_hwaddr.sa_data[i]);
		else
			printf("%02x", (unsigned char)ifr.ifr_hwaddr.sa_data[i]);
	}
	puts("");

	close(sockfd);
}

void ifindex_routine(int ifindex) {
}
