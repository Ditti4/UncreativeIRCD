#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include "connection.h"
#include "irc.h"
#include "internal.h"
#include "error.h"
#include "callback.h"
#include "callback_functions.h"
#include "main.h"

int errid; // if == 0 then no error, if > 0 then gai_strerror() it, otherwise errno is important
int keep_running = 1;

fd_set g_master;
fd_set g_read_fds;

int g_sockfd;
int g_max_sock;

user_t *users;
int usercount;

channel_t *channels;
int channelcount;

callback_t *callbacks;
int callbackcount;

char g_hostname[] = "localhost";

char *strtolower(char *string) {
    if(!string) {
        return string;
    }
    int i;
    char *newstring = malloc(LINE_LENGTH * sizeof(char));
    strcpy(newstring, string);
    for(i = 0; i < LINE_LENGTH; i++) {
        if(newstring[i] == '\0') {
            break;
        }
        if(newstring[i] >= 'A' && newstring[i] <= 'Z') {
            newstring[i] += 32;
        }
    }
    return newstring;
}

/*unsigned char *strip_newline(unsigned char *string) {
    if(!string) {
        return string;
    }
    int i;
    unsigned char *newstring = malloc(LINE_LENGTH * sizeof(unsigned char));
    strcpy(newstring, string);
    for(i = 0; i < LINE_LENGTH; i++) {
        if(newstring[i] == '\n') {
            newstring[i] = '\0';
        }
        if(newstring[i] == '\0') {
            break;
        }
    }
    return newstring;
}*/

void strip_newline(char *string) {
    if(!string) {
        return;
    }
    int i;
    for(i = 0; i < LINE_LENGTH; i++) {
        if(string[i] == '\r' || string[i] == '\n') {
            string[i] = '\0';
            break;
        }
    }
}

int main(int __attribute__((unused)) argc, char __attribute__((unused)) **argv) {
    g_sockfd = listen_on_port(g_hostname, 20000, AF_INET, 1024);
    if(g_sockfd != -1) {
        printf("Everything's fine. :)\n");
    } else {
        error_handler();
        return 1;
    }

    // TODO: read config, could probably try to go with a json config to keep it modern

    struct sockaddr_storage remote_address;
    socklen_t remote_address_size = sizeof(remote_address);
    char *message = malloc(LINE_LENGTH * sizeof(char));
    char *command = malloc(LINE_LENGTH * sizeof(char));
    char *param1 = malloc(LINE_LENGTH * sizeof(char));

    FD_ZERO(&g_master);
    FD_ZERO(&g_read_fds);
    FD_SET(g_sockfd, &g_master);

    g_max_sock = g_sockfd;
    int new_sock = 0;

    int recv_status;
    int i, j;
    channelcount = 0;
    usercount = 0;

    register_callback("join", &callback_join);
    register_callback("part", &callback_part);
    register_callback("quit", &callback_quit);
    register_callback("privmsg", &callback_privmsg);
    register_callback("notice", &callback_notice);
    register_callback("nick", &callback_nick);
    register_callback("chghost", &callback_chghost);
    register_callback("who", &callback_who);
    register_callback("user", &callback_user);
    register_callback("pong", &callback_pong);

    // "keep_running == 1" instead of just "keep_running" because there may come a time when a custom exit code comes in handy
    while(keep_running == 1) {
        g_read_fds = g_master;
        select(g_max_sock + 1, &g_read_fds, NULL, NULL, NULL);
        memset(message, 0, LINE_LENGTH);
        for(i = 0; i <= g_max_sock; i++) {
            if(FD_ISSET(i, &g_read_fds)) {
                // some socket is ready to read
                if(i == g_sockfd) {
                    // woohoo, new client \o/
                    new_sock = accept(i, (struct sockaddr*) &remote_address, &remote_address_size);
                    printf("New client connecting on socket %d from %s\n", new_sock, host_port_from_sockaddr((struct sockaddr*) &remote_address, remote_address_size));
                    // add him to the master subset so messages from him get read in the next loop cycle
                    FD_SET(new_sock, &g_master);
                    // also make him the new highest socket (if he's higher than the current highest one) so we get to him in the loop
                    if(g_max_sock < new_sock) {
                        g_max_sock = new_sock;
                    }
                    // we need a nick (and an ident but let's ignore this for now)
                    create_user(new_sock, host_from_sockaddr((struct sockaddr*) &remote_address, remote_address_size));
                } else {
                    // just an old client being ready to read
                    recv_status = recv(i, message, LINE_LENGTH, 0);
                    strip_newline(message);
                    printf("recv_status = %d, errno = %d, socket = %d, message = %s\n", recv_status, errno, i, message);
                    // if recv_status == 0 we assume the socket closed the connection on their side, so let's remove it from our side, too
                    if(!recv_status) {
                        printf("received empty message from socket %d, assuming it closed its connection, removing it from the pool\n", i);
                        quit_user(find_user_by_socket(i), ":Connection reset by peer");
                    } else if(strncmp(strtolower(message), "shutdown", strlen("shutdown")) == 0) {
                        // the client wants to shut us down
                        // such rudeness :c
                        // TODO: find out what else needs to be done when shutting down the server (apart from properly informing the user that it's shutting down)
                        printf("shutdown received from %d\n", i);
                        sprintf(message, "server shutdown received from %d\n", i);
                        for(j = 0; j <= g_max_sock; j++) {
                            if(j != g_sockfd && FD_ISSET(j, &g_master)) {
                                send_until_done(j, message, strlen(message), 0);
                                close(j);
                                FD_CLR(j, &g_master);
                            }
                        }
                        keep_running = 0;
                        break;
                    } else if(strncmp(strtolower(message), "channellist", strlen("channellist")) == 0) {
                        sscanf(message, "%s %s", command, param1);
                        int userindex = find_user(param1);
                        int k;
                        printf("channels in user %s:\n", param1);
                        for(k = 0; k < users[userindex].channelcount; k++) {
                            printf(" %s\n", users[userindex].channels[k].name);
                        }
                    } else {
                        printf("parse_message = %d\n", parse_message(i, message));
                    }
                }
            }
        }
    }

    free(users);
    free(channels);

    free(message);
    free(command);
    free(param1);

    close(g_sockfd);

    return 0;
}
