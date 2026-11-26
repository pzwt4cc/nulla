#include "editor_backend.h"

#include <QColor>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPalette>
#include <QRegularExpression>
#include <QSet>
#include <QSettings>
#include <QStandardPaths>
#include <QStyleHints>
#include <QTimer>
#include <QtEnvironmentVariables>
#include <QUrl>

#include <algorithm>
#include <cmath>

static bool detectDesktopEnvironment() {
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
  return true;
#else
  if (!qEnvironmentVariableIsEmpty("HYPRLAND_INSTANCE_SIGNATURE") ||
      !qEnvironmentVariableIsEmpty("SWAYSOCK") ||
      !qEnvironmentVariableIsEmpty("I3SOCK") ||
      !qEnvironmentVariableIsEmpty("BSPWM_SOCKET")) {
    return false;
  }

  const QString combined =
      (qEnvironmentVariable("XDG_CURRENT_DESKTOP") + QLatin1Char(' ') +
       qEnvironmentVariable("XDG_SESSION_DESKTOP") + QLatin1Char(' ') +
       qEnvironmentVariable("DESKTOP_SESSION"))
          .toLower();

  const char *const wms[] = {"hyprland", "sway",         "river",  "niri",
                             "wayfire",  "labwc",        "i3",     "bspwm",
                             "dwm",      "awesome",      "qtile",  "xmonad",
                             "openbox",  "herbstluftwm", "leftwm", "spectrwm",
                             "fluxbox",  "icewm",        "jwm"};
  for (const char *wm : wms) {
    if (combined.contains(QLatin1String(wm))) {
      return false;
    }
  }

  const char *const des[] = {
      "gnome",  "ubuntu", "kde",           "plasma", "xfce",     "cinnamon",
      "mate",   "lxqt",   "lxde",          "budgie", "pantheon", "deepin",
      "cosmic", "unity",  "enlightenment", "ukui",   "cutefish"};
  for (const char *de : des) {
    if (combined.contains(QLatin1String(de))) {
      return true;
    }
  }

  return false;
#endif
}

static QString pickMonoFontFamily() {
  const QStringList available = QFontDatabase::families();
  const char *const preferred[] = {
      "JetBrains Mono", "IBM Plex Mono",
      "Cascadia Mono",  "Cascadia Code",
      "CaskaydiaCove Nerd Font Mono",
      "Fira Code",      "FiraCode Nerd Font Mono",
      "Consolas",       "Hack",
      "Adwaita Mono",   "Ubuntu Mono",
      "Liberation Mono"};
  for (const char *name : preferred) {
    const QString qname = QLatin1String(name);
    if (available.contains(qname, Qt::CaseInsensitive)) {
      return qname;
    }
  }
  return QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
}

static QString normalizeFilePath(const QString &rawPath) {
  QString path = rawPath;
  const QUrl url(rawPath);
  if (url.isValid() && url.isLocalFile()) {
    path = url.toLocalFile();
  }
  return QFileInfo(path).absoluteFilePath();
}

static double linearChannel(double c) {
  return (c <= 0.03928) ? (c / 12.92) : std::pow((c + 0.055) / 1.055, 2.4);
}

static double relativeLuminance(const QColor &c) {
  return 0.2126 * linearChannel(c.redF()) +
         0.7152 * linearChannel(c.greenF()) +
         0.0722 * linearChannel(c.blueF());
}

static double contrastRatio(const QColor &a, const QColor &b) {
  const double l1 = relativeLuminance(a);
  const double l2 = relativeLuminance(b);
  return (std::max(l1, l2) + 0.05) / (std::min(l1, l2) + 0.05);
}

static QColor blendColors(const QColor &a, const QColor &b, double t) {
  const double k = qBound(0.0, t, 1.0);
  QColor out;
  out.setRgbF(a.redF() * (1.0 - k) + b.redF() * k,
              a.greenF() * (1.0 - k) + b.greenF() * k,
              a.blueF() * (1.0 - k) + b.blueF() * k,
              a.alphaF() * (1.0 - k) + b.alphaF() * k);
  return out;
}

static QColor withAlpha(QColor c, int alpha) {
  c.setAlpha(qBound(0, alpha, 255));
  return c;
}

static QString colorStr(const QColor &c) {
  return (c.alpha() == 255) ? c.name(QColor::HexRgb) : c.name(QColor::HexArgb);
}

struct AdaptivePaletteSeed {
  bool valid = false;
  QColor bgShell;
  QColor bgEditor;
  QColor bgSurface;
  QColor fgMain;
  QColor accent;
  QColor accentFg;
  QColor border;
  QColor danger;
};

#if defined(Q_OS_WIN)
static AdaptivePaletteSeed tryLoadWindowsPalette() {
  bool sysDark = true;
  if (QGuiApplication::styleHints() &&
      QGuiApplication::styleHints()->colorScheme() != Qt::ColorScheme::Unknown) {
    sysDark = (QGuiApplication::styleHints()->colorScheme() != Qt::ColorScheme::Light);
  } else {
    const QSettings pers(
        QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"),
        QSettings::NativeFormat);
    sysDark = (pers.value(QStringLiteral("AppsUseLightTheme"), 0).toInt() == 0);
  }

  QColor accent;
  const QSettings dwm(
      QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\DWM"),
      QSettings::NativeFormat);
  if (dwm.contains(QStringLiteral("AccentColor"))) {
    const quint32 abgr = dwm.value(QStringLiteral("AccentColor")).toUInt();
    accent = QColor(static_cast<int>(abgr & 0xffU),
                    static_cast<int>((abgr >> 8) & 0xffU),
                    static_cast<int>((abgr >> 16) & 0xffU));
  } else if (dwm.contains(QStringLiteral("ColorizationColor"))) {
    const quint32 argb = dwm.value(QStringLiteral("ColorizationColor")).toUInt();
    accent = QColor(static_cast<int>((argb >> 16) & 0xffU),
                    static_cast<int>((argb >> 8) & 0xffU),
                    static_cast<int>(argb & 0xffU));
  }
  if (!accent.isValid()) {
    const QColor palHi = QGuiApplication::palette().color(QPalette::Highlight);
    if (palHi.isValid()) {
      accent = QColor(palHi.rgb());
    }
  }
  if (!accent.isValid()) {
    return AdaptivePaletteSeed();
  }

  const QColor baseShell(sysDark ? QStringLiteral("#0d0e11") : QStringLiteral("#ebedf2"));
  const QColor baseEditor(sysDark ? QStringLiteral("#16181e") : QStringLiteral("#ffffff"));
  const QColor baseSurface(sysDark ? QStringLiteral("#1c1f27") : QStringLiteral("#f7f8fa"));

  AdaptivePaletteSeed seed;
  seed.valid = true;
  seed.bgShell = blendColors(baseShell, accent, sysDark ? 0.06 : 0.045);
  seed.bgEditor = blendColors(baseEditor, accent, sysDark ? 0.03 : 0.02);
  seed.bgSurface = blendColors(baseSurface, accent, sysDark ? 0.07 : 0.05);
  seed.fgMain = QColor(sysDark ? QStringLiteral("#f0f2f6") : QStringLiteral("#14161a"));
  seed.accent = accent;
  return seed;
}
#else
static QColor parseCssColorToken(const QString &raw,
                                 const QHash<QString, QString> &vars,
                                 int depth = 0) {
  if (depth > 8) {
    return QColor();
  }
  QString s = raw.trimmed();
  if (s.startsWith(QLatin1Char('@'))) {
    const QString ref = s.mid(1).trimmed();
    if (vars.contains(ref)) {
      return parseCssColorToken(vars.value(ref), vars, depth + 1);
    }
    return QColor();
  }
  if (s.startsWith(QLatin1Char('#'))) {
    if (s.size() == 9) {
      const QString rrggbb = s.mid(1, 6);
      const QString aa = s.mid(7, 2);
      const QColor c(QLatin1Char('#') + aa + rrggbb);
      if (c.isValid()) {
        return c;
      }
    }
    const QColor c(s);
    return c.isValid() ? c : QColor();
  }
  static const QRegularExpression rgbRe(QStringLiteral(
      "^rgba?\\s*\\(\\s*(\\d+)\\s*,\\s*(\\d+)\\s*,\\s*(\\d+)(?:\\s*,\\s*([0-9.]+%?))?\\s*\\)$"));
  const QRegularExpressionMatch m = rgbRe.match(s);
  if (m.hasMatch()) {
    const int r = qBound(0, m.captured(1).toInt(), 255);
    const int g = qBound(0, m.captured(2).toInt(), 255);
    const int b = qBound(0, m.captured(3).toInt(), 255);
    int a = 255;
    if (!m.captured(4).isEmpty()) {
      QString astr = m.captured(4);
      if (astr.endsWith(QLatin1Char('%'))) {
        astr.chop(1);
        a = qBound(0, qRound(astr.toDouble() * 2.55), 255);
      } else {
        const double v = astr.toDouble();
        a = qBound(0, (v <= 1.0) ? qRound(v * 255.0) : qRound(v), 255);
      }
    }
    return QColor(r, g, b, a);
  }
  return QColor();
}

