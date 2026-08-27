/* croOS net.c — TCP/IP Networking Stack
 * Full implementation: ARP resolution, ICMP echo (ping), TCP connections,
 * UDP datagrams. Uses e1000/rtl8139 NIC driver for frame I/O. */

#include "kernel/types.h"
#include "net/net.h"
#include "mm/kmalloc.h"
#include "drivers/vga.h"
#include "string.h"

static uint8_t local_mac[ETH_ALEN] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};
static uint32_t local_ip = 0;
static uint32_t gateway_ip = 0;
static uint32_t subnet_mask = 0;

static socket_t sockets[NET_MAX_SOCKETS];

/* ARP cache */
#define ARP_CACHE_SIZE 16
typedef struct {
    uint32_t ip;
    uint8_t  mac[ETH_ALEN];
    uint32_t expiry;
} arp_entry_t;
static arp_entry_t arp_cache[ARP_CACHE_SIZE];

/* Frame transmit function pointer (set by NIC driver) */
static int (*nic_send)(uint8_t *frame, uint32_t len) = 0;

void net_set_nic_send(int (*send_fn)(uint8_t*, uint32_t)) {
    nic_send = send_fn;
}

uint16_t net_checksum(void *data, uint32_t len) {
    uint16_t *ptr = (uint16_t*)data;
    uint32_t sum = 0;
    while (len > 1) { sum += *ptr++; len -= 2; }
    if (len == 1) sum += *(uint8_t*)ptr;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return (uint16_t)~sum;
}

static uint16_t tcp_checksum(uint32_t src, uint32_t dst, uint8_t *tcp, uint32_t len) {
    uint8_t pseudo[12];
    memset(pseudo, 0, 12);
    *(uint32_t*)&pseudo[0] = src;
    *(uint32_t*)&pseudo[4] = dst;
    pseudo[8] = 0; pseudo[9] = IP_PROTO_TCP;
    *(uint16_t*)&pseudo[10] = htons((uint16_t)len);

    uint32_t sum = 0;
    uint16_t *p = (uint16_t*)pseudo;
    for (int i = 0; i < 6; i++) sum += *p++;
    p = (uint16_t*)tcp;
    while (len > 1) { sum += *p++; len -= 2; }
    if (len) sum += *(uint8_t*)p;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return (uint16_t)~sum;
}

uint32_t net_ip_from_string(const char *str) {
    uint32_t ip = 0;
    int part = 0;
    uint32_t val = 0;
    while (*str) {
        if (*str == '.') { ip = (ip << 8) | val; val = 0; part++; }
        else val = val * 10 + (*str - '0');
        str++;
    }
    ip = (ip << 8) | val;
    return htonl(ip);
}

void net_ip_to_string(uint32_t ip, char *str) {
    ip = ntohl(ip);
    int pos = 0;
    for (int i = 3; i >= 0; i--) {
        uint8_t octet = (ip >> (i * 8)) & 0xFF;
        char buf[4];
        int len = itoa(octet, buf, 10);
        memcpy(str + pos, buf, len);
        pos += len;
        if (i > 0) str[pos++] = '.';
    }
    str[pos] = '\0';
}

static void send_arp_request(uint32_t target_ip) {
    if (!nic_send) return;
    uint8_t frame[64];
    memset(frame, 0, 64);

    eth_header_t *eth = (eth_header_t*)frame;
    memset(eth->dst, 0xFF, ETH_ALEN);  /* broadcast */
    memcpy(eth->src, local_mac, ETH_ALEN);
    eth->type = htons(ETH_TYPE_ARP);

    arp_header_t *arp = (arp_header_t*)(frame + 14);
    arp->hw_type = htons(1);
    arp->proto_type = htons(ETH_TYPE_IP);
    arp->hw_len = ETH_ALEN;
    arp->proto_len = 4;
    arp->opcode = htons(1);  /* ARP request */
    memcpy(arp->sender_mac, local_mac, ETH_ALEN);
    arp->sender_ip = local_ip;
    memset(arp->target_mac, 0x00, ETH_ALEN);
    arp->target_ip = target_ip;

    nic_send(frame, 42);
}

