#include "indent_guides.h"

#include <QAbstractTextDocumentLayout>
#include <QFontMetricsF>
#include <QPainter>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextOption>
#include <QVector>
#include <QtMath>

#include <algorithm>
#include <cmath>

namespace {

struct LineIndentInfo {
  bool isBlank = true;
  qreal leadingPx = 0.0;
  QVector<qreal> guideXs;
};

LineIndentInfo analyzeBlockIndent(const QTextBlock &block, qreal spaceWidth,
                                  qreal tabStopPx, int indentStep) {
  LineIndentInfo info;
  if (!block.isValid() || spaceWidth <= 0.1) {
    return info;
  }

  const QString text = block.text();
  const int len = text.size();
  qreal x = 0.0;
  int wsEnd = 0;
  bool hasTab = false;
  bool foundNonSpace = false;

  for (int i = 0; i < len; ++i) {
    const QChar ch = text.at(i);
    if (ch == QLatin1Char(' ')) {
      x += spaceWidth;
      wsEnd = i + 1;
    } else if (ch == QLatin1Char('\t')) {
      hasTab = true;
      x = (std::floor(x / tabStopPx + 1e-6) + 1.0) * tabStopPx;
      wsEnd = i + 1;
    } else {
      foundNonSpace = true;
      break;
    }
  }

  if (!foundNonSpace) {
    return info;
  }

  info.isBlank = false;
  info.leadingPx = x;

  const qreal threshold = x - 0.5 * spaceWidth;
  if (threshold <= 0.0) {
    return info;
  }

  const qreal stepPx = indentStep * spaceWidth;
  info.guideXs.append(0.0);

  if (!hasTab) {
    for (qreal gx = stepPx; gx < threshold; gx += stepPx) {
      info.guideXs.append(gx);
    }
    return info;
  }

  qreal curX = 0.0;
  int spaceRun = 0;
  for (int i = 0; i < wsEnd; ++i) {
    const QChar ch = text.at(i);
    if (ch == QLatin1Char('\t')) {
      curX = (std::floor(curX / tabStopPx + 1e-6) + 1.0) * tabStopPx;
      spaceRun = 0;
      if (curX < threshold) {
        info.guideXs.append(curX);
      }
    } else {
      curX += spaceWidth;
      ++spaceRun;
      if (spaceRun >= indentStep) {
        spaceRun = 0;
        if (curX < threshold) {
          info.guideXs.append(curX);
        }
      }
    }
  }

  return info;
}

bool containsGuideX(const QVector<qreal> &guides, qreal targetX) {
  for (qreal gx : guides) {
    if (qAbs(gx - targetX) < 0.5) {
      return true;
    }
  }
  return false;
}

} // namespace

IndentGuides::IndentGuides(QQuickItem *parent) : QQuickPaintedItem(parent) {
  setClip(true);
  setAcceptedMouseButtons(Qt::NoButton);
  updateMetrics();
}

void IndentGuides::setDocument(QQuickTextDocument *doc) {
  if (m_document == doc) {
    return;
  }
  if (m_document && m_document->textDocument()) {
    disconnect(m_document->textDocument(), nullptr, this, nullptr);
    if (m_document->textDocument()->documentLayout()) {
      disconnect(m_document->textDocument()->documentLayout(), nullptr, this,
                 nullptr);
    }
  }
  m_document = doc;
  if (m_document && m_document->textDocument()) {
    QTextDocument *td = m_document->textDocument();
    connect(td, &QTextDocument::contentsChanged, this,
            &IndentGuides::onContentsChanged);
    connect(td->documentLayout(),
            &QAbstractTextDocumentLayout::documentSizeChanged, this,
            &IndentGuides::refresh);
  }
  detectIndentStep();
  emit documentChanged();
  update();
}

void IndentGuides::setContentX(qreal x) {
  if (qFuzzyCompare(m_contentX, x)) {
    return;
  }
  m_contentX = x;
  emit contentXChanged();
  update();
}

void IndentGuides::setContentY(qreal y) {
  if (qFuzzyCompare(m_contentY, y)) {
    return;
  }
  m_contentY = y;
  emit contentYChanged();
  update();
}

void IndentGuides::setCurrentLine(int line) {
  if (m_currentLine == line) {
    return;
  }
  m_currentLine = line;
  emit currentLineChanged();
  update();
}

void IndentGuides::setFont(const QFont &f) {
  if (m_font == f) {
    return;
  }
  m_font = f;
  updateMetrics();
  emit fontChanged();
  update();
}

void IndentGuides::setGuideColor(const QColor &c) {
  if (m_guideColor == c) {
    return;
  }
  m_guideColor = c;
  emit guideColorChanged();
  update();
}