static void collectGtkCssDefs(const QString &filePath,
                                QHash<QString, QString> &vars,
                                QStringList &visitedFiles,
                                int depth = 0) {
  if (depth > 3) {
    return;
  }
  const QString absPath = QFileInfo(filePath).absoluteFilePath();
  if (visitedFiles.contains(absPath)) {
    return;
  }
  QFile f(absPath);
  if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return;
  }
  visitedFiles.append(absPath);

  const QString content = QString::fromUtf8(f.readAll());
  const QDir baseDir = QFileInfo(absPath).absoluteDir();

  static const QRegularExpression importRe(QStringLiteral(
      "@import\\s+(?:url\\(\\s*[\"']?([^\"')]+)[\"']?\\s*\\)|[\"']([^\"']+)[\"'])\\s*;"));
  QRegularExpressionMatchIterator iit = importRe.globalMatch(content);
  while (iit.hasNext()) {
    const QRegularExpressionMatch im = iit.next();
    QString target = im.captured(1).isEmpty() ? im.captured(2) : im.captured(1);
    target = target.trimmed();
    if (target.isEmpty() || target.startsWith(QStringLiteral("http")) ||
        target.startsWith(QStringLiteral("resource:"))) {
      continue;
    }
    const QString resolved =
        QFileInfo(target).isAbsolute() ? target : baseDir.filePath(target);
    collectGtkCssDefs(resolved, vars, visitedFiles, depth + 1);
  }

  static const QRegularExpression defRe(QStringLiteral(
      "@define-color\\s+([A-Za-z0-9_-]+)\\s+([^;]+);"));
  QRegularExpressionMatchIterator dit = defRe.globalMatch(content);
  while (dit.hasNext()) {
    const QRegularExpressionMatch dm = dit.next();
    vars.insert(dm.captured(1).trimmed(), dm.captured(2).trimmed());
  }
}

static QColor pickFirstCssColor(const QHash<QString, QString> &vars,
                                const QStringList &keys) {
  for (const QString &key : keys) {
    if (vars.contains(key)) {
      const QColor c = parseCssColorToken(vars.value(key), vars);
      if (c.isValid() && c.alpha() > 20) {
        return c;
      }
    }
  }
  return QColor();
}

static AdaptivePaletteSeed tryLoadGtkCssPalette(const QString &cfgRoot,
                                                QStringList *usedFiles) {
  const QStringList candidates = {
      QDir(cfgRoot).filePath(QStringLiteral("matugen/colors.css")),
      QDir(cfgRoot).filePath(QStringLiteral("gtk-4.0/colors.css")),
      QDir(cfgRoot).filePath(QStringLiteral("gtk-4.0/gtk.css")),
      QDir(cfgRoot).filePath(QStringLiteral("gtk-3.0/colors.css")),
      QDir(cfgRoot).filePath(QStringLiteral("gtk-3.0/gtk.css"))};

  for (const QString &path : candidates) {
    if (!QFileInfo::exists(path)) {
      continue;
    }
    QHash<QString, QString> vars;
    QStringList visited;
    collectGtkCssDefs(path, vars, visited);
    if (usedFiles) {
      usedFiles->append(visited);
    }
    const QColor shell = pickFirstCssColor(
        vars, {QStringLiteral("window_bg_color"), QStringLiteral("theme_bg_color"),
               QStringLiteral("bg_color"), QStringLiteral("view_bg_color")});
    const QColor editor = pickFirstCssColor(
        vars, {QStringLiteral("view_bg_color"), QStringLiteral("theme_base_color"),
               QStringLiteral("base_color"), QStringLiteral("window_bg_color")});
    const QColor fg = pickFirstCssColor(
        vars, {QStringLiteral("view_fg_color"), QStringLiteral("window_fg_color"),
               QStringLiteral("theme_fg_color"), QStringLiteral("fg_color")});

    if (shell.isValid() && editor.isValid() && fg.isValid()) {
      AdaptivePaletteSeed seed;
      seed.valid = true;
      seed.bgShell = QColor(shell.rgb());
      seed.bgEditor = QColor(editor.rgb());
      seed.bgSurface = pickFirstCssColor(
          vars, {QStringLiteral("headerbar_bg_color"),
                 QStringLiteral("popover_bg_color"),
                 QStringLiteral("card_bg_color"),
                 QStringLiteral("sidebar_bg_color")});
      seed.fgMain = QColor(fg.rgb());
      seed.accent = pickFirstCssColor(
          vars, {QStringLiteral("accent_bg_color"),
                 QStringLiteral("accent_color"),
                 QStringLiteral("theme_selected_bg_color")});
      seed.accentFg = pickFirstCssColor(
          vars, {QStringLiteral("accent_fg_color"),
                 QStringLiteral("theme_selected_fg_color")});
      seed.border = pickFirstCssColor(
          vars, {QStringLiteral("headerbar_border_color"),
                 QStringLiteral("borders")});
      seed.danger = pickFirstCssColor(
          vars, {QStringLiteral("destructive_bg_color"),
                 QStringLiteral("destructive_color"),
                 QStringLiteral("error_color")});
      return seed;
    }
  }
  return AdaptivePaletteSeed();
}

static QColor parseKdeColorValue(const QString &raw) {
  const QString trimmed = raw.trimmed();
  if (trimmed.startsWith(QLatin1Char('#'))) {
    return QColor(trimmed);
  }
  const QStringList parts = trimmed.split(QLatin1Char(','));
  if (parts.size() >= 3) {
    bool okR = false, okG = false, okB = false;
    const int r = parts.at(0).trimmed().toInt(&okR);
    const int g = parts.at(1).trimmed().toInt(&okG);
    const int b = parts.at(2).trimmed().toInt(&okB);
    if (okR && okG && okB) {
      return QColor(qBound(0, r, 255), qBound(0, g, 255), qBound(0, b, 255));
    }
  }
  return QColor();
}

