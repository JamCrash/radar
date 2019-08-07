
#include <fcntl.h>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include "dbg.h"
#include "util.h"

int
conf_t::read_conf(const char* path)
{
    if(path == nullptr) 
        return -1;

    struct stat st;
    if(stat(path, &st) == -1) {
        log_err("No such file");
        return -1;
    }

    if(!S_ISREG(st.st_mode)) {
        log_err("File mode error");
        return -1;
    }

    FILE* fp = fopen(path, "r");
    if(fp == NULL) {
        log_err("open file failed");
        return -1;
    }

    char buf[512];
    char* pos;
    while(fgets(buf, 512, fp) != NULL) {
        pos = strstr(buf, "=");
        if(pos == NULL) 
            continue;
        
        if(pos[strlen(pos) - 1] == '\n') 
            pos[strlen(pos) - 1] = '\0';
        if(strncmp("root", buf, 4) == 0) {
            strncpy(root, pos + 1, 128);
        }
        else if(strncmp("port", buf, 4) == 0) {
            port = atoi(pos + 1);
        }
        else if(strncmp("threadnum", buf, 9) == 0) {
            threadnum = atoi(pos + 1);
        }
    }
    if(!feof(fp)) 
        return -1;

    return 0;
}

int open_listen_port(int port)
{
    if(port <= 0) 
        return -1;

    int listen_fd, optval = 1;
    
    if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) != 0)
        return -1;

    if(setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) != 0) 
        return -1;

    struct sockaddr_in listen_addr;
    bzero(&listen_addr, sizeof(sockaddr_in));
    listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    listen_addr.sin_port = htons(port);
    listen_addr.sin_family = AF_INET;

    if(bind(listen_fd, (sockaddr*)&listen_addr, sizeof(listen_addr)) != 0) 
        return -1;

    if(listen(listen_fd, LISTENQ) != 0) 
        return -1;

    return listen_fd;
}

int make_socket_non_block(int fd)
{
    int flag;

    if((flag = fcntl(fd, F_GETFD, 0)) != 0) {
        log_err("fcntl:F_GETFD");
        return -1;
    }
    flag |= O_NONBLOCK;
    if(fcntl(fd, F_SETFD, flag) != 0) {
        log_err("fcntl:F_SETFD");
        return -1;
    }
    
    return 0;
}