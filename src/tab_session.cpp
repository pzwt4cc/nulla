#include "tab_session.h"

#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QUrl>

namespace {

const uint8_t *u8(const QByteArray &b) {
  return reinterpret_cast<const uint8_t *>(b.constData());
}

size_t u8len(const QByteArray &b) { return static_cast<size_t>(b.size()); }

QString fromOut(const uint8_t *p, size_t n) {
  if (!p || n == 0) {
    return QString();
  }
  return QString::fromUtf8(reinterpret_cast<const char *>(p),
                           static_cast<qsizetype>(n));
}

uint64_t nowMs() {
  static QElapsedTimer timer = [] {
    QElapsedTimer t;
    t.start();
    return t;
  }();
  return static_cast<uint64_t>(timer.elapsed()) + 1;
}

constexpr size_t kWidthScanChunk = 20000;

}

TabSession::TabSession(QObject *parent)
    : QObject(parent), m_doc(nulla_doc_new()) {
  m_autoDetectTimer.setSingleShot(true);
  m_autoDetectTimer.setInterval(120);
  connect(&m_autoDetectTimer, &QTimer::timeout, this,
          &TabSession::runAutoDetect);
  m_widthScanTimer.setSingleShot(true);
  m_widthScanTimer.setInterval(0);
  connect(&m_widthScanTimer, &QTimer::timeout, this,
          &TabSession::scanWidthChunk);
  applyLanguage();
}

TabSession::~TabSession() {
  m_autoDetectTimer.stop();
  m_widthScanTimer.stop();
  nulla_doc_free(m_doc);
  m_doc = nullptr;
}

QString TabSession::fileName() const {
  if (m_filePath.isEmpty()) {
    return QStringLiteral("Untitled");
  }
  const QString base = QFileInfo(m_filePath).fileName();
  return base.isEmpty() ? QStringLiteral("Untitled") : base;
}

QString TabSession::statusLanguage() const {
  if (!m_syntaxEnabled) {
    return QStringLiteral("Plain");
  }
  const uint8_t *p = nullptr;
  size_t n = 0;
  nulla_doc_language_name(m_doc, &p, &n);
  return fromOut(p, n);
}

QString TabSession::commentPrefix() const {
  const uint8_t *p = nullptr;
  size_t n = 0;
  nulla_doc_comment_prefix(m_doc, &p, &n);
  return fromOut(p, n);
}

int TabSession::lineCount() const {
  const size_t n = nulla_doc_line_count(m_doc);
  return static_cast<int>(qMin<size_t>(n, static_cast<size_t>(INT_MAX)));
}

bool TabSession::hasText() const { return nulla_doc_byte_len(m_doc) > 0; }

bool TabSession::canUndo() const { return nulla_doc_can_undo(m_doc); }

bool TabSession::canRedo() const { return nulla_doc_can_redo(m_doc); }

bool TabSession::hasSelection() const { return nulla_doc_has_selection(m_doc); }

bool TabSession::hasSingleLineSelection() const {
  return nulla_doc_has_single_line_selection(m_doc);
}

bool TabSession::isPristineUntitled() const {
  return !m_modified && m_filePath.isEmpty() && nulla_doc_byte_len(m_doc) == 0;
}

void TabSession::setLanguageId(const QString &id) {
  const QString normalized = id.isEmpty() ? QStringLiteral("auto") : id;
  if (m_languageId == normalized) {
    return;
  }
  m_languageId = normalized;
  applyLanguage();
  emit languageChanged();
  emit languageStatusChanged();
  emit contentChanged();
}

void TabSession::setScrollX(qreal x) {
  if (qFuzzyCompare(m_scrollX + 1.0, x + 1.0)) {
    return;
  }
  m_scrollX = x;
  emit scrollChanged();
}

void TabSession::setScrollY(qreal y) {
  if (qFuzzyCompare(m_scrollY + 1.0, y + 1.0)) {
    return;
  }
  m_scrollY = y;
  emit scrollChanged();
}

void TabSession::setPendingInitialFile(const QString &path, bool allowMissing) {
  if (!path.isEmpty()) {
    loadFile(path, allowMissing);
  }
}

void TabSession::setSyntaxEnabled(bool enabled) {
  if (m_syntaxEnabled == enabled) {
    return;
  }
  m_syntaxEnabled = enabled;
  applyLanguage();
  emit languageStatusChanged();
  emit contentChanged();
}

void TabSession::noteLineColumns(int cols) {
  if (cols > m_maxLineColumns) {
    m_maxLineColumns = cols;
    emit maxLineColumnsChanged();
  }
}