static AdaptivePaletteSeed tryLoadQt6ctOrKdePalette(const QString &cfgRoot,
                                                    QStringList *usedFiles) {
  const QString qt6ctConf =
      QDir(cfgRoot).filePath(QStringLiteral("qt6ct/qt6ct.conf"));
  QString schemePath;
  if (QFileInfo::exists(qt6ctConf)) {
    if (usedFiles) {
      usedFiles->append(qt6ctConf);
    }
    QSettings s(qt6ctConf, QSettings::IniFormat);
    s.beginGroup(QStringLiteral("Appearance"));
    const bool customPal =
        s.value(QStringLiteral("custom_palette"), false).toBool();
    const QString path = s.value(QStringLiteral("color_scheme_path")).toString();
    s.endGroup();
    if (customPal && !path.isEmpty() && QFileInfo::exists(path)) {
      schemePath = path;
    }
  }
  if (schemePath.isEmpty()) {
    const QString styleColors =
        QDir(cfgRoot).filePath(QStringLiteral("qt6ct/style-colors.conf"));
    if (QFileInfo::exists(styleColors)) {
      schemePath = styleColors;
    }
  }
  if (!schemePath.isEmpty()) {
    if (usedFiles) {
      usedFiles->append(schemePath);
    }
    QSettings cs(schemePath, QSettings::IniFormat);
    cs.beginGroup(QStringLiteral("ColorScheme"));
    const QStringList colors =
        cs.value(QStringLiteral("active_colors")).toStringList();
    cs.endGroup();
    if (colors.size() >= 14) {
      auto roleAt = [&](int idx) -> QColor {
        if (idx < 0 || idx >= colors.size()) {
          return QColor();
        }
        const QColor c = QColor(colors.at(idx).trimmed());
        return c.isValid() ? QColor(c.rgb()) : QColor();
      };
      const QColor fgWin = roleAt(0);
      const QColor fgText = roleAt(6);
      const QColor base = roleAt(9);
      const QColor win = roleAt(10);
      const QColor hi = roleAt(12);
      const QColor hiText = roleAt(13);
      const QColor alt = roleAt(16);
      const QColor acc = roleAt(21);
      if (win.isValid() && base.isValid() &&
          (fgText.isValid() || fgWin.isValid())) {
        AdaptivePaletteSeed seed;
        seed.valid = true;
        seed.bgShell = win;
        seed.bgEditor = base;
        seed.bgSurface = alt.isValid() ? alt : roleAt(1);
        seed.fgMain = fgText.isValid() ? fgText : fgWin;
        seed.accent = acc.isValid() ? acc : hi;
        seed.accentFg = hiText;
        return seed;
      }
    }
  }

  const QString kdeglobals =
      QDir(cfgRoot).filePath(QStringLiteral("kdeglobals"));
  if (QFileInfo::exists(kdeglobals)) {
    if (usedFiles) {
      usedFiles->append(kdeglobals);
    }
    QSettings ks(kdeglobals, QSettings::IniFormat);
    const QColor win = parseKdeColorValue(
        ks.value(QStringLiteral("Colors:Window/BackgroundNormal")).toString());
    const QColor winFg = parseKdeColorValue(
        ks.value(QStringLiteral("Colors:Window/ForegroundNormal")).toString());
    const QColor view = parseKdeColorValue(
        ks.value(QStringLiteral("Colors:View/BackgroundNormal")).toString());
    const QColor viewFg = parseKdeColorValue(
        ks.value(QStringLiteral("Colors:View/ForegroundNormal")).toString());
    const QColor sel = parseKdeColorValue(
        ks.value(QStringLiteral("Colors:Selection/BackgroundNormal"))
            .toString());
    const QColor selFg = parseKdeColorValue(
        ks.value(QStringLiteral("Colors:Selection/ForegroundNormal"))
            .toString());
    const QColor btn = parseKdeColorValue(
        ks.value(QStringLiteral("Colors:Button/BackgroundNormal")).toString());
    if (win.isValid() && (view.isValid() || winFg.isValid())) {
      AdaptivePaletteSeed seed;
      seed.valid = true;
      seed.bgShell = win;
      seed.bgEditor = view.isValid() ? view : win;
      seed.bgSurface = btn;
      seed.fgMain = viewFg.isValid() ? viewFg : winFg;
      seed.accent = sel;
      seed.accentFg = selFg;
      return seed;
    }
  }
  return AdaptivePaletteSeed();
}

static QColor parseJsonColor(const QJsonObject &obj, const QString &key) {
  const QString v = obj.value(key).toString().trimmed();
  if (v.isEmpty()) {
    return QColor();
  }
  const QColor c(v);
  return c.isValid() ? QColor(c.rgb()) : QColor();
}

static AdaptivePaletteSeed tryLoadWalOrMatugenJson(const QString &cfgRoot,
                                                   const QString &cacheRoot,
                                                   QStringList *usedFiles) {
  QStringList candidates = {
      QDir(cfgRoot).filePath(QStringLiteral("matugen/colors.json")),
      QDir(cacheRoot).filePath(QStringLiteral("wal/colors.json")),
      QDir(cfgRoot).filePath(QStringLiteral("wal/colors.json"))};
  if (qEnvironmentVariableIsEmpty("XDG_CACHE_HOME")) {
    candidates.append(
        QDir::home().filePath(QStringLiteral(".cache/wal/colors.json")));
  }

  for (const QString &path : candidates) {
    if (!QFileInfo::exists(path)) {
      continue;
    }
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
      continue;
    }
    if (usedFiles) {
      usedFiles->append(path);
    }
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) {
      continue;
    }
    const QJsonObject root = doc.object();
    const QJsonObject special = root.value(QStringLiteral("special")).toObject();
    const QJsonObject colors = root.value(QStringLiteral("colors")).toObject();
    const QColor bg = parseJsonColor(special, QStringLiteral("background"));
    const QColor fg = parseJsonColor(special, QStringLiteral("foreground"));
    if (bg.isValid() && fg.isValid()) {
      AdaptivePaletteSeed seed;
      seed.valid = true;
      seed.bgShell = bg;
      seed.bgEditor = blendColors(bg, fg, 0.04);
      seed.bgSurface = blendColors(bg, fg, 0.08);
      seed.fgMain = fg;
      seed.accent = parseJsonColor(colors, QStringLiteral("color12"));
      if (!seed.accent.isValid()) {
        seed.accent = parseJsonColor(colors, QStringLiteral("color4"));
      }
      if (!seed.accent.isValid()) {
        seed.accent = parseJsonColor(colors, QStringLiteral("color6"));
      }
      seed.danger = parseJsonColor(colors, QStringLiteral("color1"));
      return seed;
    }
  }
  return AdaptivePaletteSeed();
}

#endif

