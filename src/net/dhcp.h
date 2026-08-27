/* croOS dhcp.h - DHCP client for automatic IP configuration */
#ifndef _DHCP_H
#define _DHCP_H

#include "kernel/types.h"
#include "net.h"

#define DHCP_STATE_INIT      0
#define DHCP_STATE_SELECTING 1
#define DHCP_STATE_REQUESTING 2
#define DHCP_STATE_BOUND     3
#define DHCP_STATE_RENEWING  4

typedef struct {
    uint32_t ip;
    uint32_t gateway;
    uint32_t subnet;
    uint32_t dns;
    uint32_t dhcp_server;
    uint32_t lease_time;
    uint8_t  mac[6];
    uint8_t  state;
} dhcp_lease_t;

void dhcp_init(void);
int  dhcp_discover(void);
int  dhcp_request(uint32_t server_ip);
int  dhcp_renew(void);
void dhcp_release(void);
dhcp_lease_t *dhcp_get_lease(void);
void dhcp_print_status(void);

#endif
