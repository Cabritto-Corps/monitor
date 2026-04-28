CC = gcc

INDICATOR_LIB := $(shell pkg-config --exists ayatana-appindicator3-0.1 && echo ayatana-appindicator3-0.1 || (pkg-config --exists appindicator3-0.1 && echo appindicator3-0.1 || echo ""))

ifeq ($(INDICATOR_LIB),)
$(error Nenhuma biblioteca de indicator encontrada. Instale: libayatana-appindicator3-dev ou libappindicator3-dev)
endif

ifeq ($(INDICATOR_LIB),ayatana-appindicator3-0.1)
CFLAGS_IND = -DUSE_AYATANA
else
CFLAGS_IND =
endif

PKG_CFLAGS = $(shell pkg-config --cflags gtk+-3.0 $(INDICATOR_LIB)) $(CFLAGS_IND)
PKG_LIBS = $(shell pkg-config --libs gtk+-3.0 $(INDICATOR_LIB)) -lm -lpthread

CFLAGS = -Wall -Wextra -O2 $(PKG_CFLAGS)
LDFLAGS = $(PKG_LIBS)

SRCDIR = src
OBJDIR = obj
TARGET = monitor

SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) $(TARGET)

REAL_HOME := $(shell getent passwd $${SUDO_USER:-$$USER} | cut -d: -f6)
USER_AUTOSTART := $(REAL_HOME)/.config/autostart

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/monitor-pc-esp32
	@mkdir -p $(USER_AUTOSTART)
	cp monitor.desktop $(USER_AUTOSTART)/monitor-pc-esp32.desktop

uninstall:
	rm -f /usr/local/bin/monitor-pc-esp32
	rm -f $(USER_AUTOSTART)/monitor-pc-esp32.desktop

.PHONY: clean install uninstall