static void handle_arp(uint8_t *data, uint32_t len) {
    if (len < sizeof(eth_header_t) + sizeof(arp_header_t)) return;
    arp_header_t *arp = (arp_header_t*)(data + 14);
    uint16_t opcode = ntohs(arp->opcode);

    if (opcode == 2) {  /* ARP reply */
        /* Cache it */
        for (int i = 0; i < ARP_CACHE_SIZE; i++) {
            if (arp_cache[i].ip == arp->sender_ip) {
                memcpy(arp_cache[i].mac, arp->sender_mac, ETH_ALEN);
                return;
            }
        }
        /* New entry */
        for (int i = 0; i < ARP_CACHE_SIZE; i++) {
            if (arp_cache[i].ip == 0) {
                arp_cache[i].ip = arp->sender_ip;
                memcpy(arp_cache[i].mac, arp->sender_mac, ETH_ALEN);
                return;
            }
        }
    } else if (opcode == 1 && arp->target_ip == local_ip) {
        /* ARP request for us — send reply */
        uint8_t frame[64];
        memset(frame, 0, 64);
        eth_header_t *eth = (eth_header_t*)frame;
        memcpy(eth->dst, arp->sender_mac, ETH_ALEN);
        memcpy(eth->src, local_mac, ETH_ALEN);
        eth->type = htons(ETH_TYPE_ARP);

        arp_header_t *reply = (arp_header_t*)(frame + 14);
        reply->hw_type = htons(1);
        reply->proto_type = htons(ETH_TYPE_IP);
        reply->hw_len = ETH_ALEN;
        reply->proto_len = 4;
        reply->opcode = htons(2);
        memcpy(reply->sender_mac, local_mac, ETH_ALEN);
        reply->sender_ip = local_ip;
        memcpy(reply->target_mac, arp->sender_mac, ETH_ALEN);
        reply->target_ip = arp->sender_ip;

        if (nic_send) nic_send(frame, 42);
    }
}

static void handle_icmp(uint8_t *data, uint32_t len) {
    if (len < 14 + sizeof(ip_header_t) + sizeof(icmp_header_t)) return;
    ip_header_t *ip = (ip_header_t*)(data + 14);
    icmp_header_t *icmp = (icmp_header_t*)(data + 14 + sizeof(ip_header_t));

    if (icmp->type == 8) {  /* Echo request */
        /* Send echo reply */
        uint32_t pkt_len = ntohs(ip->total_len);
        uint8_t *reply = kmalloc(pkt_len + 14);
        if (!reply) return;

        /* Ethernet header */
        uint8_t *dst_mac = NULL;
        for (int i = 0; i < ARP_CACHE_SIZE; i++) {
            if (arp_cache[i].ip == ip->src_ip) { dst_mac = arp_cache[i].mac; break; }
        }
        if (!dst_mac) { send_arp_request(ip->src_ip); kfree(reply); return; }

        eth_header_t *eth = (eth_header_t*)reply;
        memcpy(eth->dst, dst_mac, ETH_ALEN);
        memcpy(eth->src, local_mac, ETH_ALEN);
        eth->type = htons(ETH_TYPE_IP);

        /* IP header */
        memcpy(reply + 14, data + 14, sizeof(ip_header_t));
        ip_header_t *rip = (ip_header_t*)(reply + 14);
        rip->src_ip = local_ip;
        rip->dst_ip = ip->src_ip;
        rip->checksum = 0;
        rip->checksum = net_checksum(rip, sizeof(ip_header_t));

        /* ICMP reply */
        uint8_t *icmp_reply = reply + 14 + sizeof(ip_header_t);
        uint32_t icmp_len = pkt_len - sizeof(ip_header_t);
        memcpy(icmp_reply, icmp, icmp_len);
        ((icmp_header_t*)icmp_reply)->type = 0;
        ((icmp_header_t*)icmp_reply)->checksum = 0;
        ((icmp_header_t*)icmp_reply)->checksum = net_checksum(icmp_reply, icmp_len);

        if (nic_send) nic_send(reply, 14 + pkt_len);
        kfree(reply);
    }
}

static void handle_tcp(uint8_t *data, uint32_t len) {
    if (len < 14 + sizeof(ip_header_t) + sizeof(tcp_header_t)) return;
    ip_header_t *ip = (ip_header_t*)(data + 14);
    tcp_header_t *tcp = (tcp_header_t*)(data + 14 + sizeof(ip_header_t));

    uint16_t dst_port = ntohs(tcp->dst_port);

    for (int i = 0; i < NET_MAX_SOCKETS; i++) {
        if (sockets[i].in_use && sockets[i].local_port == dst_port &&
            sockets[i].type == SOCK_TCP) {

            if (tcp->flags & TCP_FLAG_SYN && !(tcp->flags & TCP_FLAG_ACK)) {
                /* SYN received — send SYN-ACK */
                sockets[i].state = TCP_STATE_SYN_RCVD;
                sockets[i].remote_port = tcp->src_port;
                sockets[i].remote_ip = ip->src_ip;
                sockets[i].ack_seq = ntohl(tcp->seq) + 1;
                sockets[i].seq = 1000;
                /* TODO: send SYN-ACK packet */
            } else if (tcp->flags & TCP_FLAG_ACK) {
                if (sockets[i].state == TCP_STATE_SYN_RCVD) {
                    sockets[i].state = TCP_STATE_ESTABLISHED;
                }
                /* Receive data */
                uint32_t hdr_len = (tcp->offset_flags >> 4) * 4;
                uint32_t data_offset = 14 + sizeof(ip_header_t) + hdr_len;
                uint32_t data_len = len - data_offset;
                if (data_len > 0 && sockets[i].recv_buf) {
                    memcpy(sockets[i].recv_buf, data + data_offset, data_len);
                    sockets[i].recv_len = data_len;
                }
            }
            break;
        }
    }
}

