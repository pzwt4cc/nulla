#ifndef NULLA_INDENT_GUIDES_H
#define NULLA_INDENT_GUIDES_H

#include <QColor>
#include <QFont>
#include <QPointer>
#include <QQuickPaintedItem>
#include <QQuickTextDocument>
#include <QtQml/qqmlregistration.h>

class IndentGuides : public QQuickPaintedItem {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QQuickTextDocument *document READ document WRITE setDocument NOTIFY
                 documentChanged)
  Q_PROPERTY(
      qreal contentX READ contentX WRITE setContentX NOTIFY contentXChanged)
  Q_PROPERTY(
      qreal contentY READ contentY WRITE setContentY NOTIFY contentYChanged)
  Q_PROPERTY(int currentLine READ currentLine WRITE setCurrentLine NOTIFY
                 currentLineChanged)
  Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)
  Q_PROPERTY(QColor guideColor READ guideColor WRITE setGuideColor NOTIFY
                 guideColorChanged)
  Q_PROPERTY(QColor activeGuideColor READ activeGuideColor WRITE
                 setActiveGuideColor NOTIFY activeGuideColorChanged)
  Q_PROPERTY(qreal leftPadding READ leftPadding WRITE setLeftPadding NOTIFY
                 leftPaddingChanged)
  Q_PROPERTY(qreal topPadding READ topPadding WRITE setTopPadding NOTIFY
                 topPaddingChanged)

public:
  explicit IndentGuides(QQuickItem *parent = nullptr);

  QQuickTextDocument *document() const { return m_document; }
  void setDocument(QQuickTextDocument *doc);

  qreal contentX() const { return m_contentX; }
  void setContentX(qreal x);

  qreal contentY() const { return m_contentY; }
  void setContentY(qreal y);

  int currentLine() const { return m_currentLine; }
  void setCurrentLine(int line);

  QFont font() const { return m_font; }
  void setFont(const QFont &f);

  QColor guideColor() const { return m_guideColor; }
  void setGuideColor(const QColor &c);

  QColor activeGuideColor() const { return m_activeGuideColor; }
  void setActiveGuideColor(const QColor &c);

  qreal leftPadding() const { return m_leftPadding; }
  void setLeftPadding(qreal p);

  qreal topPadding() const { return m_topPadding; }
  void setTopPadding(qreal p);

  void paint(QPainter *painter) override;

public slots:
  void refresh();

signals:
  void documentChanged();
  void contentXChanged();
  void contentYChanged();
  void currentLineChanged();
  void fontChanged();
  void guideColorChanged();
  void activeGuideColorChanged();
  void leftPaddingChanged();
  void topPaddingChanged();

private slots:
  void onContentsChanged();

private:
  void updateMetrics();
  void detectIndentStep();

  QPointer<QQuickTextDocument> m_document;
  qreal m_contentX = 0.0;
  qreal m_contentY = 0.0;
  int m_currentLine = 1;
  QFont m_font;
  QColor m_guideColor = QColor(QStringLiteral("#262b36"));
  QColor m_activeGuideColor = QColor(QStringLiteral("#4d566b"));
  qreal m_leftPadding = 10.0;
  qreal m_topPadding = 8.0;
  qreal m_spaceWidth = 8.0;
  qreal m_lineHeight = 16.0;
  int m_indentStep = 4;
};

#endif
