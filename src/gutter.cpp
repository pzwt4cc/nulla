#include "gutter.h"

#include <QAbstractTextDocumentLayout>
#include <QFontMetricsF>
#include <QPainter>
#include <QTextBlock>
#include <QTextDocument>
#include <QtMath>

LineNumberGutter::LineNumberGutter(QQuickItem *parent)
    : QQuickPaintedItem(parent) {
    setClip(true);
    updateMetricsWidth();
}

void LineNumberGutter::setDocument(QQuickTextDocument *doc) {
    if (m_document == doc) {
        return;
    }
    if (m_document && m_document->textDocument()) {
        disconnect(m_document->textDocument(), nullptr, this, nullptr);
    }
    m_document = doc;
    if (m_document && m_document->textDocument()) {
        QTextDocument *td = m_document->textDocument();
        connect(td, &QTextDocument::blockCountChanged, this, &LineNumberGutter::refresh);
        connect(td->documentLayout(), &QAbstractTextDocumentLayout::documentSizeChanged,
                this, &LineNumberGutter::refresh);
    }
    emit documentChanged();
    refresh();
}

void LineNumberGutter::setContentY(qreal y) {
    if (qFuzzyCompare(m_contentY, y)) {
        return;
    }
    m_contentY = y;
    emit contentYChanged();
    update();
}

void LineNumberGutter::setCurrentLine(int line) {
    if (m_currentLine == line) {
        return;
    }
    m_currentLine = line;
    emit currentLineChanged();
    update();
}

void LineNumberGutter::setFont(const QFont &f) {
    if (m_font == f) {
        return;
    }
    m_font = f;
    m_lastDigits = -1;
    emit fontChanged();
    updateMetricsWidth();
    update();
}

void LineNumberGutter::setTextColor(const QColor &c) {
    if (m_textColor == c) {
        return;
    }
    m_textColor = c;
    emit textColorChanged();
    update();
}

void LineNumberGutter::setActiveTextColor(const QColor &c) {
    if (m_activeTextColor == c) {
        return;
    }
    m_activeTextColor = c;
    emit activeTextColorChanged();
    update();
}

void LineNumberGutter::setActiveLineColor(const QColor &c) {
    if (m_activeLineColor == c) {
        return;
    }
    m_activeLineColor = c;
    emit activeLineColorChanged();
    update();
}

void LineNumberGutter::setTopPadding(qreal p) {
    if (qFuzzyCompare(m_topPadding, p)) {
        return;
    }
    m_topPadding = p;
    emit topPaddingChanged();
    update();
}

void LineNumberGutter::refresh() {
    updateMetricsWidth();
    update();
}

void LineNumberGutter::updateMetricsWidth() {
    int blocks = 1;
    if (m_document && m_document->textDocument()) {
        blocks = qMax(1, m_document->textDocument()->blockCount());
    }
    int v = blocks;
    int count = 0;
    while (v > 0) {
        v /= 10;
        count++;
    }
    const int digits = qMax(2, count);
    if (digits == m_lastDigits) {
        return;
    }
    m_lastDigits = digits;
    const QFontMetricsF fm(m_font);
    m_lineHeight = fm.height();
    const qreal w = fm.horizontalAdvance(QString(digits, QLatin1Char('9'))) + 22.0;
    if (!qFuzzyCompare(implicitWidth(), w)) {
        setImplicitWidth(qCeil(w));
    }
}

void LineNumberGutter::paint(QPainter *painter) {
    if (!m_document || !m_document->textDocument()) {
        return;
    }
    QTextDocument *td = m_document->textDocument();
    QAbstractTextDocumentLayout *layout = td->documentLayout();
    if (!layout) {
        return;
    }

    painter->setRenderHint(QPainter::TextAntialiasing, true);
    painter->setFont(m_font);

    const qreal rightMargin = 11.0;
    const qreal docTop = qMax(0.0, m_contentY - m_topPadding - m_lineHeight);
    int startPos = layout->hitTest(QPointF(0.0, docTop), Qt::FuzzyHit);
    QTextBlock block = (startPos >= 0) ? td->findBlock(startPos) : td->begin();
    if (!block.isValid()) {
        block = td->begin();
    } else if (block.previous().isValid()) {
        block = block.previous();
    }

    int lineNum = block.isValid() ? (block.blockNumber() + 1) : 1;
    const qreal maxY = height() + m_lineHeight;

    while (block.isValid()) {
        const QRectF rect = layout->blockBoundingRect(block);
        const qreal y = rect.top() - m_contentY + m_topPadding;
        if (y > maxY) {
            break;
        }
        if (y + rect.height() >= -m_lineHeight) {
            const bool isCurrent = (lineNum == m_currentLine);
            if (isCurrent && m_activeLineColor.alpha() > 0) {
                painter->fillRect(QRectF(0.0, y, width(), m_lineHeight), m_activeLineColor);
            }
            painter->setPen(isCurrent ? m_activeTextColor : m_textColor);
            const QRectF drawRect(0.0, y, width() - rightMargin, m_lineHeight);
            painter->drawText(drawRect, Qt::AlignRight | Qt::AlignVCenter, QString::number(lineNum));
        }
        block = block.next();
        lineNum++;
    }
}
