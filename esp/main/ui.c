#include "ui.h"
#include "serial_parser.h"
#include "lvgl.h"

#define PURPLE    lv_color_hex(0x8B5CF6)
#define YELLOW    lv_color_hex(0xF59E0B)
#define DARK_BG   lv_color_hex(0x121212)
#define DARK_TAB  lv_color_hex(0x1E1E1E)
#define SURFACE   lv_color_hex(0x2A2A2A)

static lv_obj_t *arc_cpu;
static lv_obj_t *label_cpu;
static lv_obj_t *arc_ram;
static lv_obj_t *label_ram;
static lv_obj_t *table;
static lv_obj_t *disconnect_label;

static void style_dark_screen(void) {
    lv_obj_set_style_bg_color(lv_scr_act(), DARK_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, LV_PART_MAIN);
}

static void style_tabview(lv_obj_t *tv) {
    lv_obj_set_style_bg_color(tv, DARK_TAB, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(tv, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(tv, 0, LV_PART_MAIN);

    lv_obj_set_style_bg_color(tv, PURPLE, LV_PART_ITEMS);
    lv_obj_set_style_text_color(tv, lv_color_white(), LV_PART_ITEMS);
    lv_obj_set_style_text_color(tv, YELLOW, LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(tv, SURFACE, LV_PART_ITEMS | LV_STATE_CHECKED);
}

static void style_arc(lv_obj_t *arc, lv_color_t color) {
    lv_obj_set_style_arc_color(arc, SURFACE, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, color, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, 12, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 12, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(arc, DARK_BG, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(arc, LV_OPA_0, LV_PART_MAIN);
}

static void style_table(lv_obj_t *tbl) {
    lv_obj_set_style_bg_color(tbl, SURFACE, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(tbl, LV_OPA_COVER, LV_PART_MAIN);

    lv_obj_set_style_bg_color(tbl, DARK_TAB, LV_PART_ITEMS);
    lv_obj_set_style_text_color(tbl, lv_color_white(), LV_PART_ITEMS);

    lv_obj_set_style_border_color(tbl, PURPLE, LV_PART_MAIN);
    lv_obj_set_style_border_width(tbl, 1, LV_PART_MAIN);
    lv_obj_set_style_border_side(tbl, LV_BORDER_SIDE_FULL, LV_PART_MAIN);
}

void ui_init(void) {
    style_dark_screen();

    lv_obj_t *tabview = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 40);
    style_tabview(tabview);

    lv_obj_t *tab1 = lv_tabview_add_tab(tabview, "Gauges");
    lv_obj_t *tab2 = lv_tabview_add_tab(tabview, "Processos");

    /* ---- TAB 1: Gauges ---- */
    lv_obj_t *cont = lv_obj_create(tab1);
    lv_obj_set_size(cont, 310, 170);
    lv_obj_align(cont, LV_ALIGN_CENTER, 0, 5);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_AROUND,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_0, 0);
    lv_obj_set_style_pad_row(cont, 0, 0);
    lv_obj_set_style_pad_column(cont, 0, 0);

    arc_cpu = lv_arc_create(cont);
    lv_obj_set_size(arc_cpu, 120, 120);
    lv_arc_set_rotation(arc_cpu, 135);
    lv_arc_set_bg_angles(arc_cpu, 0, 270);
    lv_arc_set_value(arc_cpu, 0);
    lv_obj_remove_style(arc_cpu, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc_cpu, LV_OBJ_FLAG_CLICKABLE);
    style_arc(arc_cpu, PURPLE);

    label_cpu = lv_label_create(arc_cpu);
    lv_label_set_text(label_cpu, "--%\nCPU");
    lv_obj_align(label_cpu, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_align(label_cpu, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(label_cpu, PURPLE, 0);
    lv_obj_set_style_text_font(label_cpu, &lv_font_montserrat_14, 0);

    arc_ram = lv_arc_create(cont);
    lv_obj_set_size(arc_ram, 120, 120);
    lv_arc_set_rotation(arc_ram, 135);
    lv_arc_set_bg_angles(arc_ram, 0, 270);
    lv_arc_set_value(arc_ram, 0);
    lv_obj_remove_style(arc_ram, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc_ram, LV_OBJ_FLAG_CLICKABLE);
    style_arc(arc_ram, YELLOW);

    label_ram = lv_label_create(arc_ram);
    lv_label_set_text(label_ram, "--%\nRAM");
    lv_obj_align(label_ram, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_align(label_ram, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(label_ram, YELLOW, 0);
    lv_obj_set_style_text_font(label_ram, &lv_font_montserrat_14, 0);

    /* Disconnect overlay */
    disconnect_label = lv_label_create(lv_scr_act());
    lv_label_set_text(disconnect_label, "NO DATA");
    lv_obj_set_style_text_color(disconnect_label, lv_color_hex(0xEF4444), 0);
    lv_obj_set_style_text_font(disconnect_label, &lv_font_montserrat_14, 0);
    lv_obj_align(disconnect_label, LV_ALIGN_BOTTOM_RIGHT, -10, -5);
    lv_obj_add_flag(disconnect_label, LV_OBJ_FLAG_HIDDEN);

    /* ---- TAB 2: Table ---- */
    table = lv_table_create(tab2);
    lv_obj_set_width(table, 300);
    lv_obj_align(table, LV_ALIGN_CENTER, 0, 0);
    style_table(table);

    lv_table_set_col_width(table, 0, 190);
    lv_table_set_col_width(table, 1, 110);

    lv_table_set_cell_value(table, 0, 0, "Processo");
    lv_table_set_cell_value(table, 0, 1, "RAM (MB)");

    for (int i = 1; i <= MAX_PROCESSES; i++) {
        lv_table_set_cell_value(table, i, 0, "Aguardando...");
        lv_table_set_cell_value(table, i, 1, "-");
    }
}

void ui_update_metrics(int cpu_pct, int ram_pct,
                       const char *proc_names[],
                       const char *proc_rams[],
                       int proc_count) {
    lv_arc_set_value(arc_cpu, cpu_pct);
    lv_label_set_text_fmt(label_cpu, "%d%%\nCPU", cpu_pct);

    lv_arc_set_value(arc_ram, ram_pct);
    lv_label_set_text_fmt(label_ram, "%d%%\nRAM", ram_pct);

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (i < proc_count) {
            lv_table_set_cell_value(table, i + 1, 0, proc_names[i]);
            lv_table_set_cell_value(table, i + 1, 1, proc_rams[i]);
        } else {
            lv_table_set_cell_value(table, i + 1, 0, "");
            lv_table_set_cell_value(table, i + 1, 1, "");
        }
    }

    lv_obj_add_flag(disconnect_label, LV_OBJ_FLAG_HIDDEN);
}

void ui_show_disconnected(void) {
    lv_obj_clear_flag(disconnect_label, LV_OBJ_FLAG_HIDDEN);
}
