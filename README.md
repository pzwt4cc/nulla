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
│   └── highlighter.h
├── package/
│   └── arch/
│       └── PKGBUILD
├── qml/
│   ├── ConfirmModal.qml
│   ├── M3Button.qml
│   ├── M3Switch.qml
│   ├── SettingsModal.qml
│   └── main.qml
├── src/
│   ├── editor_backend.cpp
│   ├── gutter.cpp
│   ├── highlighter.cpp
│   └── main.cpp
├── Makefile
├── README.md
└── resources.qrc
```

## Features

- Open, edit, and save files (`Ctrl+O`, `Ctrl+S`, `Ctrl+Shift+S`, `Ctrl+N`, drag & drop)
- Custom monochrome Material 3 outlined UI with smooth micro-transitions
- Line numbers toggle
- Dark, Light, and System themes
- Built-in syntax highlighting toggle and searchable language selector (with auto-detection)
- Zoom control (`Ctrl++`, `Ctrl+-`, `Ctrl+0`, `Ctrl+Wheel`)

## Dependencies

- `gcc`
- `make`
- `pkgconf`
- `qt6-base`
- `qt6-declarative`

## Build & Run

```sh
make
./nulla [file]
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
