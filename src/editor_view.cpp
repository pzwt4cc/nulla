#include "editor_view.h"

#include "tab_session.h"

#include <QApplication>
#include <QClipboard>
#include <QCursor>
#include <QFocusEvent>
#include <QGuiApplication>
#include <QHoverEvent>
#include <QInputMethodEvent>
#include <QKeyEvent>
#include <QMetaObject>
#include <QMouseEvent>
#include <QPainter>
#include <QtMath>

#include <algorithm>
#include <cmath>

namespace {

constexpr qreal kPadLeft = 10.0;
constexpr qreal kPadTop = 8.0;
constexpr qreal kScrollPadRight = 20.0;
constexpr qreal kScrollPadBottom = 16.0;
constexpr qreal kGutterPadRight = 11.0;

size_t utf8VisWidth(const char *s, size_t len, size_t startVisCol, bool &allAscii) {
  allAscii = true;
  size_t col = startVisCol;
  size_t i = 0;
  while (i < len) {
    const unsigned char b = static_cast<unsigned char>(s[i]);
    if (b < 0x80) {
      if (b == '\t') {
        allAscii = false;
        col += 4 - (col % 4);
      } else if (b >= 0x20 && b != 0x7F) {
        col += 1;
      }
      i += 1;
    } else {
      allAscii = false;
      uint32_t cp = 0;
      size_t step = 1;
      if ((b & 0xE0) == 0xC0 && i + 1 < len) {
        cp = ((b & 0x1F) << 6) | (static_cast<unsigned char>(s[i + 1]) & 0x3F);
        step = 2;
      } else if ((b & 0xF0) == 0xE0 && i + 2 < len) {
        cp = ((b & 0x0F) << 12) |
             ((static_cast<unsigned char>(s[i + 1]) & 0x3F) << 6) |
             (static_cast<unsigned char>(s[i + 2]) & 0x3F);
        step = 3;
      } else if ((b & 0xF8) == 0xF0 && i + 3 < len) {
        cp = ((b & 0x07) << 18) |
             ((static_cast<unsigned char>(s[i + 1]) & 0x3F) << 12) |
             ((static_cast<unsigned char>(s[i + 2]) & 0x3F) << 6) |
             (static_cast<unsigned char>(s[i + 3]) & 0x3F);
        step = 4;
      }
      const bool wide =
          (cp >= 0x1100 && cp <= 0x115F) || (cp >= 0x2E80 && cp <= 0x303E) ||
          (cp >= 0x3040 && cp <= 0xA4CF) || (cp >= 0xAC00 && cp <= 0xD7A3) ||
          (cp >= 0xF900 && cp <= 0xFAFF) || (cp >= 0xFE10 && cp <= 0xFE6F) ||
          (cp >= 0xFF00 && cp <= 0xFF60) || (cp >= 0x1F300 && cp <= 0x1FAFF) ||
          (cp >= 0x20000 && cp <= 0x3FFFF);
      col += wide ? 2 : 1;
      i += step;
    }
  }
  return col - startVisCol;
}

size_t utf8PrefixVisCol(const uint8_t *lineData, size_t lineLen, size_t byteOff,
                        size_t startVisCol) {
  bool dummy = true;
  const size_t n = std::min(byteOff, lineLen);
  return startVisCol +
         utf8VisWidth(reinterpret_cast<const char *>(lineData), n, startVisCol,
                      dummy);
}

bool isRussianShortcutKey(int key) {
  return (key >= 0x0410 && key <= 0x044F) || key == 0x0401 || key == 0x0451;
}

}

NullaEditorView::NullaEditorView(QQuickItem *parent)
    : QQuickPaintedItem(parent), m_metrics(m_font) {
  setFlag(QQuickItem::ItemIsFocusScope, false);
  setFlag(QQuickItem::ItemAcceptsInputMethod, true);
  setFocus(true);
  setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton | Qt::MiddleButton);
  setAcceptHoverEvents(true);
  setCursor(Qt::IBeamCursor);
  setClip(true);
  setOpaquePainting(false);
  setSmooth(false);

  m_blinkTimer.setInterval(530);
  connect(&m_blinkTimer, &QTimer::timeout, this, &NullaEditorView::onBlink);

  m_autoScrollTimer.setInterval(40);
  connect(&m_autoScrollTimer, &QTimer::timeout, this,
          &NullaEditorView::onAutoScroll);

  updateMetrics();
  updateGutterWidth();
}

NullaEditorView::~NullaEditorView() {
  m_blinkTimer.stop();
  m_autoScrollTimer.stop();
}

void NullaEditorView::setSession(TabSession *s) {
  if (m_session == s) {
    return;
  }
  if (m_session) {
    disconnect(m_session, nullptr, this, nullptr);
  }
  m_session = s;
  m_preedit.clear();
  m_preeditCursor = 0;
  m_dragging = false;
  m_autoScrollTimer.stop();

  if (m_session) {
    connect(m_session, &TabSession::contentChanged, this,
            &NullaEditorView::onContentChanged);
    connect(m_session, &TabSession::selectionChanged, this,
            &NullaEditorView::onSelectionChanged);
    connect(m_session, &TabSession::documentLoaded, this,
            &NullaEditorView::onContentChanged);
    connect(m_session, &TabSession::maxLineColumnsChanged, this,
            &NullaEditorView::onMaxColumnsChanged);
  }

  m_gutterDigits = -1;
  updateGutterWidth();
  updateContentSize();
  syncCursor();
  updateHover();
  emit sessionChanged();
  update();
}

void NullaEditorView::setFont(const QFont &f) {
  if (m_font == f) {
    return;
  }
  m_font = f;
  updateMetrics();
  m_gutterDigits = -1;
  updateGutterWidth();
  updateContentSize();
  syncCursor();
  updateHover();
  emit metricsChanged();
  update();
}

void NullaEditorView::setDarkMode(bool v) {
  if (m_darkMode == v) {
    return;
  }
  m_darkMode = v;
  emit appearanceChanged();
  update();
}

void NullaEditorView::setShowLineNumbers(bool v) {
  if (m_showLineNumbers == v) {
    return;
  }
  m_showLineNumbers = v;
  emit metricsChanged();
  syncCursor();
  updateHover();
  update();
}

void NullaEditorView::setShowIndentGuides(bool v) {
  if (m_showIndentGuides == v) {
    return;
  }
  m_showIndentGuides = v;
  emit appearanceChanged();
  update();
}

