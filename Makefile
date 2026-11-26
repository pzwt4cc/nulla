CC = gcc
TARGET = nulla
PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin
ICONDIR = $(PREFIX)/share/icons/hicolor/scalable/apps

PKGS = gtk4 gtksourceview-5
CFLAGS = -std=c17 -O2 -Wall -Wextra -Werror -Wpedantic $(shell pkg-config --cflags $(PKGS))
DEBUG_CFLAGS = -std=c17 -g -O0 -Wall -Wextra -Werror -Wpedantic -fsanitize=address,undefined $(shell pkg-config --cflags $(PKGS))
LDFLAGS = $(shell pkg-config --libs $(PKGS))
DEBUG_LDFLAGS = -fsanitize=address,undefined $(shell pkg-config --libs $(PKGS))

SRCS = src/main.c src/window.c src/file.c src/settings.c
OBJS = $(SRCS:src/%.c=build/%.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

build/%.o: src/%.c src/nulla.h | build
	$(CC) $(CFLAGS) -c $< -o $@

build:
	mkdir -p build

debug: clean
	mkdir -p build
	$(CC) $(DEBUG_CFLAGS) $(SRCS) -o $(TARGET) $(DEBUG_LDFLAGS)

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)
	install -Dm644 assets/nulla.svg $(DESTDIR)$(ICONDIR)/nulla.svg

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)
	rm -f $(DESTDIR)$(ICONDIR)/nulla.svg

clean:
	rm -rf build $(TARGET)

.PHONY: all debug install uninstall clean
