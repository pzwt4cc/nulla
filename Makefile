CC = gcc
TARGET = nulla
PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin
DATADIR = $(PREFIX)/share
APPDIR = $(DATADIR)/applications
ICONDIR_SCALABLE = $(DATADIR)/icons/hicolor/scalable/apps
ICONDIR_512 = $(DATADIR)/icons/hicolor/512x512/apps

PKGS = gtk4 gtksourceview-5
CFLAGS = -std=c17 -O2 -Wall -Wextra -Werror -Wpedantic -Iinclude $(shell pkg-config --cflags $(PKGS))
DEBUG_CFLAGS = -std=c17 -g -O0 -Wall -Wextra -Werror -Wpedantic -fsanitize=address,undefined -Iinclude $(shell pkg-config --cflags $(PKGS))
LDFLAGS = $(shell pkg-config --libs $(PKGS))
DEBUG_LDFLAGS = -fsanitize=address,undefined $(shell pkg-config --libs $(PKGS))

SRCS = src/main.c src/window.c src/file.c src/settings.c
OBJS = $(SRCS:src/%.c=build/%.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

build/%.o: src/%.c include/nulla.h | build
	$(CC) $(CFLAGS) -c $< -o $@

build:
	mkdir -p build

debug: clean
	mkdir -p build
	$(CC) $(DEBUG_CFLAGS) $(SRCS) -o $(TARGET) $(DEBUG_LDFLAGS)

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)
	install -Dm644 assets/nulla.desktop $(DESTDIR)$(APPDIR)/nulla.desktop
	install -Dm644 assets/nulla.svg $(DESTDIR)$(ICONDIR_SCALABLE)/nulla.svg
	install -Dm644 assets/nulla.png $(DESTDIR)$(ICONDIR_512)/nulla.png

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)
	rm -f $(DESTDIR)$(APPDIR)/nulla.desktop
	rm -f $(DESTDIR)$(ICONDIR_SCALABLE)/nulla.svg
	rm -f $(DESTDIR)$(ICONDIR_512)/nulla.png

clean:
	rm -rf build $(TARGET)

.PHONY: all debug install uninstall clean
