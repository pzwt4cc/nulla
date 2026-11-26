#include "editor_backend.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QSettings>
#include <QStandardPaths>
#include <QStyleHints>
#include <QUrl>

static bool detectDesktopEnvironment() {
    if (!qEnvironmentVariableIsEmpty("HYPRLAND_INSTANCE_SIGNATURE") ||
        !qEnvironmentVariableIsEmpty("SWAYSOCK") ||
        !qEnvironmentVariableIsEmpty("I3SOCK") ||
        !qEnvironmentVariableIsEmpty("BSPWM_SOCKET")) {
        return false;
    }

    const QString combined = (qEnvironmentVariable("XDG_CURRENT_DESKTOP") + QLatin1Char(' ') +
                              qEnvironmentVariable("XDG_SESSION_DESKTOP") + QLatin1Char(' ') +
                              qEnvironmentVariable("DESKTOP_SESSION")).toLower();

    const char * const wms[] = {
        "hyprland", "sway", "river", "niri", "wayfire", "labwc",
        "i3", "bspwm", "dwm", "awesome", "qtile", "xmonad",
        "openbox", "herbstluftwm", "leftwm", "spectrwm", "fluxbox", "icewm", "jwm"
    };
    for (const char *wm : wms) {
        if (combined.contains(QLatin1String(wm))) {
            return false;
        }
    }

    const char * const des[] = {
        "gnome", "ubuntu", "kde", "plasma", "xfce", "cinnamon",
        "mate", "lxqt", "lxde", "budgie", "pantheon", "deepin",
        "cosmic", "unity", "enlightenment", "ukui", "cutefish"
    };
    for (const char *de : des) {
        if (combined.contains(QLatin1String(de))) {
            return true;
        }
    }

    return false;
}

static QString pickMonoFontFamily() {
    const QStringList available = QFontDatabase::families();
    const char * const preferred[] = {
        "JetBrains Mono",
        "IBM Plex Mono",
        "Cascadia Mono",
        "CaskaydiaCove Nerd Font Mono",
        "Fira Code",
        "FiraCode Nerd Font Mono",
        "Hack",
        "Adwaita Mono",
        "Ubuntu Mono",
        "Liberation Mono"
    };
    for (const char *name : preferred) {
        const QString qname = QLatin1String(name);
        if (available.contains(qname, Qt::CaseInsensitive)) {
            return qname;
        }
    }
    return QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
}

static QString normalizeFilePath(const QString &rawPath) {
    QString path = rawPath;
    const QUrl url(rawPath);
    if (url.isValid() && url.isLocalFile()) {
        path = url.toLocalFile();
    }
    return QFileInfo(path).absoluteFilePath();
}

TabModel::TabModel(QObject *parent)
    : QAbstractListModel(parent) {}

int TabModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return m_tabs.size();
}

QVariant TabModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_tabs.size()) {
        return QVariant();
    }
    if (role == SessionRole) {
        return QVariant::fromValue<QObject *>(m_tabs.at(index.row()));
    }
    return QVariant();
}

QHash<int, QByteArray> TabModel::roleNames() const {
    return {{SessionRole, QByteArrayLiteral("session")}};
}

TabSession *TabModel::at(int index) const {
    if (index < 0 || index >= m_tabs.size()) {
        return nullptr;
    }
    return m_tabs.at(index);
}

int TabModel::indexOf(TabSession *tab) const {
    return m_tabs.indexOf(tab);
}

void TabModel::append(TabSession *tab) {
    const int row = m_tabs.size();
    beginInsertRows(QModelIndex(), row, row);
    m_tabs.append(tab);
    endInsertRows();
}

TabSession *TabModel::takeAt(int index) {
    if (index < 0 || index >= m_tabs.size()) {
        return nullptr;
    }
    beginRemoveRows(QModelIndex(), index, index);
    TabSession *removed = m_tabs.takeAt(index);
    endRemoveRows();
    return removed;
}

EditorBackend::EditorBackend(QObject *parent)
    : QObject(parent),
      m_tabModel(this),
      m_showWindowControls(detectDesktopEnvironment()),
      m_monoFontFamily(pickMonoFontFamily()) {
    loadSettings();
    m_tabModel.append(createSession());
    m_activeTabIndex = 0;

    if (QGuiApplication::styleHints()) {
        connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
                this, &EditorBackend::onSystemColorSchemeChanged);
    }
}

TabSession *EditorBackend::createSession() {
    auto *tab = new TabSession(this);
    tab->applyThemeAndSyntax(m_syntaxEnabled, effectiveDark());
    connect(tab, &TabSession::errorOccurred, this, &EditorBackend::errorOccurred);
    connect(tab, &TabSession::languageChanged, this, &EditorBackend::languageChanged);
    return tab;
}

TabSession *EditorBackend::activeTab() const {
    return m_tabModel.at(m_activeTabIndex);
}

TabSession *EditorBackend::tabAtOrActive(int index) const {
    return (index >= 0 && index < m_tabModel.count()) ? m_tabModel.at(index) : activeTab();
}

