#include "common.h"

char buf[4096];

int main() {
	int nr;

again:
	nr = read(STDIN_FILENO, buf, 4096);
	if (nr < 0) {
		if (errno == EINTR) goto again;
		ERR_EXIT("read");
	}
	toUpper(buf, nr);
	write(STDOUT_FILENO, buf, nr);
	return 0;
}
