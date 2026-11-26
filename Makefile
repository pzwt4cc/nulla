CXX = g++
TARGET = nulla
PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin
DATADIR = $(PREFIX)/share
APPDIR = $(DATADIR)/applications
ICONDIR_SCALABLE = $(DATADIR)/icons/hicolor/scalable/apps
ICONDIR_512 = $(DATADIR)/icons/hicolor/512x512/apps

PKGS = Qt6Core Qt6Gui Qt6Widgets Qt6Quick Qt6QuickControls2
QT_LIBEXEC = $(shell pkg-config --variable=libexecdir Qt6Core)
MOC = $(QT_LIBEXEC)/moc
RCC = $(QT_LIBEXEC)/rcc

CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -Werror -Wpedantic -fPIC -Iinclude $(shell pkg-config --cflags $(PKGS))
DEBUG_CXXFLAGS = -std=c++17 -g -O0 -Wall -Wextra -Werror -Wpedantic -fPIC -fsanitize=address,undefined -Iinclude $(shell pkg-config --cflags $(PKGS))
LDFLAGS = $(shell pkg-config --libs $(PKGS))
DEBUG_LDFLAGS = -fsanitize=address,undefined $(shell pkg-config --libs $(PKGS))

HEADERS = include/editor_backend.h include/highlighter.h include/gutter.h
MOC_SRCS = build/moc_editor_backend.cpp build/moc_highlighter.cpp build/moc_gutter.cpp
RCC_SRC = build/qrc_resources.cpp
SRCS = src/main.cpp src/editor_backend.cpp src/highlighter.cpp src/gutter.cpp $(MOC_SRCS) $(RCC_SRC)
OBJS = $(patsubst %.cpp,build/%.o,$(notdir $(SRCS)))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

build:
	mkdir -p build

build/moc_%.cpp: include/%.h | build
	$(MOC) -Iinclude $< -o $@

QML_FILES = qml/main.qml qml/M3Button.qml qml/M3Switch.qml qml/SettingsModal.qml qml/ConfirmModal.qml

build/qrc_resources.cpp: resources.qrc $(QML_FILES) assets/nulla.svg | build
	$(RCC) $< -o $@

build/%.o: src/%.cpp $(HEADERS) | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/%.o: build/%.cpp $(HEADERS) | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

debug: clean
	mkdir -p build
	$(MOC) -Iinclude include/editor_backend.h -o build/moc_editor_backend.cpp
	$(MOC) -Iinclude include/highlighter.h -o build/moc_highlighter.cpp
	$(MOC) -Iinclude include/gutter.h -o build/moc_gutter.cpp
	$(RCC) resources.qrc -o build/qrc_resources.cpp
	$(CXX) $(DEBUG_CXXFLAGS) $(SRCS) -o $(TARGET) $(DEBUG_LDFLAGS)

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
