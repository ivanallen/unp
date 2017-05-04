#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <map>
#include <string>
#include <assert.h>
#include <stdint.h>
#include <pthread.h>
#include <poll.h>
#include <limits.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/epoll.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define ERR_EXIT(msg) do { perror(msg); exit(1); } while(0)
#define ERR_QUIT(msg) do { fputs(msg, stderr); exit(1); } while(0)

#define CONTAINS(container,element) (container.find(element) != container.end())
#define SETINT(args,val,opt,def) \
  do {\
    if (CONTAINS(args,opt)) {\
      (val) = atoi(args[opt].c_str());\
    } \
    else {\
      (val) = (def);\
    }\
  } while(0)

#define SETSTR(args,val,opt,def) \
  do {\
		if (CONTAINS(args, opt)) {\
			strcpy(val, args[opt].c_str());\
		} \
		else {\
			strcpy(val, def);\
		}\
	} while(0)

#define SETBOOL(args,val,opt,def) \
  do {\
    if (CONTAINS(args,opt)) {\
      (val) = 1;\
    } \
    else {\
      (val) = (def);\
    }\
  } while(0)

typedef std::map<std::string, std::string> Args;


int64_t now();
int resolve(const char* hostname, int port, struct sockaddr_in *addr);
int resolve(const char* pathname, struct sockaddr_un *addr, int abstract = 0);
int readn(int fd, char* buf, int n);
int readline(int fd, char* buf, int n);
int writen(int fd, const char* buf, int n);
int iread(int fd, char* buf, int n);
int iwrite(int fd, const char* buf, int n);
std::map<std::string, std::string> parsecmdline(int argc, char* argv[]);
void toUpper(char* str, int n);
void registSignal(int sig, void (*handler)(int), void(**oldhandler)(int) = NULL);
void ignoreSignal(int sig);
void showopts(int fd, const char* opt);
void showopts(const char* opt);
void reuseAddr(int sockfd, int onoff); 
void setLinger(int sockfd, int onoff, int seconds);
void setSendBufSize(int sockfd, int size);
void setRecvBufSize(int sockfd, int size);
void setMaxSegSize(int sockfd, int size);
void setNoDelay(int sockfd, int onoff);
void setCork(int sockfd, int onoff);
void setRecvTimeout(int sockfd, int nsec);
void setSendTimeout(int sockfd, int nsec);
int readfd(int fd, char* buf, int size, int *recvfd);
int writefd(int fd, char* buf, int size, int sendfd);
int myOpen(const char* pathname, int mode);
