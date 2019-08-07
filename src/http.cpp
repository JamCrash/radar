
#include "http.h"
#include "http_parser.h"
#include "http_request.h"
#include "dbg.h"
#include <unistd.h>
#include <sys/stat.h>
#include <cmath>
#include <time.h>
#include <fcntl.h>
#include <sys/mman.h>

int rd_parse_request_line(rd_http_request* r);
int rd_parse_request_body(rd_http_request* r);

int parse_handler(rd_http_request* r, int flag)
{
    switch(flag) {
        case PARSE_LINE:
            return rd_parse_request_line(r);
        case PARSE_BODY:
            return rd_parse_request_body(r);
        default:
            return -1;
    }
}

const char *get_shortmsg_from_status_code(int status_code) {
    /*  for code to msg mapping, please check: 
    * http://users.polytech.unice.fr/~buffa/cours/internet/POLYS/servlets/Servlet-Tutorial-Response-Status-Line.html
    */
    if (status_code == RD_HTTP_OK) {
        return "OK";
    }

    if (status_code == RD_HTTP_NOT_MODIFIED) {
        return "Not Modified";
    }

    if (status_code == RD_HTTP_NOT_FOUND) {
        return "Not Found";
    }
    

    return "Unknown";
}


static int rd_http_process_ignore(rd_http_request *r, rd_http_out *out, char *data, int len);
static int rd_http_process_connection(rd_http_request *r, rd_http_out *out, char *data, int len);
static int rd_http_process_if_modified_since(rd_http_request *r, rd_http_out *out, char *data, int len);

rd_http_header_handle_t rd_http_headers_in[] = {
    {"Host", rd_http_process_ignore},
    {"Connection", rd_http_process_connection},
    {"If-Modified-Since", rd_http_process_if_modified_since},
    {"", rd_http_process_ignore}
};

void do_request(void* arg)
{
    if(arg == nullptr) {
        log_err("argument shouldn't be nullptr");
        return ;
    }

    rd_http_request* r = static_cast<rd_http_request*>(arg);
    int fd = r->get_fd();
    int n;
    int parse_id;
    int parse_flag = PARSE_LINE;
    char filepath[FILENAMELEN];
    
    while(1) {
        //TODO 
        //check bufer overflow
        n = read(fd, r->cur_pos, r->space_remain);
        if(n < 0) {
            if(errno == EAGAIN) {
                continue;
            }else {
                log_err("read failed");
                break;
            }
        }
        if(n == 0)
            break;
        
        r->space_remain -= n;
        r->cur_pos += n;
        r->parse_byte += n;

        parse_id = parse_handler(r, parse_flag);
        switch (parse_id) {          
            case RD_AGAIN:
                continue;
            case RD_OK:
                switch (parse_flag) {
                    case PARSE_LINE:
                        log_info("parse line finish");
                        parse_flag = PARSE_BODY;
                        continue;
                    case PARSE_BODY:
                        log_info("parse body finish");
                        break;
                }
                break;
            case RD_INVALID_REQUEST:
                log_err("request error");
                //TODO
                //do_error
                break;
        }

        rd_http_out out(r->get_fd());

        parse_uri(r->uri_start, r->uri_end, filepath, r->get_root());

        struct stat st;
        if(stat(filepath, &st) < 0) {
            //TODO 
            //404 not found
            //do_error()
            //continue;
        }
        if(!S_ISREG(st.st_mode) || !(S_IRUSR & st.st_mode)) {
            //TODO
            //403 forbidden
            //do_error()
            //continue;
        }

        out.mtime = st.st_mtime;
        
        http_handle_header(r, &out);
        check(list_empty(r->list), "error: head list shoud be empty");

        if(out.status == 0) {
            out.status = RD_HTTP_OK;
        }

        serve_static(fd, filepath, st.st_size, &out);
    }
}

