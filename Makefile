CXX := g++
CARGO ?= cargo
QT_LIBEXEC := $(shell pkg-config --variable=libexecdir Qt6Core)
MOC := $(QT_LIBEXEC)/moc
RCC := $(QT_LIBEXEC)/rcc
QMLTYPEREGISTRAR := $(QT_LIBEXEC)/qmltyperegistrar
QT_METATYPES_DIR := $(shell pkg-config --variable=prefix Qt6Core)/lib/qt6/metatypes
APP_VERSION := $(shell tr ' ' '\n' < CMakeLists.txt | grep -A1 '^VERSION$$' | tail -n1)

QT_PKGS := Qt6Widgets Qt6QuickControls2 Qt6Quick Qt6Qml Qt6Network Qt6Gui Qt6Core
QT_CFLAGS := $(shell pkg-config --cflags $(QT_PKGS))
QT_LIBS := $(shell pkg-config --libs $(QT_PKGS))

CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Werror -Wpedantic -MMD -MP -fPIC -Iinclude -DNULLA_APP_VERSION=\"$(APP_VERSION)\" $(QT_CFLAGS)
TARGET := nulla
BUILD_DIR := build
RUST_LIB := core/target/release/libnulla_core.a
RUST_SRCS := $(shell find core/src -name '*.rs')

SRCS := src/main.cpp src/editor_backend.cpp src/tab_session.cpp src/editor_view.cpp src/theme_palette.cpp src/update_manager.cpp
MOC_HDRS := include/editor_backend.h include/tab_session.h include/editor_view.h include/update_manager.h

OBJS := $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))
MOC_OBJS := $(patsubst include/%.h,$(BUILD_DIR)/moc_%.o,$(MOC_HDRS))
QRC_OBJ := $(BUILD_DIR)/qrc_resources.o
DEPS := $(OBJS:.o=.d) $(MOC_OBJS:.o=.d) $(QRC_OBJ:.o=.d)

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
DATADIR ?= $(PREFIX)/share

all: $(TARGET) compile_commands.json

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(RUST_LIB): core/Cargo.toml $(RUST_SRCS)
	$(CARGO) build --release --manifest-path core/Cargo.toml

$(BUILD_DIR)/%.o: src/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/moc_%.cpp: include/%.h | $(BUILD_DIR)
	$(MOC) -Iinclude $(QT_CFLAGS) $< -o $@

$(BUILD_DIR)/moc_%.o: $(BUILD_DIR)/moc_%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/qrc_resources.cpp: resources.qrc $(shell find qml -maxdepth 1 -name '*.qml') $(wildcard i18n/*.json) assets/nulla.svg | $(BUILD_DIR)
	$(RCC) $< -o $@

$(QRC_OBJ): $(BUILD_DIR)/qrc_resources.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJS) $(MOC_OBJS) $(QRC_OBJ) $(RUST_LIB)
	$(CXX) $^ -o $@ $(QT_LIBS) -lpthread -ldl -lm

compile_commands.json: Makefile
	@printf '[\n' > $@
	@first=1; for src in $(SRCS); do \
		if [ $$first -eq 0 ]; then printf ',\n' >> $@; fi; \
		first=0; \
		printf '  {\n    "directory": "%s",\n    "command": "%s %s -c %s",\n    "file": "%s/%s"\n  }' \
			"$(CURDIR)" "$(CXX)" "$(CXXFLAGS)" "$$src" "$(CURDIR)" "$$src" >> $@; \
	done
	@printf '\n]\n' >> $@

qmltypes: $(MOC_HDRS) | $(BUILD_DIR)
	mkdir -p qml/Nulla
	$(MOC) -Iinclude $(QT_CFLAGS) --output-json -o $(BUILD_DIR)/moc_editor_backend.cpp include/editor_backend.h
	$(MOC) -Iinclude $(QT_CFLAGS) --output-json -o $(BUILD_DIR)/moc_tab_session.cpp include/tab_session.h
	$(MOC) -Iinclude $(QT_CFLAGS) --output-json -o $(BUILD_DIR)/moc_editor_view.cpp include/editor_view.h
	$(MOC) -Iinclude $(QT_CFLAGS) --output-json -o $(BUILD_DIR)/moc_update_manager.cpp include/update_manager.h
	$(QMLTYPEREGISTRAR) --import-name Nulla --major-version 1 --minor-version 0 \
		--foreign-types=$(QT_METATYPES_DIR)/qt6core_metatypes.json,$(QT_METATYPES_DIR)/qt6gui_metatypes.json,$(QT_METATYPES_DIR)/qt6qml_metatypes.json,$(QT_METATYPES_DIR)/qt6quick_metatypes.json \
		--generate-qmltypes=qml/Nulla/nulla.qmltypes \
		-o $(BUILD_DIR)/nulla_qmltyperegistrations.cpp \
		$(BUILD_DIR)/moc_editor_backend.cpp.json $(BUILD_DIR)/moc_tab_session.cpp.json $(BUILD_DIR)/moc_editor_view.cpp.json $(BUILD_DIR)/moc_update_manager.cpp.json
	python3 -c "p='qml/Nulla/nulla.qmltypes'; data=open(p).read(); open(p,'w').write(''.join(l for l in data.splitlines(True) if not l.startswith('//')))"

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)
	install -Dm644 assets/nulla.desktop $(DESTDIR)$(DATADIR)/applications/nulla.desktop
	install -Dm644 assets/nulla.svg $(DESTDIR)$(DATADIR)/icons/hicolor/scalable/apps/nulla.svg
	install -Dm644 assets/nulla.png $(DESTDIR)$(DATADIR)/icons/hicolor/512x512/apps/nulla.png

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)
	rm -f $(DESTDIR)$(DATADIR)/applications/nulla.desktop
	rm -f $(DESTDIR)$(DATADIR)/icons/hicolor/scalable/apps/nulla.svg
	rm -f $(DESTDIR)$(DATADIR)/icons/hicolor/512x512/apps/nulla.png

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

-include $(DEPS)

.PHONY: all qmltypes install uninstall clean