size_t TabSession::cursorByte() const { return nulla_doc_cursor_byte(m_doc); }

size_t TabSession::anchorByte() const { return nulla_doc_anchor_byte(m_doc); }

size_t TabSession::hitTest(size_t line, size_t visCol) const {
  return nulla_doc_hit_test_byte(m_doc, line, visCol);
}

void TabSession::cursorLineVisCol(size_t &line, size_t &visCol) const {
  line = 0;
  visCol = 0;
  nulla_doc_byte_to_line_vis_col(m_doc, nulla_doc_cursor_byte(m_doc), &line,
                                 &visCol);
}

void TabSession::moveCursor(int dir, bool extend, int pageLines) {
  nulla_doc_move_cursor(m_doc, dir, extend,
                        static_cast<size_t>(qMax(1, pageLines)));
  finishMove();
}

void TabSession::setSelectionBytes(size_t anchor, size_t cursor) {
  nulla_doc_set_selection(m_doc, anchor, cursor);
  finishMove();
}

void TabSession::selectWordAt(size_t line, size_t visCol) {
  nulla_doc_select_word_at(m_doc, line, visCol);
  finishMove();
}

void TabSession::selectLineAt(size_t line) {
  nulla_doc_select_line_at(m_doc, line);
  finishMove();
}

void TabSession::insertText(const QString &text) {
  const QByteArray b = text.toUtf8();
  nulla_doc_insert_text(m_doc, u8(b), u8len(b), nowMs());
  finishEdit();
  if (b.size() > 256) {
    restartWidthScan();
  }
}

void TabSession::replaceUtf16Range(int relStart, int repLen,
                                   const QString &text) {
  const QByteArray b = text.toUtf8();
  nulla_doc_replace_utf16(m_doc, static_cast<int32_t>(relStart),
                          static_cast<int32_t>(repLen), u8(b), u8len(b),
                          nowMs());
  finishEdit();
  if (b.size() > 256) {
    restartWidthScan();
  }
}

void TabSession::insertNewline() {
  nulla_doc_insert_newline(m_doc, nowMs());
  finishEdit();
}

void TabSession::backspace(bool word) {
  nulla_doc_backspace(m_doc, word, nowMs());
  finishEdit();
}

void TabSession::deleteForward(bool word) {
  nulla_doc_delete_forward(m_doc, word, nowMs());
  finishEdit();
}

void TabSession::indent(bool unindent) {
  nulla_doc_indent(m_doc, unindent, nowMs());
  finishEdit();
}

void TabSession::toggleComment() {
  nulla_doc_toggle_comment(m_doc, nowMs());
  finishEdit();
}

void TabSession::duplicate() {
  nulla_doc_duplicate(m_doc, nowMs());
  finishEdit();
}

void TabSession::deleteLines() {
  nulla_doc_delete_lines(m_doc, nowMs());
  finishEdit();
}

void TabSession::insertLineSibling(bool above) {
  nulla_doc_insert_line_sibling(m_doc, above, nowMs());
  finishEdit();
}

bool TabSession::moveLines(bool up) {
  const bool moved = nulla_doc_move_lines(m_doc, up, nowMs());
  if (moved) {
    finishEdit();
  }
  return moved;
}

QString TabSession::copySelectionOrLine() {
  const uint8_t *p = nullptr;
  size_t n = 0;
  nulla_doc_copy_or_line(m_doc, &p, &n);
  return fromOut(p, n);
}

QString TabSession::cutSelectionOrLine() {
  const uint8_t *p = nullptr;
  size_t n = 0;
  nulla_doc_cut_or_line(m_doc, nowMs(), &p, &n);
  const QString out = fromOut(p, n);
  finishEdit();
  return out;
}

bool TabSession::loadFile(const QString &rawPath, bool allowMissing) {
  QString path = rawPath;
  const QUrl url(rawPath);
  if (url.isValid() && url.isLocalFile()) {
    path = url.toLocalFile();
  }
  const QString absPath = QFileInfo(path).absoluteFilePath();
  const QFileInfo fi(absPath);

  if (fi.isDir()) {
    emit errorOccurred(
        QStringLiteral("Failed to open file"),
        QStringLiteral("Cannot open a directory: %1").arg(absPath));
    return false;
  }

  if (!fi.exists()) {
    if (!allowMissing) {
      emit errorOccurred(
          QStringLiteral("Failed to open file"),
          QStringLiteral("File does not exist: %1").arg(absPath));
      return false;
    }
    nulla_doc_clear(m_doc);
    m_filePath = absPath;
    applyLanguage();
    m_maxLineColumns = 0;
    emit maxLineColumnsChanged();
    emitAllAfterReset();
    return true;
  }

  QFile probe(absPath);
  if (!probe.open(QIODevice::ReadOnly)) {
    emit errorOccurred(QStringLiteral("Failed to open file"),
                       probe.errorString());
    return false;
  }
  probe.close();

  const QByteArray p = absPath.toUtf8();
  if (!nulla_doc_load_file(m_doc, u8(p), u8len(p))) {
    emit errorOccurred(QStringLiteral("Failed to open file"),
                       QStringLiteral("Unable to read: %1").arg(absPath));
    return false;
  }

  m_filePath = absPath;
  applyLanguage();
  m_maxLineColumns = 0;
  emit maxLineColumnsChanged();
  emitAllAfterReset();
  restartWidthScan();
  return true;
}

