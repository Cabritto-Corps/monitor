#ifndef UI_H
#define UI_H

#include <stdint.h>

void ui_init(void);
void ui_update_metrics(int cpu_pct, int ram_pct,
                       const char *proc_names[],
                       const char *proc_rams[],
                       int proc_count);
void ui_show_disconnected(void);

#endif
