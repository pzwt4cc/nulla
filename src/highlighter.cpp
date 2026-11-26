#include "highlighter.h"

#include <QFileInfo>
#include <QTextDocument>

NullaHighlighter::NullaHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent) {}

QVector<NullaLangDef> NullaHighlighter::supportedLanguages() {
    return {
        {QStringLiteral("plain"), QStringLiteral("Plain Text")},
        {QStringLiteral("bash"), QStringLiteral("Bash / Shell")},
        {QStringLiteral("c"), QStringLiteral("C / C++")},
        {QStringLiteral("css"), QStringLiteral("CSS")},
        {QStringLiteral("go"), QStringLiteral("Go")},
        {QStringLiteral("html"), QStringLiteral("HTML / XML")},
        {QStringLiteral("ini"), QStringLiteral("INI / Config")},
        {QStringLiteral("js"), QStringLiteral("JavaScript / TS")},
        {QStringLiteral("json"), QStringLiteral("JSON")},
        {QStringLiteral("make"), QStringLiteral("Makefile / PKGBUILD")},
        {QStringLiteral("markdown"), QStringLiteral("Markdown")},
        {QStringLiteral("python"), QStringLiteral("Python")},
        {QStringLiteral("rust"), QStringLiteral("Rust")},
        {QStringLiteral("toml"), QStringLiteral("TOML")},
        {QStringLiteral("yaml"), QStringLiteral("YAML")}
    };
}

QString NullaHighlighter::languageDisplayName(const QString &id) {
    const QVector<NullaLangDef> langs = supportedLanguages();
    for (const auto &lang : langs) {
        if (lang.id == id) {
            return lang.name;
        }
    }
    return QStringLiteral("Plain Text");
}

QString NullaHighlighter::detectLanguage(const QString &filePath, const QString &firstLine) {
    if (!filePath.isEmpty()) {
        const QFileInfo fi(filePath);
        const QString base = fi.fileName().toLower();
        const QString ext = fi.suffix().toLower();

        if (base == QStringLiteral("makefile") || base == QStringLiteral("gnumakefile") ||
            base == QStringLiteral("pkgbuild") || ext == QStringLiteral("mk")) {
            return QStringLiteral("make");
        }
        if (base == QStringLiteral(".bashrc") || base == QStringLiteral(".zshrc") ||
            base == QStringLiteral(".profile") || ext == QStringLiteral("sh") ||
            ext == QStringLiteral("bash") || ext == QStringLiteral("zsh")) {
            return QStringLiteral("bash");
        }
        if (ext == QStringLiteral("c") || ext == QStringLiteral("h") ||
            ext == QStringLiteral("cpp") || ext == QStringLiteral("hpp") ||
            ext == QStringLiteral("cc") || ext == QStringLiteral("cxx")) {
            return QStringLiteral("c");
        }
        if (ext == QStringLiteral("py") || ext == QStringLiteral("pyw")) {
            return QStringLiteral("python");
        }
        if (ext == QStringLiteral("json") || ext == QStringLiteral("jsonc")) {
            return QStringLiteral("json");
        }
        if (ext == QStringLiteral("ini") || ext == QStringLiteral("conf") ||
            ext == QStringLiteral("cfg") || ext == QStringLiteral("desktop") ||
            ext == QStringLiteral("service") || ext == QStringLiteral("env")) {
            return QStringLiteral("ini");
        }
        if (ext == QStringLiteral("yml") || ext == QStringLiteral("yaml")) {
            return QStringLiteral("yaml");
        }
        if (ext == QStringLiteral("toml")) {
            return QStringLiteral("toml");
        }
        if (ext == QStringLiteral("md") || ext == QStringLiteral("markdown")) {
            return QStringLiteral("markdown");
        }
        if (ext == QStringLiteral("js") || ext == QStringLiteral("mjs") ||
            ext == QStringLiteral("ts") || ext == QStringLiteral("tsx") ||
            ext == QStringLiteral("jsx") || ext == QStringLiteral("qml")) {
            return QStringLiteral("js");
        }
        if (ext == QStringLiteral("rs")) {
            return QStringLiteral("rust");
        }
        if (ext == QStringLiteral("go")) {
            return QStringLiteral("go");
        }
        if (ext == QStringLiteral("css") || ext == QStringLiteral("scss")) {
            return QStringLiteral("css");
        }
        if (ext == QStringLiteral("html") || ext == QStringLiteral("htm") ||
            ext == QStringLiteral("xml") || ext == QStringLiteral("svg") ||
            ext == QStringLiteral("qrc")) {
            return QStringLiteral("html");
        }
    }

    const QString trimmed = firstLine.trimmed();
    if (trimmed.startsWith(QStringLiteral("#!"))) {
        if (trimmed.contains(QStringLiteral("python"))) {
            return QStringLiteral("python");
        }
        if (trimmed.contains(QStringLiteral("sh")) || trimmed.contains(QStringLiteral("bash")) ||
            trimmed.contains(QStringLiteral("zsh"))) {
            return QStringLiteral("bash");
        }
        if (trimmed.contains(QStringLiteral("node"))) {
            return QStringLiteral("js");
        }
    }

    return QStringLiteral("plain");
}