void IndentGuides::setActiveGuideColor(const QColor &c) {
  if (m_activeGuideColor == c) {
    return;
  }
  m_activeGuideColor = c;
  emit activeGuideColorChanged();
  update();
}

void IndentGuides::setLeftPadding(qreal p) {
  if (qFuzzyCompare(m_leftPadding, p)) {
    return;
  }
  m_leftPadding = p;
  emit leftPaddingChanged();
  update();
}

void IndentGuides::setTopPadding(qreal p) {
  if (qFuzzyCompare(m_topPadding, p)) {
    return;
  }
  m_topPadding = p;
  emit topPaddingChanged();
  update();
}

void IndentGuides::refresh() { update(); }

void IndentGuides::onContentsChanged() {
  detectIndentStep();
  update();
}

void IndentGuides::updateMetrics() {
  const QFontMetricsF fm(m_font);
  m_spaceWidth = qMax(1.0, fm.horizontalAdvance(QLatin1Char(' ')));
  m_lineHeight = qMax(1.0, fm.height());
}

void IndentGuides::detectIndentStep() {
  m_indentStep = 4;
  if (!m_document || !m_document->textDocument()) {
    return;
  }

  QTextDocument *td = m_document->textDocument();
  int mod2Lines = 0;
  int mod4Lines = 0;
  int tabLines = 0;
  int scanned = 0;

  for (QTextBlock block = td->begin(); block.isValid() && scanned < 180;
       block = block.next()) {
    const QString text = block.text();
    if (text.isEmpty()) {
      continue;
    }
    int spaces = 0;
    bool hasTab = false;
    bool nonSpace = false;
    for (int i = 0; i < text.size(); ++i) {
      const QChar ch = text.at(i);
      if (ch == QLatin1Char(' ')) {
        ++spaces;
      } else if (ch == QLatin1Char('\t')) {
        hasTab = true;
        break;
      } else {
        nonSpace = true;
        break;
      }
    }
    if (!nonSpace && !hasTab) {
      continue;
    }
    ++scanned;
    if (hasTab) {
      ++tabLines;
    } else if (spaces > 0) {
      if ((spaces % 4) == 2) {
        ++mod2Lines;
      } else if ((spaces % 4) == 0) {
        ++mod4Lines;
      }
    }
  }

  if (tabLines == 0 && mod2Lines >= 2 && mod2Lines * 2 >= mod4Lines) {
    m_indentStep = 2;
  }
}

