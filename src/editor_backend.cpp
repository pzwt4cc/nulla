#include "editor_backend.h"
#include "nulla_core.h"
#include "theme_palette.h"
#include "update_manager.h"

#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QSettings>
#include <QStandardPaths>
#include <QStyleHints>
#include <QTimer>
#include <QUrl>

#include <algorithm>

namespace {

QString pickMonoFontFamily() {
  const QStringList available = QFontDatabase::families();
  const char *const preferred[] = {"JetBrains Mono",
                                   "IBM Plex Mono",
                                   "Cascadia Mono",
                                   "Cascadia Code",
                                   "CaskaydiaCove Nerd Font Mono",
                                   "Fira Code",
                                   "FiraCode Nerd Font Mono",
                                   "Consolas",
                                   "Hack",
                                   "Adwaita Mono",
                                   "Ubuntu Mono",
                                   "Liberation Mono"};
  for (const char *name : preferred) {
    const QString qname = QLatin1String(name);
    if (available.contains(qname, Qt::CaseInsensitive)) {
      return qname;
    }
  }
  return QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
}

QString normalizeFilePath(const QString &rawPath) {
  QString path = rawPath;
  const QUrl url(rawPath);
  if (url.isValid() && url.isLocalFile()) {
    path = url.toLocalFile();
  }
  return QFileInfo(path).absoluteFilePath();
}

}

TabModel::TabModel(QObject *parent) : QAbstractListModel(parent) {}

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

int TabModel::indexOf(TabSession *tab) const { return m_tabs.indexOf(tab); }

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
    : QObject(parent), m_tabModel(this),
      m_showWindowControls(nulla::theme::detectWindowControls()),
      m_monoFontFamily(pickMonoFontFamily()),
      m_themeWatcher(new QFileSystemWatcher(this)),
      m_themeReloadTimer(new QTimer(this)), m_updater(new UpdateManager(this)) {
  m_themeReloadTimer->setSingleShot(true);
  m_themeReloadTimer->setInterval(80);
  connect(m_themeReloadTimer, &QTimer::timeout, this,
          &EditorBackend::onThemeFilesChanged);
  connect(m_themeWatcher, &QFileSystemWatcher::fileChanged, m_themeReloadTimer,
          QOverload<>::of(&QTimer::start));
  connect(m_themeWatcher, &QFileSystemWatcher::directoryChanged,
          m_themeReloadTimer, QOverload<>::of(&QTimer::start));

  initTranslations();
  loadSettings();
  applyUiLanguageStrings();
  rebuildThemePalette();
  updateThemeWatches();
  m_tabModel.append(createSession());
  m_activeTabIndex = 0;

  if (QGuiApplication::styleHints()) {
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
            this, &EditorBackend::onSystemColorSchemeChanged);
  }
  if (QCoreApplication::instance()) {
    QCoreApplication::instance()->installEventFilter(this);
  }
}

void EditorBackend::initTranslations() {
  m_translations.clear();
  m_uiLanguagesModel.clear();

  QDir dir(QStringLiteral(":/i18n"));
  const QStringList files =
      dir.entryList({QStringLiteral("*.json")}, QDir::Files, QDir::Name);
  QVector<QPair<QString, QString>> metaList;

  for (const QString &fileName : files) {
    QFile f(dir.filePath(fileName));
    if (!f.open(QIODevice::ReadOnly)) {
      continue;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) {
      continue;
    }
    const QJsonObject root = doc.object();
    const QJsonObject meta = root.value(QStringLiteral("_meta")).toObject();
    const QString baseId = QFileInfo(fileName).completeBaseName();
    const QString id = meta.value(QStringLiteral("id")).toString(baseId);
    const QString name = meta.value(QStringLiteral("name")).toString(id);

    QVariantMap map;
    for (auto it = root.constBegin(); it != root.constEnd(); ++it) {
      if (it.key() != QStringLiteral("_meta") && it.value().isString()) {
        map.insert(it.key(), it.value().toString());
      }
    }
    m_translations.insert(id, map);
    metaList.append({id, name});
  }

  std::sort(metaList.begin(), metaList.end(), [](const auto &a, const auto &b) {
    if (a.first == QStringLiteral("en"))
      return true;
    if (b.first == QStringLiteral("en"))
      return false;
    return a.first < b.first;
  });

  for (const auto &entry : metaList) {
    QVariantMap item;
    item.insert(QStringLiteral("id"), entry.first);
    item.insert(QStringLiteral("name"), entry.second);
    m_uiLanguagesModel.append(item);
  }
}

