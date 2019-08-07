
#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include <stdint.h>
#include "http_request.h"

#define RD_OK               0x0000
#define RD_AGAIN            0x0001
#define RD_INVALID_REQUEST  0x0002

#define CR  '\r'
#define LF  '\n'
#define CRLFCRLF    "\r\n\r\n"

#define RD_HTTP_GET         0x1000
#define RD_HTTP_POST        0x2000
#define RD_HTTP_HEAD        0x3000
#define RD_HTTP_UNKNOWN     0xf000

#define str3cmp(m, c0, c1, c2, c3)      \
    ( *(uint32_t*)m == (c3 << 24) | (c2 << 16) | (c1 << 8) | c0 )

int rd_parse_request_line(rd_http_request* r);
int rd_parse_request_body(rd_http_request* r);

#endif  /* HTTP_PARSER_H */