void NullaEditorView::setContentX(qreal v) {
  if (qFuzzyCompare(m_contentX + 1.0, v + 1.0)) {
    return;
  }
  m_contentX = v;
  syncCursor();
  updateHover();
  emit scrollChanged();
  update();
}

void NullaEditorView::setContentY(qreal v) {
  if (qFuzzyCompare(m_contentY + 1.0, v + 1.0)) {
    return;
  }
  m_contentY = v;
  syncCursor();
  updateHover();
  emit scrollChanged();
  update();
}

void NullaEditorView::setRightInset(qreal v) {
  if (qFuzzyCompare(m_rightInset + 1.0, v + 1.0)) {
    return;
  }
  m_rightInset = v;
  updateHover();
  emit insetsChanged();
}

void NullaEditorView::setBottomInset(qreal v) {
  if (qFuzzyCompare(m_bottomInset + 1.0, v + 1.0)) {
    return;
  }
  m_bottomInset = v;
  updateHover();
  emit insetsChanged();
}

void NullaEditorView::setTextColor(const QColor &c) {
  if (m_textColor == c) {
    return;
  }
  m_textColor = c;
  emit appearanceChanged();
  update();
}

void NullaEditorView::setGutterBackground(const QColor &c) {
  if (m_gutterBackground == c) {
    return;
  }
  m_gutterBackground = c;
  emit appearanceChanged();
  update();
}

void NullaEditorView::setGutterBorder(const QColor &c) {
  if (m_gutterBorder == c) {
    return;
  }
  m_gutterBorder = c;
  emit appearanceChanged();
  update();
}

void NullaEditorView::setGutterTextColor(const QColor &c) {
  if (m_gutterTextColor == c) {
    return;
  }
  m_gutterTextColor = c;
  emit appearanceChanged();
  update();
}

void NullaEditorView::setGutterActiveTextColor(const QColor &c) {
  if (m_gutterActiveTextColor == c) {
    return;
  }
  m_gutterActiveTextColor = c;
  emit appearanceChanged();
  update();
}

void NullaEditorView::setGutterActiveLineColor(const QColor &c) {
  if (m_gutterActiveLineColor == c) {
    return;
  }
  m_gutterActiveLineColor = c;
  emit appearanceChanged();
  update();
}

void NullaEditorView::setSelectionColor(const QColor &c) {
  if (m_selectionColor == c) {
    return;
  }
  m_selectionColor = c;
  emit appearanceChanged();
  update();
}

void NullaEditorView::setCaretColor(const QColor &c) {
  if (m_caretColor == c) {
    return;
  }
  m_caretColor = c;
  emit appearanceChanged();
  update();
}

void NullaEditorView::setCaretSelectionColor(const QColor &c) {
  if (m_caretSelectionColor == c) {
    return;
  }
  m_caretSelectionColor = c;
  emit appearanceChanged();
  update();
}

void NullaEditorView::setGuideColor(const QColor &c) {
  if (m_guideColor == c) {
    return;
  }
  m_guideColor = c;
  emit appearanceChanged();
  update();
}

void NullaEditorView::setActiveGuideColor(const QColor &c) {
  if (m_activeGuideColor == c) {
    return;
  }
  m_activeGuideColor = c;
  emit appearanceChanged();
  update();
}

void NullaEditorView::performAction(const QString &action) {
  if (!m_session) {
    return;
  }
  if (action == QLatin1String("undo")) {
    m_session->undo();
    emit edited();
    emit cursorMoved();
  } else if (action == QLatin1String("redo")) {
    m_session->redo();
    emit edited();
    emit cursorMoved();
  } else if (action == QLatin1String("cut")) {
    copyToClipboard(true);
  } else if (action == QLatin1String("copy")) {
    copyToClipboard(false);
  } else if (action == QLatin1String("paste")) {
    pasteFromClipboard(false);
  } else if (action == QLatin1String("delete")) {
    if (m_session->hasSelection()) {
      m_session->deleteForward(false);
      emit edited();
      emit cursorMoved();
    }
  } else if (action == QLatin1String("duplicate")) {
    m_session->duplicate();
    emit edited();
    emit cursorMoved();
  } else if (action == QLatin1String("comment")) {
    m_session->toggleComment();
    emit edited();
    emit cursorMoved();
  } else if (action == QLatin1String("selectall")) {
    m_session->selectAll();
    pushPrimarySelection();
  } else if (action == QLatin1String("find")) {
    emit actionRequested(QStringLiteral("find"));
  }
  restartBlink();
}

bool NullaEditorView::clipboardHasText() const {
  if (const QClipboard *cb = QGuiApplication::clipboard()) {
    return !cb->text(QClipboard::Clipboard).isEmpty();
  }
  return false;
}

QPointF NullaEditorView::cursorViewportPos() const {
  return QPointF(m_cursorRect.x(), m_cursorRect.y() + m_cursorRect.height());
}

void NullaEditorView::updateMetrics() {
  m_font.setStyleStrategy(QFont::PreferDefault);
  m_fontDemi = m_font;
  m_fontDemi.setWeight(QFont::DemiBold);
  m_fontMedium = m_font;
  m_fontMedium.setWeight(QFont::Medium);
  m_fontItalic = m_font;
  m_fontItalic.setItalic(true);

  m_metrics = QFontMetricsF(m_font);
  m_charWidth = qMax(1.0, m_metrics.horizontalAdvance(QLatin1Char(' ')));
  m_lineHeight = qMax(1.0, std::ceil(m_metrics.height()));
  m_ascent = m_metrics.ascent();
}

void NullaEditorView::updateGutterWidth() {
  const int lines = m_session ? qMax(1, m_session->lineCount()) : 1;
  int v = lines;
  int digits = 0;
  while (v > 0) {
    v /= 10;
    ++digits;
  }
  digits = qMax(2, digits);
  if (digits == m_gutterDigits) {
    return;
  }
  m_gutterDigits = digits;
  const qreal w = std::ceil(
      m_metrics.horizontalAdvance(QString(digits, QLatin1Char('9'))) + 22.0);
  if (!qFuzzyCompare(m_gutterWidth, w)) {
    m_gutterWidth = w;
    emit metricsChanged();
  }
}

