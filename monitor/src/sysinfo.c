#include "sysinfo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int get_cpu_usage(void) {
    static unsigned long long prev_total = 0, prev_idle = 0;
    static int first_call = 1;

    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) return 0;

    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
    char cpu[8];
    if (fscanf(fp, "%7s %llu %llu %llu %llu %llu %llu %llu %llu",
               cpu, &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal) < 8) {
        fclose(fp);
        return 0;
    }
    fclose(fp);

    unsigned long long total = user + nice + system + idle + iowait + irq + softirq + steal;
    unsigned long long total_idle = idle + iowait;

    if (first_call) {
        prev_total = total;
        prev_idle = total_idle;
        first_call = 0;
        usleep(100000);
        return get_cpu_usage();
    }

    unsigned long long total_delta = total - prev_total;
    unsigned long long idle_delta = total_idle - prev_idle;

    prev_total = total;
    prev_idle = total_idle;

    if (total_delta == 0) return 0;
    return (int)((total_delta - idle_delta) * 100 / total_delta);
}

int get_ram_usage(void) {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) return 0;

    unsigned long long mem_total = 0, mem_available = 0;
    char line[128];

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "MemTotal:", 9) == 0) {
            sscanf(line + 9, "%llu", &mem_total);
        } else if (strncmp(line, "MemAvailable:", 13) == 0) {
            sscanf(line + 13, "%llu", &mem_available);
        }
        if (mem_total && mem_available) break;
    }
    fclose(fp);

    if (mem_total == 0) return 0;
    return (int)((mem_total - mem_available) * 100 / mem_total);
}

int get_top_processes(char *buffer, int max_len) {
    FILE *fp;
    char line[256];

    buffer[0] = '\0';

    fp = popen(
        "ps -eo comm,rss --sort=-rss --no-headers | head -n 5 | "
        "awk '{printf \"%s,%d,\", $1, $2/1024}'",
        "r");
    if (!fp) return -1;

    if (fgets(line, sizeof(line), fp)) {
        int len = (int)strlen(line);
        if (len > 0 && line[len - 1] == ',') {
            line[len - 1] = '\0';
        }
        strncat(buffer, line, (size_t)(max_len - 1));
    }
    pclose(fp);

    return 0;
}
