#ifndef NULLA_EDITOR_VIEW_H
#define NULLA_EDITOR_VIEW_H

#include "nulla_core.h"
#include "tab_session.h"

#include <QColor>
#include <QElapsedTimer>
#include <QFont>
#include <QFontMetricsF>
#include <QPointer>
#include <QQuickPaintedItem>
#include <QRectF>
#include <QString>
#include <QTimer>
#include <QtQml/qqmlregistration.h>


class NullaEditorView : public QQuickPaintedItem {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(
      TabSession *session READ session WRITE setSession NOTIFY sessionChanged)
  Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY metricsChanged)
  Q_PROPERTY(
      bool darkMode READ darkMode WRITE setDarkMode NOTIFY appearanceChanged)
  Q_PROPERTY(bool showLineNumbers READ showLineNumbers WRITE setShowLineNumbers
                 NOTIFY metricsChanged)
  Q_PROPERTY(bool showIndentGuides READ showIndentGuides WRITE
                 setShowIndentGuides NOTIFY appearanceChanged)
  Q_PROPERTY(
      qreal contentX READ contentX WRITE setContentX NOTIFY scrollChanged)
  Q_PROPERTY(
      qreal contentY READ contentY WRITE setContentY NOTIFY scrollChanged)
  Q_PROPERTY(
      qreal rightInset READ rightInset WRITE setRightInset NOTIFY insetsChanged)
  Q_PROPERTY(qreal bottomInset READ bottomInset WRITE setBottomInset NOTIFY
                 insetsChanged)
  Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor NOTIFY
                 appearanceChanged)
  Q_PROPERTY(QColor gutterBackground READ gutterBackground WRITE
                 setGutterBackground NOTIFY appearanceChanged)
  Q_PROPERTY(QColor gutterBorder READ gutterBorder WRITE setGutterBorder NOTIFY
                 appearanceChanged)
  Q_PROPERTY(QColor gutterTextColor READ gutterTextColor WRITE
                 setGutterTextColor NOTIFY appearanceChanged)
  Q_PROPERTY(QColor gutterActiveTextColor READ gutterActiveTextColor WRITE
                 setGutterActiveTextColor NOTIFY appearanceChanged)
  Q_PROPERTY(QColor gutterActiveLineColor READ gutterActiveLineColor WRITE
                 setGutterActiveLineColor NOTIFY appearanceChanged)
  Q_PROPERTY(QColor selectionColor READ selectionColor WRITE setSelectionColor
                 NOTIFY appearanceChanged)
  Q_PROPERTY(QColor caretColor READ caretColor WRITE setCaretColor NOTIFY
                 appearanceChanged)
  Q_PROPERTY(QColor caretSelectionColor READ caretSelectionColor WRITE
                 setCaretSelectionColor NOTIFY appearanceChanged)
  Q_PROPERTY(QColor guideColor READ guideColor WRITE setGuideColor NOTIFY
                 appearanceChanged)
  Q_PROPERTY(QColor activeGuideColor READ activeGuideColor WRITE
                 setActiveGuideColor NOTIFY appearanceChanged)
  Q_PROPERTY(qreal gutterWidth READ gutterWidth NOTIFY metricsChanged)
  Q_PROPERTY(qreal lineHeight READ lineHeight NOTIFY metricsChanged)
  Q_PROPERTY(qreal charWidth READ charWidth NOTIFY metricsChanged)
  Q_PROPERTY(
      qreal docContentWidth READ docContentWidth NOTIFY contentSizeChanged)
  Q_PROPERTY(
      qreal docContentHeight READ docContentHeight NOTIFY contentSizeChanged)
  Q_PROPERTY(QRectF cursorRect READ cursorRect NOTIFY cursorRectChanged)
  Q_PROPERTY(QRectF hoverRect READ hoverRect NOTIFY hoverChanged)
  Q_PROPERTY(bool hoverInside READ hoverInside NOTIFY hoverChanged)
  Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY cursorRectChanged)