void NullaHighlighter::configure(bool enabled, const QString &languageId, bool darkMode) {
    if (m_enabled == enabled && m_languageId == languageId && m_darkMode == darkMode) {
        return;
    }
    m_enabled = enabled;
    m_languageId = languageId;
    m_darkMode = darkMode;
    rebuildRules();
    rehighlight();
}

void NullaHighlighter::rebuildRules() {
    m_rules.clear();
    m_hasBlockComments = false;
    m_hasTripleQuotes = false;

    if (!m_enabled || m_languageId == QStringLiteral("plain")) {
        return;
    }

    QTextCharFormat keywordFmt;
    keywordFmt.setForeground(QColor(m_darkMode ? QStringLiteral("#f3f4f6") : QStringLiteral("#111318")));
    keywordFmt.setFontWeight(QFont::DemiBold);

    QTextCharFormat typeFmt;
    typeFmt.setForeground(QColor(m_darkMode ? QStringLiteral("#d6dae2") : QStringLiteral("#272b33")));
    typeFmt.setFontWeight(QFont::Medium);

    QTextCharFormat numberFmt;
    numberFmt.setForeground(QColor(m_darkMode ? QStringLiteral("#c8cdd6") : QStringLiteral("#3a3f4a")));

    QTextCharFormat keyFmt;
    keyFmt.setForeground(QColor(m_darkMode ? QStringLiteral("#eef0f4") : QStringLiteral("#16181d")));
    keyFmt.setFontWeight(QFont::Medium);

    m_stringFormat = QTextCharFormat();
    m_stringFormat.setForeground(QColor(m_darkMode ? QStringLiteral("#b4bac5") : QStringLiteral("#4c525e")));

    m_commentFormat = QTextCharFormat();
    m_commentFormat.setForeground(QColor(m_darkMode ? QStringLiteral("#646a76") : QStringLiteral("#8c929e")));
    m_commentFormat.setFontItalic(true);

    auto addWordList = [&](const QStringList &words, const QTextCharFormat &fmt) {
        for (const QString &w : words) {
            HighlightRule rule;
            rule.pattern = QRegularExpression(QStringLiteral("\\b%1\\b").arg(w));
            rule.format = fmt;
            m_rules.append(rule);
        }
    };

    if (m_languageId == QStringLiteral("c")) {
        m_hasBlockComments = true;
        addWordList({
            QStringLiteral("if"), QStringLiteral("else"), QStringLiteral("for"), QStringLiteral("while"),
            QStringLiteral("do"), QStringLiteral("switch"), QStringLiteral("case"), QStringLiteral("default"),
            QStringLiteral("break"), QStringLiteral("continue"), QStringLiteral("return"), QStringLiteral("sizeof"),
            QStringLiteral("typedef"), QStringLiteral("struct"), QStringLiteral("union"), QStringLiteral("enum"),
            QStringLiteral("static"), QStringLiteral("const"), QStringLiteral("volatile"), QStringLiteral("extern"),
            QStringLiteral("inline"), QStringLiteral("class"), QStringLiteral("public"), QStringLiteral("private"),
            QStringLiteral("protected"), QStringLiteral("virtual"), QStringLiteral("override"), QStringLiteral("namespace"),
            QStringLiteral("using"), QStringLiteral("template"), QStringLiteral("typename"), QStringLiteral("auto"),
            QStringLiteral("nullptr"), QStringLiteral("true"), QStringLiteral("false"), QStringLiteral("new"),
            QStringLiteral("delete"), QStringLiteral("explicit"), QStringLiteral("this"), QStringLiteral("goto")
        }, keywordFmt);
        addWordList({
            QStringLiteral("void"), QStringLiteral("int"), QStringLiteral("char"), QStringLiteral("short"),
            QStringLiteral("long"), QStringLiteral("float"), QStringLiteral("double"), QStringLiteral("bool"),
            QStringLiteral("signed"), QStringLiteral("unsigned"), QStringLiteral("size_t"), QStringLiteral("uint8_t"),
            QStringLiteral("uint16_t"), QStringLiteral("uint32_t"), QStringLiteral("uint64_t"), QStringLiteral("int32_t"),
            QStringLiteral("int64_t"), QStringLiteral("QString"), QStringLiteral("QObject"), QStringLiteral("QVector")
        }, typeFmt);
        m_rules.append({QRegularExpression(QStringLiteral("^\\s*#[A-Za-z_]+")), typeFmt, 0});
        m_rules.append({QRegularExpression(QStringLiteral("\\b(0[xX][0-9A-Fa-f]+|\\d+(\\.\\d+)?)\\b")), numberFmt, 0});
        m_rules.append({QRegularExpression(QStringLiteral("\"([^\"\\\\]|\\\\.)*\"")), m_stringFormat, 0});
        m_rules.append({QRegularExpression(QStringLiteral("'([^'\\\\]|\\\\.)*'")), m_stringFormat, 0});
        m_rules.append({QRegularExpression(QStringLiteral("\\x2f\\x2f[^\\n]*")), m_commentFormat, 0});
    } else if (m_languageId == QStringLiteral("python")) {
        m_hasTripleQuotes = true;
        addWordList({
            QStringLiteral("and"), QStringLiteral("as"), QStringLiteral("assert"), QStringLiteral("async"),
            QStringLiteral("await"), QStringLiteral("break"), QStringLiteral("class"), QStringLiteral("continue"),
            QStringLiteral("def"), QStringLiteral("del"), QStringLiteral("elif"), QStringLiteral("else"),
            QStringLiteral("except"), QStringLiteral("False"), QStringLiteral("finally"), QStringLiteral("for"),
            QStringLiteral("from"), QStringLiteral("global"), QStringLiteral("if"), QStringLiteral("import"),
            QStringLiteral("in"), QStringLiteral("is"), QStringLiteral("lambda"), QStringLiteral("None"),
            QStringLiteral("nonlocal"), QStringLiteral("not"), QStringLiteral("or"), QStringLiteral("pass"),
            QStringLiteral("raise"), QStringLiteral("return"), QStringLiteral("True"), QStringLiteral("try"),
            QStringLiteral("while"), QStringLiteral("with"), QStringLiteral("yield")
        }, keywordFmt);
        addWordList({
            QStringLiteral("int"), QStringLiteral("str"), QStringLiteral("float"), QStringLiteral("bool"),
            QStringLiteral("list"), QStringLiteral("dict"), QStringLiteral("set"), QStringLiteral("tuple"),
            QStringLiteral("self"), QStringLiteral("cls"), QStringLiteral("print"), QStringLiteral("len"),
            QStringLiteral("range")
        }, typeFmt);
        m_rules.append({QRegularExpression(QStringLiteral("@[A-Za-z_][A-Za-z0-9_.]*")), typeFmt, 0});
        m_rules.append({QRegularExpression(QStringLiteral("\\b\\d+(\\.\\d+)?\\b")), numberFmt, 0});
        m_rules.append({QRegularExpression(QStringLiteral("\"([^\"\\\\]|\\\\.)*\"")), m_stringFormat, 0});
        m_rules.append({QRegularExpression(QStringLiteral("'([^'\\\\]|\\\\.)*'")), m_stringFormat, 0});
        m_rules.append({QRegularExpression(QStringLiteral("#[^\\n]*")), m_commentFormat, 0});
    } else if (m_languageId == QStringLiteral("bash") || m_languageId == QStringLiteral("make")) {
        addWordList({
            QStringLiteral("if"), QStringLiteral("then"), QStringLiteral("else"), QStringLiteral("elif"),
            QStringLiteral("fi"), QStringLiteral("for"), QStringLiteral("while"), QStringLiteral("until"),
            QStringLiteral("do"), QStringLiteral("done"), QStringLiteral("case"), QStringLiteral("esac"),
            QStringLiteral("in"), QStringLiteral("function"), QStringLiteral("return"), QStringLiteral("local"),
            QStringLiteral("export"), QStringLiteral("readonly"), QStringLiteral("source"), QStringLiteral("exit"),
            QStringLiteral("true"), QStringLiteral("false"), QStringLiteral("ifdef"), QStringLiteral("ifndef"),
            QStringLiteral("endif"), QStringLiteral("include")
        }, keywordFmt);
        m_rules.append({QRegularExpression(QStringLiteral("\\$\\{?[A-Za-z0-9_@*#?]+\\}?")), typeFmt, 0});
        m_rules.append({QRegularExpression(QStringLiteral("^[A-Za-z0-9_.-]+(?=\\s*[:+?]?=)")), keyFmt, 0});
        m_rules.append({QRegularExpression(QStringLiteral("\\b\\d+\\b")), numberFmt, 0});
        m_rules.append({QRegularExpression(QStringLiteral("\"([^\"\\\\]|\\\\.)*\"")), m_stringFormat, 0});
        m_rules.append({QRegularExpression(QStringLiteral("'[^']*'")), m_stringFormat, 0});
        m_rules.append({QRegularExpression(QStringLiteral("#[^\\n]*")), m_commentFormat, 0});
    } else if (m_languageId == QStringLiteral("json")) {
        addWordList({QStringLiteral("true"), QStringLiteral("false"), QStringLiteral("null")}, keywordFmt);
        m_rules.append({QRegularExpression(QStringLiteral("\\b-?\\d+(\\.\\d+)?([eE][+-]?\\d+)?\\b")), numberFmt, 0});
        m_rules.append({QRegularExpression(QStringLiteral("\"([^\"\\\\]|\\\\.)*\"")), m_stringFormat, 0});
        m_rules.append({QRegularExpression(QStringLiteral("(\"([^\"\\\\]|\\\\.)*\")\\s*:")), keyFmt, 1});
    } else if (m_languageId == QStringLiteral("ini")) {
        m_rules.append({QRegularExpression(QStringLiteral("^\\s*\\[[^\\]]+\\]")), keywordFmt, 0});
        m_rules.append({QRegularExpression(QStringLiteral("^\\s*([A-Za-z0-9_.-]+)\\s*=")), keyFmt, 1});
        m_rules.append({QRegularExpression(QStringLiteral("\\b(true|false|yes|no|on|off)\\b")), typeFmt, 0});
        m_rules.append({QRegularExpression(QStringLiteral("\"([^\"\\\\]|\\\\.)*\"")), m_stringFormat, 0});
        m_rules.append({QRegularExpression(QStringLiteral("^\\s*[#;][^\\n]*")), m_commentFormat, 0});
    } else if (m_languageId == QStringLiteral("yaml") || m_languageId == QStringLiteral("toml")) {
        addWordList({QStringLiteral("true"), QStringLiteral("false"), QStringLiteral("null")}, keywordFmt);
        if (m_languageId == QStringLiteral("toml")) {
            m_rules.append({QRegularExpression(QStringLiteral("^\\s*\\[+[^\\]]+\\]+")), keywordFmt, 0});
        }
        m_rules.append({QRegularExpression(QStringLiteral("^\\s*(-\\s*)?([A-Za-z0-9_.-]+)\\s*[:=]")), keyFmt, 2});
        m_rules.append({QRegularExpression(QStringLiteral("\\b\\d+(\\.\\d+)?\\b")), numberFmt, 0});
        m_rules.append({QRegularExpression(QStringLiteral("\"([^\"\\\\]|\\\\.)*\"")), m_stringFormat, 0});
        m_rules.append({QRegularExpression(QStringLiteral("'[^']*'")), m_stringFormat, 0});
        m_rules.append({QRegularExpression(QStringLiteral("#[^\\n]*")), m_commentFormat, 0});
    } else if (m_languageId == QStringLiteral("markdown")) {
        m_rules.append({QRegularExpression(QStringLiteral("^#{1,6}\\s+[^\\n]*")), keywordFmt, 0});
        m_rules.append({QRegularExpression(QStringLiteral("`[^`]+`")), m_stringFormat, 0});
        m_rules.append({QRegularExpression(QStringLiteral("\\*\\*[^*]+\\*\\*")), keyFmt, 0});
        m_rules.append({QRegularExpression(QStringLiteral("^\\s*([-*+]|\\d+\\.)\\s+")), typeFmt, 0});
        m_rules.append({QRegularExpression(QStringLiteral("^\\s*>[^\\n]*")), m_commentFormat, 0});
    } else if (m_languageId == QStringLiteral("js") || m_languageId == QStringLiteral("rust") ||
               m_languageId == QStringLiteral("go")) {
        m_hasBlockComments = true;
        addWordList({
            QStringLiteral("if"), QStringLiteral("else"), QStringLiteral("for"), QStringLiteral("while"),
            QStringLiteral("return"), QStringLiteral("break"), QStringLiteral("continue"), QStringLiteral("switch"),
            QStringLiteral("case"), QStringLiteral("default"), QStringLiteral("match"), QStringLiteral("fn"),
            QStringLiteral("func"), QStringLiteral("function"), QStringLiteral("let"), QStringLiteral("const"),
            QStringLiteral("var"), QStringLiteral("mut"), QStringLiteral("pub"), QStringLiteral("struct"),
            QStringLiteral("enum"), QStringLiteral("impl"), QStringLiteral("trait"), QStringLiteral("type"),
            QStringLiteral("import"), QStringLiteral("export"), QStringLiteral("from"), QStringLiteral("package"),
            QStringLiteral("use"), QStringLiteral("mod"), QStringLiteral("async"), QStringLiteral("await"),
            QStringLiteral("true"), QStringLiteral("false"), QStringLiteral("null"), QStringLiteral("nil"),
            QStringLiteral("property"), QStringLiteral("signal"), QStringLiteral("on"), QStringLiteral("id")
        }, keywordFmt);
        m_rules.append({QRegularExpression(QStringLiteral("\\b(0[xX][0-9A-Fa-f]+|\\d+(\\.\\d+)?)\\b")), numberFmt, 0});
        m_rules.append({QRegularExpression(QStringLiteral("\"([^\"\\\\]|\\\\.)*\"")), m_stringFormat, 0});
        m_rules.append({QRegularExpression(QStringLiteral("'([^'\\\\]|\\\\.)*'")), m_stringFormat, 0});
        m_rules.append({QRegularExpression(QStringLiteral("`[^`]*`")), m_stringFormat, 0});
        m_rules.append({QRegularExpression(QStringLiteral("\\x2f\\x2f[^\\n]*")), m_commentFormat, 0});
    } else if (m_languageId == QStringLiteral("css")) {
        m_hasBlockComments = true;
        m_rules.append({QRegularExpression(QStringLiteral("[.#][A-Za-z_-][A-Za-z0-9_-]*")), keywordFmt, 0});
        m_rules.append({QRegularExpression(QStringLiteral("([a-z-]+)\\s*:")), keyFmt, 1});
        m_rules.append({QRegularExpression(QStringLiteral("\\b\\d+(\\.\\d+)?(px|em|rem|%|ms|s)?\\b")), numberFmt, 0});
        m_rules.append({QRegularExpression(QStringLiteral("\"([^\"\\\\]|\\\\.)*\"")), m_stringFormat, 0});
    } else if (m_languageId == QStringLiteral("html")) {
        m_rules.append({QRegularExpression(QStringLiteral("<\\x2f?[A-Za-z0-9:-]+|/?>")), keywordFmt, 0});
        m_rules.append({QRegularExpression(QStringLiteral("\\b([A-Za-z0-9:-]+)=")), keyFmt, 1});
        m_rules.append({QRegularExpression(QStringLiteral("\"[^\"]*\"")), m_stringFormat, 0});
        m_rules.append({QRegularExpression(QStringLiteral("<!--[^>]*-->")), m_commentFormat, 0});
    }
}

