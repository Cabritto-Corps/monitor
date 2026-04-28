#ifndef DAEMON_H
#define DAEMON_H

#include <pthread.h>
#include <stdbool.h>

typedef struct {
    pthread_mutex_t mutex;
    pthread_t thread;
    volatile bool connected;
    volatile int cpu_usage;
    volatile int ram_usage;
    char processes[128];
    volatile bool running;
} MonitorState;

void daemon_init(MonitorState *state);
void daemon_start(MonitorState *state);
void daemon_stop(MonitorState *state);
void daemon_join(MonitorState *state);

#endif
