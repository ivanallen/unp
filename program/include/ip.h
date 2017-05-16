#ifndef __IP_H__
#define __IP_H__
// IP首部数据结构
// 都是网络字节序
struct ip{
	// 主机字节序判断
#if __BYTE_ORDER == __LITTLE_ENDIAN
	uint8_t ip_hl:4;        // 首部长度
	uint8_t ip_v:4;     // 版本      
#endif
#if __BYTE_ORDER == __BIG_ENDIAN
	uint8_t ip_v:4;       
	uint8_t ip_hl:4;    
#endif
	uint8_t ip_tos;             // 服务类型
	uint16_t ip_len;             // 总长度
	uint16_t ip_id;                // 标识符
	uint16_t ip_off;            // 标志和片偏移
#define IP_RF 0x8000      /* reserved fragment flag */
#define IP_DF 0x4000      /* dont fragment flag */
#define IP_MF 0x2000      /* more fragments flag */
#define IP_OFFMASK 0x1fff   /* mask for fragmenting bits */
	uint8_t ip_ttl;            // 生存时间
	uint8_t ip_p;       // 协议
	uint16_t ip_sum;       // 校验和
	struct in_addr ip_src;    // 32位源ip地址
	struct in_addr ip_dst;   // 32位目的ip地址
	// 可选项、数组起始部分
};

#endif // __IP_H__
