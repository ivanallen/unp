#include "common.h"
#include "log.h"

void doServer(int);
void doClient(int);
void server_routine();
void client_routine();

struct Options {
    int isServer;
    char hostname[32];
    int port;
} g_option;

int main(int argc, char* argv[]) {
    Args args = parsecmdline(argc, argv);
    if (args.empty()){
        printf("Usage:\n  %s [-s] [-h hostname] [-p port]\n", argv[0]);
        return 1;
    }


    SETBOOL(args, g_option.isServer, "s", 0);
    SETSTR(args, g_option.hostname, "h", "0");
    SETINT(args, g_option.port, "p", 8000);

    if (g_option.isServer) {
        server_routine();
    }
    else {
        client_routine();
    }

    return 0;
}

void server_routine() {
    int ret, sockfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t cliaddrlen;

    ret = resolve(g_option.hostname, g_option.port, &servaddr);
    if (ret < 0) ERR_EXIT("resolve");

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) ERR_EXIT("socket");
    reuseAddr(sockfd, 1); 

    ret = bind(sockfd, (struct sockaddr*)&servaddr, sizeof servaddr);
    if (ret < 0) ERR_EXIT("bind");

    doServer(sockfd);

    close(sockfd);
}

void client_routine() {
    int sockfd;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) ERR_EXIT("socket");

    doClient(sockfd);

    close(sockfd);
}

int accept(int sockfd) {
    char buf[16];
    int nr, nw, newSockfd, ret;
    struct sockaddr_in cliaddr;
    struct sockaddr_in servaddr;
    pid_t pid;
    socklen_t len;
    // 第一次接受客户端发来的数据
    len = sizeof(sockaddr_in);
    nr = recvfrom(sockfd, buf, 16, 0, (struct sockaddr*)&cliaddr, &len);
    if (nr < 0) ERR_EXIT("recvfrom");
    buf[nr] = 0;  
    LOG("accept client:%s:%d %s\n",
        inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port), buf);
    // 连接客户端
    newSockfd = socket(AF_INET, SOCK_DGRAM, 0);
    ret = connect(newSockfd, (struct sockaddr*)&cliaddr, len);
    if (ret < 0) ERR_EXIT("connect");
    // 取得本地绑定地址
    ret = getsockname(newSockfd, (struct sockaddr*)&servaddr, &len);
    LOG("new addr:%s:%d\n",
        inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port));
    sprintf(buf, "Hello client");
    *(unsigned short*)(buf + 14) = ntohs(servaddr.sin_port);
    // buf = {"Hello client", port}
    nw = sendto(sockfd, buf, 16, 0, (struct sockaddr*)&cliaddr, len);
    return newSockfd;
}

void process(int conn) {
    char buf[4096];
    char all[4096] = { 0 };
    int nr, nw;
    int count = 0;
    while(1) {
        nr = iread(conn, buf, 4096);
        buf[nr] = ' ';
        buf[nr + 1] = 0;
        printf("pid:%d recv from client:%s\n", getpid(), buf);
        if (nr < 0) ERR_EXIT("iread");
        strcat(all, buf);
        nw = iwrite(conn, all, strlen(all));
        if (nw < 0) ERR_EXIT("iread");
    }
    exit(0);
}

void doServer(int sockfd) {
    int conn;
    pid_t pid;
    LOG("main process:%d\n", getpid());
    while(1) {
        conn = accept(sockfd);    
        pid = fork();
        if (pid < 0) {
            ERR_EXIT("fork");
        } else if (pid == 0) {
            process(conn);
        }
        LOG("fork child process:%d\n", pid);
    }
}

// 握手
int build(int sockfd) {
    int ret, nr, nw, port;
    socklen_t len;
    struct sockaddr_in servaddr;
    char buf[16] = "Hello server"; // 12 byte
    ret = resolve(g_option.hostname, g_option.port, &servaddr);
    if (ret < 0) ERR_EXIT("resolve");
    nw = sendto(sockfd, buf, 12, 0, (struct sockaddr*)&servaddr, sizeof(servaddr)); 
    if (nw < 0) ERR_EXIT("sendto");
    len = sizeof(servaddr);
    nr = recvfrom(sockfd, buf, 16, 0, (struct sockaddr*)&servaddr, &len);
    if (nr < 0) ERR_EXIT("recvfrom");
    buf[12] = 0;
    if (strcmp(buf, "Hello client") != 0) {
        buf[nr] = 0;
        ERR_PRINT("recv:%s, build connection faild!\n", buf);
        exit(0);
    }
    // 提取服务端新地址
    port = *(unsigned short*)(buf + 14);
    LOG("recv from %s:%d:%s, build connection success!\n", 
        inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port), buf);
    // setpeeraddr
    servaddr.sin_port = htons(port);
    LOG("new port:%d\n", port);
    ret = connect(sockfd, (struct sockaddr*)&servaddr, len);
    if (ret < 0) ERR_EXIT("connect");
    return sockfd;
}

void doClient(int sockfd) {
    int ret, len, nr, nw;
    struct sockaddr_in servaddr;
    char buf[4096];

    int conn = build(sockfd);

    while(1) {
        nr = iread(STDIN_FILENO, buf, 4096);
        if (nr < 0) {
            ERR_EXIT("iread");
        }
        else if (nr == 0) break;

        if (buf[nr-1] == '\n') nr--; // 去年换行符
        nw = iwrite(sockfd, buf, nr); // 去掉换行
        if (nw < 0) {
            if (errno == EINTR) continue;
            ERR_EXIT("sendto");
        }
        nr = iread(sockfd, buf, 4096);
        if (nr < 0) {
            if (errno == EINTR) continue;
            ERR_EXIT("recvfrom");
        }
        nw = iwrite(STDOUT_FILENO, buf, nr);
        if (nw < 0) {
            ERR_EXIT("iwrite");
        }
    }
}

