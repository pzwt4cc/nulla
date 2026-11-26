<div align="center">
  <img src="assets/nulla.svg" alt="Nulla" width="128" />
  <h1 align="center">Nulla</h1>
  <p>
    <strong>Minimalist text and code editor for scripts and configs</strong>
  </p>
  <p>
    Built with C++17 and Qt 6 Quick (QML), designed around a monochrome Material 3 outlined interface.
  </p>
  <p>
    <a href="https://github.com/pzwt4cc/nulla/releases/latest">Windows Installer (.msi)</a>
    &nbsp;·&nbsp;
    <a href="#features">Features</a>
    &nbsp;·&nbsp;
    <a href="#linux-build--run">Linux Build</a>
    &nbsp;·&nbsp;
    <a href="#windows-build--msi-auto-builder">Windows Build</a>
    &nbsp;·&nbsp;
    <a href="LICENSE">WTFPL License</a>
  </p>
</div>

---

## Features

- Multi-tab workspace (`Ctrl+T`, `Ctrl+W`, `Ctrl+Tab`, `Ctrl+Shift+Tab`, `Ctrl+1..9`, drag & drop)
- Open, edit, and save files (`Ctrl+O`, `Ctrl+S`, `Ctrl+Shift+S`) with unsaved-changes guard
- Search, Replace, and Go to Line (`Ctrl+F`, `Ctrl+H`, `Ctrl+G`, `F3`, `Shift+F3`)
- Complete editing shortcuts (`Ctrl+Z/Y/Shift+Z`, `Ctrl+X/C/V/A`, `Ctrl+D`, `Ctrl+L`, `Ctrl+Shift+K`, `Ctrl+/`, `Alt+Up/Down`, `Ctrl+Enter`, `Tab`/`Shift+Tab`) with Cyrillic layout support
- Custom monochrome Material 3 outlined UI with adaptive DE/WM/Windows window controls and `F11` fullscreen
- Dark, Light, and adaptive System themes (follows `matugen`, `pywal`, GTK/KDE/qt6ct on Linux and Light/Dark + DWM accent color on Windows)
- 13 UI languages (`Settings → Language`) and live syntax highlighting for 15 languages with searchable picker
- Smooth zoom control (`Ctrl++`, `Ctrl+-`, `Ctrl+0`, `Ctrl+Wheel`) and smooth wheel scrolling

## Project Structure

```text
.
├── .github/
│   └── workflows/
│       └── windows-msi.yml
├── assets/
│   ├── nulla.desktop
│   ├── nulla.ico
│   ├── nulla.png
│   └── nulla.svg
├── i18n/
│   └── de.json ... zh.json (13 UI languages)
├── include/
│   ├── editor_backend.h
│   ├── gutter.h
│   ├── highlighter.h
│   └── tab_session.h
├── package/
│   ├── arch/
│   │   └── PKGBUILD
│   └── windows/
│       ├── License.rtf
│       ├── Nulla.wxs
│       ├── build-msi.ps1
│       └── nulla.rc
├── qml/
│   ├── ConfirmModal.qml
│   ├── EditorContextMenu.qml
│   ├── EditorPane.qml
│   ├── FindBar.qml
│   ├── LanguageModal.qml
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
├── CMakeLists.txt
├── LICENSE
├── Makefile
├── README.md
└── resources.qrc
```

## Linux Build & Run

Dependencies: `gcc`, `make`, `pkgconf`, `qt6-base`, `qt6-declarative`.

```sh
make
./nulla [file...]
sudo make install
```

### Arch Linux Package

```sh
cd package/arch
makepkg -si
```

## Windows Build & MSI Auto-Builder

### Automated CI (GitHub Actions)

Workflow [.github/workflows/windows-msi.yml](.github/workflows/windows-msi.yml) builds `Nulla-<version>-win64.msi` automatically on `windows-latest`:

- on every push to `main` (download from **Actions → Windows MSI → Artifacts → `nulla-windows-msi`**);
- on tags matching `v*` (also attaches the `.msi` directly to **GitHub Releases**);
- manually via **Actions → Windows MSI → Run workflow** (or `gh workflow run windows-msi.yml`).

### Local Windows Build

Requires Qt 6 (`qtdeclarative`, `qtshadertools`, `qtsvg`), MSVC + CMake + Ninja, and WiX Toolset 5 (`WixToolset.UI.wixext`).

```powershell
.\package\windows\build-msi.ps1
```

The installer appears in `dist\Nulla-0.2.0-win64.msi` with selectable install location and optional components (Start Menu shortcut, Desktop shortcut, `PATH`, Explorer context menu, and Open With registration).

## License

WTFPL — see [LICENSE](LICENSE).