void NullaEditorView::updateContentSize() {
  const int lines = m_session ? qMax(1, m_session->lineCount()) : 1;
  const int cols = m_session ? qMax(0, m_session->maxLineColumns()) : 0;
  const qreal nextH = static_cast<qreal>(lines) * m_lineHeight + kPadTop +
                      kScrollPadBottom;
  const qreal nextW = static_cast<qreal>(cols) * m_charWidth + kPadLeft +
                      kScrollPadRight;
  if (!qFuzzyCompare(m_docContentWidth + 1.0, nextW + 1.0) ||
      !qFuzzyCompare(m_docContentHeight + 1.0, nextH + 1.0)) {
    m_docContentWidth = nextW;
    m_docContentHeight = nextH;
    emit contentSizeChanged();
  }
}

void NullaEditorView::syncCursor() {
  size_t line = 0;
  size_t visCol = 0;
  bool hasSel = false;
  if (m_session) {
    m_session->cursorLineVisCol(line, visCol);
    hasSel = m_session->hasSelection();
  }
  m_cursorLine0 = line;
  m_cursorVisCol = visCol;

  const qreal x =
      kPadLeft + static_cast<qreal>(visCol) * m_charWidth - m_contentX;
  const qreal y =
      kPadTop + static_cast<qreal>(line) * m_lineHeight - m_contentY;
  const QRectF nextRect(x, y, hasSel ? 2.5 : 2.0, m_lineHeight);
  if (m_cursorRect != nextRect || m_hasSelection != hasSel) {
    m_cursorRect = nextRect;
    m_hasSelection = hasSel;
    emit cursorRectChanged();
  }
  if (QGuiApplication::inputMethod()) {
    QGuiApplication::inputMethod()->update(Qt::ImCursorRectangle);
  }
}

void NullaEditorView::updateHover() {
  const qreal gw = gutterWidth();
  const qreal rightEdge = width() - m_rightInset;
  const qreal bottomEdge = height() - m_bottomInset;
  const bool inside = m_hovering && m_hoverPos.x() >= gw &&
                      m_hoverPos.x() < rightEdge && m_hoverPos.y() >= 0.0 &&
                      m_hoverPos.y() < bottomEdge;

  QRectF nextRect = m_hoverRect;
  if (inside && m_session) {
    const HitPos h = hitAt(m_hoverPos);
    const qreal hx =
        kPadLeft + static_cast<qreal>(h.visCol) * m_charWidth - m_contentX;
    const qreal hy =
        kPadTop + static_cast<qreal>(h.line) * m_lineHeight - m_contentY;
    nextRect = QRectF(hx, hy, m_charWidth, m_lineHeight);
  }

  if (m_hoverInside != inside || (inside && m_hoverRect != nextRect)) {
    m_hoverInside = inside;
    m_hoverRect = nextRect;
    emit hoverChanged();
  }
}

NullaEditorView::HitPos NullaEditorView::hitAt(const QPointF &pos) const {
  HitPos out;
  if (!m_session || !m_session->doc()) {
    return out;
  }
  const int totalLines = qMax(1, m_session->lineCount());
  const qreal docY = pos.y() + m_contentY - kPadTop;
  const qint64 rawLine = static_cast<qint64>(std::floor(docY / m_lineHeight));
  out.line = static_cast<size_t>(
      qBound<qint64>(0, rawLine, static_cast<qint64>(totalLines - 1)));

  const qreal gw = gutterWidth();
  const qreal docX = (pos.x() - gw) + m_contentX - kPadLeft;
  if (docX <= 0.0) {
    out.visCol = 0;
  } else {
    out.visCol =
        static_cast<size_t>(qMax<qint64>(0, qRound64(docX / m_charWidth)));
  }
  const size_t snappedByte = m_session->hitTest(out.line, out.visCol);
  out.byte = snappedByte;
  size_t exactLine = out.line;
  size_t exactCol = out.visCol;
  nulla_doc_byte_to_line_vis_col(m_session->doc(), snappedByte, &exactLine,
                                 &exactCol);
  out.visCol = exactCol;
  return out;
}

void NullaEditorView::restartBlink() {
  m_caretOn = true;
  if (hasActiveFocus()) {
    m_blinkTimer.start();
  }
  update();
}

void NullaEditorView::onBlink() {
  if (!hasActiveFocus()) {
    m_caretOn = false;
    m_blinkTimer.stop();
    update();
    return;
  }
  m_caretOn = !m_caretOn;
  update();
}

void NullaEditorView::onContentChanged() {
  updateGutterWidth();
  updateContentSize();
  syncCursor();
  updateHover();
  update();
}

void NullaEditorView::onSelectionChanged() {
  syncCursor();
  updateHover();
  update();
}

void NullaEditorView::onMaxColumnsChanged() { updateContentSize(); }

void NullaEditorView::onAutoScroll() {
  if (!m_dragging || !m_session) {
    m_autoScrollTimer.stop();
    return;
  }
  const qreal gw = gutterWidth();
  const qreal viewW = qMax(1.0, width() - gw - m_rightInset);
  const qreal viewH = qMax(1.0, height() - m_bottomInset);
  qreal dx = 0.0;
  qreal dy = 0.0;

  const qreal localX = m_lastMousePos.x() - gw;
  if (localX < 8.0) {
    dx = -qMax(12.0, (8.0 - localX) * 0.6);
  } else if (localX > viewW - 8.0) {
    dx = qMax(12.0, (localX - (viewW - 8.0)) * 0.6);
  }

  if (m_lastMousePos.y() < 8.0) {
    dy = -qMax(m_lineHeight, (8.0 - m_lastMousePos.y()) * 0.6);
  } else if (m_lastMousePos.y() > viewH - 8.0) {
    dy = qMax(m_lineHeight, (m_lastMousePos.y() - (viewH - 8.0)) * 0.6);
  }

  if (dx != 0.0 || dy != 0.0) {
    emit scrollByRequested(dx, dy);
  }
  const HitPos h = hitAt(m_lastMousePos);
  m_session->setSelectionBytes(m_dragAnchor, h.byte);
}

int NullaEditorView::pageLines() const {
  return qMax(1, static_cast<int>(std::floor(
                     qMax(1.0, height() - m_bottomInset - kPadTop) /
                     m_lineHeight)));
}

void NullaEditorView::copyToClipboard(bool cut) {
  if (!m_session) {
    return;
  }
  const QString txt =
      cut ? m_session->cutSelectionOrLine() : m_session->copySelectionOrLine();
  if (QClipboard *cb = QGuiApplication::clipboard()) {
    cb->setText(txt, QClipboard::Clipboard);
  }
  if (cut) {
    emit edited();
    emit cursorMoved();
  }
}