bool EditorBackend::effectiveDark() const {
    if (m_themeMode == ThemeDark) {
        return true;
    }
    if (m_themeMode == ThemeLight) {
        return false;
    }
    if (QGuiApplication::styleHints()) {
        return QGuiApplication::styleHints()->colorScheme() != Qt::ColorScheme::Light;
    }
    return true;
}

QString EditorBackend::languageId() const {
    TabSession *tab = activeTab();
    return tab ? tab->languageId() : QStringLiteral("auto");
}

QVariantList EditorBackend::languagesModel() const {
    QVariantList list;
    QVariantMap autoItem;
    autoItem.insert(QStringLiteral("id"), QStringLiteral("auto"));
    autoItem.insert(QStringLiteral("name"), QStringLiteral("Auto"));
    list.append(autoItem);

    const QVector<NullaLangDef> langs = NullaHighlighter::supportedLanguages();
    for (const auto &lang : langs) {
        QVariantMap item;
        item.insert(QStringLiteral("id"), lang.id);
        item.insert(QStringLiteral("name"), lang.name);
        list.append(item);
    }
    return list;
}

void EditorBackend::setActiveTabIndex(int index) {
    if (m_tabModel.count() == 0) {
        return;
    }
    const int clamped = qBound(0, index, m_tabModel.count() - 1);
    if (m_activeTabIndex == clamped) {
        return;
    }
    m_activeTabIndex = clamped;
    emit activeTabChanged();
    emit languageChanged();
}

void EditorBackend::setShowLineNumbers(bool show) {
    if (m_showLineNumbers == show) {
        return;
    }
    m_showLineNumbers = show;
    saveSettings();
    emit settingsChanged();
}

void EditorBackend::setThemeMode(int mode) {
    if (mode < ThemeSystem || mode > ThemeDark || m_themeMode == mode) {
        return;
    }
    m_themeMode = mode;
    saveSettings();
    propagateThemeAndSyntax();
    emit themeChanged();
    emit settingsChanged();
}

void EditorBackend::setSyntaxEnabled(bool enabled) {
    if (m_syntaxEnabled == enabled) {
        return;
    }
    m_syntaxEnabled = enabled;
    saveSettings();
    propagateThemeAndSyntax();
    emit settingsChanged();
}

void EditorBackend::setLanguageId(const QString &id) {
    if (TabSession *tab = activeTab()) {
        tab->setLanguageId(id);
    }
}

void EditorBackend::setZoomPercent(int percent) {
    const int clamped = qBound(50, percent, 300);
    if (m_zoomPercent == clamped) {
        return;
    }
    m_zoomPercent = clamped;
    saveSettings();
    emit settingsChanged();
}

void EditorBackend::zoomIn() {
    setZoomPercent(m_zoomPercent + 10);
}

void EditorBackend::zoomOut() {
    setZoomPercent(m_zoomPercent - 10);
}

void EditorBackend::zoomReset() {
    setZoomPercent(100);
}

int EditorBackend::newTab(const QString &initialFilePath) {
    TabSession *tab = createSession();
    if (!initialFilePath.isEmpty()) {
        tab->setPendingInitialFile(initialFilePath, true);
    }
    m_tabModel.append(tab);
    const int newIdx = m_tabModel.count() - 1;
    m_activeTabIndex = newIdx;
    emit tabCountChanged();
    emit activeTabChanged();
    emit languageChanged();
    return newIdx;
}

void EditorBackend::activateTab(int index) {
    setActiveTabIndex(index);
}

void EditorBackend::nextTab() {
    if (m_tabModel.count() <= 1) {
        return;
    }
    setActiveTabIndex((m_activeTabIndex + 1) % m_tabModel.count());
}

void EditorBackend::prevTab() {
    if (m_tabModel.count() <= 1) {
        return;
    }
    setActiveTabIndex((m_activeTabIndex - 1 + m_tabModel.count()) % m_tabModel.count());
}

void EditorBackend::closeTabImmediate(int index) {
    if (index < 0 || index >= m_tabModel.count()) {
        return;
    }
    if (m_tabModel.count() == 1) {
        m_tabModel.at(0)->resetEmpty();
        return;
    }
    TabSession *removed = m_tabModel.takeAt(index);
    if (removed) {
        removed->deleteLater();
    }
    if (m_activeTabIndex >= m_tabModel.count()) {
        m_activeTabIndex = m_tabModel.count() - 1;
    } else if (index < m_activeTabIndex) {
        m_activeTabIndex--;
    }
    emit tabCountChanged();
    emit activeTabChanged();
    emit languageChanged();
}

bool EditorBackend::isTabModified(int index) const {
    TabSession *tab = m_tabModel.at(index);
    return tab ? tab->modified() : false;
}

bool EditorBackend::isActivePristineUntitled() const {
    TabSession *tab = activeTab();
    return tab ? tab->isPristineUntitled() : false;
}

QString EditorBackend::tabFileName(int index) const {
    TabSession *tab = m_tabModel.at(index);
    return tab ? tab->fileName() : QStringLiteral("Untitled");
}

