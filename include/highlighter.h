#ifndef NULLA_HIGHLIGHTER_H
#define NULLA_HIGHLIGHTER_H

#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QVector>

struct NullaLangDef {
    QString id;
    QString name;
};

class NullaHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit NullaHighlighter(QTextDocument *parent = nullptr);

    void configure(bool enabled, const QString &languageId, bool darkMode);
    static QVector<NullaLangDef> supportedLanguages();
    static QString detectLanguage(const QString &filePath, const QString &firstLine);
    static QString languageDisplayName(const QString &id);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightRule {
        QRegularExpression pattern;
        QTextCharFormat format;
        int captureGroup = 0;
    };

    void rebuildRules();
    void applyCommentBlock(const QString &text, const QRegularExpression &startExpr,
                           const QRegularExpression &endExpr, int stateFlag);

    bool m_enabled = true;
    bool m_darkMode = true;
    QString m_languageId = QStringLiteral("plain");

    QVector<HighlightRule> m_rules;
    QTextCharFormat m_commentFormat;
    QTextCharFormat m_stringFormat;
    bool m_hasBlockComments = false;
    bool m_hasTripleQuotes = false;
};

#endif
