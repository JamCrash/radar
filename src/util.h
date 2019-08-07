
#ifndef UTIL_H
#define UTIL_H

#define MAXLEN 8192

#define LISTENQ 512

class conf_t {
public:
    conf_t() {}
    ~conf_t() {}
    int read_conf(const char* filepath);
    int get_port() const { 
        return port; 
    }
    int get_threadnum() const { 
        return threadnum; 
    }  
    const char* get_root() const {
        return root;
    }
private:
    char root[128];
    int port;
    int threadnum;
};

int open_listen_port(int port);
int make_socket_non_block(int fd);

#endif  /* UTIL_H */