void EditorBackend::applyUiLanguageStrings() {
  QVariantMap merged = m_translations.value(QStringLiteral("en"));
  const QVariantMap active = m_translations.value(m_uiLanguage);
  for (auto it = active.constBegin(); it != active.constEnd(); ++it) {
    merged.insert(it.key(), it.value());
  }
  m_uiStrings = merged;
}

void EditorBackend::rebuildThemePalette() {
  m_theme = nulla::theme::buildThemePalette(m_themeMode, m_effectiveDark,
                                            &m_watchedThemeFiles);
}

void EditorBackend::updateThemeWatches() {
  nulla::theme::syncThemeWatcher(m_themeWatcher, m_themeMode,
                                 m_watchedThemeFiles);
}

TabSession *EditorBackend::createSession() {
  auto *tab = new TabSession(this);
  tab->setSyntaxEnabled(m_syntaxEnabled);
  connect(tab, &TabSession::errorOccurred, this, &EditorBackend::errorOccurred);
  connect(tab, &TabSession::languageChanged, this,
          &EditorBackend::languageChanged);
  return tab;
}

TabSession *EditorBackend::activeTab() const {
  return m_tabModel.at(m_activeTabIndex);
}

TabSession *EditorBackend::tabAtOrActive(int index) const {
  return (index >= 0 && index < m_tabModel.count()) ? m_tabModel.at(index)
                                                    : activeTab();
}

