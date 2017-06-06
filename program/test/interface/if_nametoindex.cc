#include <net/if.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
	int i, ifindex;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <interface name 1> [interface name 2] [...]\n", argv[0]);
		return 1;
	}

	for (i = 1; i < argc; ++i) {
		ifindex = if_nametoindex(argv[i]);
		if (ifindex == 0) {
			fprintf(stderr, "%s: ", argv[i]);
			perror("");
		}
		else
			printf("%s: %u\n", argv[i], ifindex);
	}
	return 0;
}
