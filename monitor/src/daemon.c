#include "daemon.h"
#include "serial.h"
#include "sysinfo.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define RECONNECT_DELAY 2

static void *daemon_thread_func(void *arg) {
    MonitorState *state = (MonitorState *)arg;
    int fd = -1;

    while (1) {
        pthread_mutex_lock(&state->mutex);
        if (!state->running) {
            pthread_mutex_unlock(&state->mutex);
            break;
        }
        pthread_mutex_unlock(&state->mutex);

        int cpu = get_cpu_usage();
        int ram = get_ram_usage();

        char proc_buf[128];
        get_top_processes(proc_buf, sizeof(proc_buf));

        pthread_mutex_lock(&state->mutex);
        state->cpu_usage = cpu;
        state->ram_usage = ram;
        strncpy(state->processes, proc_buf, sizeof(state->processes) - 1);
        state->processes[sizeof(state->processes) - 1] = '\0';
        pthread_mutex_unlock(&state->mutex);

        if (fd < 0) {
            fd = serial_open(DEFAULT_SERIAL_PORT);

            pthread_mutex_lock(&state->mutex);
            state->connected = (fd >= 0);
            pthread_mutex_unlock(&state->mutex);

            if (fd < 0) {
                sleep(RECONNECT_DELAY);
                continue;
            }
        }

        char serial_buf[256];
        int written = snprintf(serial_buf, sizeof(serial_buf),
                               "C:%d;R:%d;P:%s;\n", cpu, ram, proc_buf);

        if (written < 0 || written >= (int)sizeof(serial_buf)) {
            serial_close(fd);
            fd = -1;
            pthread_mutex_lock(&state->mutex);
            state->connected = false;
            pthread_mutex_unlock(&state->mutex);
            continue;
        }

        if (serial_write_data(fd, serial_buf, written) < 0) {
            serial_close(fd);
            fd = -1;
            pthread_mutex_lock(&state->mutex);
            state->connected = false;
            pthread_mutex_unlock(&state->mutex);
            continue;
        }

        sleep(1);
    }

    if (fd >= 0) serial_close(fd);
    return NULL;
}

void daemon_init(MonitorState *state) {
    pthread_mutex_init(&state->mutex, NULL);
    state->connected = false;
    state->cpu_usage = 0;
    state->ram_usage = 0;
    state->processes[0] = '\0';
    state->running = false;
}

void daemon_start(MonitorState *state) {
    state->running = true;
    pthread_create(&state->thread, NULL, daemon_thread_func, state);
}

void daemon_stop(MonitorState *state) {
    pthread_mutex_lock(&state->mutex);
    state->running = false;
    pthread_mutex_unlock(&state->mutex);
}

void daemon_join(MonitorState *state) {
    pthread_join(state->thread, NULL);
    pthread_mutex_destroy(&state->mutex);
}