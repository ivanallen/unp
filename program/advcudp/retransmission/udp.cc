#include "common.h"
#include "send_recv.h"

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

void doServer(int sockfd) {
    char buf[4096];
    int nr, nw;
    struct sockaddr_in cliaddr;
    socklen_t len;
    struct echo_head *hdr;
    char *data;

    while(1) {
        len = sizeof(cliaddr);
        nr = recvfrom(sockfd, buf, 4096, 0, (struct sockaddr*)&cliaddr, &len); 
        if (nr < 0) {
            if (errno == EINTR) continue;
            ERR_EXIT("recvfrom");
        }
        hdr = (struct echo_head*)buf;
        data = buf + sizeof(struct echo_head);

        // 随机丢弃
        if (rand() % 100 < 50) {
            WARNING("discard from %s(seq = %d, timestamp = %d):\n", inet_ntoa(cliaddr.sin_addr),
                    hdr->seq, hdr->ts);
            continue;
        }

        LOG("from %s(seq = %d, timestamp = %d):\n", inet_ntoa(cliaddr.sin_addr),
                hdr->seq, hdr->ts);
        iwrite(STDOUT_FILENO, data, nr - sizeof(struct echo_head));

        nw = sendto(sockfd, buf, nr, 0, (struct sockaddr*)&cliaddr, len); 
        if (nw < 0) {
            if (errno == EINTR) continue;
            ERR_EXIT("sentdo");
        }
    }
}

void doClient(int sockfd) {
    int ret, len, nr, nw;
    struct sockaddr_in servaddr;
    char outbuf[4096];
    char inbuf[4096];

    ret = resolve(g_option.hostname, g_option.port, &servaddr);
    if (ret < 0) ERR_EXIT("resolve");

    while(1) {
        nr = iread(STDIN_FILENO, outbuf, 4096);
        if (nr < 0) {
            ERR_EXIT("iread");
        }
        else if (nr == 0) break;

        nr = send_recv(sockfd, outbuf, nr, inbuf, 4096, (struct sockaddr*)&servaddr, sizeof(servaddr));
        if (nr < 0) {
            ERR_EXIT("send_recv");
        }

        nw = iwrite(STDOUT_FILENO, inbuf, nr);
        if (nr < 0) {
            ERR_EXIT("iwrite");
        }
    }
}

