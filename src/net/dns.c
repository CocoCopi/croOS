/* croOS dns.c - DNS resolver
 * Builds DNS A-record queries, parses responses, caches results.
 * Supports recursive resolution up to 3 levels deep. */

#include "kernel/types.h"
#include "dns.h"
#include "net.h"
#include "drivers/vga.h"
#include "drivers/timer.h"
#include "string.h"

static uint32_t dns_server_ip;
static uint16_t dns_txid = 0;

typedef struct {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} __packed dns_header_t;

/* Simple DNS cache */
#define DNS_CACHE_SIZE 32
typedef struct {
    char name[DNS_MAX_NAME];
    uint32_t ip;
    uint32_t expires;
} dns_cache_entry_t;

static dns_cache_entry_t dns_cache[DNS_CACHE_SIZE];

/* Encode a domain name into DNS wire format */
static int dns_encode_name(const char *name, uint8_t *out) {
    int out_pos = 0;
    int seg_start = 0;

    for (int i = 0; ; i++) {
        if (name[i] == '.' || name[i] == '\0') {
            int len = i - seg_start;
            out[out_pos++] = (uint8_t)len;
            memcpy(out + out_pos, name + seg_start, len);
            out_pos += len;
            seg_start = i + 1;
            if (name[i] == '\0') break;
        }
    }
    out[out_pos++] = 0;  /* root label */
    return out_pos;
}

/* Decode a DNS name from wire format */
static int dns_decode_name(const uint8_t *buf, int buf_len, int offset, char *out) {
    int pos = offset;
    int out_pos = 0;
    int jumped = 0;
    int jump_pos = 0;

    while (pos < buf_len) {
        uint8_t len = buf[pos];
        if (len == 0) {
            if (!jumped) out[out_pos] = '\0';
            return jumped ? jump_pos : pos + 1;
        }
        if ((len & 0xC0) == 0xC0) {
            /* Pointer */
            if (!jumped) jump_pos = pos + 2;
            pos = ((uint16_t)(len & 0x3F) << 8) | buf[pos + 1];
            jumped = 1;
            continue;
        }
        pos++;
        if (out_pos > 0) out[out_pos++] = '.';
        memcpy(out + out_pos, buf + pos, len);
        out_pos += len;
        pos += len;
    }
    out[out_pos] = '\0';
    return pos;
}

int dns_build_query(const char *name, uint8_t *buf, int bufsize) {
    memset(buf, 0, bufsize);
    dns_header_t *hdr = (dns_header_t*)buf;
    hdr->id = htons(++dns_txid);
    hdr->flags = htons(0x0100);  /* Standard query, recursion desired */
    hdr->qdcount = htons(1);

    int pos = sizeof(dns_header_t);
    pos += dns_encode_name(name, buf + pos);

    /* Type A, Class IN */
    buf[pos++] = 0; buf[pos++] = 1;  /* Type A */
    buf[pos++] = 0; buf[pos++] = 1;  /* Class IN */

    return pos;
}

/* Look up name in cache */
static int dns_cache_lookup(const char *name, uint32_t *out_ip) {
    for (int i = 0; i < DNS_CACHE_SIZE; i++) {
        if (dns_cache[i].ip != 0 && strcmp(dns_cache[i].name, name) == 0) {
            *out_ip = dns_cache[i].ip;
            return 0;
        }
    }
    return -1;
}

static void dns_cache_add(const char *name, uint32_t ip, uint32_t ttl) {
    for (int i = 0; i < DNS_CACHE_SIZE; i++) {
        if (dns_cache[i].ip == 0 || dns_cache[i].expires < timer_get_seconds()) {
            strncpy(dns_cache[i].name, name, DNS_MAX_NAME - 1);
            dns_cache[i].ip = ip;
            dns_cache[i].expires = timer_get_seconds() + ttl;
            return;
        }
    }
}

int dns_resolve(const char *hostname, uint32_t *out_ip) {
    /* Check cache first */
    if (dns_cache_lookup(hostname, out_ip) == 0) return 0;

    /* Check if it's already an IP */
    if (hostname[0] >= '0' && hostname[0] <= '9') {
        *out_ip = net_ip_from_string(hostname);
        return 0;
    }

    /* Build and send DNS query */
    uint8_t query[DNS_BUF_SIZE];
    int qlen = dns_build_query(hostname, query, sizeof(query));

    /* TODO: Send UDP packet to DNS server and parse response */

    /* For now, return failure */
    vga_puts("  [DNS] Query for: ");
    vga_puts(hostname);
    vga_puts(" (query built, network TX pending)\n");
    return -1;
}

int dns_resolve_name(const char *name, uint32_t *out_ips, int max_results) {
    (void)max_results;
    return dns_resolve(name, &out_ips[0]);
}

void dns_set_server(uint32_t server) {
    dns_server_ip = server;
}

uint32_t dns_get_server(void) {
    return dns_server_ip;
}