static QVariantMap buildFixedTheme(bool dark) {
  QVariantMap t;
  t.insert(QStringLiteral("dark"), dark);
  t.insert(QStringLiteral("adaptive"), false);
  if (dark) {
    t.insert(QStringLiteral("bgShell"), QStringLiteral("#0d0e11"));
    t.insert(QStringLiteral("bgHeader"), QStringLiteral("#0d0e11"));
    t.insert(QStringLiteral("bgEditor"), QStringLiteral("#16181e"));
    t.insert(QStringLiteral("bgGutter"), QStringLiteral("#121318"));
    t.insert(QStringLiteral("bgSurfaceElev"), QStringLiteral("#1c1f27"));
    t.insert(QStringLiteral("bgCard"), QStringLiteral("#17191e"));
    t.insert(QStringLiteral("bgGroup"), QStringLiteral("#131418"));
    t.insert(QStringLiteral("bgInput"), QStringLiteral("#1a1c22"));
    t.insert(QStringLiteral("bgHover"), QStringLiteral("#252934"));
    t.insert(QStringLiteral("bgPressed"), QStringLiteral("#282b34"));
    t.insert(QStringLiteral("bgLineHighlight"), QStringLiteral("#1f232c"));
    t.insert(QStringLiteral("bgLineHighlightSel"), QStringLiteral("#1f2738"));
    t.insert(QStringLiteral("borderSubtle"), QStringLiteral("#222630"));
    t.insert(QStringLiteral("borderGroup"), QStringLiteral("#262831"));
    t.insert(QStringLiteral("borderDivider"), QStringLiteral("#20222a"));
    t.insert(QStringLiteral("borderControl"), QStringLiteral("#313644"));
    t.insert(QStringLiteral("borderHover"), QStringLiteral("#52596d"));
    t.insert(QStringLiteral("borderFocus"), QStringLiteral("#6e768b"));
    t.insert(QStringLiteral("fgMain"), QStringLiteral("#f0f2f6"));
    t.insert(QStringLiteral("fgTitle"), QStringLiteral("#eceef2"));
    t.insert(QStringLiteral("fgLabel"), QStringLiteral("#d8dce4"));
    t.insert(QStringLiteral("fgMuted"), QStringLiteral("#9ca3b4"));
    t.insert(QStringLiteral("fgSub"), QStringLiteral("#787f8e"));
    t.insert(QStringLiteral("fgGutter"), QStringLiteral("#565d6e"));
    t.insert(QStringLiteral("accent"), QStringLiteral("#7aa2f7"));
    t.insert(QStringLiteral("accentStrong"), QStringLiteral("#8fb4ff"));
    t.insert(QStringLiteral("selBg"), QStringLiteral("#2e3d59"));
    t.insert(QStringLiteral("danger"), QStringLiteral("#e0554d"));
    t.insert(QStringLiteral("fgOnPrimary"), QStringLiteral("#111215"));
    t.insert(QStringLiteral("primaryBg"), QStringLiteral("#f0f2f6"));
    t.insert(QStringLiteral("primaryHover"), QStringLiteral("#dfe2e8"));
    t.insert(QStringLiteral("primaryPress"), QStringLiteral("#c4c8d1"));
    t.insert(QStringLiteral("btnBorderNormal"), QStringLiteral("#2f333d"));
    t.insert(QStringLiteral("btnBorderHover"), QStringLiteral("#61697c"));
    t.insert(QStringLiteral("btnBorderAccent"), QStringLiteral("#d2d7e2"));
    t.insert(QStringLiteral("btnFillHover"), QStringLiteral("#1cffffff"));
    t.insert(QStringLiteral("btnFillPress"), QStringLiteral("#2cffffff"));
    t.insert(QStringLiteral("btnFillAccent"), QStringLiteral("#14ffffff"));
    t.insert(QStringLiteral("btnFillChip"), QStringLiteral("#262a34"));
    t.insert(QStringLiteral("btnFillDanger"), QStringLiteral("#34e5534b"));
    t.insert(QStringLiteral("chipBg"), QStringLiteral("#293245"));
    t.insert(QStringLiteral("chipBorder"), QStringLiteral("#44577a"));
    t.insert(QStringLiteral("chipFg"), QStringLiteral("#f3f6fc"));
    t.insert(QStringLiteral("scrollIdle"), QStringLiteral("#373b46"));
    t.insert(QStringLiteral("scrollHover"), QStringLiteral("#5a606e"));
    t.insert(QStringLiteral("scrollPress"), QStringLiteral("#7d8594"));
    t.insert(QStringLiteral("scrim"), QStringLiteral("#9608090c"));
    t.insert(QStringLiteral("sheen"), QStringLiteral("#12ffffff"));
    t.insert(QStringLiteral("sheenBright"), QStringLiteral("#14ffffff"));
    t.insert(QStringLiteral("sheenPrimary"), QStringLiteral("#26ffffff"));
    t.insert(QStringLiteral("hoverLineBand"), QStringLiteral("#0effffff"));
  } else {
    t.insert(QStringLiteral("bgShell"), QStringLiteral("#ebedf2"));
    t.insert(QStringLiteral("bgHeader"), QStringLiteral("#ebedf2"));
    t.insert(QStringLiteral("bgEditor"), QStringLiteral("#ffffff"));
    t.insert(QStringLiteral("bgGutter"), QStringLiteral("#f5f6f9"));
    t.insert(QStringLiteral("bgSurfaceElev"), QStringLiteral("#ffffff"));
    t.insert(QStringLiteral("bgCard"), QStringLiteral("#ffffff"));
    t.insert(QStringLiteral("bgGroup"), QStringLiteral("#f7f8fa"));
    t.insert(QStringLiteral("bgInput"), QStringLiteral("#eceef3"));
    t.insert(QStringLiteral("bgHover"), QStringLiteral("#e2e5ed"));
    t.insert(QStringLiteral("bgPressed"), QStringLiteral("#d8dce5"));
    t.insert(QStringLiteral("bgLineHighlight"), QStringLiteral("#eff2f8"));
    t.insert(QStringLiteral("bgLineHighlightSel"), QStringLiteral("#eaf0fb"));
    t.insert(QStringLiteral("borderSubtle"), QStringLiteral("#dadde6"));
    t.insert(QStringLiteral("borderGroup"), QStringLiteral("#dfe2ea"));
    t.insert(QStringLiteral("borderDivider"), QStringLiteral("#e7e9f0"));
    t.insert(QStringLiteral("borderControl"), QStringLiteral("#c6ccd8"));
    t.insert(QStringLiteral("borderHover"), QStringLiteral("#8b93a5"));
    t.insert(QStringLiteral("borderFocus"), QStringLiteral("#687183"));
    t.insert(QStringLiteral("fgMain"), QStringLiteral("#14161a"));
    t.insert(QStringLiteral("fgTitle"), QStringLiteral("#14161a"));
    t.insert(QStringLiteral("fgLabel"), QStringLiteral("#1f2228"));
    t.insert(QStringLiteral("fgMuted"), QStringLiteral("#565e6d"));
    t.insert(QStringLiteral("fgSub"), QStringLiteral("#6a7180"));
    t.insert(QStringLiteral("fgGutter"), QStringLiteral("#8c93a3"));
    t.insert(QStringLiteral("accent"), QStringLiteral("#2563eb"));
    t.insert(QStringLiteral("accentStrong"), QStringLiteral("#1d4ed8"));
    t.insert(QStringLiteral("selBg"), QStringLiteral("#cde0ff"));
    t.insert(QStringLiteral("danger"), QStringLiteral("#e0554d"));
    t.insert(QStringLiteral("fgOnPrimary"), QStringLiteral("#ffffff"));
    t.insert(QStringLiteral("primaryBg"), QStringLiteral("#14161a"));
    t.insert(QStringLiteral("primaryHover"), QStringLiteral("#22252d"));
    t.insert(QStringLiteral("primaryPress"), QStringLiteral("#2e323c"));
    t.insert(QStringLiteral("btnBorderNormal"), QStringLiteral("#cdd2dc"));
    t.insert(QStringLiteral("btnBorderHover"), QStringLiteral("#70788a"));
    t.insert(QStringLiteral("btnBorderAccent"), QStringLiteral("#252932"));
    t.insert(QStringLiteral("btnFillHover"), QStringLiteral("#10000000"));
    t.insert(QStringLiteral("btnFillPress"), QStringLiteral("#1b000000"));
    t.insert(QStringLiteral("btnFillAccent"), QStringLiteral("#0b000000"));
    t.insert(QStringLiteral("btnFillChip"), QStringLiteral("#e3e6ee"));
    t.insert(QStringLiteral("btnFillDanger"), QStringLiteral("#22e04840"));
    t.insert(QStringLiteral("chipBg"), QStringLiteral("#e3ebfa"));
    t.insert(QStringLiteral("chipBorder"), QStringLiteral("#b7c9ec"));
    t.insert(QStringLiteral("chipFg"), QStringLiteral("#153672"));
    t.insert(QStringLiteral("scrollIdle"), QStringLiteral("#c1c6d0"));
    t.insert(QStringLiteral("scrollHover"), QStringLiteral("#8d94a3"));
    t.insert(QStringLiteral("scrollPress"), QStringLiteral("#6c7382"));
    t.insert(QStringLiteral("scrim"), QStringLiteral("#66101216"));
    t.insert(QStringLiteral("sheen"), QStringLiteral("#80ffffff"));
    t.insert(QStringLiteral("sheenBright"), QStringLiteral("#90ffffff"));
    t.insert(QStringLiteral("sheenPrimary"), QStringLiteral("#30ffffff"));
    t.insert(QStringLiteral("hoverLineBand"), QStringLiteral("#07000000"));
  }
  return t;
}

