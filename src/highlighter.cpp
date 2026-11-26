#include "highlighter.h"

#include <QFileInfo>
#include <QRegularExpression>
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

QString NullaHighlighter::commentPrefix(const QString &languageId) {
    if (languageId == QStringLiteral("c") ||
        languageId == QStringLiteral("js") ||
        languageId == QStringLiteral("rust") ||
        languageId == QStringLiteral("go") ||
        languageId == QStringLiteral("css")) {
        return QStringLiteral("\x2f\x2f");
    }
    if (languageId == QStringLiteral("ini")) {
        return QStringLiteral("#");
    }
    if (languageId == QStringLiteral("html") ||
        languageId == QStringLiteral("markdown") ||
        languageId == QStringLiteral("json")) {
        return QString();
    }
    return QStringLiteral("#");
}

static QString detectLanguageFromContent(const QString &sample) {
    if (sample.trimmed().isEmpty()) {
        return QStringLiteral("plain");
    }

    const QString head = sample.left(4096);
    const QStringList lines = head.split(QLatin1Char('\n'));
    QString firstNonEmpty;
    for (const QString &l : lines) {
        const QString t = l.trimmed();
        if (!t.isEmpty()) {
            firstNonEmpty = t;
            break;
        }
    }

    if (firstNonEmpty.startsWith(QStringLiteral("#!"))) {
        if (firstNonEmpty.contains(QStringLiteral("python"))) {
            return QStringLiteral("python");
        }
        if (firstNonEmpty.contains(QStringLiteral("sh")) ||
            firstNonEmpty.contains(QStringLiteral("bash")) ||
            firstNonEmpty.contains(QStringLiteral("zsh"))) {
            return QStringLiteral("bash");
        }
        if (firstNonEmpty.contains(QStringLiteral("node")) ||
            firstNonEmpty.contains(QStringLiteral("deno")) ||
            firstNonEmpty.contains(QStringLiteral("bun"))) {
            return QStringLiteral("js");
        }
    }

    if (firstNonEmpty.startsWith(QLatin1Char('{')) || firstNonEmpty.startsWith(QLatin1Char('['))) {
        static const QRegularExpression jsonKeyRe(QStringLiteral("\"[^\"]+\"\\s*:"));
        if (jsonKeyRe.match(head).hasMatch()) {
            return QStringLiteral("json");
        }
    }

    if (firstNonEmpty.startsWith(QStringLiteral("<!DOCTYPE"), Qt::CaseInsensitive) ||
        firstNonEmpty.startsWith(QStringLiteral("<html"), Qt::CaseInsensitive) ||
        firstNonEmpty.startsWith(QStringLiteral("<svg"), Qt::CaseInsensitive) ||
        firstNonEmpty.startsWith(QStringLiteral("<?xml"), Qt::CaseInsensitive)) {
        return QStringLiteral("html");
    }

    static const QRegularExpression cCueRe(
        QStringLiteral("(^|\\n)\\s*#\\s*(include|define|pragma|ifndef|ifdef)\\b|\\b(std::|QString|nullptr|printf\\s*\\(|int\\s+main\\s*\\()"));
    if (cCueRe.match(head).hasMatch()) {
        return QStringLiteral("c");
    }

    static const QRegularExpression rustCueRe(
        QStringLiteral("(^|\\n)\\s*(pub\\s+)?(fn\\s+[A-Za-z_][A-Za-z0-9_]*\\s*\\(|use\\s+[A-Za-z_][A-Za-z0-9_:]*;|impl\\b|let\\s+mut\\b|println!\\()"));
    if (rustCueRe.match(head).hasMatch()) {
        return QStringLiteral("rust");
    }

    static const QRegularExpression goCueRe(
        QStringLiteral("(^|\\n)\\s*package\\s+[a-zA-Z_][a-zA-Z0-9_]*\\b|(^|\\n)\\s*func\\s+[A-Za-z_][A-Za-z0-9_]*\\s*\\(.*\\)\\s*\\{|:=\\s*"));
    if (goCueRe.match(head).hasMatch()) {
        return QStringLiteral("go");
    }

    static const QRegularExpression pyCueRe(
        QStringLiteral("(^|\\n)\\s*(def\\s+[A-Za-z_][A-Za-z0-9_]*\\s*\\(|from\\s+[A-Za-z0-9_.]+\\s+import\\s+|import\\s+[A-Za-z0-9_.]+(\\s+as\\s+|\\s*$)|if\\s+__name__\\s*==)"));
    if (pyCueRe.match(head).hasMatch()) {
        return QStringLiteral("python");
    }

    static const QRegularExpression jsCueRe(
        QStringLiteral("(^|\\n)\\s*(import\\s+QtQuick\\b|import\\s+\\{.*\\}\\s+from\\s+|export\\s+(default|const|function|class)\\b|function\\s+[A-Za-z_$]|const\\s+[A-Za-z_$][A-Za-z0-9_$]*\\s*=.*=>|console\\.log\\s*\\()"));
    if (jsCueRe.match(head).hasMatch()) {
        return QStringLiteral("js");
    }

    static const QRegularExpression cssCueRe(
        QStringLiteral("(^|\\n)\\s*([.#]?[A-Za-z_-][A-Za-z0-9_-]*|@media|@keyframes)\\s*\\{[^}]*:\\s*[^}]+;"));
    if (cssCueRe.match(head).hasMatch()) {
        return QStringLiteral("css");
    }

    static const QRegularExpression bashCueRe(
        QStringLiteral("(^|\\n)\\s*(export\\s+[A-Za-z_][A-Za-z0-9_]*=|echo\\s+|fi\\s*$|done\\s*$|esac\\s*$|if\\s+\\[\\[?\\s+)"));
    if (bashCueRe.match(head).hasMatch()) {
        return QStringLiteral("bash");
    }

    static const QRegularExpression makeCueRe(
        QStringLiteral("(^|\\n)\\s*(pkgname=|pkgver=|[A-Za-z0-9_.-]+:\\s*.*\\n\\t)"));
    if (makeCueRe.match(head).hasMatch()) {
        return QStringLiteral("make");
    }

    static const QRegularExpression iniCueRe(
        QStringLiteral("(^|\\n)\\s*\\[[A-Za-z0-9_.-]+\\]\\s*\\n"));
    static const QRegularExpression tomlQuoteRe(
        QStringLiteral("(^|\\n)\\s*[A-Za-z0-9_-]+\\s*=\\s*[\"\\[]"));
    if (iniCueRe.match(head).hasMatch()) {
        return tomlQuoteRe.match(head).hasMatch() ? QStringLiteral("toml") : QStringLiteral("ini");
    }

    static const QRegularExpression mdCueRe(
        QStringLiteral("(^|\\n)#{1,6}\\s+\\S+|(^|\\n)```[a-zA-Z0-9]*\\s*\\n"));
    if (mdCueRe.match(head).hasMatch()) {
        return QStringLiteral("markdown");
    }

    static const QRegularExpression yamlCueRe(
        QStringLiteral("(^|\\n)---\\s*\\n|(^|\\n)[A-Za-z0-9_.-]+:\\s+(\\S+|\\n\\s+-\\s+)"));
    if (yamlCueRe.match(head).hasMatch() && !head.contains(QLatin1Char(';')) && !head.contains(QLatin1Char('{'))) {
        return QStringLiteral("yaml");
    }

    static const QRegularExpression kvIniRe(
        QStringLiteral("(^|\\n)[A-Za-z0-9_.-]+=[A-Za-z0-9_.\x2f\"'-]+(\\n|$)"));
    if (kvIniRe.match(head).hasMatch() && !head.contains(QLatin1Char(';'))) {
        return QStringLiteral("ini");
    }

    return QStringLiteral("plain");
}