void NullaEditorView::pasteFromClipboard(bool selectionMode) {
  if (!m_session) {
    return;
  }
  const QClipboard *cb = QGuiApplication::clipboard();
  if (!cb) {
    return;
  }
  const QClipboard::Mode mode = (selectionMode && cb->supportsSelection())
                                    ? QClipboard::Selection
                                    : QClipboard::Clipboard;
  const QString txt = cb->text(mode);
  if (txt.isEmpty()) {
    return;
  }
  m_session->insertText(txt);
  emit edited();
  emit cursorMoved();
}

void NullaEditorView::pushPrimarySelection() {
  if (!m_session || !m_session->hasSelection()) {
    return;
  }
  if (QClipboard *cb = QGuiApplication::clipboard();
      cb && cb->supportsSelection()) {
    const size_t s =
        qMin(m_session->cursorByte(), m_session->anchorByte());
    const size_t e =
        qMax(m_session->cursorByte(), m_session->anchorByte());
    if (e - s <= 1024 * 1024) {
      cb->setText(m_session->selectedText(), QClipboard::Selection);
    }
  }
}

bool NullaEditorView::wantsShortcut(const QKeyEvent *e) const {
  const Qt::KeyboardModifiers m = e->modifiers();
  const bool ctrl = m.testFlag(Qt::ControlModifier);
  const bool alt = m.testFlag(Qt::AltModifier);
  const bool meta = m.testFlag(Qt::MetaModifier);
  const bool shift = m.testFlag(Qt::ShiftModifier);
  const int k = e->key();

  if (k == Qt::Key_Tab || k == Qt::Key_Backtab || k == Qt::Key_Menu) {
    return true;
  }
  if (alt && !ctrl && !meta && (k == Qt::Key_Up || k == Qt::Key_Down)) {
    return true;
  }
  if (ctrl && !alt && !meta) {
    if (isRussianShortcutKey(k) || k == Qt::Key_D || k == Qt::Key_L ||
        k == Qt::Key_Slash || k == Qt::Key_Question || k == Qt::Key_Return ||
        k == Qt::Key_Enter || k == Qt::Key_X || k == Qt::Key_C ||
        k == Qt::Key_V || k == Qt::Key_A || k == Qt::Key_Z || k == Qt::Key_Y ||
        (shift && k == Qt::Key_K) || k == Qt::Key_Left ||
        k == Qt::Key_Right || k == Qt::Key_Home || k == Qt::Key_End ||
        k == Qt::Key_Backspace || k == Qt::Key_Delete) {
      return true;
    }
  }
  return false;
}

bool NullaEditorView::event(QEvent *e) {
  if (e->type() == QEvent::ShortcutOverride) {
    auto *ke = static_cast<QKeyEvent *>(e);
    if (wantsShortcut(ke)) {
      ke->accept();
      return true;
    }
  } else if (e->type() == QEvent::KeyPress) {
    auto *ke = static_cast<QKeyEvent *>(e);
    if (ke->key() == Qt::Key_Tab || ke->key() == Qt::Key_Backtab) {
      keyPressEvent(ke);
      if (ke->isAccepted()) {
        return true;
      }
    }
  }
  return QQuickPaintedItem::event(e);
}

