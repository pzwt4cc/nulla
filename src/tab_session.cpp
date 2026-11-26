#include "tab_session.h"

#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QUrl>

TabSession::TabSession(QObject *parent) : QObject(parent) {
  m_autoDetectTimer.setSingleShot(true);
  m_autoDetectTimer.setInterval(120);
  connect(&m_autoDetectTimer, &QTimer::timeout, this,
          &TabSession::runAutoDetect);
}

TabSession::~TabSession() {
  m_autoDetectTimer.stop();
  if (m_quickDoc && m_quickDoc->textDocument()) {
    disconnect(m_quickDoc->textDocument(), nullptr, this, nullptr);
  }
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
  return NullaHighlighter::languageDisplayName(m_resolvedLanguageId);
}

QString TabSession::commentPrefix() const {
  return NullaHighlighter::commentPrefix(m_resolvedLanguageId);
}

int TabSession::lineCount() const {
  if (m_quickDoc && m_quickDoc->textDocument()) {
    return qMax(1, m_quickDoc->textDocument()->blockCount());
  }
  return 1;
}

bool TabSession::isPristineUntitled() const {
  if (m_modified || !m_filePath.isEmpty() || !m_pendingPath.isEmpty()) {
    return false;
  }
  if (!m_quickDoc || !m_quickDoc->textDocument()) {
    return true;
  }
  return m_quickDoc->textDocument()->isEmpty();
}

void TabSession::setLanguageId(const QString &id) {
  const QString normalized = id.isEmpty() ? QStringLiteral("auto") : id;
  if (m_languageId == normalized) {
    return;
  }
  m_languageId = normalized;
  applyHighlighter();
  emit languageChanged();
  emit languageStatusChanged();
}

void TabSession::setPendingInitialFile(const QString &path, bool allowMissing) {
  if (path.isEmpty()) {
    return;
  }
  if (m_quickDoc && m_quickDoc->textDocument()) {
    loadFile(path, allowMissing);
  } else {
    m_pendingPath = path;
    m_pendingAllowMissing = allowMissing;
  }
}

void TabSession::applyThemeAndSyntax(bool syntaxEnabled, bool darkMode) {
  const bool changed =
      (m_syntaxEnabled != syntaxEnabled) || (m_darkMode != darkMode);
  m_syntaxEnabled = syntaxEnabled;
  m_darkMode = darkMode;
  if (changed) {
    applyHighlighter();
    emit languageStatusChanged();
  }
}

void TabSession::attachDocument(QQuickTextDocument *doc) {
  if (m_quickDoc == doc) {
    return;
  }
  if (m_quickDoc && m_quickDoc->textDocument()) {
    disconnect(m_quickDoc->textDocument(), nullptr, this, nullptr);
  }
  m_quickDoc = doc;
  if (m_highlighter) {
    delete m_highlighter;
    m_highlighter = nullptr;
  }
  if (m_quickDoc && m_quickDoc->textDocument()) {
    QTextDocument *td = m_quickDoc->textDocument();
    td->setDocumentMargin(0.0);
    m_highlighter = new NullaHighlighter(td);
    connect(td, &QTextDocument::modificationChanged, this,
            &TabSession::onDocumentModifiedChanged);
    connect(td, &QTextDocument::contentsChanged, this,
            &TabSession::onDocumentContentsChanged);
    connect(td, &QTextDocument::blockCountChanged, this,
            &TabSession::cursorStatsChanged);

    if (!m_pendingPath.isEmpty()) {
      const QString target = m_pendingPath;
      const bool allow = m_pendingAllowMissing;
      m_pendingPath.clear();
      loadFile(target, allow);
    } else {
      applyHighlighter(true);
      emit languageStatusChanged();
    }
  }
}

void TabSession::updateCursorPosition(int position) {
  if (!m_quickDoc || !m_quickDoc->textDocument()) {
    return;
  }
  QTextDocument *td = m_quickDoc->textDocument();
  const int clamped = qBound(0, position, qMax(0, td->characterCount() - 1));
  const QTextBlock block = td->findBlock(clamped);
  const int line = block.isValid() ? (block.blockNumber() + 1) : 1;
  const int col = block.isValid() ? (clamped - block.position() + 1) : 1;
  if (m_cursorLine != line || m_cursorColumn != col) {
    m_cursorLine = line;
    m_cursorColumn = col;
    emit cursorStatsChanged();
  }
}

