#ifndef NULLA_EDITOR_BACKEND_H
#define NULLA_EDITOR_BACKEND_H

#include "tab_session.h"

#include <QAbstractListModel>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVector>

class TabModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles {
        SessionRole = Qt::UserRole + 1
    };

    explicit TabModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
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
    Q_PROPERTY(TabModel *tabModel READ tabModel CONSTANT)
    Q_PROPERTY(int activeTabIndex READ activeTabIndex WRITE setActiveTabIndex NOTIFY activeTabChanged)
    Q_PROPERTY(TabSession *activeTab READ activeTab NOTIFY activeTabChanged)
    Q_PROPERTY(int tabCount READ tabCount NOTIFY tabCountChanged)
    Q_PROPERTY(bool showLineNumbers READ showLineNumbers WRITE setShowLineNumbers NOTIFY settingsChanged)
    Q_PROPERTY(int themeMode READ themeMode WRITE setThemeMode NOTIFY themeChanged)
    Q_PROPERTY(bool effectiveDark READ effectiveDark NOTIFY themeChanged)
    Q_PROPERTY(bool syntaxEnabled READ syntaxEnabled WRITE setSyntaxEnabled NOTIFY settingsChanged)
    Q_PROPERTY(QString languageId READ languageId WRITE setLanguageId NOTIFY languageChanged)
    Q_PROPERTY(int zoomPercent READ zoomPercent WRITE setZoomPercent NOTIFY settingsChanged)
    Q_PROPERTY(QVariantList languagesModel READ languagesModel CONSTANT)
    Q_PROPERTY(bool showWindowControls READ showWindowControls CONSTANT)
    Q_PROPERTY(QString monoFontFamily READ monoFontFamily CONSTANT)

public:
    enum ThemeMode {
        ThemeSystem = 0,
        ThemeLight = 1,
        ThemeDark = 2
    };
    Q_ENUM(ThemeMode)

    explicit EditorBackend(QObject *parent = nullptr);

    TabModel *tabModel() { return &m_tabModel; }
    int activeTabIndex() const { return m_activeTabIndex; }
    TabSession *activeTab() const;
    int tabCount() const { return m_tabModel.count(); }

    bool showLineNumbers() const { return m_showLineNumbers; }
    int themeMode() const { return m_themeMode; }
    bool effectiveDark() const;
    bool syntaxEnabled() const { return m_syntaxEnabled; }
    QString languageId() const;
    int zoomPercent() const { return m_zoomPercent; }
    QVariantList languagesModel() const;
    bool showWindowControls() const { return m_showWindowControls; }
    QString monoFontFamily() const { return m_monoFontFamily; }

    void setActiveTabIndex(int index);
    void setShowLineNumbers(bool show);
    void setThemeMode(int mode);
    void setSyntaxEnabled(bool enabled);
    void setLanguageId(const QString &id);
    void setZoomPercent(int percent);

    Q_INVOKABLE void zoomIn();
    Q_INVOKABLE void zoomOut();
    Q_INVOKABLE void zoomReset();

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
    Q_INVOKABLE bool saveFile(int index = -1);
    Q_INVOKABLE bool saveFileAsDialog(int index = -1);
    Q_INVOKABLE void discardTabChanges(int index = -1);
    Q_INVOKABLE void setInitialFiles(const QStringList &paths);

signals:
    void activeTabChanged();
    void tabCountChanged();
    void settingsChanged();
    void themeChanged();
    void languageChanged();
    void errorOccurred(const QString &title, const QString &message);

private slots:
    void onSystemColorSchemeChanged();

private:
    TabSession *createSession();
    TabSession *tabAtOrActive(int index) const;
    int findOpenTabByPath(const QString &absPath) const;
    void propagateThemeAndSyntax();
    void loadSettings();
    void saveSettings() const;

    TabModel m_tabModel;
    int m_activeTabIndex = 0;

    bool m_showLineNumbers = true;
    int m_themeMode = ThemeDark;
    bool m_syntaxEnabled = true;
    int m_zoomPercent = 100;
    bool m_showWindowControls = false;
    QString m_monoFontFamily;
};

#endif
