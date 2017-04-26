#include "common.h"

int main(int argc, char* argv[]) {
	int i;
	if (argc > 1) {
		for (i = 1; i < argc; ++i) {
			showopts(argv[i]);
		}
	}
	else {
		showopts(NULL);
	}
  return 0;
}