bool NullaEditorView::handleKey(QKeyEvent *e) {
  if (!m_session) {
    return false;
  }

  const int k = e->key();
  const Qt::KeyboardModifiers m = e->modifiers();
  const bool ctrl = m.testFlag(Qt::ControlModifier);
  const bool shift = m.testFlag(Qt::ShiftModifier);
  const bool alt = m.testFlag(Qt::AltModifier);
  const bool meta = m.testFlag(Qt::MetaModifier);

  if (k == Qt::Key_Menu && !ctrl && !alt && !meta) {
    emit contextMenuRequested(gutterWidth() + m_cursorRect.x(),
                              m_cursorRect.y() + m_cursorRect.height());
    return true;
  }
  if (k == Qt::Key_Tab && !ctrl && !alt && !shift) {
    m_session->indent(false);
    emit edited();
    emit cursorMoved();
    return true;
  }
  if ((k == Qt::Key_Backtab || (k == Qt::Key_Tab && shift)) && !ctrl &&
      !alt) {
    m_session->indent(true);
    emit edited();
    emit cursorMoved();
    return true;
  }
  if (alt && !ctrl && !meta) {
    if (k == Qt::Key_Up) {
      if (m_session->moveLines(true)) {
        emit edited();
        emit cursorMoved();
      }
      return true;
    }
    if (k == Qt::Key_Down) {
      if (m_session->moveLines(false)) {
        emit edited();
        emit cursorMoved();
      }
      return true;
    }
  }

  if (ctrl && !alt && !meta) {
    if (k == Qt::Key_Return || k == Qt::Key_Enter) {
      m_session->insertLineSibling(shift);
      emit edited();
      emit cursorMoved();
      return true;
    }
    if ((k == Qt::Key_Z && shift) || k == Qt::Key_Y || k == 0x041D ||
        k == 0x043D) {
      m_session->redo();
      emit edited();
      emit cursorMoved();
      return true;
    }
    if (k == Qt::Key_Z || k == 0x042F || k == 0x044F) {
      if (shift) {
        m_session->redo();
      } else {
        m_session->undo();
      }
      emit edited();
      emit cursorMoved();
      return true;
    }
    if ((k == Qt::Key_X || k == 0x0427 || k == 0x0447) && !shift) {
      copyToClipboard(true);
      return true;
    }
    if ((k == Qt::Key_C || k == 0x0421 || k == 0x0441) && !shift) {
      copyToClipboard(false);
      return true;
    }
    if ((k == Qt::Key_V || k == 0x041C || k == 0x043C) && !shift) {
      pasteFromClipboard(false);
      return true;
    }
    if ((k == Qt::Key_A || k == 0x0424 || k == 0x0444) && !shift) {
      m_session->selectAll();
      pushPrimarySelection();
      return true;
    }
    if (k == Qt::Key_D || k == 0x0412 || k == 0x0432) {
      m_session->duplicate();
      emit edited();
      emit cursorMoved();
      return true;
    }
    if (shift && (k == Qt::Key_K || k == 0x041B || k == 0x043B)) {
      m_session->deleteLines();
      emit edited();
      emit cursorMoved();
      return true;
    }
    if ((k == Qt::Key_L || k == 0x0414 || k == 0x0434) && !shift) {
      m_session->selectCurrentLine();
      pushPrimarySelection();
      return true;
    }
    if (k == Qt::Key_Slash || k == Qt::Key_Question) {
      m_session->toggleComment();
      emit edited();
      emit cursorMoved();
      return true;
    }
    if (k == 0x0410 || k == 0x0430) {
      emit actionRequested(QStringLiteral("find"));
      return true;
    }
    if (k == 0x0420 || k == 0x0440) {
      emit actionRequested(QStringLiteral("replace"));
      return true;
    }
    if (k == 0x041F || k == 0x043F) {
      emit actionRequested(QStringLiteral("goto"));
      return true;
    }
    if (k == 0x042B || k == 0x044B) {
      emit actionRequested(shift ? QStringLiteral("saveAs")
                                 : QStringLiteral("save"));
      return true;
    }
    if (k == 0x0429 || k == 0x0449) {
      emit actionRequested(QStringLiteral("open"));
      return true;
    }
    if (k == 0x0415 || k == 0x0435 || k == 0x0422 || k == 0x0442) {
      emit actionRequested(QStringLiteral("newTab"));
      return true;
    }
    if (k == 0x0426 || k == 0x0446) {
      emit actionRequested(QStringLiteral("closeTab"));
      return true;
    }
    if (k == 0x0419 || k == 0x0439) {
      emit actionRequested(QStringLiteral("quit"));
      return true;
    }
    if (k == 0x0411 || k == 0x0431) {
      emit actionRequested(QStringLiteral("settings"));
      return true;
    }
  }

  if (!alt && !meta) {
    if (k == Qt::Key_Left) {
      m_session->moveCursor(ctrl ? NULLA_MOVE_WORD_LEFT : NULLA_MOVE_LEFT,
                            shift, pageLines());
      emit cursorMoved();
      return true;
    }
    if (k == Qt::Key_Right) {
      m_session->moveCursor(ctrl ? NULLA_MOVE_WORD_RIGHT : NULLA_MOVE_RIGHT,
                            shift, pageLines());
      emit cursorMoved();
      return true;
    }
    if (k == Qt::Key_Up && !ctrl) {
      m_session->moveCursor(NULLA_MOVE_UP, shift, pageLines());
      emit cursorMoved();
      return true;
    }
    if (k == Qt::Key_Down && !ctrl) {
      m_session->moveCursor(NULLA_MOVE_DOWN, shift, pageLines());
      emit cursorMoved();
      return true;
    }
    if (k == Qt::Key_Home) {
      m_session->moveCursor(ctrl ? NULLA_MOVE_DOC_START : NULLA_MOVE_HOME,
                            shift, pageLines());
      emit cursorMoved();
      return true;
    }
    if (k == Qt::Key_End) {
      m_session->moveCursor(ctrl ? NULLA_MOVE_DOC_END : NULLA_MOVE_END, shift,
                            pageLines());
      emit cursorMoved();
      return true;
    }
    if (k == Qt::Key_PageUp && !ctrl) {
      m_session->moveCursor(NULLA_MOVE_PAGE_UP, shift, pageLines());
      emit cursorMoved();
      return true;
    }
    if (k == Qt::Key_PageDown && !ctrl) {
      m_session->moveCursor(NULLA_MOVE_PAGE_DOWN, shift, pageLines());
      emit cursorMoved();
      return true;
    }
    if (k == Qt::Key_Backspace) {
      m_session->backspace(ctrl);
      emit edited();
      emit cursorMoved();
      return true;
    }
    if (k == Qt::Key_Delete) {
      m_session->deleteForward(ctrl);
      emit edited();
      emit cursorMoved();
      return true;
    }
    if ((k == Qt::Key_Return || k == Qt::Key_Enter) && !ctrl) {
      m_session->insertNewline();
      emit edited();
      emit cursorMoved();
      return true;
    }
  }

  if (!ctrl && !meta && (!alt || !e->text().isEmpty())) {
    const QString txt = e->text();
    if (!txt.isEmpty()) {
      const QChar first = txt.at(0);
      if (first.unicode() >= 0x20 && first.unicode() != 0x7F) {
        m_session->insertText(txt);
        emit edited();
        emit cursorMoved();
        return true;
      }
    }
  }

  return false;
}

void NullaEditorView::keyPressEvent(QKeyEvent *e) {
  if (handleKey(e)) {
    restartBlink();
    e->accept();
  } else {
    e->ignore();
  }
}

void NullaEditorView::inputMethodEvent(QInputMethodEvent *e) {
  if (!m_session) {
    e->ignore();
    return;
  }
  const int repStart = e->replacementStart();
  const int repLen = e->replacementLength();
  if (repLen > 0) {
    if (repStart < 0) {
      for (int i = 0; i < -repStart; ++i) {
        m_session->backspace(false);
      }
    }
  }
  m_preedit = e->preeditString();
  m_preeditCursor = m_preedit.size();
  for (const auto &attr : e->attributes()) {
    if (attr.type == QInputMethodEvent::Cursor) {
      m_preeditCursor = attr.start;
    }
  }
  const QString commit = e->commitString();
  if (!commit.isEmpty()) {
    m_preedit.clear();
    m_preeditCursor = 0;
    m_session->insertText(commit);
    emit edited();
    emit cursorMoved();
  }
  restartBlink();
  update();
  e->accept();
}

QVariant NullaEditorView::inputMethodQuery(Qt::InputMethodQuery query) const {
  switch (query) {
  case Qt::ImEnabled:
    return true;
  case Qt::ImCursorRectangle:
    return QRectF(gutterWidth() + m_cursorRect.x(), m_cursorRect.y(), 2.0,
                  m_lineHeight);
  case Qt::ImFont:
    return m_font;
  default:
    return QQuickPaintedItem::inputMethodQuery(query);
  }
}

