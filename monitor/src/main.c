#include "daemon.h"
#include "serial.h"

#include <gtk/gtk.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#ifdef USE_AYATANA
#include <libayatana-appindicator/app-indicator.h>
#else
#include <libappindicator/app-indicator.h>
#endif

#define ICON_SIZE 24
#define ICON_DIR "/tmp/monitor-pc-esp32"

static volatile sig_atomic_t keep_running = 1;

static void sigint_handler(int sig) {
    (void)sig;
    keep_running = 0;
}

static int run_terminal(void) {
    MonitorState tstate;

    signal(SIGINT, sigint_handler);
    signal(SIGTERM, sigint_handler);

    daemon_init(&tstate);
    daemon_start(&tstate);

    printf("\033[2J\033[H");
    printf("\033[35m=== Monitor PC-ESP32 (Terminal) ===\033[0m\n\n");

    while (keep_running) {
        int cpu, ram;
        bool connected;
        char processes[128];

        pthread_mutex_lock(&tstate.mutex);
        connected = tstate.connected;
        cpu = tstate.cpu_usage;
        ram = tstate.ram_usage;
        strncpy(processes, tstate.processes, sizeof(processes) - 1);
        processes[sizeof(processes) - 1] = '\0';
        pthread_mutex_unlock(&tstate.mutex);

        if (connected) {
            printf("\033[32m● Conectado\033[0m   "
                   "\033[95mCPU\033[0m \033[1m%3d%%\033[0m   "
                   "\033[93mRAM\033[0m \033[1m%3d%%\033[0m\n",
                   cpu, ram);
        } else {
            printf("\033[31m● Desconectado\033[0m  "
                   "aguardando %s...\n",
                   DEFAULT_SERIAL_PORT);
        }

        printf("\033[90m----------------------------------\033[0m\n");
        printf("\033[95mProcesso\033[0m            \033[93mRAM (MB)\033[0m\n");

        const char *p = processes;
        int line = 0;
        while (*p && line < 5) {
            char name[64] = {0};
            char ram_mb[16] = {0};

            const char *comma1 = strchr(p, ',');
            if (!comma1) break;
            int name_len = comma1 - p;
            if (name_len > 63) name_len = 63;
            memcpy(name, p, name_len);

            const char *comma2 = strchr(comma1 + 1, ',');
            if (!comma2) comma2 = comma1 + 1 + strlen(comma1 + 1);
            int ram_len = comma2 - (comma1 + 1);
            if (ram_len > 15) ram_len = 15;
            memcpy(ram_mb, comma1 + 1, ram_len);

            printf("  \033[37m%-18s\033[0m %6s\n", name, ram_mb);
            p = comma2 + 1;
            line++;
        }
        for (; line < 5; line++) {
            printf("\n");
        }

        sleep(1);
        printf("\033[8A");
    }

    printf("\033[9B\n\033[0mEncerrando...\n");
    daemon_stop(&tstate);
    daemon_join(&tstate);

    return 0;
}

static char icon_path_green[512];
static char icon_path_red[512];

static MonitorState state;
static AppIndicator *indicator;
static GtkWidget *menu_status;
static GtkWidget *menu_stats;

static void save_dot_icon(const char *path, guchar r, guchar g, guchar b) {
    GdkPixbuf *pb = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8,
                                   ICON_SIZE, ICON_SIZE);
    guchar *pixels = gdk_pixbuf_get_pixels(pb);
    int rs = gdk_pixbuf_get_rowstride(pb);

    memset(pixels, 0, (size_t)(rs * ICON_SIZE));

    int cx = ICON_SIZE / 2;
    int cy = ICON_SIZE / 2;
    int hx = cx - 2;
    int hy = cy - 3;

    for (int y = 0; y < ICON_SIZE; y++) {
        for (int x = 0; x < ICON_SIZE; x++) {
            int dx = x - cx, dy = y - cy;
            if (dx * dx + dy * dy <= 100) {
                guchar *p = pixels + y * rs + x * 4;
                p[0] = r;
                p[1] = g;
                p[2] = b;
                p[3] = 255;
            }
        }
    }

    for (int y = 0; y < ICON_SIZE; y++) {
        for (int x = 0; x < ICON_SIZE; x++) {
            int dx = x - hx, dy = y - hy;
            if (dx * dx + dy * dy <= 16) {
                guchar *p = pixels + y * rs + x * 4;
                p[0] = (guchar)((int)p[0] + 64 > 255 ? 255 : (int)p[0] + 64);
                p[1] = (guchar)((int)p[1] + 64 > 255 ? 255 : (int)p[1] + 64);
                p[2] = (guchar)((int)p[2] + 64 > 255 ? 255 : (int)p[2] + 64);
            }
        }
    }

    gdk_pixbuf_save(pb, path, "png", NULL, NULL);
    g_object_unref(pb);
}

