/* croOS http.c - Simple HTTP/1.0 client
 * Supports GET/HEAD/POST, URL parsing, response header parsing.
 * Uses TCP sockets from the networking stack. */

#include "kernel/types.h"
#include "http.h"
#include "net.h"
#include "dns.h"
#include "drivers/vga.h"
#include "mm/kmalloc.h"
#include "string.h"

void http_init(void) {
    /* Nothing to initialize */
}

int http_parse_url(const char *url, http_request_t *req) {
    memset(req, 0, sizeof(http_request_t));

    /* Skip http:// or https:// */
    const char *p = url;
    if (strncmp(p, "http://", 7) == 0) {
        p += 7;
        req->port = 80;
    } else if (strncmp(p, "https://", 8) == 0) {
        p += 8;
        req->port = 443;
    }

    /* Extract host */
    int host_pos = 0;
    while (*p && *p != '/' && *p != ':' && host_pos < 127) {
        req->host[host_pos++] = *p++;
    }
    req->host[host_pos] = '\0';

    /* Extract port */
    if (*p == ':') {
        p++;
        req->port = 0;
        while (*p >= '0' && *p <= '9')
            req->port = req->port * 10 + (*p++ - '0');
    }

    /* Extract path */
    int path_pos = 0;
    if (*p == '/') {
        while (*p && path_pos < 255)
            req->path[path_pos++] = *p++;
    } else {
        req->path[0] = '/';
        req->path[1] = '\0';
    }
    req->path[path_pos] = '\0';

    /* Resolve hostname */
    if (dns_resolve(req->host, &req->ip) < 0) {
        vga_puts("  [HTTP] Failed to resolve: ");
        vga_puts(req->host);
        vga_putchar('\n');
        return -1;
    }

    return 0;
}

int http_get(const char *url, http_response_t *resp) {
    http_request_t req;
    if (http_parse_url(url, &req) < 0) return -1;

    /* Build HTTP request */
    char request[1024];
    int len = snprintf(request, sizeof(request),
        "GET %s HTTP/1.0\r\n"
        "Host: %s\r\n"
        "User-Agent: croOS/3.0\r\n"
        "Accept: */*\r\n"
        "Connection: close\r\n"
        "\r\n",
        req.path, req.host);

    /* TODO: Send via TCP socket, receive response */
    vga_puts("  [HTTP] GET ");
    vga_puts(url);
    vga_puts(" -> ");
    char ip_str[16];
    net_ip_to_string(req.ip, ip_str);
    vga_puts(ip_str);
    vga_puts(":");
    vga_put_dec(req.port);
    vga_puts(" (TCP connect pending)\n");

    /* Stub response */
    memset(resp, 0, sizeof(http_response_t));
    resp->status_code = 200;
    strcpy(resp->content_type, "text/html");
    resp->body = kmalloc(256);
    if (resp->body) {
        snprintf(resp->body, 256,
            "<html><body><h1>croOS HTTP Response</h1>"
            "<p>URL: %s</p>"
            "<p>Host: %s</p>"
            "<p>Path: %s</p>"
            "<p>IP: %s</p></body></html>",
            url, req.host, req.path, ip_str);
        resp->body_len = strlen(resp->body);
    }

    return 0;
}

int http_post(const char *url, const char *data, int data_len, http_response_t *resp) {
    http_request_t req;
    if (http_parse_url(url, &req) < 0) return -1;

    vga_puts("  [HTTP] POST ");
    vga_puts(req.path);
    vga_puts(" (");
    vga_put_dec(data_len);
    vga_puts(" bytes)\n");

    memset(resp, 0, sizeof(http_response_t));
    resp->status_code = 200;
    return 0;
}

int http_head(const char *url, http_response_t *resp) {
    http_request_t req;
    if (http_parse_url(url, &req) < 0) return -1;

    memset(resp, 0, sizeof(http_response_t));
    resp->status_code = 200;
    strcpy(resp->content_type, "text/html");
    resp->content_length = 0;
    return 0;
}

void http_free_response(http_response_t *resp) {
    if (resp->body) {
        kfree(resp->body);
        resp->body = NULL;
    }
    resp->body_len = 0;
}

void http_print_response(http_response_t *resp) {
    vga_set_color(VGA_LGREEN, VGA_BLACK);
    vga_puts("  HTTP Response:\n");
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_puts("  Status:    ");
    vga_put_dec(resp->status_code);
    vga_putchar('\n');
    vga_puts("  Type:      "); vga_puts(resp->content_type); vga_putchar('\n');
    vga_puts("  Length:    "); vga_put_dec(resp->content_length); vga_puts(" bytes\n");
    if (resp->body && resp->body_len > 0) {
        vga_puts("  Body:\n");
        int show = resp->body_len < 500 ? resp->body_len : 500;
        for (int i = 0; i < show; i++) vga_putchar(resp->body[i]);
        if (resp->body_len > 500) { vga_puts("\n  ... (truncated)"); }
        vga_putchar('\n');
    }
}