QString NullaHighlighter::detectLanguage(const QString &filePath, const QString &contentSample) {
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

    return detectLanguageFromContent(contentSample);
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
    keywordFmt.setForeground(QColor(m_darkMode ? QStringLiteral("#c792ea") : QStringLiteral("#7c3aed")));
    keywordFmt.setFontWeight(QFont::DemiBold);

    QTextCharFormat typeFmt;
    typeFmt.setForeground(QColor(m_darkMode ? QStringLiteral("#7dcfff") : QStringLiteral("#0284c7")));
    typeFmt.setFontWeight(QFont::Medium);

    QTextCharFormat numberFmt;
    numberFmt.setForeground(QColor(m_darkMode ? QStringLiteral("#ff9e64") : QStringLiteral("#c2410c")));

    QTextCharFormat keyFmt;
    keyFmt.setForeground(QColor(m_darkMode ? QStringLiteral("#7aa2f7") : QStringLiteral("#2563eb")));
    keyFmt.setFontWeight(QFont::Medium);

    m_stringFormat = QTextCharFormat();
    m_stringFormat.setForeground(QColor(m_darkMode ? QStringLiteral("#9ece6a") : QStringLiteral("#15803d")));

    m_commentFormat = QTextCharFormat();
    m_commentFormat.setForeground(QColor(m_darkMode ? QStringLiteral("#636a78") : QStringLiteral("#858c99")));
    m_commentFormat.setFontItalic(true);

    auto addRule = [&](const QString &expr, const QTextCharFormat &fmt, int group = 0) {
        QRegularExpression re(expr);
        m_rules.append({re, fmt, group});
    };

    auto addWordList = [&](const QStringList &words, const QTextCharFormat &fmt) {
        if (words.isEmpty()) {
            return;
        }
        addRule(QStringLiteral("\\b(?:%1)\\b").arg(words.join(QLatin1Char('|'))), fmt, 0);
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
        addRule(QStringLiteral("^\\s*#[A-Za-z_]+"), typeFmt);
        addRule(QStringLiteral("\\b(0[xX][0-9A-Fa-f]+|\\d+(\\.\\d+)?)\\b"), numberFmt);
        addRule(QStringLiteral("\"([^\"\\\\]|\\\\.)*\""), m_stringFormat);
        addRule(QStringLiteral("'([^'\\\\]|\\\\.)*'"), m_stringFormat);
        addRule(QStringLiteral("\\x2f\\x2f[^\\n]*"), m_commentFormat);
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
        addRule(QStringLiteral("@[A-Za-z_][A-Za-z0-9_.]*"), typeFmt);
        addRule(QStringLiteral("\\b\\d+(\\.\\d+)?\\b"), numberFmt);
        addRule(QStringLiteral("\"([^\"\\\\]|\\\\.)*\""), m_stringFormat);
        addRule(QStringLiteral("'([^'\\\\]|\\\\.)*'"), m_stringFormat);
        addRule(QStringLiteral("#[^\\n]*"), m_commentFormat);
    } else if (m_languageId == QStringLiteral("bash") || m_languageId == QStringLiteral("make")) {
        addWordList({
            QStringLiteral("if"), QStringLiteral("then"), QStringLiteral("else"), QStringLiteral("elif"),
            QStringLiteral("fi"), QStringLiteral("for"), QStringLiteral("while"), QStringLiteral("do"),
            QStringLiteral("done"), QStringLiteral("case"), QStringLiteral("esac"), QStringLiteral("in"),
            QStringLiteral("function"), QStringLiteral("local"), QStringLiteral("export"), QStringLiteral("return"),
            QStringLiteral("exit"), QStringLiteral("sudo"), QStringLiteral("echo"), QStringLiteral("printf"),
            QStringLiteral("cd"), QStringLiteral("rm"), QStringLiteral("mkdir"), QStringLiteral("install"),
            QStringLiteral("make"), QStringLiteral("true"), QStringLiteral("false")
        }, keywordFmt);
        addRule(QStringLiteral("\\$\\{?[A-Za-z_][A-Za-z0-9_]*\\}?"), typeFmt);
        addRule(QStringLiteral("^([A-Za-z0-9_.-]+)\\s*(?=[+:?]?=)"), keyFmt, 1);
        addRule(QStringLiteral("^([A-Za-z0-9_.\x2f-]+):"), keyFmt, 1);
        addRule(QStringLiteral("\\b\\d+\\b"), numberFmt);
        addRule(QStringLiteral("\"([^\"\\\\]|\\\\.)*\""), m_stringFormat);
        addRule(QStringLiteral("'[^']*'"), m_stringFormat);
        addRule(QStringLiteral("#[^\\n]*"), m_commentFormat);
    } else if (m_languageId == QStringLiteral("js")) {
        m_hasBlockComments = true;
        addWordList({
            QStringLiteral("import"), QStringLiteral("from"), QStringLiteral("export"), QStringLiteral("default"),
            QStringLiteral("function"), QStringLiteral("const"), QStringLiteral("let"), QStringLiteral("var"),
            QStringLiteral("if"), QStringLiteral("else"), QStringLiteral("for"), QStringLiteral("while"),
            QStringLiteral("switch"), QStringLiteral("case"), QStringLiteral("break"), QStringLiteral("continue"),
            QStringLiteral("return"), QStringLiteral("class"), QStringLiteral("new"), QStringLiteral("async"),
            QStringLiteral("await"), QStringLiteral("try"), QStringLiteral("catch"), QStringLiteral("throw"),
            QStringLiteral("true"), QStringLiteral("false"), QStringLiteral("null"), QStringLiteral("undefined"),
            QStringLiteral("property"), QStringLiteral("readonly"), QStringLiteral("signal")
        }, keywordFmt);
        addRule(QStringLiteral("\\b[A-Z][A-Za-z0-9_]*\\b"), typeFmt);
        addRule(QStringLiteral("\\b(0[xX][0-9A-Fa-f]+|\\d+(\\.\\d+)?)\\b"), numberFmt);
        addRule(QStringLiteral("\"([^\"\\\\]|\\\\.)*\""), m_stringFormat);
        addRule(QStringLiteral("'([^'\\\\]|\\\\.)*'"), m_stringFormat);
        addRule(QStringLiteral("`[^`]*`"), m_stringFormat);
        addRule(QStringLiteral("\\x2f\\x2f[^\\n]*"), m_commentFormat);
    } else if (m_languageId == QStringLiteral("rust")) {
        m_hasBlockComments = true;
        addWordList({
            QStringLiteral("fn"), QStringLiteral("let"), QStringLiteral("mut"), QStringLiteral("const"),
            QStringLiteral("pub"), QStringLiteral("mod"), QStringLiteral("use"), QStringLiteral("struct"),
            QStringLiteral("enum"), QStringLiteral("impl"), QStringLiteral("trait"), QStringLiteral("for"),
            QStringLiteral("while"), QStringLiteral("loop"), QStringLiteral("if"), QStringLiteral("else"),
            QStringLiteral("match"), QStringLiteral("return"), QStringLiteral("self"), QStringLiteral("Self"),
            QStringLiteral("crate"), QStringLiteral("where"), QStringLiteral("async"), QStringLiteral("await"),
            QStringLiteral("true"), QStringLiteral("false")
        }, keywordFmt);
        addWordList({
            QStringLiteral("i32"), QStringLiteral("i64"), QStringLiteral("u32"), QStringLiteral("u64"),
            QStringLiteral("usize"), QStringLiteral("isize"), QStringLiteral("f64"), QStringLiteral("bool"),
            QStringLiteral("str"), QStringLiteral("String"), QStringLiteral("Vec"), QStringLiteral("Option"),
            QStringLiteral("Result")
        }, typeFmt);
        addRule(QStringLiteral("\\b\\d+(\\.\\d+)?\\b"), numberFmt);
        addRule(QStringLiteral("\"([^\"\\\\]|\\\\.)*\""), m_stringFormat);
        addRule(QStringLiteral("\\x2f\\x2f[^\\n]*"), m_commentFormat);
    } else if (m_languageId == QStringLiteral("go")) {
        m_hasBlockComments = true;
        addWordList({
            QStringLiteral("package"), QStringLiteral("import"), QStringLiteral("func"), QStringLiteral("var"),
            QStringLiteral("const"), QStringLiteral("type"), QStringLiteral("struct"), QStringLiteral("interface"),
            QStringLiteral("if"), QStringLiteral("else"), QStringLiteral("for"), QStringLiteral("range"),
            QStringLiteral("switch"), QStringLiteral("case"), QStringLiteral("return"), QStringLiteral("go"),
            QStringLiteral("defer"), QStringLiteral("select"), QStringLiteral("true"), QStringLiteral("false"),
            QStringLiteral("nil")
        }, keywordFmt);
        addWordList({
            QStringLiteral("int"), QStringLiteral("int64"), QStringLiteral("uint"), QStringLiteral("string"),
            QStringLiteral("bool"), QStringLiteral("byte"), QStringLiteral("error"), QStringLiteral("float64")
        }, typeFmt);
        addRule(QStringLiteral("\\b\\d+(\\.\\d+)?\\b"), numberFmt);
        addRule(QStringLiteral("\"([^\"\\\\]|\\\\.)*\""), m_stringFormat);
        addRule(QStringLiteral("`[^`]*`"), m_stringFormat);
        addRule(QStringLiteral("\\x2f\\x2f[^\\n]*"), m_commentFormat);
    } else if (m_languageId == QStringLiteral("json")) {
        addRule(QStringLiteral("\"([^\"]+)\"\\s*:"), keyFmt, 1);
        addRule(QStringLiteral(":\\s*\"([^\"\\\\]|\\\\.)*\""), m_stringFormat);
        addWordList({QStringLiteral("true"), QStringLiteral("false"), QStringLiteral("null")}, keywordFmt);
        addRule(QStringLiteral("-?\\b\\d+(\\.\\d+)?\\b"), numberFmt);
    } else if (m_languageId == QStringLiteral("ini") ||
               m_languageId == QStringLiteral("toml") ||
               m_languageId == QStringLiteral("yaml")) {
        addRule(QStringLiteral("^\\s*\\[[^\\]]+\\]"), keywordFmt);
        addRule(QStringLiteral("^\\s*([A-Za-z0-9_.-]+)\\s*(?=[:=])"), keyFmt, 1);
        addWordList({
            QStringLiteral("true"), QStringLiteral("false"), QStringLiteral("yes"),
            QStringLiteral("no"), QStringLiteral("null")
        }, keywordFmt);
        addRule(QStringLiteral("\\b\\d+(\\.\\d+)?\\b"), numberFmt);
        addRule(QStringLiteral("\"([^\"\\\\]|\\\\.)*\""), m_stringFormat);
        addRule(QStringLiteral("'[^']*'"), m_stringFormat);
        addRule(QStringLiteral("(^|\\s)[#;][^\\n]*"), m_commentFormat);
    } else if (m_languageId == QStringLiteral("css")) {
        m_hasBlockComments = true;
        addRule(QStringLiteral("([A-Za-z-]+)\\s*:"), keyFmt, 1);
        addRule(QStringLiteral("#[0-9A-Fa-f]{3,8}\\b"), numberFmt);
        addRule(QStringLiteral("\\b\\d+(\\.\\d+)?(px|em|rem|%|vh|vw|ms|s)?\\b"), numberFmt);
        addRule(QStringLiteral("\"([^\"\\\\]|\\\\.)*\""), m_stringFormat);
        addRule(QStringLiteral("'[^']*'"), m_stringFormat);
    } else if (m_languageId == QStringLiteral("html")) {
        addRule(QStringLiteral("<\\x2f?([A-Za-z0-9:-]+)"), keywordFmt, 1);
        addRule(QStringLiteral("\\s([A-Za-z0-9:-]+)="), keyFmt, 1);
        addRule(QStringLiteral("\"([^\"]*)\""), m_stringFormat);
        addRule(QStringLiteral("<!--[^>]*-->"), m_commentFormat);
    } else if (m_languageId == QStringLiteral("markdown")) {
        addRule(QStringLiteral("^#{1,6}\\s.*$"), keywordFmt);
        addRule(QStringLiteral("`[^`]+`"), typeFmt);
        addRule(QStringLiteral("^\\s*([-*+]|\\d+\\.)\\s"), keyFmt);
        addRule(QStringLiteral("\\[([^\\]]+)\\]\\([^\\)]+\\)"), m_stringFormat);
    }
}

