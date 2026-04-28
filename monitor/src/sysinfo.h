#ifndef SYSINFO_H
#define SYSINFO_H

int get_cpu_usage(void);
int get_ram_usage(void);
int get_top_processes(char *buffer, int max_len);

#endif
