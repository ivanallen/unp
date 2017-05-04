#include "common.h"


// please man 3 cmsg

void showsize(int size);
void showdata(int count);
void printrawdata(char* buf, int n);

int main(int argc, char* argv[]) {
	int ret, size, datasize;
	Args args = parsecmdline(argc, argv);
	if (CONTAINS(args, "h")) {
		ERR_QUIT("usage: %s [-h] [--showsize size] [--showdata datasize]\n");
	}

	
	SETINT(args, size, "showsize", -1);
	SETINT(args, datasize, "showdata", -1);

	if (size != -1) showsize(size);
	if (datasize != -1) showdata(datasize);
	
	return 0;
}

void showsize(int size) {
	int i;

	printf("size cmsghdr = %d\n", sizeof(struct cmsghdr));

	for (i = 0; i < size; ++i) {
		printf("CMSG_ALIGN(%d) = %d, CMSG_SPACE(%d) = %d, CMSG_LEN(%d) = %d\n", 
				i, CMSG_ALIGN(i), i, CMSG_SPACE(i), i, CMSG_LEN(i));
	}
}

void showdata(int count) {
	int i, controllen;
	struct msghdr msg;
	struct cmsghdr *cmptr;
	union control{
		struct cmsghdr cm;
		char control[CMSG_SPACE(sizeof(int))];
	};
	union control *control_un;
	
	controllen = count * sizeof(union control);
	control_un = (union control*)malloc(controllen);;

	for (i = 0; i < count; ++i) {
		control_un[i].cm.cmsg_len = CMSG_LEN(sizeof(int));
		control_un[i].cm.cmsg_level = i;
		control_un[i].cm.cmsg_type = i;
		*((int*) CMSG_DATA(&control_un[i].cm)) = 2*i + 1;
	}

	printf("controllen = %d\n", controllen);

	msg.msg_control = control_un;
	msg.msg_controllen = controllen;

	for (cmptr = CMSG_FIRSTHDR(&msg); cmptr != NULL; cmptr = CMSG_NXTHDR(&msg, cmptr)) {
		printf("cmsg_len = %d, cmsg_level = %d, cmsg_type = %d, data = %d\n"
				, cmptr->cmsg_len, cmptr->cmsg_level, cmptr->cmsg_type, *((int*) CMSG_DATA(cmptr)));
	}

	printrawdata((char*)control_un, controllen);
}

void printrawdata(char* buf, int n) {
	int i;
	for (i = 0; i < n; ++i) {
		if (i % 8 == 0) {
			printf("%02x", buf[i]);
		}
		else 
			printf(" %02x", buf[i]);
		if ((i + 1) % 8 == 0) puts("");
	}
}