void NullaEditorView::mousePressEvent(QMouseEvent *e) {
  forceActiveFocus();
  if (!m_session) {
    return;
  }
  const QPointF p = e->position();

  if (e->button() == Qt::RightButton) {
    const HitPos h = hitAt(p);
    const size_t s = qMin(m_session->cursorByte(), m_session->anchorByte());
    const size_t end = qMax(m_session->cursorByte(), m_session->anchorByte());
    if (!m_session->hasSelection() || h.byte < s || h.byte > end) {
      m_session->setSelectionBytes(h.byte, h.byte);
    }
    emit contextMenuRequested(p.x(), p.y());
    e->accept();
    return;
  }

  if (e->button() == Qt::MiddleButton) {
    const HitPos h = hitAt(p);
    m_session->setSelectionBytes(h.byte, h.byte);
    pasteFromClipboard(true);
    restartBlink();
    e->accept();
    return;
  }

  if (e->button() == Qt::LeftButton) {
    const HitPos h = hitAt(p);
    if (m_lastDoubleClick.isValid() && m_lastDoubleClick.elapsed() <= 450 &&
        (p - m_lastDoubleClickPos).manhattanLength() < 8.0) {
      m_session->selectLineAt(h.line);
      m_dragging = false;
      m_lastDoubleClick.invalidate();
      pushPrimarySelection();
      restartBlink();
      e->accept();
      return;
    }

    if (p.x() < gutterWidth()) {
      m_session->selectLineAt(h.line);
      m_dragAnchor = m_session->anchorByte();
      m_dragging = true;
    } else if (e->modifiers().testFlag(Qt::ShiftModifier)) {
      m_dragAnchor = m_session->anchorByte();
      m_session->setSelectionBytes(m_dragAnchor, h.byte);
      m_dragging = true;
    } else {
      m_dragAnchor = h.byte;
      m_session->setSelectionBytes(h.byte, h.byte);
      m_dragging = true;
    }
    m_lastMousePos = p;
    restartBlink();
    e->accept();
  }
}

void NullaEditorView::mouseMoveEvent(QMouseEvent *e) {
  if (!m_dragging || !m_session ||
      !e->buttons().testFlag(Qt::LeftButton)) {
    return;
  }
  const QPointF p = e->position();
  m_lastMousePos = p;
  const HitPos h = hitAt(p);
  m_session->setSelectionBytes(m_dragAnchor, h.byte);

  const qreal gw = gutterWidth();
  const qreal viewW = width() - gw - m_rightInset;
  const qreal viewH = height() - m_bottomInset;
  const bool outside = (p.x() - gw) < 8.0 || (p.x() - gw) > viewW - 8.0 ||
                       p.y() < 8.0 || p.y() > viewH - 8.0;
  if (outside) {
    if (!m_autoScrollTimer.isActive()) {
      m_autoScrollTimer.start();
    }
  } else {
    m_autoScrollTimer.stop();
  }
  e->accept();
}

void NullaEditorView::mouseReleaseEvent(QMouseEvent *e) {
  if (e->button() == Qt::LeftButton && m_dragging) {
    m_dragging = false;
    m_autoScrollTimer.stop();
    pushPrimarySelection();
    e->accept();
  }
}

void NullaEditorView::mouseDoubleClickEvent(QMouseEvent *e) {
  if (e->button() == Qt::LeftButton && m_session &&
      e->position().x() >= gutterWidth()) {
    const HitPos h = hitAt(e->position());
    m_session->selectWordAt(h.line, h.visCol);
    m_dragging = false;
    m_lastDoubleClick.start();
    m_lastDoubleClickPos = e->position();
    pushPrimarySelection();
    restartBlink();
    e->accept();
  }
}

void NullaEditorView::hoverEnterEvent(QHoverEvent *e) {
  m_hovering = true;
  m_hoverPos = e->position();
  updateHover();
}

void NullaEditorView::hoverMoveEvent(QHoverEvent *e) {
  m_hovering = true;
  m_hoverPos = e->position();
  updateHover();
}

void NullaEditorView::hoverLeaveEvent(QHoverEvent *) {
  m_hovering = false;
  updateHover();
}

void NullaEditorView::focusInEvent(QFocusEvent *e) {
  QQuickPaintedItem::focusInEvent(e);
  restartBlink();
}

void NullaEditorView::focusOutEvent(QFocusEvent *e) {
  QQuickPaintedItem::focusOutEvent(e);
  m_blinkTimer.stop();
  m_caretOn = false;
  m_preedit.clear();
  update();
}

void NullaEditorView::geometryChange(const QRectF &newGeometry,
                                     const QRectF &oldGeometry) {
  QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
  updateHover();
  if (newGeometry.size() != oldGeometry.size()) {
    update();
  }
}

void NullaEditorView::applyTokenStyle(QPainter *p, uint8_t kind,
                                      const QColor *overrideColor) {
  if (overrideColor) {
    if (!m_lastStyleOverride || m_lastStyleKind != static_cast<int>(kind)) {
      p->setFont(m_font);
      p->setPen(*overrideColor);
      m_lastStyleOverride = true;
      m_lastStyleKind = static_cast<int>(kind);
    }
    return;
  }
  if (!m_lastStyleOverride && m_lastStyleKind == static_cast<int>(kind)) {
    return;
  }
  m_lastStyleOverride = false;
  m_lastStyleKind = static_cast<int>(kind);

  switch (kind) {
  case NULLA_TOK_KEYWORD:
    p->setFont(m_fontDemi);
    p->setPen(m_darkMode ? QColor(0xC7, 0x92, 0xEA)
                         : QColor(0x7C, 0x3A, 0xED));
    break;
  case NULLA_TOK_TYPE:
    p->setFont(m_fontMedium);
    p->setPen(m_darkMode ? QColor(0x7D, 0xCF, 0xFF)
                         : QColor(0x02, 0x84, 0xC7));
    break;
  case NULLA_TOK_NUMBER:
    p->setFont(m_font);
    p->setPen(m_darkMode ? QColor(0xFF, 0x9E, 0x64)
                         : QColor(0xC2, 0x41, 0x0C));
    break;
  case NULLA_TOK_KEY:
    p->setFont(m_fontMedium);
    p->setPen(m_darkMode ? QColor(0x7A, 0xA2, 0xF7)
                         : QColor(0x25, 0x63, 0xEB));
    break;
  case NULLA_TOK_STRING:
    p->setFont(m_font);
    p->setPen(m_darkMode ? QColor(0x9E, 0xCE, 0x6A)
                         : QColor(0x15, 0x80, 0x3D));
    break;
  case NULLA_TOK_COMMENT:
    p->setFont(m_fontItalic);
    p->setPen(m_darkMode ? QColor(0x63, 0x6A, 0x78)
                         : QColor(0x85, 0x8C, 0x99));
    break;
  default:
    p->setFont(m_font);
    p->setPen(m_textColor);
    break;
  }
}