bool TabSession::loadFile(const QString &rawPath, bool allowMissing) {
  if (!m_quickDoc || !m_quickDoc->textDocument()) {
    setPendingInitialFile(rawPath, allowMissing);
    return true;
  }

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

  QTextDocument *td = m_quickDoc->textDocument();
  QFile file(absPath);

  if (!file.exists()) {
    if (!allowMissing) {
      emit errorOccurred(
          QStringLiteral("Failed to open file"),
          QStringLiteral("File does not exist: %1").arg(absPath));
      return false;
    }
    m_loading = true;
    td->setPlainText(QString());
    td->clearUndoRedoStacks();
    td->setModified(false);
    m_loading = false;
    m_filePath = absPath;
    m_modified = false;
    m_cursorLine = 1;
    m_cursorColumn = 1;
    applyHighlighter(true);
    emit fileInfoChanged();
    emit modifiedChanged();
    emit languageStatusChanged();
    emit cursorStatsChanged();
    emit documentLoaded();
    return true;
  }

  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    emit errorOccurred(QStringLiteral("Failed to open file"),
                       file.errorString());
    return false;
  }

  const QByteArray bytes = file.readAll();
  const QString text = QString::fromUtf8(bytes);

  m_loading = true;
  td->clear();
  m_filePath = absPath;
  if (m_highlighter) {
    const QString resolved = !m_syntaxEnabled
                                 ? QStringLiteral("plain")
                                 : (m_languageId != QStringLiteral("auto")
                                        ? m_languageId
                                        : NullaHighlighter::detectLanguage(
                                              absPath, text.left(4096)));
    m_resolvedLanguageId = resolved;
    m_highlighter->configure(m_syntaxEnabled, m_resolvedLanguageId, m_darkMode);
  }
  td->setPlainText(text);
  td->clearUndoRedoStacks();
  td->setModified(false);
  m_loading = false;

  m_modified = false;
  m_cursorLine = 1;
  m_cursorColumn = 1;
  emit fileInfoChanged();
  emit modifiedChanged();
  emit languageStatusChanged();
  emit cursorStatsChanged();
  emit documentLoaded();
  return true;
}

bool TabSession::writeToFile(const QString &path) {
  if (!m_quickDoc || !m_quickDoc->textDocument()) {
    return false;
  }
  QTextDocument *td = m_quickDoc->textDocument();
  QSaveFile out(path);
  if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
    emit errorOccurred(QStringLiteral("Failed to save file"),
                       out.errorString());
    return false;
  }
  const QByteArray utf8 = td->toPlainText().toUtf8();
  if (out.write(utf8) != utf8.size() || !out.commit()) {
    emit errorOccurred(QStringLiteral("Failed to save file"),
                       out.errorString());
    return false;
  }

  m_filePath = QFileInfo(path).absoluteFilePath();
  m_loading = true;
  td->setModified(false);
  m_loading = false;
  if (m_modified) {
    m_modified = false;
    emit modifiedChanged();
  }
  applyHighlighter();
  emit fileInfoChanged();
  emit languageStatusChanged();
  return true;
}

void TabSession::resetEmpty() {
  if (!m_quickDoc || !m_quickDoc->textDocument()) {
    m_filePath.clear();
    m_pendingPath.clear();
    m_modified = false;
    emit fileInfoChanged();
    emit modifiedChanged();
    return;
  }
  QTextDocument *td = m_quickDoc->textDocument();
  m_loading = true;
  td->setPlainText(QString());
  td->clearUndoRedoStacks();
  td->setModified(false);
  m_loading = false;

  m_filePath.clear();
  m_pendingPath.clear();
  m_modified = false;
  m_cursorLine = 1;
  m_cursorColumn = 1;
  applyHighlighter(true);
  emit fileInfoChanged();
  emit modifiedChanged();
  emit languageStatusChanged();
  emit cursorStatsChanged();
  emit documentLoaded();
}

void TabSession::discardChanges() {
  if (m_quickDoc && m_quickDoc->textDocument()) {
    m_loading = true;
    m_quickDoc->textDocument()->setModified(false);
    m_loading = false;
  }
  if (m_modified) {
    m_modified = false;
    emit modifiedChanged();
  }
}

void TabSession::onDocumentModifiedChanged(bool m) {
  if (m_loading) {
    return;
  }
  if (m_modified != m) {
    m_modified = m;
    emit modifiedChanged();
  }
}

void TabSession::onDocumentContentsChanged() {
  if (m_loading) {
    return;
  }
  if (m_languageId == QStringLiteral("auto") && m_filePath.isEmpty() &&
      m_syntaxEnabled) {
    m_autoDetectTimer.start();
  }
}

void TabSession::runAutoDetect() {
  if (m_languageId != QStringLiteral("auto") || !m_filePath.isEmpty() ||
      !m_syntaxEnabled) {
    return;
  }
  const QString nextLang = computeResolvedLanguageId();
  if (nextLang != m_resolvedLanguageId) {
    applyHighlighter();
    emit languageStatusChanged();
  }
}

QString TabSession::sampleDocumentHead() const {
  if (!m_quickDoc || !m_quickDoc->textDocument()) {
    return QString();
  }
  QString out;
  out.reserve(1024);
  QTextBlock block = m_quickDoc->textDocument()->begin();
  while (block.isValid() && out.size() < 4096) {
    if (!out.isEmpty()) {
      out.append(QLatin1Char('\n'));
    }
    out.append(block.text());
    block = block.next();
  }
  return out;
}

QString TabSession::computeResolvedLanguageId() const {
  if (!m_syntaxEnabled) {
    return QStringLiteral("plain");
  }
  if (m_languageId != QStringLiteral("auto")) {
    return m_languageId;
  }
  return NullaHighlighter::detectLanguage(m_filePath, sampleDocumentHead());
}

