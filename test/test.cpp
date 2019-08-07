
//#include "../src/util.h"
#include <iostream>
#include <pthread.h>
#include <stdio.h>
//#include <string>
#include "list.h"

using namespace std;

typedef struct zv_http_header_s {
    void *key_start, *key_end;          /* not include end */
    void *value_start, *value_end;
    list_head list;
} zv_http_header_t;

int main()
{
    list_head* head = new list_head;
    INIT_LIST_HEAD(head);
    list_entry(head, zv_http_header_t, list);

    return 0;
}