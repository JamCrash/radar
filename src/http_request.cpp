
#include "http_request.h"
#include <cstdlib>
#include <cstring>

rd_http_request::rd_http_request(int inepfd, int infd, const char* inroot)
    :epfd(inepfd), fd(infd), root(inroot)
    ,state(0), parse_ptr(bufer), parse_byte(0)
    ,cur_pos(bufer)
{
    list = new list_head;
    INIT_LIST_HEAD(list);
}

rd_http_header::rd_http_header(char* key_start, char* key_end, char* value_start, char* value_end)
{
    unsigned key_len = key_end - key_start;
    unsigned value_len = value_end - value_start;
    //TODO
    //change malloc to new
    key = new char[key_len + 1];
    value = new char[value_len + 1];

    strncpy(key, key_start, key_len);
    strncpy(value, value_start, value_len);

    node = new list_head;
    INIT_LIST_HEAD(&node);
}

void add_list(rd_http_request* r, rd_http_header* head) 
{
    list_add(head->node, r->list);
}