void NullaEditorView::drawRun(QPainter *p, const uint8_t *text, size_t from,
                              size_t to, size_t startCol, size_t endCol,
                              bool ascii, qreal xBase, qreal baseline) {
  if (to <= from) {
    return;
  }
  const qreal x = xBase + static_cast<qreal>(startCol) * m_charWidth;
  if (ascii && (endCol - startCol) == (to - from)) {
    const QString str = QString::fromUtf8(
        reinterpret_cast<const char *>(text + from),
        static_cast<qsizetype>(to - from));
    p->drawText(QPointF(x, baseline), str);
    return;
  }

  size_t i = from;
  size_t col = startCol;
  while (i < to) {
    if (text[i] == '\t') {
      col += 4 - (col % 4);
      i += 1;
      continue;
    }
    const size_t chunkStart = i;
    const size_t chunkCol = col;
    while (i < to && text[i] != '\t') {
      const unsigned char b = text[i];
      size_t step = 1;
      if (b < 0x80) {
        if (b >= 0x20 && b != 0x7F) {
          col += 1;
        }
      } else {
        uint32_t cp = 0;
        if ((b & 0xE0) == 0xC0 && i + 1 < to) {
          cp = ((b & 0x1F) << 6) | (text[i + 1] & 0x3F);
          step = 2;
        } else if ((b & 0xF0) == 0xE0 && i + 2 < to) {
          cp = ((b & 0x0F) << 12) | ((text[i + 1] & 0x3F) << 6) |
               (text[i + 2] & 0x3F);
          step = 3;
        } else if ((b & 0xF8) == 0xF0 && i + 3 < to) {
          cp = ((b & 0x07) << 18) | ((text[i + 1] & 0x3F) << 12) |
               ((text[i + 2] & 0x3F) << 6) | (text[i + 3] & 0x3F);
          step = 4;
        }
        const bool wide =
            (cp >= 0x1100 && cp <= 0x115F) || (cp >= 0x2E80 && cp <= 0x303E) ||
            (cp >= 0x3040 && cp <= 0xA4CF) || (cp >= 0xAC00 && cp <= 0xD7A3) ||
            (cp >= 0xF900 && cp <= 0xFAFF) || (cp >= 0xFE10 && cp <= 0xFE6F) ||
            (cp >= 0xFF00 && cp <= 0xFF60) || (cp >= 0x1F300 && cp <= 0x1FAFF) ||
            (cp >= 0x20000 && cp <= 0x3FFFF);
        col += wide ? 2 : 1;
      }
      i += step;
    }
    const QString seg = QString::fromUtf8(
        reinterpret_cast<const char *>(text + chunkStart),
        static_cast<qsizetype>(i - chunkStart));
    p->drawText(QPointF(xBase + static_cast<qreal>(chunkCol) * m_charWidth,
                        baseline),
                seg);
  }
}

void NullaEditorView::drawLineText(QPainter *p,
                                   const NullaViewportSnapshot &snap,
                                   const NullaVisLine &ln, qreal xBase,
                                   qreal baseline,
                                   const QColor *overrideColor) {
  if (ln.text_len == 0 || !snap.utf8_data) {
    return;
  }
  const uint8_t *lineData = snap.utf8_data + ln.text_offset;
  if (ln.token_count == 0 || !snap.tokens) {
    bool allAscii = false;
    const size_t cols =
        utf8VisWidth(reinterpret_cast<const char *>(lineData), ln.text_len,
                     ln.start_vis_col, allAscii);
    applyTokenStyle(p, NULLA_TOK_PLAIN, overrideColor);
    drawRun(p, lineData, 0, ln.text_len, ln.start_vis_col,
            ln.start_vis_col + cols, allAscii, xBase, baseline);
    return;
  }

  size_t col = ln.start_vis_col;
  for (uint32_t ti = 0; ti < ln.token_count; ++ti) {
    const NullaVisToken &tok = snap.tokens[ln.token_offset + ti];
    const size_t from = std::min<size_t>(tok.rel_start, ln.text_len);
    const size_t to = std::min<size_t>(from + tok.rel_len, ln.text_len);
    if (to <= from) {
      continue;
    }
    bool allAscii = false;
    const size_t cols =
        utf8VisWidth(reinterpret_cast<const char *>(lineData + from), to - from,
                     col, allAscii);
    applyTokenStyle(p, tok.kind, overrideColor);
    drawRun(p, lineData, from, to, col, col + cols, allAscii, xBase, baseline);
    col += cols;
  }
}