//TODO
//process error
void parse_uri(const char* uri_start, const char* uri_end, char* filepath, const char* ROOT) 
{
    char uri_bufer[FILENAMELEN];
    strncpy(uri_bufer, uri_start, uri_end - uri_start);

    char* query_mark = strchr(uri_bufer, '?');
    int path_len;
    path_len = query_mark ? (query_mark - uri_start) : (uri_end - uri_start);

    strncat(filepath, ROOT, strlen(ROOT));
    strncat(filepath, uri_start, path_len);

    char* last_dot = strrchr(filepath, '.');
    char* last_slash = strrchr(filepath, '/');

    if(!last_dot && filepath[strlen(filepath) - 1] != '/') {
        strncat(filepath, "/", 1);
    }
    
    if(filepath[strlen(filepath) - 1] == '/') {
        strcat(filepath, "index.html");
    }
}

void http_handle_header(rd_http_request* r, rd_http_out* o) 
{
    list_head* pos;
    rd_http_header* head;
    rd_http_header_handle_t* header_in;

    list_for_each(pos, r->list) {
        head = list_entry(pos, rd_http_header, node);
        
        for(header_in = rd_http_headers_in; 
            strlen(header_in->value) != 0;
            header_in++) {
                if(strncmp(header_in->value, head->key, strlen(head->key)) == 0) {
                    (*(header_in->http_header_handler))(r, o, head->value, strlen(head->value));
                    break;
                }
            }

        list_del(pos);
        delete head;
    }
}

static int rd_http_process_ignore(rd_http_request* r, rd_http_out* o, char* data, int len)
{
    (void*) r;
    (void*) o;
    (void*) data;
    (void*) &len;
    return RD_OK;
}

static int rd_http_process_connection(rd_http_request* r, rd_http_out* o, char* data, int len)
{
    if(strncasecmp("keep-alive", data, len) == 0) {
        o->modified = 1;
    }
    return RD_OK;
}

static int rd_http_process_if_modified_since(rd_http_request* r, rd_http_out* o, char* data, int len)
{
    (void) r;
    (void) len;

    struct tm tm;
    if (strptime(data, "%a, %d %b %Y %H:%M:%S GMT", &tm) == (char *)NULL) {
        return RD_OK;
    }
    time_t client_time = mktime(&tm);

    double time_diff = difftime(o->mtime, client_time);
    if (fabs(time_diff) < 1e-6) {
        // log_info("content not modified clienttime = %d, mtime = %d\n", client_time, out->mtime);
        /* Not modified */
        o->modified = 0;
        o->status = RD_HTTP_NOT_MODIFIED;
    }
    
    return RD_OK;
}

void serve_static(int fd, char* filepath, size_t filelen, rd_http_out* o)
{
    char header[MAXSIZE];
    char buf[512];
    size_t n;
    struct tm tm;
    
    const char *file_type;
    const char *dot_pos = strrchr(filepath, '.');
    file_type = get_file_type(dot_pos);

    sprintf(header, "HTTP/1.1 %d %s\r\n", o->status, get_shortmsg_from_status_code(o->status));

    if (out->keep_alive) {
        sprintf(header, "%sConnection: keep-alive\r\n", header);
        sprintf(header, "%sKeep-Alive: timeout=%d\r\n", header, 500);
    }

    if (out->modified) {
        sprintf(header, "%sContent-type: %s\r\n", header, file_type);
        sprintf(header, "%sContent-length: %zu\r\n", header, filelen);
        localtime_r(&(out->mtime), &tm);
        strftime(buf, 512,  "%a, %d %b %Y %H:%M:%S GMT", &tm);
        sprintf(header, "%sLast-Modified: %s\r\n", header, buf);
    }

    sprintf(header, "%sServer: Zaver\r\n", header);
    sprintf(header, "%s\r\n", header);

    n = (size_t)rio_writen(fd, header, strlen(header));
    check(n == strlen(header), "rio_writen error, errno = %d", errno);
    if (n != strlen(header)) {
        log_err("n != strlen(header)");
        goto out; 
    }

    if (!out->modified) {
        goto out;
    }

    int srcfd = open(filepath, O_RDONLY, 0);
    check(srcfd > 2, "open error");
    // can use sendfile
    char *srcaddr = (char*)mmap(NULL, filelen, PROT_READ, MAP_PRIVATE, srcfd, 0);
    check(srcaddr != (void *) -1, "mmap error");
    close(srcfd);

    n = rio_writen(fd, srcaddr, filelen);
    // check(n == filesize, "rio_writen error");

    munmap(srcaddr, filelen);

out:
    return;
}