void IndentGuides::paint(QPainter *painter) {
  if (!m_document || !m_document->textDocument() || width() <= 0.0 ||
      height() <= 0.0) {
    return;
  }

  QTextDocument *td = m_document->textDocument();
  QAbstractTextDocumentLayout *layout = td->documentLayout();
  if (!layout) {
    return;
  }

  qreal tabStopPx = td->defaultTextOption().tabStopDistance();
  if (tabStopPx <= 0.0) {
    tabStopPx = 80.0;
  }

  const int totalBlocks = qMax(1, td->blockCount());
  const qreal docTop = qMax(0.0, m_contentY - m_topPadding - m_lineHeight);
  int startPos = layout->hitTest(QPointF(0.0, docTop), Qt::FuzzyHit);
  QTextBlock firstVisBlock =
      (startPos >= 0) ? td->findBlock(startPos) : td->begin();
  if (!firstVisBlock.isValid()) {
    firstVisBlock = td->begin();
  } else if (firstVisBlock.previous().isValid()) {
    firstVisBlock = firstVisBlock.previous();
  }

  const qreal maxScreenY = height() + m_lineHeight;
  QTextBlock lastVisBlock = firstVisBlock;
  for (QTextBlock b = firstVisBlock; b.isValid(); b = b.next()) {
    const QRectF rect = layout->blockBoundingRect(b);
    const qreal y = rect.top() - m_contentY + m_topPadding;
    lastVisBlock = b;
    if (y > maxScreenY) {
      break;
    }
  }

  const int firstVisIdx = firstVisBlock.blockNumber();
  const int lastVisIdx = lastVisBlock.blockNumber();
  const int curBlockIdx = qBound(0, m_currentLine - 1, totalBlocks - 1);

  // Include a window around visible blocks and current line to resolve blank
  // lines and active indent scope accurately without scanning huge files.
  const int minAnalyzeIdx = qMax(0, qMin(firstVisIdx, curBlockIdx) - 160);
  const int maxAnalyzeIdx =
      qMin(totalBlocks - 1, qMax(lastVisIdx, curBlockIdx) + 160);

  const int count = maxAnalyzeIdx - minAnalyzeIdx + 1;
  QVector<LineIndentInfo> lines(count);
  QVector<QTextBlock> blocks(count);

  QTextBlock iterBlock = td->findBlockByNumber(minAnalyzeIdx);
  for (int i = 0; i < count && iterBlock.isValid();
       ++i, iterBlock = iterBlock.next()) {
    blocks[i] = iterBlock;
    lines[i] =
        analyzeBlockIndent(iterBlock, m_spaceWidth, tabStopPx, m_indentStep);
  }

  // Resolve blank lines using min(prevNonBlank, nextNonBlank)
  QVector<qreal> prevLeading(count, -1.0);
  QVector<int> prevIdx(count, -1);
  int lastNonBlank = -1;
  for (int i = 0; i < count; ++i) {
    if (!lines[i].isBlank) {
      lastNonBlank = i;
    }
    prevIdx[i] = lastNonBlank;
    if (lastNonBlank >= 0) {
      prevLeading[i] = lines[lastNonBlank].leadingPx;
    }
  }

  int nextNonBlank = -1;
  for (int i = count - 1; i >= 0; --i) {
    if (!lines[i].isBlank) {
      nextNonBlank = i;
      continue;
    }
    const int p = prevIdx[i];
    const int n = nextNonBlank;
    if (p >= 0 && n >= 0) {
      const qreal minPx = qMin(prevLeading[i], lines[n].leadingPx);
      const qreal threshold = minPx - 0.5 * m_spaceWidth;
      if (threshold > 0.0) {
        lines[i].leadingPx = minPx;
        for (qreal gx : std::as_const(lines[p].guideXs)) {
          if (gx < threshold) {
            lines[i].guideXs.append(gx);
          }
        }
      }
    }
  }

  // Determine active indent scope around current cursor line
  qreal activeGuideX = -1.0;
  int activeScopeStart = -1;
  int activeScopeEnd = -1;
  const int curRelIdx = curBlockIdx - minAnalyzeIdx;
  if (curRelIdx >= 0 && curRelIdx < count && m_activeGuideColor.alpha() > 0) {
    const qreal curPx = lines[curRelIdx].leadingPx;
    qreal nextPx = -1.0;
    for (int i = curRelIdx + 1; i < count; ++i) {
      if (!lines[i].isBlank) {
        nextPx = lines[i].leadingPx;
        break;
      }
    }

    if (nextPx > curPx + 0.5 * m_spaceWidth) {
      // Cursor is on a block opening line; highlight the block body below
      activeGuideX = curPx;
      int bodyStart = curRelIdx + 1;
      if (bodyStart < count &&
          containsGuideX(lines[bodyStart].guideXs, activeGuideX)) {
        activeScopeStart = bodyStart;
        activeScopeEnd = bodyStart;
        while (
            activeScopeEnd + 1 < count &&
            containsGuideX(lines[activeScopeEnd + 1].guideXs, activeGuideX)) {
          ++activeScopeEnd;
        }
      }
    } else if (!lines[curRelIdx].guideXs.isEmpty()) {
      activeGuideX = lines[curRelIdx].guideXs.last();
      activeScopeStart = curRelIdx;
      activeScopeEnd = curRelIdx;
      while (
          activeScopeStart - 1 >= 0 &&
          containsGuideX(lines[activeScopeStart - 1].guideXs, activeGuideX)) {
        --activeScopeStart;
      }
      while (activeScopeEnd + 1 < count &&
             containsGuideX(lines[activeScopeEnd + 1].guideXs, activeGuideX)) {
        ++activeScopeEnd;
      }
    }
  }

  painter->setRenderHint(QPainter::Antialiasing, false);

  const int visStartRel = qMax(0, firstVisIdx - minAnalyzeIdx);
  const int visEndRel = qMin(count - 1, lastVisIdx - minAnalyzeIdx);

  for (int i = visStartRel; i <= visEndRel; ++i) {
    const QVector<qreal> &guides = lines[i].guideXs;
    if (guides.isEmpty() || !blocks[i].isValid()) {
      continue;
    }
    const QRectF rect = layout->blockBoundingRect(blocks[i]);
    const qreal y0 = std::round(rect.top() - m_contentY + m_topPadding);
    const qreal y1 = std::round(rect.bottom() - m_contentY + m_topPadding);
    if (y1 <= 0.0 || y0 >= height()) {
      continue;
    }
    const qreal h = qMax(1.0, y1 - y0);
    const bool lineInActiveScope =
        (activeGuideX >= 0.0 && i >= activeScopeStart && i <= activeScopeEnd);

    for (qreal gx : guides) {
      const qreal screenX = std::round(m_leftPadding + gx - m_contentX);
      if (screenX < -1.0 || screenX > width() + 1.0) {
        continue;
      }
      const bool isActive = lineInActiveScope && qAbs(gx - activeGuideX) < 0.5;
      painter->fillRect(QRectF(screenX, y0, 1.0, h),
                        isActive ? m_activeGuideColor : m_guideColor);
    }
  }
}
