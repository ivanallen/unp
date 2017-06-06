#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
	int i, ifindex;
	char interface[IF_NAMESIZE + 1];

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <interface index 1> [interface index 2] [...]\n", argv[0]);
		return 1;
	}

	for (i = 1; i < argc; ++i) {
		ifindex = atoi(argv[i]);
		if (if_indextoname(ifindex, interface)) {
			printf("%d : %s\n", ifindex, interface);
		}
		else {
			fprintf(stderr, "%s: ", argv[i]);
			perror("");
		}
	}
	return 0;
}