static void quit_app(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;
    gtk_main_quit();
}

static gboolean update_status(gpointer user_data) {
    (void)user_data;

    int cpu, ram;
    gboolean connected;
    char status_text[128];
    char stats_text[128];
    char tooltip[256];

    pthread_mutex_lock(&state.mutex);
    connected = state.connected;
    cpu = state.cpu_usage;
    ram = state.ram_usage;
    pthread_mutex_unlock(&state.mutex);

    if (connected) {
        snprintf(status_text, sizeof(status_text), "\xe2\x97\x8f Conectado");
        snprintf(tooltip, sizeof(tooltip),
                 "Monitor PC \xe2\x86\x92 ESP32: Conectado");
    } else {
        snprintf(status_text, sizeof(status_text), "\xe2\x97\x8f Desconectado");
        snprintf(tooltip, sizeof(tooltip),
                 "Monitor PC \xe2\x86\x92 ESP32: Aguardando %s...",
                 DEFAULT_SERIAL_PORT);
    }

    snprintf(stats_text, sizeof(stats_text), "CPU: %d%%  |  RAM: %d%%", cpu, ram);

    gtk_menu_item_set_label(GTK_MENU_ITEM(menu_status), status_text);
    gtk_menu_item_set_label(GTK_MENU_ITEM(menu_stats), stats_text);

    app_indicator_set_icon_full(indicator,
                                connected ? icon_path_green : icon_path_red,
                                tooltip);

    return G_SOURCE_CONTINUE;
}

static void setup_icons(void) {
    mkdir(ICON_DIR, 0755);
    snprintf(icon_path_green, sizeof(icon_path_green),
             ICON_DIR "/connected.png");
    snprintf(icon_path_red, sizeof(icon_path_red),
             ICON_DIR "/disconnected.png");

    save_dot_icon(icon_path_green, 38, 191, 38);
    save_dot_icon(icon_path_red, 217, 51, 51);
}

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--terminal") == 0 ||
            strcmp(argv[i], "-t") == 0) {
            return run_terminal();
        }
        if (strcmp(argv[i], "--help") == 0 ||
            strcmp(argv[i], "-h") == 0) {
            printf("Uso: monitor [opcao]\n\n"
                   "  --terminal, -t   Modo terminal (sem interface grafica)\n"
                   "  --help, -h       Esta mensagem\n\n"
                   "Sem opcoes, roda na bandeja do sistema (systray).\n");
            return 0;
        }
    }

    gtk_init(&argc, &argv);

    setup_icons();

    daemon_init(&state);
    daemon_start(&state);

    indicator = app_indicator_new(
        "monitor-pc-esp32",
        icon_path_red,
        APP_INDICATOR_CATEGORY_HARDWARE);

    app_indicator_set_status(indicator, APP_INDICATOR_STATUS_ACTIVE);
    app_indicator_set_title(indicator, "Monitor PC-ESP32");

    GtkWidget *menu = gtk_menu_new();

    menu_status = gtk_menu_item_new_with_label("\xe2\x97\x8f Iniciando...");
    gtk_widget_set_sensitive(menu_status, FALSE);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_status);

    menu_stats = gtk_menu_item_new_with_label("CPU: --%  |  RAM: --%");
    gtk_widget_set_sensitive(menu_stats, FALSE);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_stats);

    GtkWidget *sep = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), sep);

    GtkWidget *quit_item = gtk_menu_item_new_with_label("Sair");
    g_signal_connect(quit_item, "activate", G_CALLBACK(quit_app), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), quit_item);

    gtk_widget_show_all(menu);
    app_indicator_set_menu(indicator, GTK_MENU(menu));

    g_timeout_add(1000, update_status, NULL);

    gtk_main();

    daemon_stop(&state);
    daemon_join(&state);

    unlink(icon_path_green);
    unlink(icon_path_red);
    rmdir(ICON_DIR);

    return 0;
}