#include "editor_backend.h"

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QStyleHints>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QUrl>

static bool detectDesktopEnvironment() {
    if (!qEnvironmentVariableIsEmpty("HYPRLAND_INSTANCE_SIGNATURE") ||
        !qEnvironmentVariableIsEmpty("SWAYSOCK") ||
        !qEnvironmentVariableIsEmpty("I3SOCK") ||
        !qEnvironmentVariableIsEmpty("BSPWM_SOCKET")) {
        return false;
    }

    const QString combined = (qEnvironmentVariable("XDG_CURRENT_DESKTOP") + QLatin1Char(' ') +
                              qEnvironmentVariable("XDG_SESSION_DESKTOP") + QLatin1Char(' ') +
                              qEnvironmentVariable("DESKTOP_SESSION")).toLower();

    const char * const wms[] = {
        "hyprland", "sway", "river", "niri", "wayfire", "labwc",
        "i3", "bspwm", "dwm", "awesome", "qtile", "xmonad",
        "openbox", "herbstluftwm", "leftwm", "spectrwm", "fluxbox", "icewm", "jwm"
    };
    for (const char *wm : wms) {
        if (combined.contains(QLatin1String(wm))) {
            return false;
        }
    }

    const char * const des[] = {
        "gnome", "ubuntu", "kde", "plasma", "xfce", "cinnamon",
        "mate", "lxqt", "lxde", "budgie", "pantheon", "deepin",
        "cosmic", "unity", "enlightenment", "ukui", "cutefish"
    };
    for (const char *de : des) {
        if (combined.contains(QLatin1String(de))) {
            return true;
        }
    }

    return false;
}

EditorBackend::EditorBackend(QObject *parent)
    : QObject(parent),
      m_showWindowControls(detectDesktopEnvironment()) {
    loadSettings();
    if (QGuiApplication::styleHints()) {
        connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
                this, &EditorBackend::onSystemColorSchemeChanged);
    }
}

QString EditorBackend::fileName() const {
    if (m_filePath.isEmpty()) {
        return QStringLiteral("Untitled");
    }
    const QString base = QFileInfo(m_filePath).fileName();
    return base.isEmpty() ? QStringLiteral("Untitled") : base;
}

bool EditorBackend::effectiveDark() const {
    if (m_themeMode == ThemeDark) {
        return true;
    }
    if (m_themeMode == ThemeLight) {
        return false;
    }
    if (QGuiApplication::styleHints()) {
        return QGuiApplication::styleHints()->colorScheme() != Qt::ColorScheme::Light;
    }
    return true;
}

QString EditorBackend::statusLanguage() const {
    if (!m_syntaxEnabled) {
        return QStringLiteral("Plain");
    }
    return NullaHighlighter::languageDisplayName(resolvedLanguageId());
}

QVariantList EditorBackend::languagesModel() const {
    QVariantList list;
    QVariantMap autoItem;
    autoItem.insert(QStringLiteral("id"), QStringLiteral("auto"));
    autoItem.insert(QStringLiteral("name"), QStringLiteral("Auto"));
    list.append(autoItem);

    const QVector<NullaLangDef> langs = NullaHighlighter::supportedLanguages();
    for (const auto &lang : langs) {
        QVariantMap item;
        item.insert(QStringLiteral("id"), lang.id);
        item.insert(QStringLiteral("name"), lang.name);
        list.append(item);
    }
    return list;
}

int EditorBackend::lineCount() const {
    if (m_quickDoc && m_quickDoc->textDocument()) {
        return qMax(1, m_quickDoc->textDocument()->blockCount());
    }
    return 1;
}

void EditorBackend::setShowLineNumbers(bool show) {
    if (m_showLineNumbers == show) {
        return;
    }
    m_showLineNumbers = show;
    saveSettings();
    emit settingsChanged();
}

void EditorBackend::setThemeMode(int mode) {
    if (mode < ThemeSystem || mode > ThemeDark) {
        return;
    }
    if (m_themeMode == mode) {
        return;
    }
    m_themeMode = mode;
    saveSettings();
    applyHighlighter();
    emit themeChanged();
    emit settingsChanged();
}

void EditorBackend::setSyntaxEnabled(bool enabled) {
    if (m_syntaxEnabled == enabled) {
        return;
    }
    m_syntaxEnabled = enabled;
    saveSettings();
    applyHighlighter();
    emit settingsChanged();
    emit languageStatusChanged();
}

void EditorBackend::setLanguageId(const QString &id) {
    const QString normalized = id.isEmpty() ? QStringLiteral("auto") : id;
    if (m_languageId == normalized) {
        return;
    }
    m_languageId = normalized;
    saveSettings();
    applyHighlighter();
    emit settingsChanged();
    emit languageStatusChanged();
}

