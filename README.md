<p align="center">
  <img src="assets/nulla.svg" alt="Nulla" width="128">
</p>

# Nulla

Minimalist text editor for scripts and configs built with C++17 and Qt 6 Quick (QML), designed around a monochrome Material 3 outlined interface.

## Project Structure

```
.
├── assets/
│   ├── nulla.desktop
│   ├── nulla.png
│   └── nulla.svg
├── include/
│   ├── editor_backend.h
│   ├── gutter.h
│   ├── highlighter.h
│   └── tab_session.h
├── package/
│   └── arch/
│       └── PKGBUILD
├── qml/
│   ├── ConfirmModal.qml
│   ├── EditorPane.qml
│   ├── FindBar.qml
│   ├── M3Button.qml
│   ├── M3Switch.qml
│   ├── SettingsModal.qml
│   ├── StatusBar.qml
│   ├── TabStrip.qml
│   ├── TopBar.qml
│   └── main.qml
├── src/
│   ├── editor_backend.cpp
│   ├── gutter.cpp
│   ├── highlighter.cpp
│   ├── main.cpp
│   └── tab_session.cpp
├── Makefile
├── README.md
└── resources.qrc
```

## Features

- Multi-tab workspace (`Ctrl+T`, `Ctrl+W`, `Ctrl+Tab`, `Ctrl+Shift+Tab`, `Ctrl+1..9`, drag & drop)
- Open, edit, and save files (`Ctrl+O`, `Ctrl+S`, `Ctrl+Shift+S`) with unsaved-changes guard
- Search, Replace, and Go to Line (`Ctrl+F`, `Ctrl+H`, `Ctrl+G`, `F3`, `Shift+F3`)
- Complete editing shortcuts (`Ctrl+Z/Y/Shift+Z`, `Ctrl+X/C/V/A`, `Ctrl+D`, `Ctrl+L`, `Ctrl+Shift+K`, `Ctrl+/`, `Alt+Up/Down`, `Ctrl+Enter`, `Tab`/`Shift+Tab`) with Cyrillic layout support
- Custom monochrome Material 3 outlined UI with adaptive DE/WM window controls and `F11` fullscreen
- Line numbers toggle and Dark / Light / System themes
- Live syntax highlighting with content-based auto-detection on untitled buffers and searchable language picker
- Smooth zoom control (`Ctrl++`, `Ctrl+-`, `Ctrl+0`, `Ctrl+Wheel`) and smooth wheel scrolling

## Dependencies

- `gcc`
- `make`
- `pkgconf`
- `qt6-base`
- `qt6-declarative`

## Build & Run

```sh
make
./nulla [file...]
```

## Install

```sh
sudo make install
```

## Arch Linux Package

```sh
cd package/arch
makepkg -si
```