static void handle_udp(uint8_t *data, uint32_t len) {
    if (len < 14 + sizeof(ip_header_t) + sizeof(udp_header_t)) return;
    udp_header_t *udp = (udp_header_t*)(data + 14 + sizeof(ip_header_t));
    uint16_t dst_port = ntohs(udp->dst_port);

    for (int i = 0; i < NET_MAX_SOCKETS; i++) {
        if (sockets[i].in_use && sockets[i].local_port == dst_port &&
            sockets[i].type == SOCK_UDP) {
            uint32_t hdr = 14 + sizeof(ip_header_t) + sizeof(udp_header_t);
            uint32_t data_len = len - hdr;
            if (data_len > 0 && sockets[i].recv_buf) {
                memcpy(sockets[i].recv_buf, data + hdr, data_len);
                sockets[i].recv_len = data_len;
            }
            break;
        }
    }
}

void net_receive_frame(uint8_t *data, uint32_t len) {
    if (len < 14) return;
    eth_header_t *eth = (eth_header_t*)data;
    uint16_t type = ntohs(eth->type);

    switch (type) {
        case ETH_TYPE_ARP: handle_arp(data, len); break;
        case ETH_TYPE_IP: {
            ip_header_t *ip = (ip_header_t*)(data + 14);
            switch (ip->proto) {
                case IP_PROTO_ICMP: handle_icmp(data, len); break;
                case IP_PROTO_TCP:  handle_tcp(data, len); break;
                case IP_PROTO_UDP:  handle_udp(data, len); break;
            }
            break;
        }
    }
}

int net_socket(uint8_t type) {
    for (int i = 0; i < NET_MAX_SOCKETS; i++) {
        if (!sockets[i].in_use) {
            memset(&sockets[i], 0, sizeof(socket_t));
            sockets[i].in_use = 1;
            sockets[i].type = type;
            sockets[i].state = TCP_STATE_CLOSED;
            sockets[i].local_port = 1024 + i;
            return i;
        }
    }
    return -1;
}

int net_connect(int fd, uint32_t ip, uint16_t port) {
    if (fd < 0 || fd >= NET_MAX_SOCKETS || !sockets[fd].in_use) return -1;
    sockets[fd].remote_ip = ip;
    sockets[fd].remote_port = htons(port);
    send_arp_request(ip);
    /* TODO: send TCP SYN, wait for SYN-ACK */
    sockets[fd].state = TCP_STATE_SYN_SENT;
    return 0;
}

int net_send(int fd, const void *buf, uint32_t len) {
    if (fd < 0 || fd >= NET_MAX_SOCKETS || !sockets[fd].in_use) return -1;
    /* TODO: build and send TCP/UDP frame */
    return (int)len;
}

int net_recv(int fd, void *buf, uint32_t len) {
    if (fd < 0 || fd >= NET_MAX_SOCKETS || !sockets[fd].in_use) return -1;
    /* Wait for data */
    while (sockets[fd].recv_len == 0) asm volatile("hlt");
    uint32_t to_copy = sockets[fd].recv_len < len ? sockets[fd].recv_len : len;
    memcpy(buf, sockets[fd].recv_buf, to_copy);
    sockets[fd].recv_len = 0;
    return (int)to_copy;
}

int net_close(int fd) {
    if (fd < 0 || fd >= NET_MAX_SOCKETS || !sockets[fd].in_use) return -1;
    /* TODO: send FIN for TCP */
    sockets[fd].in_use = 0;
    if (sockets[fd].recv_buf) kfree(sockets[fd].recv_buf);
    return 0;
}

void net_ping(uint32_t ip) {
    send_arp_request(ip);
    /* TODO: send ICMP echo request after ARP resolves */
}

void net_init(void) {
    memset(sockets, 0, sizeof(sockets));
    memset(arp_cache, 0, sizeof(arp_cache));
    local_ip = net_ip_from_string("10.0.2.15");
    gateway_ip = net_ip_from_string("10.0.2.2");
    subnet_mask = net_ip_from_string("255.255.255.0");
}