void EditorBackend::setZoomPercent(int percent) {
    const int clamped = qBound(50, percent, 300);
    if (m_zoomPercent == clamped) {
        return;
    }
    m_zoomPercent = clamped;
    saveSettings();
    emit settingsChanged();
}

void EditorBackend::zoomIn() {
    setZoomPercent(m_zoomPercent + 10);
}

void EditorBackend::zoomOut() {
    setZoomPercent(m_zoomPercent - 10);
}

void EditorBackend::zoomReset() {
    setZoomPercent(100);
}

void EditorBackend::attachDocument(QQuickTextDocument *doc) {
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
        connect(td, &QTextDocument::modificationChanged,
                this, &EditorBackend::onDocumentModifiedChanged);
        connect(td, &QTextDocument::blockCountChanged,
                this, &EditorBackend::cursorStatsChanged);
        if (!m_pendingInitialPath.isEmpty()) {
            const QString target = m_pendingInitialPath;
            m_pendingInitialPath.clear();
            loadFile(target, true);
        } else {
            applyHighlighter();
            emit languageStatusChanged();
        }
    }
}

void EditorBackend::updateCursorPosition(int position) {
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

void EditorBackend::setInitialFile(const QString &path) {
    if (path.isEmpty()) {
        return;
    }
    if (m_quickDoc && m_quickDoc->textDocument()) {
        loadFile(path, true);
    } else {
        m_pendingInitialPath = path;
    }
}

bool EditorBackend::loadFile(const QString &rawPath, bool allowMissing) {
    if (!m_quickDoc || !m_quickDoc->textDocument()) {
        return false;
    }

    QString path = rawPath;
    const QUrl url(rawPath);
    if (url.isValid() && url.isLocalFile()) {
        path = url.toLocalFile();
    }
    const QString absPath = QFileInfo(path).absoluteFilePath();

    QTextDocument *td = m_quickDoc->textDocument();
    QFile file(absPath);

    if (!file.exists()) {
        if (!allowMissing) {
            emit errorOccurred(QStringLiteral("Failed to open file"),
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
        applyHighlighter();
        emit fileInfoChanged();
        emit modifiedChanged();
        emit languageStatusChanged();
        emit cursorStatsChanged();
        return true;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit errorOccurred(QStringLiteral("Failed to open file"), file.errorString());
        return false;
    }

    const QByteArray bytes = file.readAll();
    const QString text = QString::fromUtf8(bytes);

    m_loading = true;
    td->setPlainText(text);
    td->clearUndoRedoStacks();
    td->setModified(false);
    m_loading = false;

    m_filePath = absPath;
    m_modified = false;
    m_cursorLine = 1;
    m_cursorColumn = 1;
    applyHighlighter();
    emit fileInfoChanged();
    emit modifiedChanged();
    emit languageStatusChanged();
    emit cursorStatsChanged();
    return true;
}

void EditorBackend::newFile() {
    if (!m_quickDoc || !m_quickDoc->textDocument()) {
        return;
    }
    QTextDocument *td = m_quickDoc->textDocument();
    m_loading = true;
    td->setPlainText(QString());
    td->clearUndoRedoStacks();
    td->setModified(false);
    m_loading = false;

    m_filePath.clear();
    m_modified = false;
    m_cursorLine = 1;
    m_cursorColumn = 1;
    applyHighlighter();
    emit fileInfoChanged();
    emit modifiedChanged();
    emit languageStatusChanged();
    emit cursorStatsChanged();
}

bool EditorBackend::openFileDialog() {
    const QString initialDir = m_filePath.isEmpty()
                                   ? QDir::currentPath()
                                   : QFileInfo(m_filePath).absolutePath();
    const QString chosen = QFileDialog::getOpenFileName(nullptr,
                                                        QStringLiteral("Open File"),
                                                        initialDir);
    if (chosen.isEmpty()) {
        return false;
    }
    return loadFile(chosen, false);
}

bool EditorBackend::writeToFile(const QString &path) {
    if (!m_quickDoc || !m_quickDoc->textDocument()) {
        return false;
    }
    QTextDocument *td = m_quickDoc->textDocument();
    QSaveFile out(path);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit errorOccurred(QStringLiteral("Failed to save file"), out.errorString());
        return false;
    }
    const QByteArray utf8 = td->toPlainText().toUtf8();
    if (out.write(utf8) != utf8.size() || !out.commit()) {
        emit errorOccurred(QStringLiteral("Failed to save file"), out.errorString());
        return false;
    }

    m_filePath = QFileInfo(path).absoluteFilePath();
    td->setModified(false);
    m_modified = false;
    applyHighlighter();
    emit fileInfoChanged();
    emit modifiedChanged();
    emit languageStatusChanged();
    return true;
}

bool EditorBackend::saveFile() {
    if (m_filePath.isEmpty()) {
        return saveFileAsDialog();
    }
    return writeToFile(m_filePath);
}

bool EditorBackend::saveFileAsDialog() {
    const QString initial = m_filePath.isEmpty()
                                ? QDir(QDir::currentPath()).filePath(QStringLiteral("Untitled"))
                                : m_filePath;
    const QString chosen = QFileDialog::getSaveFileName(nullptr,
                                                        QStringLiteral("Save File"),
                                                        initial);
    if (chosen.isEmpty()) {
        return false;
    }
    return writeToFile(chosen);
}

void EditorBackend::discardChanges() {
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

void EditorBackend::onDocumentModifiedChanged(bool m) {
    if (m_loading) {
        return;
    }
    if (m_modified != m) {
        m_modified = m;
        emit modifiedChanged();
    }
    if (m_languageId == QStringLiteral("auto") && m_filePath.isEmpty()) {
        applyHighlighter();
        emit languageStatusChanged();
    }
}

void EditorBackend::onSystemColorSchemeChanged() {
    if (m_themeMode == ThemeSystem) {
        applyHighlighter();
        emit themeChanged();
    }
}

QString EditorBackend::resolvedLanguageId() const {
    if (!m_syntaxEnabled) {
        return QStringLiteral("plain");
    }
    if (m_languageId != QStringLiteral("auto")) {
        return m_languageId;
    }
    QString firstLine;
    if (m_quickDoc && m_quickDoc->textDocument()) {
        firstLine = m_quickDoc->textDocument()->begin().text();
    }
    return NullaHighlighter::detectLanguage(m_filePath, firstLine);
}

void EditorBackend::applyHighlighter() {
    if (!m_highlighter) {
        return;
    }
    m_highlighter->configure(m_syntaxEnabled, resolvedLanguageId(), effectiveDark());
}

void EditorBackend::loadSettings() {
    const QString cfgDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    const QString iniPath = QDir(cfgDir).filePath(QStringLiteral("nulla/settings.ini"));
    QSettings s(iniPath, QSettings::IniFormat);
    s.beginGroup(QStringLiteral("editor"));
    m_showLineNumbers = s.value(QStringLiteral("line_numbers"), true).toBool();
    const int mode = s.value(QStringLiteral("theme"), ThemeDark).toInt();
    m_themeMode = (mode >= ThemeSystem && mode <= ThemeDark) ? mode : ThemeDark;
    m_syntaxEnabled = s.value(QStringLiteral("syntax"), true).toBool();
    const QString lang = s.value(QStringLiteral("language"), QStringLiteral("auto")).toString();
    m_languageId = lang.isEmpty() ? QStringLiteral("auto") : lang;
    m_zoomPercent = qBound(50, s.value(QStringLiteral("zoom"), 100).toInt(), 300);
    s.endGroup();
}

void EditorBackend::saveSettings() const {
    const QString cfgDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    const QString iniPath = QDir(cfgDir).filePath(QStringLiteral("nulla/settings.ini"));
    QDir().mkpath(QFileInfo(iniPath).absolutePath());
    QSettings s(iniPath, QSettings::IniFormat);
    s.beginGroup(QStringLiteral("editor"));
    s.setValue(QStringLiteral("line_numbers"), m_showLineNumbers);
    s.setValue(QStringLiteral("theme"), m_themeMode);
    s.setValue(QStringLiteral("syntax"), m_syntaxEnabled);
    s.setValue(QStringLiteral("language"), m_languageId);
    s.setValue(QStringLiteral("zoom"), m_zoomPercent);
    s.endGroup();
    s.sync();
}

QVariantMap EditorBackend::findText(const QString &query, int fromPos, bool backward) {
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

    QTextCursor found = td->find(query, fromPos, flags);
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

QVariantMap EditorBackend::matchStats(const QString &query, int selectionStart) {
    QVariantMap res;
    res.insert(QStringLiteral("total"), 0);
    res.insert(QStringLiteral("current"), 0);

    if (query.isEmpty() || !m_quickDoc || !m_quickDoc->textDocument()) {
        return res;
    }
    QTextDocument *td = m_quickDoc->textDocument();
    int total = 0;
    int current = 0;
    QTextCursor cur(td);
    while (true) {
        cur = td->find(query, cur);
        if (cur.isNull()) {
            break;
        }
        total++;
        if (cur.selectionStart() == selectionStart) {
            current = total;
        }
    }
    res.insert(QStringLiteral("total"), total);
    res.insert(QStringLiteral("current"), current);
    return res;
}
