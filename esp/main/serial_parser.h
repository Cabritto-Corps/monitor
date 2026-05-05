#ifndef SERIAL_PARSER_H
#define SERIAL_PARSER_H

#include <stdint.h>

#define MAX_PROCESSES 5
#define MAX_PROC_NAME_LEN 32
#define MAX_PROC_RAM_LEN 8

typedef struct {
    char name[MAX_PROC_NAME_LEN];
    char ram_mb[MAX_PROC_RAM_LEN];
} ProcessEntry;

typedef struct {
    int cpu_pct;
    int ram_pct;
    ProcessEntry processes[MAX_PROCESSES];
    int process_count;
} MonitorData;

int serial_parser_parse(const char *line, MonitorData *out);

#endif