bool TabSession::writeToFile(const QString &path) {
  const QString target = QFileInfo(path).absoluteFilePath();
  const QFileInfo fi(target);
  QString writePath = target;
  if (fi.isSymLink()) {
    const QString linkTarget = fi.symLinkTarget();
    if (!linkTarget.isEmpty()) {
      writePath = linkTarget;
    }
  }

  const bool existed = QFileInfo::exists(writePath);
  const QFileDevice::Permissions perms =
      existed ? QFile::permissions(writePath) : QFileDevice::Permissions();
  const QFileInfo dirInfo(QFileInfo(writePath).absolutePath());
  if (!dirInfo.exists()) {
    emit errorOccurred(QStringLiteral("Failed to save file"),
                       QStringLiteral("Directory does not exist: %1")
                           .arg(dirInfo.absoluteFilePath()));
    return false;
  }

  const QByteArray p = writePath.toUtf8();
  if (!nulla_doc_save_file(m_doc, u8(p), u8len(p))) {
    const bool denied = !dirInfo.isWritable() ||
                        (existed && !QFileInfo(writePath).isWritable());
    emit errorOccurred(
        QStringLiteral("Failed to save file"),
        denied ? QStringLiteral("Permission denied: %1").arg(writePath)
               : QStringLiteral("Unable to write: %1").arg(writePath));
    return false;
  }
  if (existed) {
    QFile::setPermissions(writePath, perms);
  }

  m_filePath = target;
  refreshModified();
  applyLanguage();
  emit fileInfoChanged();
  emit languageStatusChanged();
  emit contentChanged();
  return true;
}

void TabSession::resetEmpty() {
  nulla_doc_clear(m_doc);
  m_filePath.clear();
  applyLanguage();
  m_maxLineColumns = 0;
  emit maxLineColumnsChanged();
  emitAllAfterReset();
}

void TabSession::discardChanges() {
  nulla_doc_mark_saved(m_doc);
  refreshModified();
}

void TabSession::undo() {
  if (nulla_doc_undo(m_doc)) {
    finishEdit();
  }
}

void TabSession::redo() {
  if (nulla_doc_redo(m_doc)) {
    finishEdit();
  }
}

void TabSession::selectAll() {
  nulla_doc_select_all(m_doc);
  finishMove();
}

void TabSession::selectCurrentLine() {
  nulla_doc_select_current_line(m_doc);
  finishMove();
}

QString TabSession::selectedText() const {
  const uint8_t *p = nullptr;
  size_t n = 0;
  nulla_doc_selected_text(m_doc, &p, &n);
  return fromOut(p, n);
}

bool TabSession::findNext(const QString &query, bool backward) {
  if (query.isEmpty()) {
    return false;
  }
  const QByteArray q = query.toUtf8();
  const bool found = nulla_doc_find(m_doc, u8(q), u8len(q), !backward, true);
  finishMove();
  return found;
}

QVariantMap TabSession::matchStats(const QString &query) const {
  size_t cur = 0;
  size_t total = 0;
  if (!query.isEmpty()) {
    const QByteArray q = query.toUtf8();
    nulla_doc_match_stats(m_doc, u8(q), u8len(q), &cur, &total);
  }
  QVariantMap res;
  res.insert(QStringLiteral("current"), static_cast<int>(cur));
  res.insert(QStringLiteral("total"), static_cast<int>(total));
  return res;
}

bool TabSession::replaceMatch(const QString &query,
                              const QString &replacement) {
  if (query.isEmpty()) {
    return false;
  }
  const QByteArray q = query.toUtf8();
  const QByteArray r = replacement.toUtf8();
  const bool ok = nulla_doc_replace_current_and_find(m_doc, u8(q), u8len(q),
                                                     u8(r), u8len(r), nowMs());
  finishEdit();
  return ok;
}

