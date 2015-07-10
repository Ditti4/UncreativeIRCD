#ifndef __CONNECTION_H
#define __CONNECTION_H

typedef struct __sockinfo {
    struct addrinfo *info;
    int sockfd;
    int errid; 
} sockinfo_t;

extern int errid;

sockinfo_t create_socket(char *host, int port, int addr_family);
int host_connect(char *host, int port, int addr_family);
int listen_on_port(char *host, int port, int maxclients, int addr_family);
int send_until_done(int sockfd, char *message, int msglen, int flags);
void error_handler();
char *ip_from_addrinfo(struct addrinfo *info);
char *ip_from_sockaddr(struct sockaddr *info, int size);
char *host_from_sockaddr(struct sockaddr *info, int size);
char *ip_port_from_sockaddr(struct sockaddr *info, int size);
char *host_port_from_sockaddr(struct sockaddr *info, int size);

#endif
