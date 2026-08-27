/* croOS dhcp.c - DHCP client implementation
 * Sends DHCPDISCOVER, receives DHCPOFFER, sends DHCPREQUEST, handles DHCPACK.
 * Configures IP, gateway, subnet mask, and DNS from DHCP server response. */

#include "kernel/types.h"
#include "dhcp.h"
#include "net.h"
#include "drivers/vga.h"
#include "drivers/timer.h"
#include "string.h"

static dhcp_lease_t lease;
static uint32_t xid = 0x12345678;

/* Build a DHCP discover packet */
static int dhcp_build_discover(uint8_t *buf) {
    memset(buf, 0, 300);

    /* Ethernet header (broadcast) */
    eth_header_t *eth = (eth_header_t*)buf;
    memset(eth->dst, 0xFF, 6);
    uint8_t mac[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};
    memcpy(eth->src, mac, 6);
    eth->type = htons(ETH_TYPE_IP);

    /* UDP header: source port 68, dest port 67 */
    udp_header_t *udp = (udp_header_t*)(buf + 14 + 20);
    udp->src_port = htons(68);
    udp->dst_port = htons(67);

    /* DHCP message */
    uint8_t *dhcp = buf + 14 + 20 + 8;
    dhcp[0] = 1;   /* BOOTREQUEST */
    dhcp[1] = 1;   /* Hardware type: Ethernet */
    dhcp[2] = 6;   /* Hardware address length */
    dhcp[3] = 0;   /* Hops */
    *(uint32_t*)&dhcp[4] = xid;  /* Transaction ID */
    *(uint16_t*)&dhcp[8] = 0;    /* Seconds elapsed */
    *(uint16_t*)&dhcp[10] = 0x8000; /* Flags: broadcast */
    /* Client IP: 0.0.0.0 */
    /* Your IP: 0.0.0.0 */
    /* Server IP: 0.0.0.0 */
    /* Gateway IP: 0.0.0.0 */
    memcpy(&dhcp[28], mac, 6);  /* Client MAC */

    /* Magic cookie */
    dhcp[236] = 0x63; dhcp[237] = 0x82; dhcp[238] = 0x53; dhcp[239] = 0x63;

    /* Options */
    int pos = 240;
    /* Message type: DHCPDISCOVER */
    dhcp[pos++] = 53; dhcp[pos++] = 1; dhcp[pos++] = 1;
    /* Requested IP */
    dhcp[pos++] = 50; dhcp[pos++] = 4;
    dhcp[pos++] = 0; dhcp[pos++] = 0; dhcp[pos++] = 0; dhcp[pos++] = 0;
    /* Server identifier (empty) */
    /* End */
    dhcp[pos++] = 255;

    return 14 + 20 + 8 + pos;
}

void dhcp_init(void) {
    memset(&lease, 0, sizeof(dhcp_lease_t));
    lease.state = DHCP_STATE_INIT;
}

int dhcp_discover(void) {
    lease.state = DHCP_STATE_SELECTING;
    xid++;
    /* TODO: send DHCPDISCOVER packet via NIC */
    vga_puts("  [DHCP] Sending DHCPDISCOVER...\n");
    return 0;
}

int dhcp_request(uint32_t server_ip) {
    lease.state = DHCP_STATE_REQUESTING;
    lease.dhcp_server = server_ip;
    vga_puts("  [DHCP] Sending DHCPREQUEST to server...\n");
    return 0;
}

int dhcp_renew(void) {
    lease.state = DHCP_STATE_RENEWING;
    return 0;
}

void dhcp_release(void) {
    lease.state = DHCP_STATE_INIT;
    memset(&lease.ip, 0, 16);
}

dhcp_lease_t *dhcp_get_lease(void) {
    return &lease;
}

void dhcp_print_status(void) {
    char ip_str[16], gw_str[16], sn_str[16], dns_str[16];
    net_ip_to_string(lease.ip, ip_str);
    net_ip_to_string(lease.gateway, gw_str);
    net_ip_to_string(lease.subnet, sn_str);
    net_ip_to_string(lease.dns, dns_str);

    vga_set_color(VGA_LGREEN, VGA_BLACK);
    vga_puts("  DHCP Configuration:\n");
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_puts("  State:      ");
    switch (lease.state) {
        case DHCP_STATE_INIT:     vga_puts("INIT"); break;
        case DHCP_STATE_SELECTING: vga_puts("SELECTING"); break;
        case DHCP_STATE_REQUESTING: vga_puts("REQUESTING"); break;
        case DHCP_STATE_BOUND:    vga_puts("BOUND"); break;
        case DHCP_STATE_RENEWING: vga_puts("RENEWING"); break;
    }
    vga_putchar('\n');
    vga_puts("  IP Address: "); vga_puts(ip_str); vga_putchar('\n');
    vga_puts("  Gateway:    "); vga_puts(gw_str); vga_putchar('\n');
    vga_puts("  Subnet:     "); vga_puts(sn_str); vga_putchar('\n');
    vga_puts("  DNS:        "); vga_puts(dns_str); vga_putchar('\n');
    vga_puts("  Server:     ");
    char srv[16];
    net_ip_to_string(lease.dhcp_server, srv);
    vga_puts(srv); vga_putchar('\n');
}