int TabSession::replaceAllMatches(const QString &query,
                                  const QString &replacement) {
  if (query.isEmpty()) {
    return 0;
  }
  const QByteArray q = query.toUtf8();
  const QByteArray r = replacement.toUtf8();
  const size_t count =
      nulla_doc_replace_all(m_doc, u8(q), u8len(q), u8(r), u8len(r), nowMs());
  if (count > 0) {
    finishEdit();
    restartWidthScan();
  }
  return static_cast<int>(qMin<size_t>(count, static_cast<size_t>(INT_MAX)));
}

void TabSession::gotoLine(int lineNumber) {
  const size_t total = nulla_doc_line_count(m_doc);
  const size_t line0 = static_cast<size_t>(qBound<qint64>(
      0, static_cast<qint64>(lineNumber) - 1, static_cast<qint64>(total) - 1));
  const size_t pos = nulla_doc_line_start_byte(m_doc, line0);
  nulla_doc_set_selection(m_doc, pos, pos);
  finishMove();
}

void TabSession::runAutoDetect() {
  if (m_languageId != QStringLiteral("auto") || !m_filePath.isEmpty() ||
      !m_syntaxEnabled) {
    return;
  }
  if (nulla_doc_refresh_auto_language(m_doc, nullptr, 0)) {
    emit languageStatusChanged();
    emit contentChanged();
  }
}

void TabSession::scanWidthChunk() {
  const size_t total = nulla_doc_line_count(m_doc);
  if (m_widthScanLine >= total) {
    return;
  }
  nulla_doc_set_syntax_enabled(m_doc, false);
  NullaViewportSnapshot snap{};
  const bool ok = nulla_doc_build_viewport(m_doc, m_widthScanLine,
                                           kWidthScanChunk, 0, 1, false, &snap);
  nulla_doc_set_syntax_enabled(m_doc, m_syntaxEnabled);
  if (!ok) {
    return;
  }
  size_t best = static_cast<size_t>(qMax(0, m_maxLineColumns));
  for (size_t i = 0; i < snap.line_count; ++i) {
    best = qMax(best, snap.lines[i].line_total_vis_cols);
  }
  noteLineColumns(static_cast<int>(qMin<size_t>(best, 50000000)));
  m_widthScanLine += kWidthScanChunk;
  if (m_widthScanLine < total) {
    m_widthScanTimer.start();
  }
}

void TabSession::finishEdit() {
  refreshModified();
  refreshCursorStats();
  m_selectedChars = static_cast<int>(qMin<size_t>(
      nulla_doc_selected_chars(m_doc), static_cast<size_t>(INT_MAX)));
  emit contentChanged();
  emit selectionChanged();
  if (m_languageId == QStringLiteral("auto") && m_filePath.isEmpty() &&
      m_syntaxEnabled) {
    m_autoDetectTimer.start();
  }
}

void TabSession::finishMove() {
  refreshCursorStats();
  m_selectedChars = static_cast<int>(qMin<size_t>(
      nulla_doc_selected_chars(m_doc), static_cast<size_t>(INT_MAX)));
  emit selectionChanged();
}

void TabSession::refreshCursorStats() {
  size_t line = 1;
  size_t col = 1;
  size_t vis = 0;
  nulla_doc_cursor_pos(m_doc, &line, &col, &vis);
  const int l =
      static_cast<int>(qMin<size_t>(line, static_cast<size_t>(INT_MAX)));
  const int c =
      static_cast<int>(qMin<size_t>(col, static_cast<size_t>(INT_MAX)));
  if (l != m_cursorLine || c != m_cursorColumn) {
    m_cursorLine = l;
    m_cursorColumn = c;
    emit cursorStatsChanged();
  }
}

void TabSession::refreshModified() {
  const bool m = nulla_doc_is_modified(m_doc);
  if (m != m_modified) {
    m_modified = m;
    emit modifiedChanged();
  }
}

void TabSession::applyLanguage() {
  nulla_doc_set_syntax_enabled(m_doc, m_syntaxEnabled);
  const QByteArray lang = m_languageId.toUtf8();
  const QByteArray p = m_filePath.toUtf8();
  nulla_doc_set_language(m_doc, u8(lang), u8len(lang), u8(p), u8len(p));
}

void TabSession::restartWidthScan() {
  m_widthScanLine = 0;
  m_widthScanTimer.start();
}

void TabSession::emitAllAfterReset() {
  m_modified = nulla_doc_is_modified(m_doc);
  m_selectedChars = 0;
  m_cursorLine = 0;
  refreshCursorStats();
  emit fileInfoChanged();
  emit modifiedChanged();
  emit languageStatusChanged();
  emit contentChanged();
  emit selectionChanged();
  emit documentLoaded();
}
