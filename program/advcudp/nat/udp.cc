#include "common.h"
#include "log.h"

void doServer(int);
void doClient(int);
void server_routine();
void client_routine();
struct sockaddr_in *cliaddrs[16] = { 0 };

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

void updateList(struct sockaddr_in *addr) {
    // 选个最简单的策略，找第一个为空的槽位
    int i;
    for (i = 0; i < 16; ++i) {
        if (cliaddrs[i] == NULL) {
            struct sockaddr_in *tmp = (struct sockaddr_in*)malloc(sizeof(sockaddr_in));
            *tmp = *addr;
            cliaddrs[i] = tmp;
            break;
        }
    }

    if (i == 16) {
        // 如果满了，清除前一半
        for (i = 0; i < 8; ++i) {
            free(cliaddrs[i]);
            cliaddrs[i] = cliaddrs[i + 8];
            cliaddrs[i + 8] = NULL;
        }

        struct sockaddr_in *tmp = (struct sockaddr_in*)malloc(sizeof(sockaddr_in));
        *tmp = *addr;
        cliaddrs[8] = tmp;
    }
}

void sendList(int sockfd, struct sockaddr_in *addr) {
    struct sockaddr_in addrs[16];
    int nw, i, j;
    j = 0;
    for (i = 0; i < 16; ++i) {
        if (!cliaddrs[i]
            || (cliaddrs[i]->sin_addr.s_addr == addr->sin_addr.s_addr
            && cliaddrs[i]->sin_port == addr->sin_port)) continue;
        addrs[j] = *cliaddrs[i];
        ++j;
    }

    socklen_t len = sizeof(sockaddr_in);
    nw = sendto(sockfd, addrs, j * len, 0, (struct sockaddr*)addr, len);
    if (nw < 0) ERR_EXIT("sendto");
}


void doServer(int sockfd) {
    char buf[64];
    int ret, nr, nw;
    struct sockaddr_in cliaddr;

    while(1) {
        socklen_t len = sizeof(sockaddr);
        nr = recvfrom(sockfd, buf, 64, 0, (struct sockaddr*)&cliaddr, &len);
        if (nr < 0) ERR_EXIT("recvfrom");
        LOG("recv from %s:%d %s\n",
            inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port), buf);

        // 指令格式
        // 前 12 字节存储指令名字，不足 12 字节 0 填充。第 12 字节开始存储数据。
        // 例如 feedback 指令，[0, 8) 字节存储 "feedback"，[8, 12) 是 0 填充
        // [12, ...) 是 sockaddr_in 

        // 处理不同的指令
        if (!strcmp("beat", buf)) {
            updateList(&cliaddr);
            sendList(sockfd, &cliaddr);
        }

        // 服务器帮忙发送反馈包给目标，期待目标回复信息给该客户端
        if (!strcmp("feedback", buf)) {
            struct sockaddr_in targetAddr = *(struct sockaddr_in*)(buf + 12);
            LOG("转发 feedback 到 %s:%d\n",
                inet_ntoa(targetAddr.sin_addr), ntohs(targetAddr.sin_port));
            *(struct sockaddr_in*)(buf + 12) = cliaddr;
            sendto(sockfd, buf, 12 + sizeof(sockaddr_in), 0,
                (struct sockaddr*)&targetAddr, sizeof(sockaddr_in));
        }
    }
}


/* -------------------------------- client ------------------------------- */

void beat(int sockfd, struct sockaddr_in *servaddr) {
    // 发心跳给 server
    int i, nr, count;
    char buf[64] = "beat";
    socklen_t len;
    struct sockaddr_in thisAddr;
    struct sockaddr_in addrs[16] = { 0 };
    sendto(sockfd, buf, 16, 0, (struct sockaddr*)servaddr, sizeof(sockaddr_in));
    nr = recvfrom(sockfd, addrs, 16 * sizeof(sockaddr_in), 0, NULL, NULL);

    count = nr / 16; // 总条数

    len = sizeof(sockaddr_in);
    getsockname(sockfd, (struct sockaddr*)&thisAddr, &len);

    WARNING("this addr %s:%d\n",
        inet_ntoa(thisAddr.sin_addr), ntohs(thisAddr.sin_port));

    if (nr < 0) ERR_EXIT("recvfrom");


    if (count > 0)
        printf("设备列表(%d):\n", count);
    else 
        printf("暂无设备信息\n");

    for (i = 0; i < count; ++i) {
        LOG("[%02d] %s:%d\n", i, inet_ntoa(addrs[i].sin_addr),
            ntohs(addrs[i].sin_port));
    }

    if (count > 0) {
        int index = 0;
        printf("选一个你想打洞的目标:");
        scanf("%d", &index);
        printf("ok! 你选择了第 %d 个设备\n", index);
        strcpy(buf, "probe");
        // 发送探测包
        LOG("正确激活通道...");
        printf("%s:%d -> ",
            inet_ntoa(thisAddr.sin_addr), ntohs(thisAddr.sin_port));
        printf("%s:%d\n",
            inet_ntoa(addrs[index].sin_addr), ntohs(addrs[index].sin_port));
        sendto(sockfd, buf, 64, 0, (struct sockaddr*)&addrs[index], sizeof(sockaddr_in));
        LOG("完成!\n");
        // 发送 feedback 包，由服务转发给目标
        strcpy(buf, "feedback");
        *(struct sockaddr_in*)(buf + 12) = addrs[index];
        LOG("发送 feedback, 请求目标 %s:%d 反馈\n",
            inet_ntoa(addrs[index].sin_addr), ntohs(addrs[index].sin_port));
        sendto(sockfd, buf, 12 + sizeof(sockaddr_in), 0, (struct sockaddr*)servaddr, sizeof(sockaddr_in));
    }
}

void doClient(int sockfd) {
    int ret, nr, nw, status;
    socklen_t len;
    struct sockaddr_in servaddr; // 服务端地址
    struct sockaddr_in peeraddr; // 不知道谁发来的，可能是服务器，也可能是其它 client
    struct sockaddr_in addr;     // 保存打洞成功的地址
    char buf[4096];

    ret = resolve(g_option.hostname, g_option.port, &servaddr);

    beat(sockfd, &servaddr);

    // 0: 打洞准备阶段
    // 1: 打洞成功
    status = 0;


    while(1) {
        len = sizeof(sockaddr_in);
        if (status == 0) {
            nr = recvfrom(sockfd, buf, 4096, 0, (struct sockaddr*)&peeraddr, &len);
            LOG("recv from %s:%d %s\n",
                inet_ntoa(peeraddr.sin_addr), ntohs(peeraddr.sin_port), buf);

            if (!strcmp("feedback", buf)) {
                addr = *(struct sockaddr_in*)(buf + 12);
                LOG("say hello to %s:%d\n",
                    inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
                sendto(sockfd, "Hello friend!", strlen("Hello friend!") + 1,
                    0, (struct sockaddr*)&addr, sizeof(sockaddr_in));
                status = 1;
            }
        }


        // 被打通的这一端将可以主动发信息给请求者，而不经过服务器。
        if (status == 1) {
            printf("input:");
            scanf("%s", buf);
            sendto(sockfd, buf, strlen(buf) + 1, 0, (struct sockaddr*)&addr, sizeof(sockaddr_in));
        }
    }
}