QString EditorBackend::workspaceName() const {
  if (m_workspaceRoot.isEmpty()) {
    return QString();
  }
  const QString name = QFileInfo(m_workspaceRoot).fileName();
  return name.isEmpty() ? m_workspaceRoot : name;
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

  const size_t count = nulla_supported_languages_count();
  for (size_t i = 0; i < count; ++i) {
    const uint8_t *idPtr = nullptr;
    size_t idLen = 0;
    const uint8_t *namePtr = nullptr;
    size_t nameLen = 0;
    if (nulla_supported_language_at(i, &idPtr, &idLen, &namePtr, &nameLen)) {
      QVariantMap item;
      item.insert(QStringLiteral("id"),
                  QString::fromUtf8(reinterpret_cast<const char *>(idPtr),
                                    static_cast<qsizetype>(idLen)));
      item.insert(QStringLiteral("name"),
                  QString::fromUtf8(reinterpret_cast<const char *>(namePtr),
                                    static_cast<qsizetype>(nameLen)));
      list.append(item);
    }
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

void EditorBackend::setShowIndentGuides(bool show) {
  if (m_showIndentGuides == show) {
    return;
  }
  m_showIndentGuides = show;
  saveSettings();
  emit settingsChanged();
}

void EditorBackend::setThemeMode(int mode) {
  if (mode < ThemeSystem || mode > ThemeGraphite || m_themeMode == mode) {
    return;
  }
  m_themeMode = mode;
  saveSettings();
  rebuildThemePalette();
  updateThemeWatches();
  propagateThemeAndSyntax();
  emit themeChanged();
  emit settingsChanged();
}

void EditorBackend::setLayoutMode(int mode) {
  if (mode < LayoutMinimal || mode > LayoutExpanded || m_layoutMode == mode) {
    return;
  }
  m_layoutMode = mode;
  saveSettings();
  emit settingsChanged();
}

void EditorBackend::setSidebarOpen(bool open) {
  if (m_sidebarOpen == open) {
    return;
  }
  m_sidebarOpen = open;
  saveSettings();
  emit settingsChanged();
}

void EditorBackend::setSidebarWidth(int width) {
  const int clamped = qBound(140, width, 480);
  if (m_sidebarWidth == clamped) {
    return;
  }
  m_sidebarWidth = clamped;
  saveSettings();
  emit settingsChanged();
}

void EditorBackend::setSidebarScalePercent(int percent) {
  const int clamped = qBound(80, percent, 150);
  if (m_sidebarScalePercent == clamped) {
    return;
  }
  m_sidebarScalePercent = clamped;
  saveSettings();
  emit settingsChanged();
}

void EditorBackend::setExpandedIconSize(int size) {
  const int clamped = qBound(14, size, 28);
  if (m_expandedIconSize == clamped) {
    return;
  }
  m_expandedIconSize = clamped;
  saveSettings();
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

void EditorBackend::setUiLanguage(const QString &lang) {
  const QString target =
      m_translations.contains(lang) ? lang : QStringLiteral("en");
  if (m_uiLanguage == target) {
    return;
  }
  m_uiLanguage = target;
  applyUiLanguageStrings();
  saveSettings();
  emit uiLanguageChanged();
}

void EditorBackend::zoomIn() { setZoomPercent(m_zoomPercent + 10); }

void EditorBackend::zoomOut() { setZoomPercent(m_zoomPercent - 10); }

void EditorBackend::zoomReset() { setZoomPercent(100); }

void EditorBackend::sidebarScaleIn() {
  setSidebarScalePercent(m_sidebarScalePercent + 10);
}

void EditorBackend::sidebarScaleOut() {
  setSidebarScalePercent(m_sidebarScalePercent - 10);
}

void EditorBackend::sidebarScaleReset() { setSidebarScalePercent(100); }

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

void EditorBackend::activateTab(int index) { setActiveTabIndex(index); }

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
  setActiveTabIndex((m_activeTabIndex - 1 + m_tabModel.count()) %
                    m_tabModel.count());
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
    if (!tabs.at(i)->filePath().isEmpty() &&
        tabs.at(i)->filePath() == absPath) {
      return i;
    }
  }
  return -1;
}

bool EditorBackend::loadFile(const QString &path, bool allowMissing) {
  const QString absPath = normalizeFilePath(path);
  if (QFileInfo(absPath).isDir()) {
    return openFolder(absPath);
  }
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
  if (!created || !created->loadFile(absPath, allowMissing)) {
    closeTabImmediate(idx);
    return false;
  }
  return true;
}

bool EditorBackend::openFileDialog() {
  TabSession *cur = activeTab();
  const QString initialDir =
      (cur && !cur->filePath().isEmpty())
          ? QFileInfo(cur->filePath()).absolutePath()
          : (!m_workspaceRoot.isEmpty() ? m_workspaceRoot
                                        : QDir::currentPath());
  const QString chosen = QFileDialog::getOpenFileName(
      nullptr, QStringLiteral("Open File"), initialDir);
  if (chosen.isEmpty()) {
    return false;
  }
  return loadFile(chosen, false);
}

bool EditorBackend::openFolderDialog() {
  TabSession *cur = activeTab();
  const QString initialDir =
      !m_workspaceRoot.isEmpty()
          ? m_workspaceRoot
          : ((cur && !cur->filePath().isEmpty())
                 ? QFileInfo(cur->filePath()).absolutePath()
                 : QDir::currentPath());
  const QString chosen = QFileDialog::getExistingDirectory(
      nullptr, QStringLiteral("Open Folder"), initialDir,
      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
  if (chosen.isEmpty()) {
    return false;
  }
  return openFolder(chosen);
}

bool EditorBackend::openFolder(const QString &path) {
  const QString absPath = normalizeFilePath(path);
  const QFileInfo info(absPath);
  if (!info.exists() || !info.isDir()) {
    return false;
  }
  m_workspaceRoot = absPath;
  m_expandedFolders.clear();
  rebuildFolderEntries();
  if (!m_sidebarOpen) {
    m_sidebarOpen = true;
    saveSettings();
    emit settingsChanged();
  }
  emit folderChanged();
  return true;
}

void EditorBackend::closeFolder() {
  if (m_workspaceRoot.isEmpty() && m_folderEntries.isEmpty()) {
    return;
  }
  m_workspaceRoot.clear();
  m_expandedFolders.clear();
  m_folderEntries.clear();
  emit folderChanged();
}

void EditorBackend::toggleFolderExpanded(const QString &dirPath) {
  if (dirPath.isEmpty()) {
    return;
  }
  if (m_expandedFolders.contains(dirPath)) {
    m_expandedFolders.remove(dirPath);
  } else {
    m_expandedFolders.insert(dirPath);
  }
  rebuildFolderEntries();
  emit folderChanged();
}

void EditorBackend::refreshFolder() {
  if (m_workspaceRoot.isEmpty()) {
    return;
  }
  rebuildFolderEntries();
  emit folderChanged();
}

void EditorBackend::rebuildFolderEntries() {
  m_folderEntries.clear();
  if (m_workspaceRoot.isEmpty()) {
    return;
  }
  int budget = 2500;
  collectFolderLevel(m_workspaceRoot, 0, budget);
}

void EditorBackend::collectFolderLevel(const QString &dirPath, int depth,
                                       int &budget) {
  if (budget <= 0 || depth > 16) {
    return;
  }
  const QDir dir(dirPath);
  const QFileInfoList entries =
      dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden,
                        QDir::DirsFirst | QDir::IgnoreCase | QDir::Name);
  for (const QFileInfo &fi : entries) {
    if (budget <= 0) {
      break;
    }
    const QString name = fi.fileName();
    if (name == QStringLiteral(".git")) {
      continue;
    }
    const bool isDir = fi.isDir();
    const QString abs = fi.absoluteFilePath();
    const bool expanded = isDir && m_expandedFolders.contains(abs);

    QVariantMap item;
    item.insert(QStringLiteral("path"), abs);
    item.insert(QStringLiteral("name"), name);
    item.insert(QStringLiteral("isDir"), isDir);
    item.insert(QStringLiteral("depth"), depth);
    item.insert(QStringLiteral("expanded"), expanded);
    m_folderEntries.append(item);
    --budget;

    if (expanded) {
      collectFolderLevel(abs, depth + 1, budget);
    }
  }
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
  const QString initial =
      tab->filePath().isEmpty()
          ? QDir(!m_workspaceRoot.isEmpty() ? m_workspaceRoot
                                            : QDir::currentPath())
                .filePath(QStringLiteral("Untitled"))
          : tab->filePath();
  const QString chosen = QFileDialog::getSaveFileName(
      nullptr, QStringLiteral("Save File"), initial);
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
  int fileIdx = 0;
  for (const QString &p : paths) {
    const QString abs = normalizeFilePath(p);
    if (QFileInfo(abs).isDir()) {
      openFolder(abs);
      continue;
    }
    if (fileIdx == 0 && m_tabModel.count() == 1 &&
        m_tabModel.at(0)->isPristineUntitled()) {
      m_tabModel.at(0)->setPendingInitialFile(p, true);
    } else {
      newTab(p);
    }
    ++fileIdx;
  }
  setActiveTabIndex(0);
}

bool EditorBackend::eventFilter(QObject *watched, QEvent *event) {
  if (watched == QCoreApplication::instance() &&
      (event->type() == QEvent::ApplicationPaletteChange ||
       event->type() == QEvent::ThemeChange)) {
    onSystemColorSchemeChanged();
  }
  return QObject::eventFilter(watched, event);
}

void EditorBackend::onSystemColorSchemeChanged() {
  if (m_themeMode == ThemeSystem) {
    const QVariantMap oldTheme = m_theme;
    const bool oldDark = m_effectiveDark;
    rebuildThemePalette();
    if (m_theme != oldTheme || m_effectiveDark != oldDark) {
      propagateThemeAndSyntax();
      emit themeChanged();
    }
  }
}

void EditorBackend::onThemeFilesChanged() {
  if (m_themeMode == ThemeSystem) {
    const QVariantMap oldTheme = m_theme;
    const bool oldDark = m_effectiveDark;
    rebuildThemePalette();
    updateThemeWatches();
    if (m_theme != oldTheme || m_effectiveDark != oldDark) {
      propagateThemeAndSyntax();
      emit themeChanged();
    }
  }
}

void EditorBackend::propagateThemeAndSyntax() {
  const auto &tabs = m_tabModel.tabs();
  for (TabSession *tab : tabs) {
    tab->setSyntaxEnabled(m_syntaxEnabled);
  }
}

void EditorBackend::loadSettings() {
  const QString cfgDir =
      QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
  const QString iniPath =
      QDir(cfgDir).filePath(QStringLiteral("nulla/settings.ini"));
  QSettings s(iniPath, QSettings::IniFormat);
  s.beginGroup(QStringLiteral("editor"));
  m_showLineNumbers = s.value(QStringLiteral("line_numbers"), true).toBool();
  m_showIndentGuides = s.value(QStringLiteral("indent_guides"), true).toBool();
  const int mode = s.value(QStringLiteral("theme"), ThemeDark).toInt();
  m_themeMode =
      (mode >= ThemeSystem && mode <= ThemeGraphite) ? mode : ThemeDark;
  const int layout =
      s.value(QStringLiteral("layout_mode"), LayoutMinimal).toInt();
  m_layoutMode = (layout >= LayoutMinimal && layout <= LayoutExpanded)
                     ? layout
                     : LayoutMinimal;
  m_sidebarOpen = s.value(QStringLiteral("sidebar_open"), true).toBool();
  m_sidebarWidth =
      qBound(140, s.value(QStringLiteral("sidebar_width"), 180).toInt(), 480);
  m_sidebarScalePercent =
      qBound(80, s.value(QStringLiteral("sidebar_scale"), 100).toInt(), 150);
  m_expandedIconSize =
      qBound(14, s.value(QStringLiteral("expanded_icon_size"), 18).toInt(), 28);
  m_syntaxEnabled = s.value(QStringLiteral("syntax"), true).toBool();
  m_zoomPercent = qBound(50, s.value(QStringLiteral("zoom"), 100).toInt(), 300);
  const QString lang =
      s.value(QStringLiteral("ui_language"), QStringLiteral("en")).toString();
  m_uiLanguage = m_translations.contains(lang) ? lang : QStringLiteral("en");
  s.endGroup();
}

void EditorBackend::saveSettings() const {
  const QString cfgDir =
      QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
  const QString iniPath =
      QDir(cfgDir).filePath(QStringLiteral("nulla/settings.ini"));
  QDir().mkpath(QFileInfo(iniPath).absolutePath());
  QSettings s(iniPath, QSettings::IniFormat);
  s.beginGroup(QStringLiteral("editor"));
  s.setValue(QStringLiteral("line_numbers"), m_showLineNumbers);
  s.setValue(QStringLiteral("indent_guides"), m_showIndentGuides);
  s.setValue(QStringLiteral("theme"), m_themeMode);
  s.setValue(QStringLiteral("layout_mode"), m_layoutMode);
  s.setValue(QStringLiteral("sidebar_open"), m_sidebarOpen);
  s.setValue(QStringLiteral("sidebar_width"), m_sidebarWidth);
  s.setValue(QStringLiteral("sidebar_scale"), m_sidebarScalePercent);
  s.setValue(QStringLiteral("expanded_icon_size"), m_expandedIconSize);
  s.setValue(QStringLiteral("syntax"), m_syntaxEnabled);
  s.setValue(QStringLiteral("zoom"), m_zoomPercent);
  s.setValue(QStringLiteral("ui_language"), m_uiLanguage);
  s.endGroup();
  s.sync();
}
