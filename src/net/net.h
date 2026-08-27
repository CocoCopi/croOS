/* croOS net.h — TCP/IP networking stack */
#ifndef _NET_H
#define _NET_H

#include "kernel/types.h"

/* Network byte order helpers */
#define htons(x) (((x) >> 8) | ((x) << 8))
#define ntohs(x) htons(x)
#define htonl(x) (((x) >> 24) | (((x) >> 8) & 0xFF00) | \
                  (((x) << 8) & 0xFF0000) | ((x) << 24))
#define ntohl(x) htonl(x)

/* Ethernet frame */
#define ETH_ALEN 6
#define ETH_TYPE_IP  0x0800
#define ETH_TYPE_ARP 0x0806

typedef struct {
    uint8_t  dst[ETH_ALEN];
    uint8_t  src[ETH_ALEN];
    uint16_t type;
} __packed eth_header_t;

/* IPv4 header */
typedef struct {
    uint8_t  ver_ihl;
    uint8_t  tos;
    uint16_t total_len;
    uint16_t id;
    uint16_t flags_frag;
    uint8_t  ttl;
    uint8_t  proto;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dst_ip;
} __packed ip_header_t;

#define IP_PROTO_ICMP 1
#define IP_PROTO_TCP  6
#define IP_PROTO_UDP  17

/* TCP header */
typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq;
    uint32_t ack;
    uint8_t  offset_flags;
    uint8_t  flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent;
} __packed tcp_header_t;

#define TCP_FLAG_SYN  0x02
#define TCP_FLAG_ACK  0x10
#define TCP_FLAG_FIN  0x01
#define TCP_FLAG_RST  0x04
#define TCP_FLAG_PSH  0x08

/* UDP header */
typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
} __packed udp_header_t;

/* ICMP header */
typedef struct {
    uint8_t  type;
    uint8_t  code;
    uint16_t checksum;
    uint16_t id;
    uint16_t seq;
} __packed icmp_header_t;

/* ARP header */
typedef struct {
    uint16_t hw_type;
    uint16_t proto_type;
    uint8_t  hw_len;
    uint8_t  proto_len;
    uint16_t opcode;
    uint8_t  sender_mac[ETH_ALEN];
    uint32_t sender_ip;
    uint8_t  target_mac[ETH_ALEN];
    uint32_t target_ip;
} __packed arp_header_t;

/* Socket */
typedef struct {
    uint8_t  in_use;
    uint8_t  type;       /* 1=TCP, 2=UDP, 3=RAW */
    uint16_t local_port;
    uint16_t remote_port;
    uint32_t remote_ip;
    uint8_t  state;      /* TCP state machine */
    uint32_t seq;
    uint32_t ack_seq;
    void    *recv_buf;
    uint32_t recv_len;
} socket_t;

#define SOCK_TCP   1
#define SOCK_UDP   2
#define SOCK_RAW   3

#define TCP_STATE_CLOSED    0
#define TCP_STATE_LISTEN    1
#define TCP_STATE_SYN_SENT  2
#define TCP_STATE_SYN_RCVD  3
#define TCP_STATE_ESTABLISHED 4
#define TCP_STATE_FIN_WAIT  5
#define TCP_STATE_CLOSE_WAIT 6
#define TCP_STATE_TIME_WAIT 7

#define NET_MAX_SOCKETS 32
#define NET_BUF_SIZE    2048

void     net_init(void);
void     net_receive_frame(uint8_t *data, uint32_t len);
uint32_t net_ip_from_string(const char *str);
void     net_ip_to_string(uint32_t ip, char *str);
uint16_t net_checksum(void *data, uint32_t len);
int      net_socket(uint8_t type);
int      net_connect(int fd, uint32_t ip, uint16_t port);
int      net_send(int fd, const void *buf, uint32_t len);
int      net_recv(int fd, void *buf, uint32_t len);
int      net_close(int fd);
void     net_ping(uint32_t ip);

#endif
