#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "connection.h"
#include "error.h"

/*
 * Here you can find the most sensitive functions
 * in the whole project. Don't screw with them,
 * they will hate you and karma will strike back.
 * If you really need to tamper with them then do
 * it but don't come back here and argue that you
 * haven't been warned.
 * (seriously, please don't touch them)
 *
 */

sockinfo_t create_socket(char *host, int port, int addr_family) {
    struct addrinfo hints;
    struct addrinfo *info;
    sockinfo_t retsockinfo;
    char charport[6];
    int getaddrinfo_status;
    int sockfd;
    int setsockopt_status;

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = addr_family;
    if(!host) {
        hints.ai_flags = AI_PASSIVE;
    }

    snprintf(charport, 6, "%d", port);
    getaddrinfo_status = getaddrinfo(host, charport, &hints, &info);
    if(getaddrinfo_status) {
        retsockinfo.info = NULL;
        retsockinfo.sockfd = -1;
        errid = getaddrinfo_status;
        return retsockinfo;
    }

    printf("  %s\n", ip_from_addrinfo(info));

    sockfd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
    if(sockfd == -1) {
        retsockinfo.info = info;
        retsockinfo.sockfd = sockfd;
        errid = ERROR_SOCKET;
        return retsockinfo;
    }

    int reuseaddr_opt = 1;
    setsockopt_status = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_opt, sizeof(reuseaddr_opt));
    if(setsockopt_status) {
        retsockinfo.info = NULL;
        retsockinfo.sockfd = sockfd;
        errid = ERROR_SETSOCKOPT;
        return retsockinfo;
    }

    retsockinfo.info = info;
    retsockinfo.sockfd = sockfd;
    errid = 0;
    return retsockinfo;
}

int host_connect(char *host, int port, int addr_family) {
    int connect_status;
    sockinfo_t sockinfo = create_socket(host, port, addr_family);
    if(errid != 0) {
        return -1;
    }

    connect_status = connect(sockinfo.sockfd, sockinfo.info->ai_addr, sockinfo.info->ai_addrlen);
    if(connect_status) {
        errid = ERROR_CONNECT;
        return -1;
    }

    freeaddrinfo(sockinfo.info);

    return sockinfo.sockfd;
}

int listen_on_port(char *host, int port, int addr_family, int maxclients) {
    int bind_status;
    int listen_status;
    sockinfo_t sockinfo = create_socket(host, port, addr_family);
    if(errid != 0) {
        return -1;
    }

    bind_status = bind(sockinfo.sockfd, sockinfo.info->ai_addr, sockinfo.info->ai_addrlen);
    if(bind_status) {
        errid =  ERROR_BIND;
        return -1;
    }

    listen_status = listen(sockinfo.sockfd, maxclients);
    if(listen_status) {
        errid = ERROR_LISTEN;
        return -1;
    }

    freeaddrinfo(sockinfo.info);

    return sockinfo.sockfd;
}

int send_until_done(int sockfd, char *message, int msglen, int flags) {
    int unsigned chars_sent = 0;
    do {
        message += chars_sent;
    } while((chars_sent = send(sockfd, message, msglen - chars_sent, flags)) >= 1);

    // returns the unsigned chars left after send() tried everything, is 0 in best case scenarios, msglen in worst case ones
    return chars_sent;
}

void error_handler() {
    if(errid == 0) {
        return;
    } else if(errid > 0) {
        printf("getaddrinfo: %s", gai_strerror(errid));
        return;
    } else {
        switch(errid) {
            case ERROR_ACCEPT:
                perror("accept");
                break;
            case ERROR_BIND:
                perror("bind");
                break;
            case ERROR_CONNECT:
                perror("connect");
                break;
            case ERROR_LISTEN:
                perror("listen");
                break;
            case ERROR_SELECT:
                perror("select");
                break;
            case ERROR_SOCKET:
                perror("socket");
                break;
            case ERROR_SETSOCKOPT:
                perror("setsockopt");
                break;
        }
    }
    errid = 0;
}

// used for establishing connections
char *ip_from_addrinfo(struct addrinfo *info) {
    if(!info) {
        return NULL;
    }
    void *addr;
    char *ipstring = malloc(INET6_ADDRSTRLEN * sizeof(char));
    if(info->ai_family == AF_INET) {
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)info->ai_addr;
        addr = &(ipv4->sin_addr);
    } else {
        struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)info->ai_addr;
        addr = &(ipv6->sin6_addr);
    }
    inet_ntop(info->ai_family, addr, ipstring, INET6_ADDRSTRLEN);
    return ipstring;
}

// used for new incoming connections
char *ip_from_sockaddr(struct sockaddr *info, int size) {
    char *ipstring = malloc(INET6_ADDRSTRLEN * sizeof(char));
    getnameinfo(info, size, ipstring, INET6_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST | NI_NUMERICSERV);
    return ipstring;
}

char *host_from_sockaddr(struct sockaddr *info, int size) {
    char *ipstring = malloc(NI_MAXHOST * sizeof(char));
    getnameinfo(info, size, ipstring, NI_MAXHOST, NULL, 0, NI_NUMERICSERV);
    return ipstring;
}

char *ip_port_from_sockaddr(struct sockaddr *info, int size) {
    char *ipstring = malloc(INET6_ADDRSTRLEN * sizeof(char));
    char *portstring = malloc(NI_MAXSERV * sizeof(char));
    char *combinedstring = malloc((INET6_ADDRSTRLEN + NI_MAXSERV + 1) * sizeof(char));
    getnameinfo(info, size, ipstring, INET6_ADDRSTRLEN, portstring, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);
    sprintf(combinedstring, "%s:%s", ipstring, portstring);
    return combinedstring;
}

char *host_port_from_sockaddr(struct sockaddr *info, int size) {
    char *ipstring = malloc(NI_MAXHOST * sizeof(char));
    char *portstring = malloc(NI_MAXSERV * sizeof(char));
    char *combinedstring = malloc((NI_MAXHOST + NI_MAXSERV + 1) * sizeof(char));
    getnameinfo(info, size, ipstring, NI_MAXHOST, portstring, NI_MAXSERV, NI_NUMERICSERV);
    sprintf(combinedstring, "%s:%s", ipstring, portstring);
    return combinedstring;
}