int EditorBackend::firstModifiedTabIndex() const {
    const auto &tabs = m_tabModel.tabs();
    for (int i = 0; i < tabs.size(); ++i) {
        if (tabs.at(i)->modified()) {
            return i;
        }
    }
    return -1;
}

int EditorBackend::findOpenTabByPath(const QString &absPath) const {
    const auto &tabs = m_tabModel.tabs();
    for (int i = 0; i < tabs.size(); ++i) {
        if (!tabs.at(i)->filePath().isEmpty() && tabs.at(i)->filePath() == absPath) {
            return i;
        }
    }
    return -1;
}

bool EditorBackend::loadFile(const QString &path, bool allowMissing) {
    const QString absPath = normalizeFilePath(path);
    const int existing = findOpenTabByPath(absPath);
    if (existing >= 0) {
        setActiveTabIndex(existing);
        return true;
    }
    TabSession *cur = activeTab();
    if (cur && cur->isPristineUntitled()) {
        return cur->loadFile(absPath, allowMissing);
    }
    const int idx = newTab();
    TabSession *created = m_tabModel.at(idx);
    return created ? created->loadFile(absPath, allowMissing) : false;
}

bool EditorBackend::openFileDialog() {
    TabSession *cur = activeTab();
    const QString initialDir = (cur && !cur->filePath().isEmpty())
                                   ? QFileInfo(cur->filePath()).absolutePath()
                                   : QDir::currentPath();
    const QString chosen = QFileDialog::getOpenFileName(nullptr,
                                                        QStringLiteral("Open File"),
                                                        initialDir);
    if (chosen.isEmpty()) {
        return false;
    }
    return loadFile(chosen, false);
}

bool EditorBackend::saveFile(int index) {
    TabSession *tab = tabAtOrActive(index);
    if (!tab) {
        return false;
    }
    if (tab->filePath().isEmpty()) {
        return saveFileAsDialog(index);
    }
    return tab->writeToFile(tab->filePath());
}

bool EditorBackend::saveFileAsDialog(int index) {
    TabSession *tab = tabAtOrActive(index);
    if (!tab) {
        return false;
    }
    const QString initial = tab->filePath().isEmpty()
                                ? QDir(QDir::currentPath()).filePath(QStringLiteral("Untitled"))
                                : tab->filePath();
    const QString chosen = QFileDialog::getSaveFileName(nullptr,
                                                        QStringLiteral("Save File"),
                                                        initial);
    if (chosen.isEmpty()) {
        return false;
    }
    return tab->writeToFile(chosen);
}

void EditorBackend::discardTabChanges(int index) {
    if (TabSession *tab = tabAtOrActive(index)) {
        tab->discardChanges();
    }
}

void EditorBackend::setInitialFiles(const QStringList &paths) {
    if (paths.isEmpty()) {
        return;
    }
    for (int i = 0; i < paths.size(); ++i) {
        if (i == 0 && m_tabModel.count() == 1 && m_tabModel.at(0)->isPristineUntitled()) {
            m_tabModel.at(0)->setPendingInitialFile(paths.at(i), true);
        } else {
            newTab(paths.at(i));
        }
    }
    setActiveTabIndex(0);
}

void EditorBackend::onSystemColorSchemeChanged() {
    if (m_themeMode == ThemeSystem) {
        propagateThemeAndSyntax();
        emit themeChanged();
    }
}

void EditorBackend::propagateThemeAndSyntax() {
    const bool dark = effectiveDark();
    const auto &tabs = m_tabModel.tabs();
    for (TabSession *tab : tabs) {
        tab->applyThemeAndSyntax(m_syntaxEnabled, dark);
    }
}

void EditorBackend::loadSettings() {
    const QString cfgDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    const QString iniPath = QDir(cfgDir).filePath(QStringLiteral("nulla/settings.ini"));
    QSettings s(iniPath, QSettings::IniFormat);
    s.beginGroup(QStringLiteral("editor"));
    m_showLineNumbers = s.value(QStringLiteral("line_numbers"), true).toBool();
    const int mode = s.value(QStringLiteral("theme"), ThemeDark).toInt();
    m_themeMode = (mode >= ThemeSystem && mode <= ThemeDark) ? mode : ThemeDark;
    m_syntaxEnabled = s.value(QStringLiteral("syntax"), true).toBool();
    m_zoomPercent = qBound(50, s.value(QStringLiteral("zoom"), 100).toInt(), 300);
    s.endGroup();
}

void EditorBackend::saveSettings() const {
    const QString cfgDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    const QString iniPath = QDir(cfgDir).filePath(QStringLiteral("nulla/settings.ini"));
    QDir().mkpath(QFileInfo(iniPath).absolutePath());
    QSettings s(iniPath, QSettings::IniFormat);
    s.beginGroup(QStringLiteral("editor"));
    s.setValue(QStringLiteral("line_numbers"), m_showLineNumbers);
    s.setValue(QStringLiteral("theme"), m_themeMode);
    s.setValue(QStringLiteral("syntax"), m_syntaxEnabled);
    s.setValue(QStringLiteral("zoom"), m_zoomPercent);
    s.endGroup();
    s.sync();
}