static QVariantMap buildAdaptiveTheme(const AdaptivePaletteSeed &seed) {
  const QColor bgEditor = seed.bgEditor;
  const bool dark = relativeLuminance(bgEditor) < 0.25;
  const QColor bgShell = seed.bgShell.isValid()
                             ? seed.bgShell
                             : blendColors(bgEditor, dark ? QColor(0, 0, 0) : QColor(235, 238, 244), 0.25);
  QColor fgMain = seed.fgMain.isValid() ? seed.fgMain : (dark ? QColor("#eef1f6") : QColor("#14161a"));
  if (contrastRatio(bgEditor, fgMain) < 4.5) {
    fgMain = dark ? QColor("#f0f2f6") : QColor("#14161a");
  }

  const QColor bgSurface =
      seed.bgSurface.isValid()
          ? seed.bgSurface
          : blendColors(bgEditor, fgMain, dark ? 0.06 : 0.04);
  const QColor bgGutter = blendColors(bgEditor, bgShell, 0.55);
  const QColor bgCard = blendColors(bgEditor, bgShell, 0.30);
  const QColor bgGroup = blendColors(bgShell, bgEditor, 0.42);
  const QColor bgInput = blendColors(bgSurface, bgShell, 0.35);
  const QColor bgHover = blendColors(bgSurface, fgMain, dark ? 0.08 : 0.06);
  const QColor bgPressed = blendColors(bgSurface, fgMain, dark ? 0.14 : 0.11);

  QColor accent = seed.accent.isValid()
                      ? seed.accent
                      : (dark ? QColor("#7aa2f7") : QColor("#2563eb"));
  if (contrastRatio(bgEditor, accent) < 2.2) {
    accent = blendColors(accent, fgMain, 0.45);
  }
  QColor accentFg = seed.accentFg;
  if (!accentFg.isValid() || contrastRatio(accent, accentFg) < 4.0) {
    accentFg = (relativeLuminance(accent) > 0.35) ? QColor("#101418") : QColor("#ffffff");
  }
  const QColor accentStrong = blendColors(accent, fgMain, 0.22);

  const QColor borderSubtle =
      seed.border.isValid()
          ? blendColors(bgShell, seed.border, 0.55)
          : blendColors(bgShell, fgMain, dark ? 0.12 : 0.13);
  const QColor borderControl =
      seed.border.isValid()
          ? blendColors(bgSurface, seed.border, 0.65)
          : blendColors(bgSurface, fgMain, dark ? 0.18 : 0.18);
  const QColor borderHover = blendColors(borderControl, fgMain, 0.28);
  const QColor borderFocus = blendColors(accent, fgMain, 0.20);

  const QColor bgLineHighlight = blendColors(bgEditor, fgMain, dark ? 0.045 : 0.04);
  const QColor bgLineHighlightSel = blendColors(bgEditor, accent, dark ? 0.14 : 0.11);
  const QColor selBg = blendColors(bgEditor, accent, dark ? 0.28 : 0.24);
  const QColor danger = seed.danger.isValid() ? seed.danger : QColor("#e0554d");

  QVariantMap t;
  t.insert(QStringLiteral("dark"), dark);
  t.insert(QStringLiteral("adaptive"), true);
  t.insert(QStringLiteral("bgShell"), colorStr(bgShell));
  t.insert(QStringLiteral("bgHeader"), colorStr(bgShell));
  t.insert(QStringLiteral("bgEditor"), colorStr(bgEditor));
  t.insert(QStringLiteral("bgGutter"), colorStr(bgGutter));
  t.insert(QStringLiteral("bgSurfaceElev"), colorStr(bgSurface));
  t.insert(QStringLiteral("bgCard"), colorStr(bgCard));
  t.insert(QStringLiteral("bgGroup"), colorStr(bgGroup));
  t.insert(QStringLiteral("bgInput"), colorStr(bgInput));
  t.insert(QStringLiteral("bgHover"), colorStr(bgHover));
  t.insert(QStringLiteral("bgPressed"), colorStr(bgPressed));
  t.insert(QStringLiteral("bgLineHighlight"), colorStr(bgLineHighlight));
  t.insert(QStringLiteral("bgLineHighlightSel"), colorStr(bgLineHighlightSel));
  t.insert(QStringLiteral("borderSubtle"), colorStr(borderSubtle));
  t.insert(QStringLiteral("borderGroup"), colorStr(borderControl));
  t.insert(QStringLiteral("borderDivider"), colorStr(borderSubtle));
  t.insert(QStringLiteral("borderControl"), colorStr(borderControl));
  t.insert(QStringLiteral("borderHover"), colorStr(borderHover));
  t.insert(QStringLiteral("borderFocus"), colorStr(borderFocus));
  t.insert(QStringLiteral("fgMain"), colorStr(fgMain));
  t.insert(QStringLiteral("fgTitle"), colorStr(fgMain));
  t.insert(QStringLiteral("fgLabel"), colorStr(blendColors(fgMain, bgEditor, 0.10)));
  t.insert(QStringLiteral("fgMuted"), colorStr(blendColors(fgMain, bgEditor, 0.36)));
  t.insert(QStringLiteral("fgSub"), colorStr(blendColors(fgMain, bgEditor, 0.48)));
  t.insert(QStringLiteral("fgGutter"), colorStr(blendColors(fgMain, bgEditor, 0.60)));
  t.insert(QStringLiteral("accent"), colorStr(accent));
  t.insert(QStringLiteral("accentStrong"), colorStr(accentStrong));
  t.insert(QStringLiteral("selBg"), colorStr(selBg));
  t.insert(QStringLiteral("danger"), colorStr(danger));
  t.insert(QStringLiteral("fgOnPrimary"), colorStr(accentFg));
  t.insert(QStringLiteral("primaryBg"), colorStr(accent));
  t.insert(QStringLiteral("primaryHover"), colorStr(blendColors(accent, accentFg, 0.10)));
  t.insert(QStringLiteral("primaryPress"), colorStr(blendColors(accent, accentFg, 0.20)));
  t.insert(QStringLiteral("btnBorderNormal"), colorStr(borderControl));
  t.insert(QStringLiteral("btnBorderHover"), colorStr(borderHover));
  t.insert(QStringLiteral("btnBorderAccent"), colorStr(blendColors(accent, fgMain, 0.25)));
  t.insert(QStringLiteral("btnFillHover"), colorStr(withAlpha(fgMain, dark ? 28 : 16)));
  t.insert(QStringLiteral("btnFillPress"), colorStr(withAlpha(fgMain, dark ? 44 : 28)));
  t.insert(QStringLiteral("btnFillAccent"), colorStr(withAlpha(accent, dark ? 34 : 24)));
  t.insert(QStringLiteral("btnFillChip"), colorStr(blendColors(bgSurface, accent, dark ? 0.18 : 0.16)));
  t.insert(QStringLiteral("btnFillDanger"), colorStr(withAlpha(danger, dark ? 52 : 34)));
  t.insert(QStringLiteral("chipBg"), colorStr(blendColors(bgSurface, accent, dark ? 0.20 : 0.18)));
  t.insert(QStringLiteral("chipBorder"), colorStr(blendColors(borderControl, accent, 0.45)));
  t.insert(QStringLiteral("chipFg"), colorStr(fgMain));
  t.insert(QStringLiteral("scrollIdle"), colorStr(blendColors(bgEditor, fgMain, dark ? 0.20 : 0.22)));
  t.insert(QStringLiteral("scrollHover"), colorStr(blendColors(bgEditor, fgMain, dark ? 0.34 : 0.38)));
  t.insert(QStringLiteral("scrollPress"), colorStr(blendColors(bgEditor, fgMain, dark ? 0.48 : 0.52)));
  t.insert(QStringLiteral("scrim"), colorStr(withAlpha(bgShell.darker(220), dark ? 155 : 105)));
  t.insert(QStringLiteral("sheen"), colorStr(withAlpha(QColor(255, 255, 255), dark ? 18 : 128)));
  t.insert(QStringLiteral("sheenBright"), colorStr(withAlpha(QColor(255, 255, 255), dark ? 22 : 144)));
  t.insert(QStringLiteral("sheenPrimary"), colorStr(withAlpha(QColor(255, 255, 255), dark ? 38 : 48)));
  t.insert(QStringLiteral("hoverLineBand"), colorStr(withAlpha(fgMain, dark ? 14 : 10)));
  return t;
}