void NullaHighlighter::applyCommentBlock(const QString &text, const QRegularExpression &startExpr,
                                         const QRegularExpression &endExpr, int stateFlag) {
    int startIndex = 0;
    if (previousBlockState() != stateFlag) {
        const QRegularExpressionMatch m = startExpr.match(text);
        startIndex = m.hasMatch() ? m.capturedStart() : -1;
    }

    while (startIndex >= 0) {
        const int searchFrom = (previousBlockState() == stateFlag && startIndex == 0)
                                   ? 0
                                   : startIndex + startExpr.match(text, startIndex).capturedLength();
        const QRegularExpressionMatch endMatch = endExpr.match(text, searchFrom);
        const int endIndex = endMatch.hasMatch() ? endMatch.capturedStart() : -1;
        int len = 0;
        if (endIndex == -1) {
            setCurrentBlockState(stateFlag);
            len = text.length() - startIndex;
        } else {
            len = endIndex - startIndex + endMatch.capturedLength();
        }
        setFormat(startIndex, len, m_commentFormat);
        const QRegularExpressionMatch next = startExpr.match(text, startIndex + len);
        startIndex = next.hasMatch() ? next.capturedStart() : -1;
    }
}

void NullaHighlighter::highlightBlock(const QString &text) {
    setCurrentBlockState(0);
    if (!m_enabled || m_languageId == QStringLiteral("plain")) {
        return;
    }

    for (const HighlightRule &rule : m_rules) {
        QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            const QRegularExpressionMatch match = it.next();
            const int start = match.capturedStart(rule.captureGroup);
            const int len = match.capturedLength(rule.captureGroup);
            if (start >= 0 && len > 0) {
                setFormat(start, len, rule.format);
            }
        }
    }

    if (m_hasBlockComments) {
        applyCommentBlock(text,
                          QRegularExpression(QStringLiteral("\\x2f\\*")),
                          QRegularExpression(QStringLiteral("\\*\\x2f")),
                          1);
    }

    if (m_hasTripleQuotes) {
        applyCommentBlock(text,
                          QRegularExpression(QStringLiteral("\"\"\"")),
                          QRegularExpression(QStringLiteral("\"\"\"")),
                          2);
    }
}
