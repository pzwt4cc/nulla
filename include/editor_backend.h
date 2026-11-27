#ifndef NULLA_EDITOR_BACKEND_H
#define NULLA_EDITOR_BACKEND_H

#include "tab_session.h"

#include <QAbstractListModel>
#include <QHash>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>
#include <QtQml/qqmlregistration.h>

class QFileSystemWatcher;
class QTimer;

class TabModel : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("Managed by EditorBackend")

public:
  enum Roles { SessionRole = Qt::UserRole + 1 };

  explicit TabModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  int count() const { return m_tabs.size(); }
  TabSession *at(int index) const;
  int indexOf(TabSession *tab) const;
  void append(TabSession *tab);
  TabSession *takeAt(int index);
  const QVector<TabSession *> &tabs() const { return m_tabs; }

private:
  QVector<TabSession *> m_tabs;
};

class EditorBackend : public QObject {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(TabModel *tabModel READ tabModel CONSTANT)
  Q_PROPERTY(int activeTabIndex READ activeTabIndex WRITE setActiveTabIndex
                 NOTIFY activeTabChanged)
  Q_PROPERTY(TabSession *activeTab READ activeTab NOTIFY activeTabChanged)
  Q_PROPERTY(int tabCount READ tabCount NOTIFY tabCountChanged)
  Q_PROPERTY(bool showLineNumbers READ showLineNumbers WRITE setShowLineNumbers
                 NOTIFY settingsChanged)
  Q_PROPERTY(bool showIndentGuides READ showIndentGuides WRITE
                 setShowIndentGuides NOTIFY settingsChanged)
  Q_PROPERTY(
      int themeMode READ themeMode WRITE setThemeMode NOTIFY themeChanged)
  Q_PROPERTY(
      int layoutMode READ layoutMode WRITE setLayoutMode NOTIFY settingsChanged)
  Q_PROPERTY(bool sidebarOpen READ sidebarOpen WRITE setSidebarOpen NOTIFY
                 settingsChanged)
  Q_PROPERTY(int sidebarWidth READ sidebarWidth WRITE setSidebarWidth NOTIFY
                 settingsChanged)
  Q_PROPERTY(int sidebarScalePercent READ sidebarScalePercent WRITE
                 setSidebarScalePercent NOTIFY settingsChanged)
  Q_PROPERTY(int expandedIconSize READ expandedIconSize WRITE
                 setExpandedIconSize NOTIFY settingsChanged)
  Q_PROPERTY(QString workspaceRoot READ workspaceRoot NOTIFY folderChanged)
  Q_PROPERTY(QString workspaceName READ workspaceName NOTIFY folderChanged)
  Q_PROPERTY(QVariantList folderEntries READ folderEntries NOTIFY folderChanged)
  Q_PROPERTY(bool effectiveDark READ effectiveDark NOTIFY themeChanged)
  Q_PROPERTY(QVariantMap theme READ theme NOTIFY themeChanged)
  Q_PROPERTY(bool syntaxEnabled READ syntaxEnabled WRITE setSyntaxEnabled NOTIFY
                 settingsChanged)
  Q_PROPERTY(QString languageId READ languageId WRITE setLanguageId NOTIFY
                 languageChanged)
  Q_PROPERTY(int zoomPercent READ zoomPercent WRITE setZoomPercent NOTIFY
                 settingsChanged)
  Q_PROPERTY(QVariantList languagesModel READ languagesModel CONSTANT)
  Q_PROPERTY(QString uiLanguage READ uiLanguage WRITE setUiLanguage NOTIFY
                 uiLanguageChanged)
  Q_PROPERTY(QVariantList uiLanguagesModel READ uiLanguagesModel CONSTANT)
  Q_PROPERTY(QVariantMap ui READ ui NOTIFY uiLanguageChanged)
  Q_PROPERTY(bool showWindowControls READ showWindowControls CONSTANT)
  Q_PROPERTY(QString monoFontFamily READ monoFontFamily CONSTANT)

public:
  enum ThemeMode {
    ThemeSystem = 0,
    ThemeLight = 1,
    ThemeDark = 2,
    ThemeGraphite = 3
  };
  Q_ENUM(ThemeMode)

  enum LayoutMode { LayoutMinimal = 0, LayoutExpanded = 1 };
  Q_ENUM(LayoutMode)

  explicit EditorBackend(QObject *parent = nullptr);

  bool eventFilter(QObject *watched, QEvent *event) override;

  TabModel *tabModel() { return &m_tabModel; }
  int activeTabIndex() const { return m_activeTabIndex; }
  TabSession *activeTab() const;
  int tabCount() const { return m_tabModel.count(); }