TabModel::TabModel(QObject *parent) : QAbstractListModel(parent) {}

int TabModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return m_tabs.size();
}

QVariant TabModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= m_tabs.size()) {
    return QVariant();
  }
  if (role == SessionRole) {
    return QVariant::fromValue<QObject *>(m_tabs.at(index.row()));
  }
  return QVariant();
}

QHash<int, QByteArray> TabModel::roleNames() const {
  return {{SessionRole, QByteArrayLiteral("session")}};
}

TabSession *TabModel::at(int index) const {
  if (index < 0 || index >= m_tabs.size()) {
    return nullptr;
  }
  return m_tabs.at(index);
}

int TabModel::indexOf(TabSession *tab) const { return m_tabs.indexOf(tab); }

void TabModel::append(TabSession *tab) {
  const int row = m_tabs.size();
  beginInsertRows(QModelIndex(), row, row);
  m_tabs.append(tab);
  endInsertRows();
}

TabSession *TabModel::takeAt(int index) {
  if (index < 0 || index >= m_tabs.size()) {
    return nullptr;
  }
  beginRemoveRows(QModelIndex(), index, index);
  TabSession *removed = m_tabs.takeAt(index);
  endRemoveRows();
  return removed;
}

EditorBackend::EditorBackend(QObject *parent)
    : QObject(parent), m_tabModel(this),
      m_showWindowControls(detectDesktopEnvironment()),
      m_monoFontFamily(pickMonoFontFamily()),
      m_themeWatcher(new QFileSystemWatcher(this)),
      m_themeReloadTimer(new QTimer(this)) {
  m_themeReloadTimer->setSingleShot(true);
  m_themeReloadTimer->setInterval(80);
  connect(m_themeReloadTimer, &QTimer::timeout, this,
          &EditorBackend::onThemeFilesChanged);
  connect(m_themeWatcher, &QFileSystemWatcher::fileChanged, m_themeReloadTimer,
          QOverload<>::of(&QTimer::start));
  connect(m_themeWatcher, &QFileSystemWatcher::directoryChanged,
          m_themeReloadTimer, QOverload<>::of(&QTimer::start));

  initTranslations();
  loadSettings();
  applyUiLanguageStrings();
  rebuildThemePalette();
  updateThemeWatches();
  m_tabModel.append(createSession());
  m_activeTabIndex = 0;

  if (QGuiApplication::styleHints()) {
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
            this, &EditorBackend::onSystemColorSchemeChanged);
  }
  if (QCoreApplication::instance()) {
    QCoreApplication::instance()->installEventFilter(this);
  }
}

void EditorBackend::initTranslations() {
  m_translations.clear();
  m_uiLanguagesModel.clear();

  QDir dir(QStringLiteral(":/i18n"));
  const QStringList files =
      dir.entryList({QStringLiteral("*.json")}, QDir::Files, QDir::Name);
  QVector<QPair<QString, QString>> metaList;

  for (const QString &fileName : files) {
    QFile f(dir.filePath(fileName));
    if (!f.open(QIODevice::ReadOnly)) {
      continue;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) {
      continue;
    }
    const QJsonObject root = doc.object();
    const QJsonObject meta = root.value(QStringLiteral("_meta")).toObject();
    const QString baseId = QFileInfo(fileName).completeBaseName();
    const QString id = meta.value(QStringLiteral("id")).toString(baseId);
    const QString name = meta.value(QStringLiteral("name")).toString(id);

    QVariantMap map;
    for (auto it = root.constBegin(); it != root.constEnd(); ++it) {
      if (it.key() != QStringLiteral("_meta") && it.value().isString()) {
        map.insert(it.key(), it.value().toString());
      }
    }
    m_translations.insert(id, map);
    metaList.append({id, name});
  }

  std::sort(metaList.begin(), metaList.end(), [](const auto &a, const auto &b) {
    if (a.first == QStringLiteral("en"))
      return true;
    if (b.first == QStringLiteral("en"))
      return false;
    return a.first < b.first;
  });

  for (const auto &entry : metaList) {
    QVariantMap item;
    item.insert(QStringLiteral("id"), entry.first);
    item.insert(QStringLiteral("name"), entry.second);
    m_uiLanguagesModel.append(item);
  }
}

void EditorBackend::applyUiLanguageStrings() {
  QVariantMap merged = m_translations.value(QStringLiteral("en"));
  const QVariantMap active = m_translations.value(m_uiLanguage);
  for (auto it = active.constBegin(); it != active.constEnd(); ++it) {
    merged.insert(it.key(), it.value());
  }
  m_uiStrings = merged;
}