public:
  explicit NullaEditorView(QQuickItem *parent = nullptr);
  ~NullaEditorView() override;

  TabSession *session() const { return m_session; }
  QFont font() const { return m_font; }
  bool darkMode() const { return m_darkMode; }
  bool showLineNumbers() const { return m_showLineNumbers; }
  bool showIndentGuides() const { return m_showIndentGuides; }
  qreal contentX() const { return m_contentX; }
  qreal contentY() const { return m_contentY; }
  qreal rightInset() const { return m_rightInset; }
  qreal bottomInset() const { return m_bottomInset; }
  QColor textColor() const { return m_textColor; }
  QColor gutterBackground() const { return m_gutterBackground; }
  QColor gutterBorder() const { return m_gutterBorder; }
  QColor gutterTextColor() const { return m_gutterTextColor; }
  QColor gutterActiveTextColor() const { return m_gutterActiveTextColor; }
  QColor gutterActiveLineColor() const { return m_gutterActiveLineColor; }
  QColor selectionColor() const { return m_selectionColor; }
  QColor caretColor() const { return m_caretColor; }
  QColor caretSelectionColor() const { return m_caretSelectionColor; }
  QColor guideColor() const { return m_guideColor; }
  QColor activeGuideColor() const { return m_activeGuideColor; }
  qreal gutterWidth() const { return m_showLineNumbers ? m_gutterWidth : 0.0; }
  qreal lineHeight() const { return m_lineHeight; }
  qreal charWidth() const { return m_charWidth; }
  qreal docContentWidth() const { return m_docContentWidth; }
  qreal docContentHeight() const { return m_docContentHeight; }
  QRectF cursorRect() const { return m_cursorRect; }
  QRectF hoverRect() const { return m_hoverRect; }
  bool hoverInside() const { return m_hoverInside; }
  bool hasSelection() const { return m_hasSelection; }

  void setSession(TabSession *s);
  void setFont(const QFont &f);
  void setDarkMode(bool v);
  void setShowLineNumbers(bool v);
  void setShowIndentGuides(bool v);
  void setContentX(qreal v);
  void setContentY(qreal v);
  void setRightInset(qreal v);
  void setBottomInset(qreal v);
  void setTextColor(const QColor &c);
  void setGutterBackground(const QColor &c);
  void setGutterBorder(const QColor &c);
  void setGutterTextColor(const QColor &c);
  void setGutterActiveTextColor(const QColor &c);
  void setGutterActiveLineColor(const QColor &c);
  void setSelectionColor(const QColor &c);
  void setCaretColor(const QColor &c);
  void setCaretSelectionColor(const QColor &c);
  void setGuideColor(const QColor &c);
  void setActiveGuideColor(const QColor &c);

  Q_INVOKABLE void performAction(const QString &action);
  Q_INVOKABLE bool clipboardHasText() const;
  Q_INVOKABLE QPointF cursorViewportPos() const;

  void paint(QPainter *painter) override;
  QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;

signals:
  void sessionChanged();
  void metricsChanged();
  void appearanceChanged();
  void scrollChanged();
  void insetsChanged();
  void contentSizeChanged();
  void cursorRectChanged();
  void hoverChanged();
  void edited();
  void cursorMoved();
  void contextMenuRequested(qreal x, qreal y);
  void scrollByRequested(qreal dx, qreal dy);
  void actionRequested(const QString &action);

protected:
  bool event(QEvent *e) override;
  void keyPressEvent(QKeyEvent *e) override;
  void inputMethodEvent(QInputMethodEvent *e) override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseMoveEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;
  void mouseDoubleClickEvent(QMouseEvent *e) override;
  void hoverEnterEvent(QHoverEvent *e) override;
  void hoverMoveEvent(QHoverEvent *e) override;
  void hoverLeaveEvent(QHoverEvent *e) override;
  void focusInEvent(QFocusEvent *e) override;
  void focusOutEvent(QFocusEvent *e) override;
  void geometryChange(const QRectF &newGeometry,
                      const QRectF &oldGeometry) override;

