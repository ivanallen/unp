#ifndef __ICMP_H__
#define __ICMP_H__

// icmp 头部
struct icmp {
	uint8_t icmp_type;
	uint8_t icmp_code;
	uint16_t icmp_cksum;
	// 不同类型的 icmp 报文，后面都不一样
};


// icmp 回显报文头部
struct icmp_echo {
	uint8_t icmp_type;
	uint8_t icmp_code;
	uint16_t icmp_cksum;
	uint16_t icmp_id;
	uint16_t icmp_seq;
	char icmp_data[0];
};

// icmp 子网掩码头部
struct icmp_mask {
	uint8_t icmp_type;
	uint8_t icmp_code;
	uint16_t icmp_cksum;
	uint16_t icmp_id;
	uint16_t icmp_seq;
	struct in_addr icmp_mask;
};

// icmp 时间戳头部
struct icmp_time {
	uint8_t icmp_type;
	uint8_t icmp_code;
	uint16_t icmp_cksum;
	uint16_t icmp_id;
	uint16_t icmp_seq;
	uint32_t icmp_origtime;
	uint32_t icmp_recvtime;
	uint32_t icmp_sendtime;
};

#endif //__ICMP_H__

