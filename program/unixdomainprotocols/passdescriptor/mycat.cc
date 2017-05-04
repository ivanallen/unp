#include "common.h"

char buf[4096];

int main(int argc, char* argv[]) {
	int nr, fd;

	if (argc < 2) {
		fprintf(stderr, "usage: %s <filename>\n", argv[0]);
		return 1;
	}

	fd = myOpen(argv[1], O_RDONLY);
	if (fd < 0) ERR_EXIT("myOpen");

	printf("mycat: fd = %d\n", fd);
	puts("content:");

	while(1) {
		nr = iread(fd, buf, 4096);
		if (nr == 0) break;
		if (nr < 0) ERR_EXIT("read");
		iwrite(STDOUT_FILENO, buf, nr);	
	}
	return 0;
}