private slots:
  void onContentChanged();
  void onSelectionChanged();
  void onMaxColumnsChanged();
  void onBlink();
  void onAutoScroll();

private:
  struct HitPos {
    size_t line = 0;
    size_t visCol = 0;
    size_t byte = 0;
  };

  bool wantsShortcut(const QKeyEvent *e) const;
  bool handleKey(QKeyEvent *e);
  HitPos hitAt(const QPointF &pos) const;
  void updateMetrics();
  void updateGutterWidth();
  void updateContentSize();
  void syncCursor();
  void updateHover();
  void restartBlink();
  void copyToClipboard(bool cut);
  void pasteFromClipboard(bool selectionMode);
  void pushPrimarySelection();
  int pageLines() const;
  void drawLineText(QPainter *p, const NullaViewportSnapshot &snap,
                    const NullaVisLine &ln, qreal xBase, qreal baseline,
                    const QColor *overrideColor);
  void drawRun(QPainter *p, const uint8_t *text, size_t from, size_t to,
               size_t startCol, size_t endCol, bool ascii, qreal xBase,
               qreal baseline);
  void applyTokenStyle(QPainter *p, uint8_t kind, const QColor *overrideColor);

  QPointer<TabSession> m_session;
  QFont m_font;
  QFont m_fontDemi;
  QFont m_fontMedium;
  QFont m_fontItalic;
  QFontMetricsF m_metrics;
  bool m_darkMode = true;
  bool m_showLineNumbers = true;
  bool m_showIndentGuides = true;
  qreal m_contentX = 0.0;
  qreal m_contentY = 0.0;
  qreal m_rightInset = 0.0;
  qreal m_bottomInset = 0.0;

  QColor m_textColor = QColor(0xec, 0xee, 0xf2);
  QColor m_gutterBackground = QColor(0x13, 0x14, 0x18);
  QColor m_gutterBorder = QColor(0x22, 0x24, 0x2c);
  QColor m_gutterTextColor = QColor(0x4a, 0x4f, 0x5c);
  QColor m_gutterActiveTextColor = QColor(0xec, 0xee, 0xf2);
  QColor m_gutterActiveLineColor = Qt::transparent;
  QColor m_selectionColor = QColor(0x2e, 0x3d, 0x59);
  QColor m_caretColor = QColor(0xec, 0xee, 0xf2);
  QColor m_caretSelectionColor = QColor(0x8f, 0xb4, 0xff);
  QColor m_guideColor = QColor(0x26, 0x2b, 0x36);
  QColor m_activeGuideColor = QColor(0x4e, 0x58, 0x6e);

  qreal m_gutterWidth = 40.0;
  int m_gutterDigits = -1;
  qreal m_lineHeight = 18.0;
  qreal m_charWidth = 8.0;
  qreal m_ascent = 14.0;
  qreal m_docContentWidth = 0.0;
  qreal m_docContentHeight = 0.0;

  size_t m_cursorLine0 = 0;
  size_t m_cursorVisCol = 0;
  QRectF m_cursorRect;
  bool m_hasSelection = false;

  bool m_hovering = false;
  QPointF m_hoverPos;
  QRectF m_hoverRect;
  bool m_hoverInside = false;

  bool m_caretOn = true;
  QTimer m_blinkTimer;

  bool m_dragging = false;
  size_t m_dragAnchor = 0;
  QPointF m_lastMousePos;
  QTimer m_autoScrollTimer;
  QElapsedTimer m_lastDoubleClick;
  QPointF m_lastDoubleClickPos;

  QString m_preedit;
  int m_preeditCursor = 0;

  int m_lastStyleKind = -1;
  bool m_lastStyleOverride = false;
};

#endif
