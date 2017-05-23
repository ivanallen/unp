#include "common.h"

int max_ttl = 30;
int verbose = 0;
int nprobes = 3;
int gotalarm = 0;
int sendfd, recvfd;
int srcport, dstport;
char *hostname;
char sendbuf[4096];
char recvbuf[4096];


struct record {
	unsigned short rec_seq;
	unsigned short rec_ttl;
	int64_t rec_tv;
};

struct udp {
	unsigned short udp_srcport;
	unsigned short udp_dstport;
	unsigned short udp_len;
	unsigned short udp_cksum;
};

int recver(int seq, struct sockaddr_in *from, socklen_t *addrlen, int64_t* tv);
void handler(int sig);
void run();
int verify(struct sockaddr_in *a, int len1, struct sockaddr_in *b, int len2);

int main(int argc, char* argv[]) {
	int opt;

	while((opt = getopt(argc, argv, "m:v")) != -1) {
		switch(opt) {
			case 'm':
				max_ttl = atoi(optarg);
				if (max_ttl <=1 ) ERR_QUIT("invalid -m value\n");
				break;
			case 'v':
				++verbose;
				break;
			case '?':
				ERR_QUIT("unrecognized option: %c\n", opt);
		}
	}

	if (optind != argc - 1)
		ERR_QUIT("usage: %s [-m <maxttl>] [-v] <hostname>\n", argv[0]);

	hostname = argv[optind];

	dstport = 32767 + 666;
	srcport = (getpid() & 0xffff) | 0x8000;

	registSignal(SIGALRM, handler);

	run();

	return 0;
}

void run() {
	int ret, code, i, seq, ttl, done;
	int64_t tvrecv;
	struct record *rec;
	sockaddr_in to, from, local, last;
	socklen_t addrlen, lastlen;
	double rtt;

	seq = 0;

	sendfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sendfd < 0) ERR_EXIT("socket");
	recvfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (recvfd < 0) ERR_EXIT("socket");

	ret = resolve(hostname, dstport, &to);
	if (ret < 0) ERR_EXIT("resolve");
	ret = resolve("0", srcport, &local);
	if (ret < 0) ERR_EXIT("resolve");

	bind(sendfd, (struct sockaddr*)&local, sizeof(local));

	LOG("traceroute to %s (%s), %d hops max, %d byte packets\n",
			hostname, inet_ntoa(to.sin_addr), max_ttl, sizeof(struct record));

	for (ttl = 1; ttl <= max_ttl && !done; ++ttl) {
		setsockopt(sendfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(int));
		WARNING("%02d ", ttl);
		bzero(&last, sizeof(last));
		lastlen = 0;

		for (i = 0; i < nprobes; ++i) {
			rec = (struct record*)sendbuf;
			rec->rec_seq = ++seq;
			rec->rec_ttl = ttl;
			rec->rec_tv = now();

			sendto(sendfd, sendbuf, sizeof(struct record), 0, (struct sockaddr*)&to, sizeof(to));

			code = recver(seq, &from, &addrlen, &tvrecv);
			if (code == -3) {
				WARNING(" *");
			}
			else {
		    if (!verify(&last, lastlen, &from, addrlen)) {
					WARNING(" (%s)", inet_ntoa(from.sin_addr));
				}
				rtt = (tvrecv - rec->rec_tv) / 1000.0;
				WARNING(" %.3f ms", rtt);
				memcpy(&last, &from, addrlen);
				lastlen = addrlen;
			}

			if (code == -1) done = 1;
		}
		WARNING("\n");
	}
}

int recver(int seq, struct sockaddr_in *from, socklen_t *addrlen, int64_t* tv) {
	int ret, nr, len, icmplen, hlen1, hlen2;
	struct ip *ip, *errip;
	struct icmp *icmp;
	struct udp *udp;

	gotalarm = 0;

	alarm(1);
	while(1) {
		if (gotalarm) return -3;
		*addrlen = sizeof(from);
		nr = recvfrom(recvfd, recvbuf, 4096, 0, (struct sockaddr*)from, addrlen);
		if (nr < 0) {
			if (errno == EINTR) continue;
			ERR_EXIT("recvfrom");
		}
		ip = (struct ip*)recvbuf;
		hlen1 = ip->ip_hl << 2;
		icmp = (struct icmp*)(recvbuf + hlen1);
		icmplen = nr - hlen1;
		if (icmplen < 8) continue;

		if (verbose) {
			LOG(" \n(from %s: type = %d, code = %d)",
					inet_ntoa(from->sin_addr), icmp->icmp_type, icmp->icmp_code);
		}

		if (icmp->icmp_type == 11 && icmp->icmp_code == 0) {
			// 传输期间 TTL = 0
			// 差错报文，从第 8 字节开始是原始的 ip 数据报废首部 + 8 字节数据
			errip = (struct ip*)((char*)icmp + 8);
			hlen2 = errip->ip_hl << 2;
			// 拿到原始 udp 报文
			udp = (struct udp*)((char*)errip + hlen2);

			if (errip->ip_p == IPPROTO_UDP && 
					udp->udp_srcport == htons(srcport) &&
					udp->udp_dstport == htons(dstport)) {
				ret = -2;
				break;
			}
		}
		else if (icmp->icmp_type == 3) {
			// 不可达
			errip = (struct ip*)((char*)icmp + 8);
			hlen2 = errip->ip_hl << 2;
			udp = (struct udp*)((char*)errip + hlen2);

			if (errip->ip_p == IPPROTO_UDP && 
					udp->udp_srcport == htons(srcport) &&
					udp->udp_dstport == htons(dstport)) {
				if (icmp->icmp_code == 3) {
					// 端口不可达
					ret = -1;
				}
				else {
					ret = icmp->icmp_code;
				}
				break;
			}
		}
	}
	alarm(0);
	*tv = now();
	return ret;
}

void handler(int sig) {
	if (sig == SIGALRM) {
		gotalarm = 1;
	}
}

int verify(struct sockaddr_in *a, int len1, struct sockaddr_in *b, int len2) {
	if (len1 != len2 || memcmp(a, b, len1) != 0) {
		return 0;
	}
	return 1;
}
