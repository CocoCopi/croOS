/* croOS http.h - Simple HTTP client */
#ifndef _HTTP_H
#define _HTTP_H

#include "kernel/types.h"

#define HTTP_MAX_URL    512
#define HTTP_MAX_HEADER 1024
#define HTTP_MAX_BODY   65536

typedef struct {
    int    status_code;
    char   content_type[64];
    int    content_length;
    char  *body;
    int    body_len;
    char   headers[HTTP_MAX_HEADER];
} http_response_t;

typedef struct {
    char method[8];
    char host[128];
    char path[256];
    int  port;
    uint32_t ip;
} http_request_t;

void http_init(void);
int  http_get(const char *url, http_response_t *resp);
int  http_post(const char *url, const char *data, int data_len, http_response_t *resp);
int  http_head(const char *url, http_response_t *resp);
void http_free_response(http_response_t *resp);
int  http_parse_url(const char *url, http_request_t *req);
void http_print_response(http_response_t *resp);

#endif
