
#ifndef HTTP_H
#define HTTP_H

#include "http_request.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#define PARSE_LINE  0x1234
#define PARSE_BODY  0x5678

#define FILENAMELEN 512

#define RD_HTTP_NOT_MODIFIED 0x0030

#define RD_HTTP_OK 200
#define RD_HTTP_NOT_FOUND 404

void do_request(void* arg);
int parse_handler(rd_http_request*, int flag);
void http_handler_header(rd_http_request*, rd_http_out*);

#endif  /* HTTP_H */