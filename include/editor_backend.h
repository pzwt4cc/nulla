#ifndef NULLA_EDITOR_BACKEND_H
#define NULLA_EDITOR_BACKEND_H

#include "highlighter.h"

#include <QObject>
#include <QQuickTextDocument>
#include <QString>
#include <QVariantList>

class EditorBackend : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString filePath READ filePath NOTIFY fileInfoChanged)
    Q_PROPERTY(QString fileName READ fileName NOTIFY fileInfoChanged)
    Q_PROPERTY(bool modified READ modified NOTIFY modifiedChanged)
    Q_PROPERTY(bool showLineNumbers READ showLineNumbers WRITE setShowLineNumbers NOTIFY settingsChanged)
    Q_PROPERTY(int themeMode READ themeMode WRITE setThemeMode NOTIFY themeChanged)
    Q_PROPERTY(bool effectiveDark READ effectiveDark NOTIFY themeChanged)
    Q_PROPERTY(bool syntaxEnabled READ syntaxEnabled WRITE setSyntaxEnabled NOTIFY settingsChanged)
    Q_PROPERTY(QString languageId READ languageId WRITE setLanguageId NOTIFY settingsChanged)
    Q_PROPERTY(QString statusLanguage READ statusLanguage NOTIFY languageStatusChanged)
    Q_PROPERTY(int zoomPercent READ zoomPercent WRITE setZoomPercent NOTIFY settingsChanged)
    Q_PROPERTY(QVariantList languagesModel READ languagesModel CONSTANT)
    Q_PROPERTY(int lineCount READ lineCount NOTIFY cursorStatsChanged)
    Q_PROPERTY(int cursorLine READ cursorLine NOTIFY cursorStatsChanged)
    Q_PROPERTY(int cursorColumn READ cursorColumn NOTIFY cursorStatsChanged)
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

    QString filePath() const { return m_filePath; }
    QString fileName() const;
    bool modified() const { return m_modified; }
    bool showLineNumbers() const { return m_showLineNumbers; }
    int themeMode() const { return m_themeMode; }
    bool effectiveDark() const;
    bool syntaxEnabled() const { return m_syntaxEnabled; }
    QString languageId() const { return m_languageId; }
    QString statusLanguage() const;
    int zoomPercent() const { return m_zoomPercent; }
    QVariantList languagesModel() const;
    int lineCount() const;
    int cursorLine() const { return m_cursorLine; }
    int cursorColumn() const { return m_cursorColumn; }
    bool showWindowControls() const { return m_showWindowControls; }
    QString monoFontFamily() const { return m_monoFontFamily; }

    void setShowLineNumbers(bool show);
    void setThemeMode(int mode);
    void setSyntaxEnabled(bool enabled);
    void setLanguageId(const QString &id);
    void setZoomPercent(int percent);

    Q_INVOKABLE void attachDocument(QQuickTextDocument *doc);
    Q_INVOKABLE void updateCursorPosition(int position);
    Q_INVOKABLE void zoomIn();
    Q_INVOKABLE void zoomOut();
    Q_INVOKABLE void zoomReset();
    Q_INVOKABLE QVariantMap findText(const QString &query, int fromPos, bool backward);
    Q_INVOKABLE QVariantMap matchStats(const QString &query, int selectionStart);

    Q_INVOKABLE bool loadFile(const QString &path, bool allowMissing = false);
    Q_INVOKABLE void newFile();
    Q_INVOKABLE bool openFileDialog();
    Q_INVOKABLE bool saveFile();
    Q_INVOKABLE bool saveFileAsDialog();
    Q_INVOKABLE void discardChanges();
    Q_INVOKABLE void setInitialFile(const QString &path);

signals:
    void fileInfoChanged();
    void documentLoaded();
    void modifiedChanged();
    void settingsChanged();
    void themeChanged();
    void languageStatusChanged();
    void cursorStatsChanged();
    void errorOccurred(const QString &title, const QString &message);

private slots:
    void onDocumentModifiedChanged(bool m);
    void onSystemColorSchemeChanged();

private:
    void loadSettings();
    void saveSettings() const;
    void applyHighlighter();
    bool writeToFile(const QString &path);
    QString resolvedLanguageId() const;

    QQuickTextDocument *m_quickDoc = nullptr;
    NullaHighlighter *m_highlighter = nullptr;
    QString m_filePath;
    QString m_pendingInitialPath;
    bool m_modified = false;
    bool m_loading = false;

    bool m_showLineNumbers = true;
    int m_themeMode = ThemeDark;
    bool m_syntaxEnabled = true;
    QString m_languageId = QStringLiteral("auto");
    int m_zoomPercent = 100;

    int m_cursorLine = 1;
    int m_cursorColumn = 1;
    bool m_showWindowControls = false;
    QString m_monoFontFamily;
};

#endif