  bool showLineNumbers() const { return m_showLineNumbers; }
  bool showIndentGuides() const { return m_showIndentGuides; }
  int themeMode() const { return m_themeMode; }
  int layoutMode() const { return m_layoutMode; }
  bool sidebarOpen() const { return m_sidebarOpen; }
  int sidebarWidth() const { return m_sidebarWidth; }
  int sidebarScalePercent() const { return m_sidebarScalePercent; }
  int expandedIconSize() const { return m_expandedIconSize; }
  QString workspaceRoot() const { return m_workspaceRoot; }
  QString workspaceName() const;
  QVariantList folderEntries() const { return m_folderEntries; }
  bool effectiveDark() const { return m_effectiveDark; }
  QVariantMap theme() const { return m_theme; }
  bool syntaxEnabled() const { return m_syntaxEnabled; }
  QString languageId() const;
  int zoomPercent() const { return m_zoomPercent; }
  QVariantList languagesModel() const;
  QString uiLanguage() const { return m_uiLanguage; }
  QVariantList uiLanguagesModel() const { return m_uiLanguagesModel; }
  QVariantMap ui() const { return m_uiStrings; }
  bool showWindowControls() const { return m_showWindowControls; }
  QString monoFontFamily() const { return m_monoFontFamily; }

  void setActiveTabIndex(int index);
  void setShowLineNumbers(bool show);
  void setShowIndentGuides(bool show);
  void setThemeMode(int mode);
  void setLayoutMode(int mode);
  void setSidebarOpen(bool open);
  void setSidebarWidth(int width);
  void setSidebarScalePercent(int percent);
  void setExpandedIconSize(int size);
  void setSyntaxEnabled(bool enabled);
  void setLanguageId(const QString &id);
  void setZoomPercent(int percent);
  void setUiLanguage(const QString &lang);

  Q_INVOKABLE void zoomIn();
  Q_INVOKABLE void zoomOut();
  Q_INVOKABLE void zoomReset();
  Q_INVOKABLE void sidebarScaleIn();
  Q_INVOKABLE void sidebarScaleOut();
  Q_INVOKABLE void sidebarScaleReset();

  Q_INVOKABLE int newTab(const QString &initialFilePath = QString());
  Q_INVOKABLE void activateTab(int index);
  Q_INVOKABLE void nextTab();
  Q_INVOKABLE void prevTab();
  Q_INVOKABLE void closeTabImmediate(int index);
  Q_INVOKABLE bool isTabModified(int index) const;
  Q_INVOKABLE bool isActivePristineUntitled() const;
  Q_INVOKABLE QString tabFileName(int index) const;
  Q_INVOKABLE int firstModifiedTabIndex() const;

  Q_INVOKABLE bool loadFile(const QString &path, bool allowMissing = false);
  Q_INVOKABLE bool openFileDialog();
  Q_INVOKABLE bool openFolderDialog();
  Q_INVOKABLE bool openFolder(const QString &path);
  Q_INVOKABLE void closeFolder();
  Q_INVOKABLE void toggleFolderExpanded(const QString &dirPath);
  Q_INVOKABLE void refreshFolder();
  Q_INVOKABLE bool saveFile(int index = -1);
  Q_INVOKABLE bool saveFileAsDialog(int index = -1);
  Q_INVOKABLE void discardTabChanges(int index = -1);
  Q_INVOKABLE void setInitialFiles(const QStringList &paths);

signals:
  void activeTabChanged();
  void tabCountChanged();
  void settingsChanged();
  void folderChanged();
  void themeChanged();
  void languageChanged();
  void uiLanguageChanged();
  void errorOccurred(const QString &title, const QString &message);

private slots:
  void onSystemColorSchemeChanged();
  void onThemeFilesChanged();

private:
  TabSession *createSession();
  TabSession *tabAtOrActive(int index) const;
  int findOpenTabByPath(const QString &absPath) const;
  void rebuildFolderEntries();
  void collectFolderLevel(const QString &dirPath, int depth, int &budget);
  void propagateThemeAndSyntax();
  void rebuildThemePalette();
  void updateThemeWatches();
  void initTranslations();
  void applyUiLanguageStrings();
  void loadSettings();
  void saveSettings() const;

  TabModel m_tabModel;
  int m_activeTabIndex = 0;

  bool m_showLineNumbers = true;
  bool m_showIndentGuides = true;
  int m_themeMode = ThemeDark;
  int m_layoutMode = LayoutMinimal;
  bool m_sidebarOpen = true;
  int m_sidebarWidth = 180;
  int m_sidebarScalePercent = 100;
  int m_expandedIconSize = 18;
  QString m_workspaceRoot;
  QSet<QString> m_expandedFolders;
  QVariantList m_folderEntries;
  bool m_effectiveDark = true;
  QVariantMap m_theme;
  bool m_syntaxEnabled = true;
  int m_zoomPercent = 100;
  QString m_uiLanguage = QStringLiteral("en");
  QHash<QString, QVariantMap> m_translations;
  QVariantList m_uiLanguagesModel;
  QVariantMap m_uiStrings;
  bool m_showWindowControls = false;
  QString m_monoFontFamily;
  QFileSystemWatcher *m_themeWatcher = nullptr;
  QTimer *m_themeReloadTimer = nullptr;
  QStringList m_watchedThemeFiles;
};

#endif
