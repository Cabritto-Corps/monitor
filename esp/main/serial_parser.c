#include "serial_parser.h"
#include <string.h>
#include <stdlib.h>

int serial_parser_parse(const char *line, MonitorData *out) {
    memset(out, 0, sizeof(*out));

    const char *p = line;

    if (strncmp(p, "C:", 2) == 0) {
        p += 2;
        out->cpu_pct = (int)strtol(p, (char **)&p, 10);
    }

    if (*p == ';') p++;
    if (strncmp(p, "R:", 2) == 0) {
        p += 2;
        out->ram_pct = (int)strtol(p, (char **)&p, 10);
    }

    if (*p == ';') p++;
    if (strncmp(p, "P:", 2) == 0) {
        p += 2;
        const char *seg_start = p;
        int proc_idx = 0;

        while (proc_idx < MAX_PROCESSES && *seg_start && *seg_start != ';') {
            const char *comma1 = strchr(seg_start, ',');
            if (!comma1) break;

            int name_len = (int)(comma1 - seg_start);
            if (name_len >= MAX_PROC_NAME_LEN) name_len = MAX_PROC_NAME_LEN - 1;
            memcpy(out->processes[proc_idx].name, seg_start, name_len);
            out->processes[proc_idx].name[name_len] = '\0';

            const char *comma2 = strchr(comma1 + 1, ',');
            if (!comma2) {
                comma2 = seg_start + strlen(seg_start);
            }

            int ram_len = (int)(comma2 - (comma1 + 1));
            if (ram_len >= MAX_PROC_RAM_LEN) ram_len = MAX_PROC_RAM_LEN - 1;
            memcpy(out->processes[proc_idx].ram_mb, comma1 + 1, ram_len);
            out->processes[proc_idx].ram_mb[ram_len] = '\0';

            seg_start = comma2 + 1;
            proc_idx++;
        }
        out->process_count = proc_idx;
    }

    return 0;
}
