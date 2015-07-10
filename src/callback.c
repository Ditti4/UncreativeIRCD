// #include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <errno.h>
// #include <netdb.h>
// #include <arpa/inet.h>
// #include <sys/socket.h>
// #include <sys/types.h>
// #include "connection.h"
#include "irc.h"
#include "internal.h"
#include "error.h"
#include "main.h"
#include "callback.h"
#include "callback_functions.h"

int register_callback(char *name, void *function) {
    if(!name) {
        return ERROR_INVALID_CALLBACK_NAME;
    }
    if(!function) {
        return ERROR_INVALID_CALLBACK_FUNCTION;
    }
    callback_t *new_callbacks = realloc(callbacks, (++callbackcount) * sizeof(callback_t));
    if(!new_callbacks) {
        --callbackcount;
        free(callbacks);
        return ERROR_REALLOC_FAILED;
    }
    callbacks = new_callbacks;
    int stringlen = strlen(name) + 2;
    callbacks[callbackcount - 1].name = malloc(stringlen * sizeof(char));
    if(!callbacks[callbackcount - 1].name) {
        return ERROR_MALLOC_FAILED;
    }
    strcpy(callbacks[callbackcount - 1].name, strtolower(name));
    callbacks[callbackcount - 1].name[stringlen - 2] = ' ';
    callbacks[callbackcount - 1].name[stringlen - 1] = '\0';
    callbacks[callbackcount - 1].callback = function;
    return ERROR_NONE;
}

int unregister_callback(char *name) {
    if(!name) {
        return ERROR_INVALID_CALLBACK_NAME;
    }
    char callbackname[51] = "";
    strcat(callbackname, strtolower(name));
    strcat(callbackname, " ");
    int i, callbackpos = -1;
    for(i = 0; i < callbackcount; i++) {
        if(strncmp(callbacks[i].name, callbackname, strlen(callbackname)) == 0) {
            callbackpos = i;
            break;
        }
    }
    if(callbackpos < 0) {
        return ERROR_NO_CALLBACK_FOUND;
    }
    callbackcount--;
    for(i = callbackpos; i < callbackcount; i++) {
        callbacks[i] = callbacks[i + 1];
    }
    return ERROR_NONE;
}

int trigger_callback(int sockfd, char *name, char *message) {
    if(!name) {
        return ERROR_INVALID_CALLBACK_NAME;
    }
    if(!message) {
        return ERROR_INVALID_CALLBACK_MESSAGE;
    }
    int i;
    char callbackname[51] = "";
    strcat(callbackname, strtolower(name));
    strcat(callbackname, " ");
    for(i = 0; i < callbackcount; i++) {
        if(strncmp(callbacks[i].name, callbackname, strlen(callbackname)) == 0) {
            callbacks[i].callback(sockfd, message);
            return ERROR_NONE;
        }
    }
    return ERROR_NO_CALLBACK_FOUND;
}

int parse_message(int sockfd, char *message) {
    if(!message) {
        return ERROR_INVALID_CALLBACK_MESSAGE;
    }
    int i;
    char *lower_message = strtolower(message);
    for(i = 0; i < callbackcount; i++) {
        if(strncmp(lower_message, callbacks[i].name, strlen(callbacks[i].name)) == 0) {
            callbacks[i].callback(sockfd, message);
            return ERROR_NONE;
        }
    }
    return ERROR_NO_CALLBACK_FOUND;
}
