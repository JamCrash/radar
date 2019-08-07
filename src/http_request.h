
#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <ctime>
#include "http_parser.h"
#include "list.h"

#define MAXSIZE 8192

class rd_http_request {
public:
    rd_http_request() = delete;
    rd_http_request(int inepfd, int infd, const char* inroot);
    ~rd_http_request();

    int get_state() const {
        return state;
    }
    void set_state(int st) {
        state = st;
    }
    int get_fd() const {
        return fd;
    }
    void set_method(int m) {
        method = m;
    }
    int get_method() const {
        return method;
    }
    const char* get_root() const {
        return root;
    }
    
    char* method_start;
    char* parse_ptr;
    char* cur_pos;
    char* uri_start;
    char* uri_end;
    char* cur_key_start;
    char* cur_key_end;
    char* cur_value_start;
    char* cur_value_end;
    int space_remain = MAXSIZE;
    int parse_byte;
    list_head* list;

private:
    int fd;
    int epfd;
    int state;
    int method;
    char bufer[MAXSIZE];
    char* uri;
    const char* root;
};

struct rd_http_out {
    rd_http_out(int f): fd(f), keep_alive(0), modified(1), status(0) {}

    int fd;
    int keep_alive;
    time_t mtime;       /* the modified time of the file*/
    int modified;       /* compare If-modified-since field with mtime to decide whether the file is modified since last time*/

    int status;
};

struct rd_http_header {
    rd_http_header(char* key_start, char* key_end, char* value_start, char* value_end);
    char* key;
    char* value;
    list_head* node;
};

typedef int (*rd_http_header_handler_pt)(rd_http_request *r, rd_http_out *o, char *data, int len);

struct rd_http_header_handle_t {
    char* value;
    rd_http_header_handler_pt http_header_handler;
};

void add_list(rd_http_request*, rd_http_header*);

#endif  /* HTTP_REQUEST_H */