void NullaHighlighter::highlightBlock(const QString &text) {
    if (!m_enabled || m_languageId == QStringLiteral("plain")) {
        setCurrentBlockState(0);
        return;
    }

    for (const HighlightRule &rule : m_rules) {
        QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            const QRegularExpressionMatch m = it.next();
            const int start = m.capturedStart(rule.captureGroup);
            const int len = m.capturedLength(rule.captureGroup);
            if (start >= 0 && len > 0) {
                setFormat(start, len, rule.format);
            }
        }
    }

    setCurrentBlockState(0);
    if (m_hasBlockComments) {
        static const QRegularExpression startExpr(QStringLiteral("\\x2f\\x2a"));
        static const QRegularExpression endExpr(QStringLiteral("\\x2a\\x2f"));
        int startIndex = 0;
        if (previousBlockState() != 1) {
            startIndex = text.indexOf(startExpr);
        }
        while (startIndex >= 0) {
            const QRegularExpressionMatch endMatch = endExpr.match(text, startIndex + 2);
            int endIndex = endMatch.capturedStart();
            int commentLen = 0;
            if (endIndex == -1) {
                setCurrentBlockState(1);
                commentLen = text.length() - startIndex;
            } else {
                commentLen = endIndex - startIndex + endMatch.capturedLength();
            }
            setFormat(startIndex, commentLen, m_commentFormat);
            startIndex = text.indexOf(startExpr, startIndex + commentLen);
        }
    } else if (m_hasTripleQuotes) {
        static const QRegularExpression tqExpr(QStringLiteral("\"\"\"|'''"));
        int startIndex = 0;
        int state = previousBlockState();
        if (state != 2) {
            startIndex = text.indexOf(tqExpr);
        }
        while (startIndex >= 0) {
            const int searchFrom = (state == 2) ? startIndex : (startIndex + 3);
            const QRegularExpressionMatch endMatch = tqExpr.match(text, searchFrom);
            int endIndex = endMatch.capturedStart();
            int len = 0;
            if (endIndex == -1) {
                setCurrentBlockState(2);
                len = text.length() - startIndex;
            } else {
                len = endIndex - startIndex + 3;
            }
            setFormat(startIndex, len, m_stringFormat);
            state = 0;
            startIndex = text.indexOf(tqExpr, startIndex + len);
        }
    }
}
