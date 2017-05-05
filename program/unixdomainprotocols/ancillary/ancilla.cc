#include "common.h"


// please man 3 cmsg

void showsize(int size);
void showdata(int count);
void printrawdata(char* buf, int n);

int main(int argc, char* argv[]) {
	int ret, size, datacount;
	Args args = parsecmdline(argc, argv);
	if (args.empty()) {
		fprintf(stderr, "usage: %s [--showsize size] [--showdata datacount]\n", argv[0]);
		exit(0);
	}

	
	SETINT(args, size, "showsize", -1);
	SETINT(args, datacount, "showdata", -1);

	if (size != -1) showsize(size);
	if (datacount != -1) showdata(datacount);
	
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
	// 定义辅助数据对象大小，根据 CMSG_SPACE 宏计算
	// 其中，数据部分大小是 sizeof(int)，因为后面我们需要保存 4 字节长度的数据
	union control{
		struct cmsghdr cm;
		char control[CMSG_SPACE(sizeof(int))];
	};

	// control_un 将指向第一个辅助数据首地址
	union control *control_un;
	
	// 一共有 count 个辅助数据，分配空间
	controllen = count * sizeof(union control);
	control_un = (union control*)malloc(controllen);;

	// 填充所有辅助数据，你可以随便填充值
	for (i = 0; i < count; ++i) {
		control_un[i].cm.cmsg_len = CMSG_LEN(sizeof(int));
		control_un[i].cm.cmsg_level = i;
		control_un[i].cm.cmsg_type = i;
		*((int*) CMSG_DATA(&control_un[i].cm)) = 2*i + 1;
	}

	// 打印所有辅助数据总长度
	printf("controllen = %d\n", controllen);

	msg.msg_control = control_un;
	msg.msg_controllen = controllen;

	// 根据 msghdr{} 遍历所有辅助数据对象
	for (cmptr = CMSG_FIRSTHDR(&msg); cmptr != NULL; cmptr = CMSG_NXTHDR(&msg, cmptr)) {
		printf("cmsg_len = %d, cmsg_level = %d, cmsg_type = %d, data = %d\n"
				, cmptr->cmsg_len, cmptr->cmsg_level, cmptr->cmsg_type, *((int*) CMSG_DATA(cmptr)));
	}

	// 打印 16 进制辅助数据。
	printrawdata((char*)control_un, controllen);
	free(control_un);
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