void EditorBackend::rebuildThemePalette() {
  if (m_themeMode == ThemeDark) {
    m_theme = buildFixedTheme(true);
    m_effectiveDark = true;
    return;
  }
  if (m_themeMode == ThemeLight) {
    m_theme = buildFixedTheme(false);
    m_effectiveDark = false;
    return;
  }

#if defined(Q_OS_WIN)
  const AdaptivePaletteSeed seed = tryLoadWindowsPalette();
  if (seed.valid) {
    m_theme = buildAdaptiveTheme(seed);
    m_effectiveDark = m_theme.value(QStringLiteral("dark"), true).toBool();
    return;
  }
#else
  const QString cfgRoot =
      QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
  const QString cacheRoot =
      QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);

  QStringList watched;
  AdaptivePaletteSeed seed = tryLoadGtkCssPalette(cfgRoot, &watched);
  if (!seed.valid) {
    seed = tryLoadQt6ctOrKdePalette(cfgRoot, &watched);
  }
  if (!seed.valid) {
    seed = tryLoadWalOrMatugenJson(cfgRoot, cacheRoot, &watched);
  }

  m_watchedThemeFiles = watched;

  if (seed.valid) {
    m_theme = buildAdaptiveTheme(seed);
    m_effectiveDark = m_theme.value(QStringLiteral("dark"), true).toBool();
    return;
  }
#endif

  const bool sysDark = QGuiApplication::styleHints()
                           ? (QGuiApplication::styleHints()->colorScheme() !=
                              Qt::ColorScheme::Light)
                           : true;
  m_theme = buildFixedTheme(sysDark);
  m_effectiveDark = sysDark;
}

void EditorBackend::updateThemeWatches() {
#if defined(Q_OS_WIN)
  return;
#else
  if (!m_themeWatcher) {
    return;
  }
  const QStringList oldFiles = m_themeWatcher->files();
  if (!oldFiles.isEmpty()) {
    m_themeWatcher->removePaths(oldFiles);
  }
  const QStringList oldDirs = m_themeWatcher->directories();
  if (!oldDirs.isEmpty()) {
    m_themeWatcher->removePaths(oldDirs);
  }

  if (m_themeMode != ThemeSystem) {
    return;
  }

  const QString cfgRoot =
      QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
  const QString cacheRoot =
      QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);

  QSet<QString> filesToWatch;
  for (const QString &f : m_watchedThemeFiles) {
    if (QFileInfo::exists(f)) {
      filesToWatch.insert(QFileInfo(f).absoluteFilePath());
    }
  }
  const QStringList extraFiles = {
      QDir(cfgRoot).filePath(QStringLiteral("matugen/colors.css")),
      QDir(cfgRoot).filePath(QStringLiteral("matugen/colors.json")),
      QDir(cfgRoot).filePath(QStringLiteral("gtk-4.0/colors.css")),
      QDir(cfgRoot).filePath(QStringLiteral("gtk-4.0/gtk.css")),
      QDir(cfgRoot).filePath(QStringLiteral("gtk-3.0/colors.css")),
      QDir(cfgRoot).filePath(QStringLiteral("gtk-3.0/gtk.css")),
      QDir(cfgRoot).filePath(QStringLiteral("qt6ct/qt6ct.conf")),
      QDir(cfgRoot).filePath(QStringLiteral("qt6ct/style-colors.conf")),
      QDir(cfgRoot).filePath(QStringLiteral("kdeglobals")),
      QDir(cacheRoot).filePath(QStringLiteral("wal/colors.json"))};
  for (const QString &f : extraFiles) {
    if (QFileInfo::exists(f)) {
      filesToWatch.insert(QFileInfo(f).absoluteFilePath());
    }
  }
  if (!filesToWatch.isEmpty()) {
    m_themeWatcher->addPaths(filesToWatch.values());
  }

  const QStringList dirsToWatch = {
      QDir(cfgRoot).filePath(QStringLiteral("matugen")),
      QDir(cfgRoot).filePath(QStringLiteral("gtk-4.0")),
      QDir(cfgRoot).filePath(QStringLiteral("gtk-3.0")),
      QDir(cfgRoot).filePath(QStringLiteral("qt6ct")),
      QDir(cacheRoot).filePath(QStringLiteral("wal"))};
  for (const QString &d : dirsToWatch) {
    if (QFileInfo::exists(d)) {
      m_themeWatcher->addPath(d);
    }
  }
#endif
}

TabSession *EditorBackend::createSession() {
  auto *tab = new TabSession(this);
  tab->applyThemeAndSyntax(m_syntaxEnabled, m_effectiveDark);
  connect(tab, &TabSession::errorOccurred, this, &EditorBackend::errorOccurred);
  connect(tab, &TabSession::languageChanged, this,
          &EditorBackend::languageChanged);
  return tab;
}

TabSession *EditorBackend::activeTab() const {
  return m_tabModel.at(m_activeTabIndex);
}

TabSession *EditorBackend::tabAtOrActive(int index) const {
  return (index >= 0 && index < m_tabModel.count()) ? m_tabModel.at(index)
                                                    : activeTab();
}

