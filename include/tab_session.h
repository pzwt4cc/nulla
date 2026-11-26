#ifndef NULLA_TAB_SESSION_H
#define NULLA_TAB_SESSION_H

#include "highlighter.h"

#include <QObject>
#include <QQuickTextDocument>
#include <QString>
#include <QTimer>
#include <QVariantMap>

class TabSession : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString filePath READ filePath NOTIFY fileInfoChanged)
    Q_PROPERTY(QString fileName READ fileName NOTIFY fileInfoChanged)
    Q_PROPERTY(bool modified READ modified NOTIFY modifiedChanged)
    Q_PROPERTY(QString languageId READ languageId WRITE setLanguageId NOTIFY languageChanged)
    Q_PROPERTY(QString statusLanguage READ statusLanguage NOTIFY languageStatusChanged)
    Q_PROPERTY(QString commentPrefix READ commentPrefix NOTIFY languageStatusChanged)
    Q_PROPERTY(int lineCount READ lineCount NOTIFY cursorStatsChanged)
    Q_PROPERTY(int cursorLine READ cursorLine NOTIFY cursorStatsChanged)
    Q_PROPERTY(int cursorColumn READ cursorColumn NOTIFY cursorStatsChanged)

public:
    explicit TabSession(QObject *parent = nullptr);

    QString filePath() const { return m_filePath; }
    QString fileName() const;
    bool modified() const { return m_modified; }
    QString languageId() const { return m_languageId; }
    QString statusLanguage() const;
    QString commentPrefix() const;
    int lineCount() const;
    int cursorLine() const { return m_cursorLine; }
    int cursorColumn() const { return m_cursorColumn; }

    bool isPristineUntitled() const;
    void setLanguageId(const QString &id);
    void setPendingInitialFile(const QString &path, bool allowMissing = true);
    void applyThemeAndSyntax(bool syntaxEnabled, bool darkMode);

    Q_INVOKABLE void attachDocument(QQuickTextDocument *doc);
    Q_INVOKABLE void updateCursorPosition(int position);
    Q_INVOKABLE bool loadFile(const QString &path, bool allowMissing = false);
    Q_INVOKABLE bool writeToFile(const QString &path);
    Q_INVOKABLE void resetEmpty();
    Q_INVOKABLE void discardChanges();
    Q_INVOKABLE QVariantMap findText(const QString &query, int fromPos, bool backward);
    Q_INVOKABLE QVariantMap matchStats(const QString &query, int selectionStart);
    Q_INVOKABLE QVariantMap replaceMatch(const QString &query, const QString &replacement, int selStart, int selEnd);
    Q_INVOKABLE int replaceAllMatches(const QString &query, const QString &replacement);
    Q_INVOKABLE int lineStartPosition(int lineNumber) const;

signals:
    void fileInfoChanged();
    void modifiedChanged();
    void languageChanged();
    void languageStatusChanged();
    void cursorStatsChanged();
    void documentLoaded();
    void errorOccurred(const QString &title, const QString &message);

private slots:
    void onDocumentModifiedChanged(bool m);
    void onDocumentContentsChanged();
    void runAutoDetect();

private:
    QString sampleDocumentHead() const;
    QString computeResolvedLanguageId() const;
    void applyHighlighter(bool forceRehighlight = false);

    QQuickTextDocument *m_quickDoc = nullptr;
    NullaHighlighter *m_highlighter = nullptr;
    QTimer m_autoDetectTimer;

    QString m_filePath;
    QString m_pendingPath;
    bool m_pendingAllowMissing = true;
    bool m_modified = false;
    bool m_loading = false;

    bool m_syntaxEnabled = true;
    bool m_darkMode = true;
    QString m_languageId = QStringLiteral("auto");
    QString m_resolvedLanguageId = QStringLiteral("plain");

    int m_cursorLine = 1;
    int m_cursorColumn = 1;
};

#endif
