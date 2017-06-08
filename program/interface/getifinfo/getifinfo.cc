#include "common.h"

int main() {
	int count, i;
	struct ifi_info *ifi;	

	count = getIfiInfo(&ifi);
	if (count < 0) {
		ERR_PRINT("no device found!\n");
		return 1;
	}

	for (i = 0; i < count; ++i) {
		printf("%s:", ifi[i].ifi_name);
		printf(" <");
		if (ifi[i].ifi_flags & IFF_UP) printf("UP");
		if (ifi[i].ifi_flags & IFF_BROADCAST) printf(",BROADCAST");
		if (ifi[i].ifi_flags & IFF_RUNNING) printf(",RUNNING");
		if (ifi[i].ifi_flags & IFF_MULTICAST) printf(",MULTICAST");
		if (ifi[i].ifi_flags & IFF_LOOPBACK) printf(",LOOPBACK");
		if (ifi[i].ifi_flags & IFF_POINTOPOINT) printf(",P2P");
		printf(">");
		printf(" IF %d ", ifi[i].ifi_index);
		printf(" mtu %d\n", ifi[i].ifi_mtu);
		printf("\tmacaddr %02x:%02x:%02x:%02x:%02x:%02x\n", 
				ifi[i].ifi_haddr[0], ifi[i].ifi_haddr[1], ifi[i].ifi_haddr[2],
				ifi[i].ifi_haddr[3], ifi[i].ifi_haddr[4], ifi[i].ifi_haddr[5]);


		printf("\tinet %s", 
				inet_ntoa(((struct sockaddr_in*)ifi[i].ifi_addr)->sin_addr));
		printf(" netmask %s", 
				inet_ntoa(((struct sockaddr_in*)ifi[i].ifi_netmask)->sin_addr));

		if (ifi[i].ifi_brdaddr) {
			printf(" broadcast %s", 
					inet_ntoa(((struct sockaddr_in*)ifi[i].ifi_brdaddr)->sin_addr));
		}
		if (ifi[i].ifi_dstaddr) {
			printf(" destaddr %s", 
					inet_ntoa(((struct sockaddr_in*)ifi[i].ifi_dstaddr)->sin_addr));
		}
		printf("\n\n");
	}

	freeIfiInfo(ifi, count);
}

