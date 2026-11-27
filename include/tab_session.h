#ifndef NULLA_TAB_SESSION_H
#define NULLA_TAB_SESSION_H

#include "nulla_core.h"

#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

class TabSession : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("Managed by EditorBackend")
  Q_PROPERTY(QString filePath READ filePath NOTIFY fileInfoChanged)
  Q_PROPERTY(QString fileName READ fileName NOTIFY fileInfoChanged)
  Q_PROPERTY(bool modified READ modified NOTIFY modifiedChanged)
  Q_PROPERTY(QString languageId READ languageId WRITE setLanguageId NOTIFY
                 languageChanged)
  Q_PROPERTY(
      QString statusLanguage READ statusLanguage NOTIFY languageStatusChanged)
  Q_PROPERTY(
      QString commentPrefix READ commentPrefix NOTIFY languageStatusChanged)
  Q_PROPERTY(int lineCount READ lineCount NOTIFY contentChanged)
  Q_PROPERTY(bool hasText READ hasText NOTIFY contentChanged)
  Q_PROPERTY(bool canUndo READ canUndo NOTIFY contentChanged)
  Q_PROPERTY(bool canRedo READ canRedo NOTIFY contentChanged)
  Q_PROPERTY(int cursorLine READ cursorLine NOTIFY cursorStatsChanged)
  Q_PROPERTY(int cursorColumn READ cursorColumn NOTIFY cursorStatsChanged)
  Q_PROPERTY(int selectedChars READ selectedChars NOTIFY selectionChanged)
  Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY selectionChanged)
  Q_PROPERTY(bool hasSingleLineSelection READ hasSingleLineSelection NOTIFY
                 selectionChanged)
  Q_PROPERTY(qreal scrollX READ scrollX WRITE setScrollX NOTIFY scrollChanged)
  Q_PROPERTY(qreal scrollY READ scrollY WRITE setScrollY NOTIFY scrollChanged)
  Q_PROPERTY(
      int maxLineColumns READ maxLineColumns NOTIFY maxLineColumnsChanged)

public:
  explicit TabSession(QObject *parent = nullptr);
  ~TabSession() override;

  NullaDoc *doc() const { return m_doc; }

  QString filePath() const { return m_filePath; }
  QString fileName() const;
  bool modified() const { return m_modified; }
  QString languageId() const { return m_languageId; }
  QString statusLanguage() const;
  QString commentPrefix() const;
  int lineCount() const;
  bool hasText() const;
  bool canUndo() const;
  bool canRedo() const;
  int cursorLine() const { return m_cursorLine; }
  int cursorColumn() const { return m_cursorColumn; }
  int selectedChars() const { return m_selectedChars; }
  bool hasSelection() const;
  bool hasSingleLineSelection() const;
  qreal scrollX() const { return m_scrollX; }
  qreal scrollY() const { return m_scrollY; }
  int maxLineColumns() const { return m_maxLineColumns; }

  bool isPristineUntitled() const;
  void setLanguageId(const QString &id);
  void setScrollX(qreal x);
  void setScrollY(qreal y);
  void setPendingInitialFile(const QString &path, bool allowMissing = true);
  void setSyntaxEnabled(bool enabled);
  void noteLineColumns(int cols);

  size_t cursorByte() const;
  size_t anchorByte() const;
  size_t hitTest(size_t line, size_t visCol) const;
  void cursorLineVisCol(size_t &line, size_t &visCol) const;

  void moveCursor(int dir, bool extend, int pageLines);
  void setSelectionBytes(size_t anchor, size_t cursor);
  void selectWordAt(size_t line, size_t visCol);
  void selectLineAt(size_t line);
  void insertText(const QString &text);
  void replaceUtf16Range(int relStart, int repLen, const QString &text);
  void insertNewline();
  void backspace(bool word);
  void deleteForward(bool word);
  void indent(bool unindent);
  void toggleComment();
  void duplicate();
  void deleteLines();
  void insertLineSibling(bool above);
  bool moveLines(bool up);
  QString copySelectionOrLine();
  QString cutSelectionOrLine();

  Q_INVOKABLE bool loadFile(const QString &path, bool allowMissing = false);
  Q_INVOKABLE bool writeToFile(const QString &path);
  Q_INVOKABLE void resetEmpty();
  Q_INVOKABLE void discardChanges();
  Q_INVOKABLE void undo();
  Q_INVOKABLE void redo();
  Q_INVOKABLE void selectAll();
  Q_INVOKABLE void selectCurrentLine();
  Q_INVOKABLE QString selectedText() const;
  Q_INVOKABLE bool findNext(const QString &query, bool backward);
  Q_INVOKABLE QVariantMap matchStats(const QString &query) const;
  Q_INVOKABLE bool replaceMatch(const QString &query,
                                const QString &replacement);
  Q_INVOKABLE int replaceAllMatches(const QString &query,
                                    const QString &replacement);
  Q_INVOKABLE void gotoLine(int lineNumber);

signals:
  void fileInfoChanged();
  void modifiedChanged();
  void languageChanged();
  void languageStatusChanged();
  void contentChanged();
  void cursorStatsChanged();
  void selectionChanged();
  void scrollChanged();
  void maxLineColumnsChanged();
  void documentLoaded();
  void errorOccurred(const QString &title, const QString &message);

private slots:
  void runAutoDetect();
  void scanWidthChunk();

private:
  void finishEdit();
  void finishMove();
  void refreshCursorStats();
  void refreshModified();
  void applyLanguage();
  void restartWidthScan();
  void emitAllAfterReset();

  NullaDoc *m_doc = nullptr;
  QTimer m_autoDetectTimer;
  QTimer m_widthScanTimer;
  size_t m_widthScanLine = 0;

  QString m_filePath;
  bool m_modified = false;
  bool m_syntaxEnabled = true;
  QString m_languageId = QStringLiteral("auto");

  int m_cursorLine = 1;
  int m_cursorColumn = 1;
  int m_selectedChars = 0;
  qreal m_scrollX = 0.0;
  qreal m_scrollY = 0.0;
  int m_maxLineColumns = 0;
};

#endif
