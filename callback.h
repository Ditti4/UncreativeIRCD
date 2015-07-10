#ifndef __CALLBACK_H
#define __CALLBACK_H

typedef struct __callback {
    char *name;
    void (*callback) (int, char*);
} callback_t;

extern callback_t *callbacks;
extern int callbackcount;

int register_callback(char *name, void *function);
int unregister_callback(char *name);
int trigger_callback(int sockfd, char *name, char *message);
int parse_message(int sockfd, char *message);

#endif
