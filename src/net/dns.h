/* croOS dns.h - DNS resolver */
#ifndef _DNS_H
#define _DNS_H

#include "kernel/types.h"

#define DNS_PORT 53
#define DNS_MAX_NAME 256
#define DNS_MAX_ANSWERS 8
#define DNS_BUF_SIZE 512

typedef struct {
    char name[DNS_MAX_NAME];
    uint32_t ip;
    uint32_t ttl;
    uint16_t type;
    uint16_t class;
} dns_record_t;

void     dns_init(uint32_t dns_server);
int      dns_resolve(const char *hostname, uint32_t *out_ip);
int      dns_resolve_name(const char *name, uint32_t *out_ips, int max_results);
void     dns_set_server(uint32_t server);
uint32_t dns_get_server(void);
int      dns_build_query(const char *name, uint8_t *buf, int bufsize);

#endif
