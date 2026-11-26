#ifndef NULLA_GUTTER_H
#define NULLA_GUTTER_H

#include <QColor>
#include <QFont>
#include <QQuickPaintedItem>
#include <QQuickTextDocument>

class LineNumberGutter : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(QQuickTextDocument *document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(qreal contentY READ contentY WRITE setContentY NOTIFY contentYChanged)
    Q_PROPERTY(int currentLine READ currentLine WRITE setCurrentLine NOTIFY currentLineChanged)
    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)
    Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor NOTIFY textColorChanged)
    Q_PROPERTY(QColor activeTextColor READ activeTextColor WRITE setActiveTextColor NOTIFY activeTextColorChanged)
    Q_PROPERTY(qreal topPadding READ topPadding WRITE setTopPadding NOTIFY topPaddingChanged)

public:
    explicit LineNumberGutter(QQuickItem *parent = nullptr);

    QQuickTextDocument *document() const { return m_document; }
    void setDocument(QQuickTextDocument *doc);

    qreal contentY() const { return m_contentY; }
    void setContentY(qreal y);

    int currentLine() const { return m_currentLine; }
    void setCurrentLine(int line);

    QFont font() const { return m_font; }
    void setFont(const QFont &f);

    QColor textColor() const { return m_textColor; }
    void setTextColor(const QColor &c);

    QColor activeTextColor() const { return m_activeTextColor; }
    void setActiveTextColor(const QColor &c);

    qreal topPadding() const { return m_topPadding; }
    void setTopPadding(qreal p);

    void paint(QPainter *painter) override;

public slots:
    void refresh();

signals:
    void documentChanged();
    void contentYChanged();
    void currentLineChanged();
    void fontChanged();
    void textColorChanged();
    void activeTextColorChanged();
    void topPaddingChanged();

private:
    void updateMetricsWidth();

    QQuickTextDocument *m_document = nullptr;
    qreal m_contentY = 0.0;
    int m_currentLine = 1;
    QFont m_font;
    QColor m_textColor = QColor(QStringLiteral("#5b616e"));
    QColor m_activeTextColor = QColor(QStringLiteral("#e6e8ec"));
    qreal m_topPadding = 8.0;
};

#endif
