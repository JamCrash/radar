
#include <netinet/in.h>
#include <sys/socket.h>
#include <getopt.h>
#include <strings.h>
#include "threadpool.h"
#include "util.h"
#include "epoll.h"
#include "http_request.h"
#include "http.h"
#include "dbg.h"

#define CONF "radar.conf"
#define PROGRAM_VERSION "0.1"

extern struct epoll_event* events;

static const struct option long_options[] =
{
    {"help", no_argument, NULL, '?'},
    {"version", no_argument, NULL, 'V'},
    {"conf", required_argument, NULL, 'c'},
    {NULL, 0, NULL, 0}
};

static void usage() 
{
    fprintf(stderr,
	"zaver [option]... \n"
	"  -c|--conf <config file>  Specify config file. Default ./zaver.conf.\n"
	"  -?|-h|--help             This information.\n"
	"  -V|--version             Display program version.\n"
	);
}

int main(int argc, char* argv[])
{
    if(argc < 2) {
        usage();
        return 0;
    }

    int c;
    int optlong_index;
    char* conf_file = CONF;
    while( (c = getopt_long(argc, argv, "c:?Vh,", long_options, &optlong_index)) != -1 ) {
        switch (c) {
            case 0:
                break;
            case '?':
            case 'h':
            case ':':
                usage();
                return 0;
            case 'c':
                conf_file = optarg;
                break;
            case 'V':
                printf(PROGRAM_VERSION"\n");
                return 0;
        }
    }

    if(optind < argc) {
        log_err("illegal arguments: ");
        while(optind < argc) 
            fprintf(stderr, "%s ", argv[optind++]);
        fprintf(stderr, "\n");
        return 0;
    }

    conf_t conf;
    conf.read_conf(conf_file);

    int listen_fd;
    listen_fd = open_listen_port(conf.get_port());
    if(listen_fd < 0) {
        log_err("create socket error");
        return 0;
    }

    if(make_socket_non_block(listen_fd) != 0) {
        log_err("make listen socket unblock failed");
        return 0;
    }

    //initialize new epoll
    epoll_t epoll(MAXEVENT);

    //handle the listen socket with http_request
    rd_http_request* request = new rd_http_request(epoll.get_epfd(), listen_fd, conf.get_root());

    //register the listen event
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.ptr = (void*)request;
    epoll.add(listen_fd, &event);

    threadpool pool(8);

    int n;
    int fd;
    int infd;
    rd_http_request* r;
    struct sockaddr_in clientaddr;
    socklen_t clientaddr_len;

    while(1) {
        n = epoll.wait(events, MAXEVENT, 400);
        if(n < 0) {
            log_err();
            continue;
        }

        for(int i = 0; i < n; ++i) {
            r = (rd_http_request*)events[i].data.ptr;
            fd = r->get_fd();

            if(fd == listen_fd) {
                while(1) {  
                    infd = accept(fd, (sockaddr*)&clientaddr, &clientaddr_len);
                    if(infd <= 0) {
                        if(errno == EAGAIN || errno == EWOULDBLOCK) {
                            break;
                        }else {
                            log_err("accept");
                            break;
                        }
                    }
                    log_info("new connection: fd = %d", infd);

                    if(make_socket_non_block(infd) != 0) {
                        log_err("make socket %d unblock fail", infd);
                        continue;
                    }

                    rd_http_request* request = new rd_http_request(epoll.get_epfd(), infd, conf.get_root());
                    bzero(&event, sizeof(event));
                    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
                    event.data.ptr = (void*)request;
                    epoll.add(infd, &event);
                }
            }else {
                pool.add_task(do_request, (void*)r);
                //log_info("")
            }
        }
    }
}