void TabSession::applyHighlighter(bool forceRehighlight) {
  const QString resolved = computeResolvedLanguageId();
  const bool langChanged = (resolved != m_resolvedLanguageId);
  m_resolvedLanguageId = resolved;
  if (!m_highlighter) {
    return;
  }
  if (forceRehighlight || langChanged || m_highlighter->document()) {
    m_highlighter->configure(m_syntaxEnabled, m_resolvedLanguageId, m_darkMode);
  }
}

QVariantMap TabSession::findText(const QString &query, int fromPos,
                                 bool backward) {
  QVariantMap res;
  res.insert(QStringLiteral("found"), false);
  res.insert(QStringLiteral("start"), -1);
  res.insert(QStringLiteral("end"), -1);

  if (query.isEmpty() || !m_quickDoc || !m_quickDoc->textDocument()) {
    return res;
  }
  QTextDocument *td = m_quickDoc->textDocument();
  QTextDocument::FindFlags flags;
  if (backward) {
    flags |= QTextDocument::FindBackward;
  }

  const int safeFrom = qBound(0, fromPos, qMax(0, td->characterCount() - 1));
  QTextCursor found = td->find(query, safeFrom, flags);
  if (found.isNull()) {
    const int wrapPos = backward ? qMax(0, td->characterCount() - 1) : 0;
    found = td->find(query, wrapPos, flags);
  }

  if (!found.isNull()) {
    res.insert(QStringLiteral("found"), true);
    res.insert(QStringLiteral("start"), found.selectionStart());
    res.insert(QStringLiteral("end"), found.selectionEnd());
  }
  return res;
}

QVariantMap TabSession::matchStats(const QString &query, int selectionStart) {
  QVariantMap res;
  res.insert(QStringLiteral("total"), 0);
  res.insert(QStringLiteral("current"), 0);

  if (query.isEmpty() || !m_quickDoc || !m_quickDoc->textDocument()) {
    return res;
  }
  QTextDocument *td = m_quickDoc->textDocument();
  int total = 0;
  int current = 0;
  const int maxMatches = 10000;

  if (!query.contains(QLatin1Char('\n'))) {
    const qsizetype qLen = query.size();
    for (QTextBlock b = td->begin(); b.isValid(); b = b.next()) {
      const QString line = b.text();
      const int blockPos = b.position();
      qsizetype idx = 0;
      while ((idx = line.indexOf(query, idx, Qt::CaseInsensitive)) != -1) {
        total++;
        if (blockPos + idx == selectionStart) {
          current = total;
        }
        idx += qLen;
        if (total >= maxMatches) {
          res.insert(QStringLiteral("total"), total);
          res.insert(QStringLiteral("current"), current);
          return res;
        }
      }
    }
  } else {
    QTextCursor cur(td);
    while (total < maxMatches) {
      cur = td->find(query, cur);
      if (cur.isNull()) {
        break;
      }
      total++;
      if (cur.selectionStart() == selectionStart) {
        current = total;
      }
    }
  }
  res.insert(QStringLiteral("total"), total);
  res.insert(QStringLiteral("current"), current);
  return res;
}

QVariantMap TabSession::replaceMatch(const QString &query,
                                     const QString &replacement, int selStart,
                                     int selEnd) {
  if (query.isEmpty() || !m_quickDoc || !m_quickDoc->textDocument()) {
    return findText(query, selEnd, false);
  }
  QTextDocument *td = m_quickDoc->textDocument();
  const int maxPos = qMax(0, td->characterCount() - 1);
  QTextCursor cur(td);
  cur.setPosition(qBound(0, selStart, maxPos));
  cur.setPosition(qBound(0, selEnd, maxPos), QTextCursor::KeepAnchor);
  int nextFrom = qBound(0, selEnd, maxPos);
  QString selected = cur.selectedText();
  selected.replace(QChar::ParagraphSeparator, QLatin1Char('\n'));
  if (selected.compare(query, Qt::CaseInsensitive) == 0) {
    cur.beginEditBlock();
    cur.insertText(replacement);
    cur.endEditBlock();
    nextFrom = cur.position();
  }
  return findText(query, nextFrom, false);
}

int TabSession::replaceAllMatches(const QString &query,
                                  const QString &replacement) {
  if (query.isEmpty() || !m_quickDoc || !m_quickDoc->textDocument()) {
    return 0;
  }
  QTextDocument *td = m_quickDoc->textDocument();
  QTextCursor edit(td);
  edit.beginEditBlock();
  int count = 0;
  int pos = 0;
  while (true) {
    QTextCursor found = td->find(query, pos);
    if (found.isNull()) {
      break;
    }
    found.insertText(replacement);
    pos = found.position();
    count++;
  }
  edit.endEditBlock();
  return count;
}

int TabSession::lineStartPosition(int lineNumber) const {
  if (!m_quickDoc || !m_quickDoc->textDocument()) {
    return 0;
  }
  QTextDocument *td = m_quickDoc->textDocument();
  const int target = qBound(1, lineNumber, qMax(1, td->blockCount()));
  const QTextBlock block = td->findBlockByNumber(target - 1);
  return block.isValid() ? block.position() : 0;
}