void NullaEditorView::paint(QPainter *painter) {
  if (!m_session || !m_session->doc() || width() <= 0.0 || height() <= 0.0) {
    return;
  }

  const qreal gw = gutterWidth();
  const qreal textViewW = qMax(1.0, width() - gw);
  const qreal docTop = qMax(0.0, m_contentY - kPadTop);
  const size_t totalLines =
      static_cast<size_t>(qMax(1, m_session->lineCount()));
  const size_t firstLine =
      std::min<size_t>(static_cast<size_t>(std::floor(docTop / m_lineHeight)),
                       totalLines - 1);
  const size_t visLines = static_cast<size_t>(
      std::ceil((height() + kPadTop) / m_lineHeight)) + 2;

  const qreal docLeft = qMax(0.0, m_contentX - kPadLeft);
  const size_t firstVisCol =
      static_cast<size_t>(std::floor(docLeft / m_charWidth));
  const size_t maxVisCols =
      static_cast<size_t>(std::ceil((textViewW + kPadLeft) / m_charWidth)) + 4;

  NullaViewportSnapshot snap{};
  if (!nulla_doc_build_viewport(m_session->doc(), firstLine, visLines,
                                firstVisCol, maxVisCols, m_showIndentGuides,
                                &snap)) {
    return;
  }

  size_t maxSeenCols = 0;
  for (size_t i = 0; i < snap.line_count; ++i) {
    maxSeenCols = std::max(maxSeenCols, snap.lines[i].line_total_vis_cols);
  }
  const int seenCols =
      static_cast<int>(std::min<size_t>(maxSeenCols, 50000000));
  if (seenCols > m_session->maxLineColumns()) {
    QPointer<TabSession> sessionPtr = m_session;
    QMetaObject::invokeMethod(
        m_session,
        [sessionPtr, seenCols]() {
          if (sessionPtr) {
            sessionPtr->noteLineColumns(seenCols);
          }
        },
        Qt::QueuedConnection);
  }

  const bool focused = hasActiveFocus();

  if (m_showLineNumbers && gw > 0.0) {
    painter->save();
    painter->setClipRect(QRectF(0.0, 0.0, gw, height()));
    painter->fillRect(QRectF(0.0, 0.0, gw - 1.0, height()), m_gutterBackground);
    painter->setRenderHint(QPainter::TextAntialiasing, true);
    painter->setFont(m_font);

    for (size_t i = 0; i < snap.line_count; ++i) {
      const NullaVisLine &ln = snap.lines[i];
      const qreal y = kPadTop + static_cast<qreal>(ln.line_idx) * m_lineHeight -
                      m_contentY;
      if (y + m_lineHeight < 0.0 || y > height()) {
        continue;
      }
      const bool isCur = (ln.line_idx == m_cursorLine0);
      if (isCur && focused && m_gutterActiveLineColor.alpha() > 0) {
        painter->fillRect(QRectF(0.0, y, gw - 1.0, m_lineHeight),
                          m_gutterActiveLineColor);
      }
      painter->setPen(isCur ? m_gutterActiveTextColor : m_gutterTextColor);
      painter->drawText(QRectF(0.0, y, gw - 1.0 - kGutterPadRight, m_lineHeight),
                        Qt::AlignRight | Qt::AlignVCenter,
                        QString::number(ln.line_idx + 1));
    }

    painter->fillRect(QRectF(gw - 1.0, 0.0, 1.0, height()), m_gutterBorder);
    painter->restore();
  }

  painter->save();
  painter->setClipRect(QRectF(gw, 0.0, textViewW, height()));

  const qreal xOrigin = gw + kPadLeft - m_contentX;

  for (size_t i = 0; i < snap.line_count; ++i) {
    const NullaVisLine &ln = snap.lines[i];
    if (!ln.has_selection) {
      continue;
    }
    const qreal y = kPadTop + static_cast<qreal>(ln.line_idx) * m_lineHeight -
                    m_contentY;
    if (y + m_lineHeight < 0.0 || y > height()) {
      continue;
    }
    const uint8_t *lineData =
        snap.utf8_data ? (snap.utf8_data + ln.text_offset) : nullptr;
    size_t selColStart = ln.start_vis_col;
    size_t selColEnd = ln.start_vis_col;
    if (ln.sel_before_slice) {
      selColStart = firstVisCol;
    } else if (lineData) {
      selColStart = utf8PrefixVisCol(lineData, ln.text_len, ln.sel_rel_start,
                                     ln.start_vis_col);
    }
    if (ln.eol_selected) {
      selColEnd = std::max(ln.line_total_vis_cols + 1,
                           firstVisCol + maxVisCols + 2);
    } else if (lineData && ln.sel_rel_end > ln.sel_rel_start) {
      selColEnd = utf8PrefixVisCol(lineData, ln.text_len, ln.sel_rel_end,
                                   ln.start_vis_col);
    }
    if (selColEnd > selColStart) {
      const qreal sx = xOrigin + static_cast<qreal>(selColStart) * m_charWidth;
      const qreal sw =
          static_cast<qreal>(selColEnd - selColStart) * m_charWidth;
      painter->fillRect(QRectF(sx, y, sw, m_lineHeight), m_selectionColor);
    }
  }

  if (m_showIndentGuides && snap.guide_count > 0 && snap.guides) {
    painter->setRenderHint(QPainter::Antialiasing, false);
    for (size_t i = 0; i < snap.line_count; ++i) {
      const NullaVisLine &ln = snap.lines[i];
      if (ln.guide_count == 0) {
        continue;
      }
      const qreal y0 = std::round(
          kPadTop + static_cast<qreal>(ln.line_idx) * m_lineHeight - m_contentY);
      const qreal y1 = std::round(
          kPadTop + static_cast<qreal>(ln.line_idx + 1) * m_lineHeight -
          m_contentY);
      if (y1 <= 0.0 || y0 >= height()) {
        continue;
      }
      const qreal gh = qMax(1.0, y1 - y0);
      for (uint32_t gi = 0; gi < ln.guide_count; ++gi) {
        const NullaVisGuide &g = snap.guides[ln.guide_offset + gi];
        const qreal gx =
            std::round(xOrigin + static_cast<qreal>(g.vis_col) * m_charWidth);
        if (gx < gw - 1.0 || gx > width() + 1.0) {
          continue;
        }
        painter->fillRect(QRectF(gx, y0, 1.0, gh),
                          g.is_active ? m_activeGuideColor : m_guideColor);
      }
    }
  }

  painter->setRenderHint(QPainter::TextAntialiasing, true);
  m_lastStyleKind = -1;
  m_lastStyleOverride = false;
  const qreal baselineDelta =
      (m_lineHeight - m_metrics.height()) * 0.5 + m_ascent;

  for (size_t i = 0; i < snap.line_count; ++i) {
    const NullaVisLine &ln = snap.lines[i];
    const qreal y = kPadTop + static_cast<qreal>(ln.line_idx) * m_lineHeight -
                    m_contentY;
    if (y + m_lineHeight < 0.0 || y > height()) {
      continue;
    }
    drawLineText(painter, snap, ln, xOrigin, y + baselineDelta, nullptr);
  }

  if (!m_preedit.isEmpty()) {
    const qreal px =
        xOrigin + static_cast<qreal>(m_cursorVisCol) * m_charWidth;
    const qreal py =
        kPadTop + static_cast<qreal>(m_cursorLine0) * m_lineHeight - m_contentY;
    const qreal pw = qMax(4.0, m_metrics.horizontalAdvance(m_preedit));
    painter->fillRect(QRectF(px, py, pw, m_lineHeight), m_selectionColor);
    painter->setFont(m_font);
    painter->setPen(m_textColor);
    painter->drawText(QPointF(px, py + baselineDelta), m_preedit);
    painter->fillRect(QRectF(px, py + m_lineHeight - 1.5, pw, 1.5),
                      m_caretSelectionColor);
  }

  if (focused && m_caretOn) {
    const qreal preOff = m_preedit.isEmpty()
                             ? 0.0
                             : m_metrics.horizontalAdvance(
                                   m_preedit.left(m_preeditCursor));
    const qreal cx = gw + m_cursorRect.x() + preOff;
    const qreal cy = m_cursorRect.y();
    const qreal cw = m_hasSelection ? 2.5 : 2.0;
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(Qt::NoPen);
    painter->setBrush(m_hasSelection ? m_caretSelectionColor : m_caretColor);
    painter->drawRoundedRect(QRectF(cx, cy, cw, m_lineHeight), 1.0, 1.0);
  }

  painter->restore();
}
