#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
// #include <arpa/inet.h>
// #include <sys/socket.h>
// #include <sys/types.h>
#include "connection.h"
#include "irc.h"
#include "internal.h"
#include "replies.h"
#include "main.h"
#include "error.h"
#include "callback.h"
#include "callback_functions.h"

// TODO: check of argument count in callback functions

void callback_join(int sockfd, char *message) {
    char *command = malloc(50 * sizeof(char));
    char *param1 = malloc(LINE_LENGTH * sizeof(char));
    sscanf(message, "%s %s", command, param1);
    printf("user %s on socket %d wants to join channel %s with %d users in it\n", users[find_user_by_socket(sockfd)].nick, sockfd, param1, channels[find_channel(param1)].usercount);
    join_channel(find_user_by_socket(sockfd), param1);
    printf("usercount of channel %s after the user tried to join: %d\n", param1, channels[find_channel(param1)].usercount);
    free(command);
    free(param1);
}

void callback_part(int sockfd, char *message) {
    char *command = malloc(50 * sizeof(char));
    char *param1 = malloc(LINE_LENGTH * sizeof(char));
    char *param2 = malloc(LINE_LENGTH * sizeof(char));
    if(sscanf(message, "%s %s %s", command, param1, param2) < 3) {
        free(param2);
        param2 = NULL;
    }
    int channelindex = find_channel(param1);
    int userindex = find_user_by_socket(sockfd);
    printf("user %s on socket %d wants to part from %s with %d users in it\n", users[userindex].nick, sockfd, param1, channels[channelindex].usercount);
    part_channel(userindex, channelindex, param2);
    printf("usercount of channel %s after the user tried to part: %d\n", param1, channels[channelindex].usercount);
    free(command);
    free(param1);
    free(param2);
}

void callback_notice(int sockfd, char *message) {
    char *command = malloc(50 * sizeof(char));
    char *param1 = malloc(LINE_LENGTH * sizeof(char));
    char *param2 = malloc(LINE_LENGTH * sizeof(char));
     sscanf(message, "%s %s %[^\n]s", command, param1, param2);
    int userindex = find_user_by_socket(sockfd);
    printf("notice to destination %s from user %d: %s\n", param1, userindex, param2);
    privmsg(userindex, param1, param2);
    free(command);
    free(param1);
    free(param2);
}

void callback_privmsg(int sockfd, char *message) {
    char *command = malloc(50 * sizeof(char));
    char *param1 = malloc(LINE_LENGTH * sizeof(char));
    char *param2 = malloc(LINE_LENGTH * sizeof(char));
    sscanf(message, "%s %s %[^\n]s", command, param1, param2);
    int userindex = find_user_by_socket(sockfd);
    printf("privmsg to destination %s from user %d: %s\n", param1, userindex, param2);
    privmsg(userindex, param1, param2);
    free(command);
    free(param1);
    free(param2);
}

void callback_quit(int sockfd, char *message) {
    // possible replies: any error reply (ERR_NOSUCHNICK for example) to tell the client that we disconnected him
    char *command = malloc(50 * sizeof(char));
    char *param1 = malloc(LINE_LENGTH * sizeof(char));
    sscanf(message, "%s %s", command, param1);
    printf("received quit command from socket %d, closing it and saying good bye to the client\n", sockfd);
    quit_user(find_user_by_socket(sockfd), param1);
    free(command);
    free(param1);
}

void callback_nick(int sockfd, char *message) {
    // possible replies: ERR_NONICKNAMEGIVEN, ERR_NICKNAMEINUSE, ERR_ERRONEUSNICKNAME
    char *command = malloc(50 * sizeof(char));
    char *param1 = malloc(LINE_LENGTH * sizeof(char));
    int userindex = find_user_by_socket(sockfd);
    sscanf(message, "%s %s", command, param1); // ERR_NONICKNAMEGIVEN
    printf("user on socket %d attempts to change his nick from %s to %s\n", sockfd, users[userindex].nick, param1);
    printf("set_nick() = %d\n", set_nick(userindex, param1));
    printf("nick of user on socket %d now is %s\n", sockfd, users[userindex].nick);
    free(command);
    free(param1);
}

void callback_chghost(int sockfd, char *message) {
    char *command = malloc(50 * sizeof(char));
    char *param1 = malloc(LINE_LENGTH * sizeof(char));
    sscanf(message, "%s %s", command, param1);
    int userindex = find_user_by_socket(sockfd);
    printf("user %d with nick %s attempts to change his hostname from %s to %s\n", userindex, users[userindex].nick, users[userindex].hostname, param1);
    set_hostname(userindex, param1);
    printf("hostname after user's try to change it: %s\n", users[userindex].hostname);
    free(command);
    free(param1);
}

void callback_who(int sockfd, char *message) {
    // possible replies:
    char *command = malloc(50 * sizeof(char));
    char *param1 = malloc(LINE_LENGTH * sizeof(char));
    sscanf(message, "%s %s", command, param1);
    int channelindex = find_channel(param1);
    int userindex = find_user_by_socket(sockfd);
    if(channelindex < 0) {
        // ERR_NOSUCHCHANNEL
        free(command);
        free(param1);
        return;
    }
    int k;
    // TODO: seg fault when running who when there's no channel
    printf("users in channel %s:\n", param1);
    for(k = 0; k < channels[channelindex].usercount; k++) {
        printf(" %s\n", channels[channelindex].users[k].nick);
    }
    int *matching_nicks = get_matching_nicks("bla*");
    int i;
    printf("nicks matching \"bla*\":\n");
    for(i = 0; *(matching_nicks + i) != -1; i++) {
        printf("  matches[%d] = %d: %s\n", i, *(matching_nicks + i), users[*(matching_nicks + i)].nick);
    }
    free(command);
    free(param1);
}

void callback_user(int sockfd, char *message) {
    // possible replies: ERR_NEEDMOREPARAMS, ERR_ALREADYREGISTERED
    char *command = malloc(50 * sizeof(char));
    char *param1 = malloc(LINE_LENGTH * sizeof(char));
    char *param2 = malloc(LINE_LENGTH * sizeof(char));
    int dummyint;
    int parsed_args;
    if((parsed_args = sscanf(message, "%s %s %d * :%[^\n]s", command, param1, &dummyint, param2)) < 4) {
        parsed_args = sscanf(message, "%s %s %d * %s", command, param1, &dummyint, param2);
    }
    if(parsed_args < 4) {
        //ERR_NEEDMOREPARAMS
        free(command);
        free(param1);
        free(param2);
        return;
    }
    int userindex = find_user_by_socket(sockfd);
    printf("user on socket %d attempts to change his ident from %s to %s\n", sockfd, users[userindex].ident, param1);
    switch(set_ident(userindex, param1, 0)) {
        case ERR_NEEDMOREPARAMS:
            free(command);
            free(param1);
            free(param2);
            return;
        case ERR_ALREADYREGISTERED:
            free(command);
            free(param1);
            free(param2);
            return;
    }
    switch(set_realname(userindex, param2, 0)) {
        case ERR_NEEDMOREPARAMS:
            free(command);
            free(param1);
            free(param2);
            return;
        case ERR_ALREADYREGISTERED:
            free(command);
            free(param1);
            free(param2);
            return;
    }
    printf("ident of user on socket %d now is %s, realname is %s\n", sockfd, users[userindex].ident, users[userindex].realname);
    free(command);
    free(param1);
    free(param2);
}