QString EditorBackend::languageId() const {
  TabSession *tab = activeTab();
  return tab ? tab->languageId() : QStringLiteral("auto");
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

void EditorBackend::setActiveTabIndex(int index) {
  if (m_tabModel.count() == 0) {
    return;
  }
  const int clamped = qBound(0, index, m_tabModel.count() - 1);
  if (m_activeTabIndex == clamped) {
    return;
  }
  m_activeTabIndex = clamped;
  emit activeTabChanged();
  emit languageChanged();
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
  if (mode < ThemeSystem || mode > ThemeDark || m_themeMode == mode) {
    return;
  }
  m_themeMode = mode;
  saveSettings();
  rebuildThemePalette();
  updateThemeWatches();
  propagateThemeAndSyntax();
  emit themeChanged();
  emit settingsChanged();
}

void EditorBackend::setSyntaxEnabled(bool enabled) {
  if (m_syntaxEnabled == enabled) {
    return;
  }
  m_syntaxEnabled = enabled;
  saveSettings();
  propagateThemeAndSyntax();
  emit settingsChanged();
}

void EditorBackend::setLanguageId(const QString &id) {
  if (TabSession *tab = activeTab()) {
    tab->setLanguageId(id);
  }
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

void EditorBackend::setUiLanguage(const QString &lang) {
  const QString target =
      m_translations.contains(lang) ? lang : QStringLiteral("en");
  if (m_uiLanguage == target) {
    return;
  }
  m_uiLanguage = target;
  applyUiLanguageStrings();
  saveSettings();
  emit uiLanguageChanged();
}

void EditorBackend::zoomIn() { setZoomPercent(m_zoomPercent + 10); }

void EditorBackend::zoomOut() { setZoomPercent(m_zoomPercent - 10); }

void EditorBackend::zoomReset() { setZoomPercent(100); }

int EditorBackend::newTab(const QString &initialFilePath) {
  TabSession *tab = createSession();
  if (!initialFilePath.isEmpty()) {
    tab->setPendingInitialFile(initialFilePath, true);
  }
  m_tabModel.append(tab);
  const int newIdx = m_tabModel.count() - 1;
  m_activeTabIndex = newIdx;
  emit tabCountChanged();
  emit activeTabChanged();
  emit languageChanged();
  return newIdx;
}

void EditorBackend::activateTab(int index) { setActiveTabIndex(index); }

void EditorBackend::nextTab() {
  if (m_tabModel.count() <= 1) {
    return;
  }
  setActiveTabIndex((m_activeTabIndex + 1) % m_tabModel.count());
}

void EditorBackend::prevTab() {
  if (m_tabModel.count() <= 1) {
    return;
  }
  setActiveTabIndex((m_activeTabIndex - 1 + m_tabModel.count()) %
                    m_tabModel.count());
}

void EditorBackend::closeTabImmediate(int index) {
  if (index < 0 || index >= m_tabModel.count()) {
    return;
  }
  if (m_tabModel.count() == 1) {
    m_tabModel.at(0)->resetEmpty();
    return;
  }
  TabSession *removed = m_tabModel.takeAt(index);
  if (removed) {
    removed->deleteLater();
  }
  if (m_activeTabIndex >= m_tabModel.count()) {
    m_activeTabIndex = m_tabModel.count() - 1;
  } else if (index < m_activeTabIndex) {
    m_activeTabIndex--;
  }
  emit tabCountChanged();
  emit activeTabChanged();
  emit languageChanged();
}

bool EditorBackend::isTabModified(int index) const {
  TabSession *tab = m_tabModel.at(index);
  return tab ? tab->modified() : false;
}

bool EditorBackend::isActivePristineUntitled() const {
  TabSession *tab = activeTab();
  return tab ? tab->isPristineUntitled() : false;
}

QString EditorBackend::tabFileName(int index) const {
  TabSession *tab = m_tabModel.at(index);
  return tab ? tab->fileName() : QStringLiteral("Untitled");
}

int EditorBackend::firstModifiedTabIndex() const {
  const auto &tabs = m_tabModel.tabs();
  for (int i = 0; i < tabs.size(); ++i) {
    if (tabs.at(i)->modified()) {
      return i;
    }
  }
  return -1;
}

int EditorBackend::findOpenTabByPath(const QString &absPath) const {
  const auto &tabs = m_tabModel.tabs();
  for (int i = 0; i < tabs.size(); ++i) {
    if (!tabs.at(i)->filePath().isEmpty() &&
        tabs.at(i)->filePath() == absPath) {
      return i;
    }
  }
  return -1;
}

bool EditorBackend::loadFile(const QString &path, bool allowMissing) {
  const QString absPath = normalizeFilePath(path);
  const int existing = findOpenTabByPath(absPath);
  if (existing >= 0) {
    setActiveTabIndex(existing);
    return true;
  }
  TabSession *cur = activeTab();
  if (cur && cur->isPristineUntitled()) {
    return cur->loadFile(absPath, allowMissing);
  }
  const int idx = newTab();
  TabSession *created = m_tabModel.at(idx);
  if (!created || !created->loadFile(absPath, allowMissing)) {
    closeTabImmediate(idx);
    return false;
  }
  return true;
}

bool EditorBackend::openFileDialog() {
  TabSession *cur = activeTab();
  const QString initialDir = (cur && !cur->filePath().isEmpty())
                                 ? QFileInfo(cur->filePath()).absolutePath()
                                 : QDir::currentPath();
  const QString chosen = QFileDialog::getOpenFileName(
      nullptr, QStringLiteral("Open File"), initialDir);
  if (chosen.isEmpty()) {
    return false;
  }
  return loadFile(chosen, false);
}

bool EditorBackend::saveFile(int index) {
  TabSession *tab = tabAtOrActive(index);
  if (!tab) {
    return false;
  }
  if (tab->filePath().isEmpty()) {
    return saveFileAsDialog(index);
  }
  return tab->writeToFile(tab->filePath());
}

bool EditorBackend::saveFileAsDialog(int index) {
  TabSession *tab = tabAtOrActive(index);
  if (!tab) {
    return false;
  }
  const QString initial =
      tab->filePath().isEmpty()
          ? QDir(QDir::currentPath()).filePath(QStringLiteral("Untitled"))
          : tab->filePath();
  const QString chosen = QFileDialog::getSaveFileName(
      nullptr, QStringLiteral("Save File"), initial);
  if (chosen.isEmpty()) {
    return false;
  }
  return tab->writeToFile(chosen);
}

void EditorBackend::discardTabChanges(int index) {
  if (TabSession *tab = tabAtOrActive(index)) {
    tab->discardChanges();
  }
}

void EditorBackend::setInitialFiles(const QStringList &paths) {
  if (paths.isEmpty()) {
    return;
  }
  for (int i = 0; i < paths.size(); ++i) {
    if (i == 0 && m_tabModel.count() == 1 &&
        m_tabModel.at(0)->isPristineUntitled()) {
      m_tabModel.at(0)->setPendingInitialFile(paths.at(i), true);
    } else {
      newTab(paths.at(i));
    }
  }
  setActiveTabIndex(0);
}

bool EditorBackend::eventFilter(QObject *watched, QEvent *event) {
  if (watched == QCoreApplication::instance() &&
      (event->type() == QEvent::ApplicationPaletteChange ||
       event->type() == QEvent::ThemeChange)) {
    onSystemColorSchemeChanged();
  }
  return QObject::eventFilter(watched, event);
}

void EditorBackend::onSystemColorSchemeChanged() {
  if (m_themeMode == ThemeSystem) {
    const QVariantMap oldTheme = m_theme;
    const bool oldDark = m_effectiveDark;
    rebuildThemePalette();
    if (m_theme != oldTheme || m_effectiveDark != oldDark) {
      propagateThemeAndSyntax();
      emit themeChanged();
    }
  }
}

void EditorBackend::onThemeFilesChanged() {
  if (m_themeMode == ThemeSystem) {
    const QVariantMap oldTheme = m_theme;
    const bool oldDark = m_effectiveDark;
    rebuildThemePalette();
    updateThemeWatches();
    if (m_theme != oldTheme || m_effectiveDark != oldDark) {
      propagateThemeAndSyntax();
      emit themeChanged();
    }
  }
}

void EditorBackend::propagateThemeAndSyntax() {
  const auto &tabs = m_tabModel.tabs();
  for (TabSession *tab : tabs) {
    tab->applyThemeAndSyntax(m_syntaxEnabled, m_effectiveDark);
  }
}

void EditorBackend::loadSettings() {
  const QString cfgDir =
      QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
  const QString iniPath =
      QDir(cfgDir).filePath(QStringLiteral("nulla/settings.ini"));
  QSettings s(iniPath, QSettings::IniFormat);
  s.beginGroup(QStringLiteral("editor"));
  m_showLineNumbers = s.value(QStringLiteral("line_numbers"), true).toBool();
  const int mode = s.value(QStringLiteral("theme"), ThemeDark).toInt();
  m_themeMode = (mode >= ThemeSystem && mode <= ThemeDark) ? mode : ThemeDark;
  m_syntaxEnabled = s.value(QStringLiteral("syntax"), true).toBool();
  m_zoomPercent = qBound(50, s.value(QStringLiteral("zoom"), 100).toInt(), 300);
  const QString lang =
      s.value(QStringLiteral("ui_language"), QStringLiteral("en")).toString();
  m_uiLanguage = m_translations.contains(lang) ? lang : QStringLiteral("en");
  s.endGroup();
}

void EditorBackend::saveSettings() const {
  const QString cfgDir =
      QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
  const QString iniPath =
      QDir(cfgDir).filePath(QStringLiteral("nulla/settings.ini"));
  QDir().mkpath(QFileInfo(iniPath).absolutePath());
  QSettings s(iniPath, QSettings::IniFormat);
  s.beginGroup(QStringLiteral("editor"));
  s.setValue(QStringLiteral("line_numbers"), m_showLineNumbers);
  s.setValue(QStringLiteral("theme"), m_themeMode);
  s.setValue(QStringLiteral("syntax"), m_syntaxEnabled);
  s.setValue(QStringLiteral("zoom"), m_zoomPercent);
  s.setValue(QStringLiteral("ui_language"), m_uiLanguage);
  s.endGroup();
  s.sync();
}
