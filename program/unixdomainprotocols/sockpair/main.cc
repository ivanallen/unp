#include "common.h"

char buf[4096];

int main() {
	int ret, status, nr, sockfd[2];
	pid_t pid;


	while(1) {
		ret = socketpair(AF_LOCAL, SOCK_STREAM, 0, sockfd);
		if (ret < 0) ERR_EXIT("socketpair");

		pid = fork();

		if (pid < 0) ERR_EXIT("fork");
		if (pid == 0) {
			// child
			close(sockfd[0]);
			dup2(sockfd[1], STDIN_FILENO);
			dup2(sockfd[1], STDOUT_FILENO);
			execlp("./upper", "./upper");
		}

		// father
		close(sockfd[1]);

		nr = iread(STDIN_FILENO, buf, 4096); 
		if (nr < 0) {
			ERR_EXIT("read");
		}
		if (nr == 0) return 0;

		iwrite(sockfd[0], buf, nr);

		ret = waitpid(pid, &status, 0);
		if (ret < 0) ERR_EXIT("waitpid");
		if (WIFEXITED(status) == 0) {
			fputs("upper executed error!\n", stderr);
			return 1;
		}

		nr = iread(sockfd[0], buf, 4096);
		if (nr < 0) {
			ERR_EXIT("read");
		}

		iwrite(STDOUT_FILENO, buf, nr);

		close(sockfd[0]);
	}

  return 0; 
}
