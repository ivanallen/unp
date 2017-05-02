#include "common.h"

int r, w;
char filename[256];

void read_routine();
void write_routine();

int main(int argc, char* argv[]) {
	Args args = parsecmdline(argc, argv);
	if (args.empty()) {
		printf("Usage:\n  %s <-r filename | -w filename>\n", argv[0]);
		return 1;
	}

	SETBOOL(args, r, "r", 0);
	SETBOOL(args, w, "w", 0);

	if (r) {
		SETSTR(args, filename, "r", "input");
		read_routine();
	}

	if (w) {
		SETSTR(args, filename, "w", "output");
		write_routine();
	}

	return 0;
}

void read_routine() {
	int i, fd, nw;
	struct iovec iov[10];
	char buf[64] = { 0 };
	
	for (i = 0; i < 63; ++i) buf[i] = '-'; 

	for (i = 0; i < 10; ++i) {
    iov[i].iov_base = buf + 2*i;
		iov[i].iov_len = 1;
	}

	fd = open(filename, O_RDONLY);
	if (fd < 0) ERR_EXIT("open");

	nw = readv(fd, iov, 10);
	if (nw < 0) ERR_EXIT("readv");

	puts(buf);
}

void write_routine() {
	int i, fd, nw;
	struct iovec iov[10];
	char buf[64];

	strcpy(buf, "abcdefghijklmnopqrstuvwxyz");

	for (i = 0; i < 10; ++i) {
    iov[i].iov_base = buf + 2*i;
		iov[i].iov_len = 1;
	}

	fd = open(filename, O_WRONLY | O_CREAT, 0666);
	if (fd < 0) ERR_EXIT("open");

	nw = writev(fd, iov, 10);
	if (nw < 0) ERR_EXIT("writev");
}
