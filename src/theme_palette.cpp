#include "theme_palette.h"

#include <QColor>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QGuiApplication>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPalette>
#include <QRegularExpression>
#include <QSet>
#include <QSettings>
#include <QStandardPaths>
#include <QStyleHints>
#include <QtEnvironmentVariables>

#include <algorithm>
#include <cmath>

namespace {

double linearChannel(double c) {
  return (c <= 0.03928) ? (c / 12.92) : std::pow((c + 0.055) / 1.055, 2.4);
}

double relativeLuminance(const QColor &c) {
  return 0.2126 * linearChannel(c.redF()) + 0.7152 * linearChannel(c.greenF()) +
         0.0722 * linearChannel(c.blueF());
}

double contrastRatio(const QColor &a, const QColor &b) {
  const double l1 = relativeLuminance(a);
  const double l2 = relativeLuminance(b);
  return (std::max(l1, l2) + 0.05) / (std::min(l1, l2) + 0.05);
}

QColor blendColors(const QColor &a, const QColor &b, double t) {
  const double k = qBound(0.0, t, 1.0);
  QColor out;
  out.setRgbF(a.redF() * (1.0 - k) + b.redF() * k,
              a.greenF() * (1.0 - k) + b.greenF() * k,
              a.blueF() * (1.0 - k) + b.blueF() * k,
              a.alphaF() * (1.0 - k) + b.alphaF() * k);
  return out;
}

QColor withAlpha(QColor c, int alpha) {
  c.setAlpha(qBound(0, alpha, 255));
  return c;
}

QString colorStr(const QColor &c) {
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
AdaptivePaletteSeed tryLoadWindowsPalette() {
  bool sysDark = true;
  if (QGuiApplication::styleHints() &&
      QGuiApplication::styleHints()->colorScheme() !=
          Qt::ColorScheme::Unknown) {
    sysDark = (QGuiApplication::styleHints()->colorScheme() !=
               Qt::ColorScheme::Light);
  } else {
    const QSettings pers(QStringLiteral("HKEY_CURRENT_"
                                        "USER\\Software\\Microsoft\\Windows\\Cu"
                                        "rrentVersion\\Themes\\Personalize"),
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
    const quint32 argb =
        dwm.value(QStringLiteral("ColorizationColor")).toUInt();
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

  const QColor baseShell(sysDark ? QStringLiteral("#0d0e11")
                                 : QStringLiteral("#ebedf2"));
  const QColor baseEditor(sysDark ? QStringLiteral("#16181e")
                                  : QStringLiteral("#ffffff"));
  const QColor baseSurface(sysDark ? QStringLiteral("#1c1f27")
                                   : QStringLiteral("#f7f8fa"));

  AdaptivePaletteSeed seed;
  seed.valid = true;
  seed.bgShell = blendColors(baseShell, accent, sysDark ? 0.06 : 0.045);
  seed.bgEditor = blendColors(baseEditor, accent, sysDark ? 0.03 : 0.02);
  seed.bgSurface = blendColors(baseSurface, accent, sysDark ? 0.07 : 0.05);
  seed.fgMain =
      QColor(sysDark ? QStringLiteral("#f0f2f6") : QStringLiteral("#14161a"));
  seed.accent = accent;
  return seed;
}
#else
QColor parseCssColorToken(const QString &raw,
                          const QHash<QString, QString> &vars, int depth = 0) {
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
  static const QRegularExpression rgbRe(
      QStringLiteral("^rgba?\\s*\\(\\s*(\\d+)\\s*,\\s*(\\d+)\\s*,\\s*(\\d+)(?:"
                     "\\s*,\\s*([0-9.]+%?))?\\s*\\)$"));
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

void collectGtkCssDefs(const QString &filePath, QHash<QString, QString> &vars,
                       QStringList &visitedFiles, int depth = 0) {
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

  static const QRegularExpression importRe(
      QStringLiteral("@import\\s+(?:url\\(\\s*[\"']?([^\"')]+)[\"']?\\s*\\)|["
                     "\"']([^\"']+)[\"'])\\s*;"));
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

  static const QRegularExpression defRe(
      QStringLiteral("@define-color\\s+([A-Za-z0-9_-]+)\\s+([^;]+);"));
  QRegularExpressionMatchIterator dit = defRe.globalMatch(content);
  while (dit.hasNext()) {
    const QRegularExpressionMatch dm = dit.next();
    vars.insert(dm.captured(1).trimmed(), dm.captured(2).trimmed());
  }
}

QColor pickFirstCssColor(const QHash<QString, QString> &vars,
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

AdaptivePaletteSeed tryLoadGtkCssPalette(const QString &cfgRoot,
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
        vars,
        {QStringLiteral("window_bg_color"), QStringLiteral("theme_bg_color"),
         QStringLiteral("bg_color"), QStringLiteral("view_bg_color")});
    const QColor editor = pickFirstCssColor(
        vars,
        {QStringLiteral("view_bg_color"), QStringLiteral("theme_base_color"),
         QStringLiteral("base_color"), QStringLiteral("window_bg_color")});
    const QColor fg = pickFirstCssColor(
        vars,
        {QStringLiteral("view_fg_color"), QStringLiteral("window_fg_color"),
         QStringLiteral("theme_fg_color"), QStringLiteral("fg_color")});

    if (shell.isValid() && editor.isValid() && fg.isValid()) {
      AdaptivePaletteSeed seed;
      seed.valid = true;
      seed.bgShell = QColor(shell.rgb());
      seed.bgEditor = QColor(editor.rgb());
      seed.bgSurface =
          pickFirstCssColor(vars, {QStringLiteral("headerbar_bg_color"),
                                   QStringLiteral("popover_bg_color"),
                                   QStringLiteral("card_bg_color"),
                                   QStringLiteral("sidebar_bg_color")});
      seed.fgMain = QColor(fg.rgb());
      seed.accent =
          pickFirstCssColor(vars, {QStringLiteral("accent_bg_color"),
                                   QStringLiteral("accent_color"),
                                   QStringLiteral("theme_selected_bg_color")});
      seed.accentFg =
          pickFirstCssColor(vars, {QStringLiteral("accent_fg_color"),
                                   QStringLiteral("theme_selected_fg_color")});
      seed.border =
          pickFirstCssColor(vars, {QStringLiteral("headerbar_border_color"),
                                   QStringLiteral("borders")});
      seed.danger =
          pickFirstCssColor(vars, {QStringLiteral("destructive_bg_color"),
                                   QStringLiteral("destructive_color"),
                                   QStringLiteral("error_color")});
      return seed;
    }
  }
  return AdaptivePaletteSeed();
}

QColor parseKdeColorValue(const QString &raw) {
  const QString trimmed = raw.trimmed();
  if (trimmed.startsWith(QLatin1Char('#'))) {
    return QColor(trimmed);
  }
  const QStringList parts = trimmed.split(QLatin1Char(','));
  if (parts.size() >= 3) {
    bool okR = false;
    bool okG = false;
    bool okB = false;
    const int r = parts.at(0).trimmed().toInt(&okR);
    const int g = parts.at(1).trimmed().toInt(&okG);
    const int b = parts.at(2).trimmed().toInt(&okB);
    if (okR && okG && okB) {
      return QColor(qBound(0, r, 255), qBound(0, g, 255), qBound(0, b, 255));
    }
  }
  return QColor();
}

AdaptivePaletteSeed tryLoadQt6ctOrKdePalette(const QString &cfgRoot,
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
    const QString path =
        s.value(QStringLiteral("color_scheme_path")).toString();
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
    auto readKdeColor = [&](const QString &key) -> QColor {
      const QVariant val = ks.value(key);
      if (val.typeId() == QMetaType::QStringList) {
        return parseKdeColorValue(val.toStringList().join(QLatin1Char(',')));
      }
      return parseKdeColorValue(val.toString());
    };
    const QColor win =
        readKdeColor(QStringLiteral("Colors:Window/BackgroundNormal"));
    const QColor winFg =
        readKdeColor(QStringLiteral("Colors:Window/ForegroundNormal"));
    const QColor view =
        readKdeColor(QStringLiteral("Colors:View/BackgroundNormal"));
    const QColor viewFg =
        readKdeColor(QStringLiteral("Colors:View/ForegroundNormal"));
    const QColor sel =
        readKdeColor(QStringLiteral("Colors:Selection/BackgroundNormal"));
    const QColor selFg =
        readKdeColor(QStringLiteral("Colors:Selection/ForegroundNormal"));
    const QColor btn =
        readKdeColor(QStringLiteral("Colors:Button/BackgroundNormal"));
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

QColor parseJsonColor(const QJsonObject &obj, const QString &key) {
  const QString v = obj.value(key).toString().trimmed();
  if (v.isEmpty()) {
    return QColor();
  }
  const QColor c(v);
  return c.isValid() ? QColor(c.rgb()) : QColor();
}

AdaptivePaletteSeed tryLoadWalOrMatugenJson(const QString &cfgRoot,
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
    const QJsonObject special =
        root.value(QStringLiteral("special")).toObject();
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

QVariantMap buildFixedTheme(bool dark) {
  QVariantMap t;
  t.insert(QStringLiteral("dark"), dark);
  t.insert(QStringLiteral("adaptive"), false);
  if (dark) {
    t.insert(QStringLiteral("bgShell"), QStringLiteral("#0d0e11"));
    t.insert(QStringLiteral("bgHeader"), QStringLiteral("#0d0e11"));
    t.insert(QStringLiteral("bgEditor"), QStringLiteral("#16181e"));
    t.insert(QStringLiteral("bgGutter"), QStringLiteral("#121318"));
    t.insert(QStringLiteral("bgDrawer"), QStringLiteral("#101217"));
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
    t.insert(QStringLiteral("indentGuide"), QStringLiteral("#262b36"));
    t.insert(QStringLiteral("indentGuideActive"), QStringLiteral("#4e586e"));
  } else {
    t.insert(QStringLiteral("bgShell"), QStringLiteral("#ebedf2"));
    t.insert(QStringLiteral("bgHeader"), QStringLiteral("#ebedf2"));
    t.insert(QStringLiteral("bgEditor"), QStringLiteral("#ffffff"));
    t.insert(QStringLiteral("bgGutter"), QStringLiteral("#f5f6f9"));
    t.insert(QStringLiteral("bgDrawer"), QStringLiteral("#f2f4f8"));
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
    t.insert(QStringLiteral("indentGuide"), QStringLiteral("#e5e8ef"));
    t.insert(QStringLiteral("indentGuideActive"), QStringLiteral("#aeb6c7"));
  }
  return t;
}

QVariantMap buildGraphiteTheme() {
  QVariantMap t;
  t.insert(QStringLiteral("dark"), true);
  t.insert(QStringLiteral("adaptive"), false);
  t.insert(QStringLiteral("bgShell"), QStringLiteral("#101012"));
  t.insert(QStringLiteral("bgHeader"), QStringLiteral("#101012"));
  t.insert(QStringLiteral("bgEditor"), QStringLiteral("#18181b"));
  t.insert(QStringLiteral("bgGutter"), QStringLiteral("#151518"));
  t.insert(QStringLiteral("bgDrawer"), QStringLiteral("#131316"));
  t.insert(QStringLiteral("bgSurfaceElev"), QStringLiteral("#202025"));
  t.insert(QStringLiteral("bgCard"), QStringLiteral("#18181c"));
  t.insert(QStringLiteral("bgGroup"), QStringLiteral("#141417"));
  t.insert(QStringLiteral("bgInput"), QStringLiteral("#1d1d22"));
  t.insert(QStringLiteral("bgHover"), QStringLiteral("#27272d"));
  t.insert(QStringLiteral("bgPressed"), QStringLiteral("#2e2e36"));
  t.insert(QStringLiteral("bgLineHighlight"), QStringLiteral("#212126"));
  t.insert(QStringLiteral("bgLineHighlightSel"), QStringLiteral("#262933"));
  t.insert(QStringLiteral("borderSubtle"), QStringLiteral("#242429"));
  t.insert(QStringLiteral("borderGroup"), QStringLiteral("#2a2a30"));
  t.insert(QStringLiteral("borderDivider"), QStringLiteral("#222227"));
  t.insert(QStringLiteral("borderControl"), QStringLiteral("#34343c"));
  t.insert(QStringLiteral("borderHover"), QStringLiteral("#565663"));
  t.insert(QStringLiteral("borderFocus"), QStringLiteral("#727280"));
  t.insert(QStringLiteral("fgMain"), QStringLiteral("#f2f2f5"));
  t.insert(QStringLiteral("fgTitle"), QStringLiteral("#eeeef2"));
  t.insert(QStringLiteral("fgLabel"), QStringLiteral("#d8d8df"));
  t.insert(QStringLiteral("fgMuted"), QStringLiteral("#9d9da9"));
  t.insert(QStringLiteral("fgSub"), QStringLiteral("#787885"));
  t.insert(QStringLiteral("fgGutter"), QStringLiteral("#5c5c68"));
  t.insert(QStringLiteral("accent"), QStringLiteral("#d4d4dc"));
  t.insert(QStringLiteral("accentStrong"), QStringLiteral("#f2f2f5"));
  t.insert(QStringLiteral("selBg"), QStringLiteral("#343640"));
  t.insert(QStringLiteral("danger"), QStringLiteral("#e0554d"));
  t.insert(QStringLiteral("fgOnPrimary"), QStringLiteral("#111113"));
  t.insert(QStringLiteral("primaryBg"), QStringLiteral("#f2f2f5"));
  t.insert(QStringLiteral("primaryHover"), QStringLiteral("#e2e2e8"));
  t.insert(QStringLiteral("primaryPress"), QStringLiteral("#c8c8d0"));
  t.insert(QStringLiteral("btnBorderNormal"), QStringLiteral("#313139"));
  t.insert(QStringLiteral("btnBorderHover"), QStringLiteral("#626270"));
  t.insert(QStringLiteral("btnBorderAccent"), QStringLiteral("#d4d4dc"));
  t.insert(QStringLiteral("btnFillHover"), QStringLiteral("#1cffffff"));
  t.insert(QStringLiteral("btnFillPress"), QStringLiteral("#2cffffff"));
  t.insert(QStringLiteral("btnFillAccent"), QStringLiteral("#14ffffff"));
  t.insert(QStringLiteral("btnFillChip"), QStringLiteral("#27272e"));
  t.insert(QStringLiteral("btnFillDanger"), QStringLiteral("#34e5534b"));
  t.insert(QStringLiteral("chipBg"), QStringLiteral("#292930"));
  t.insert(QStringLiteral("chipBorder"), QStringLiteral("#4a4a56"));
  t.insert(QStringLiteral("chipFg"), QStringLiteral("#f2f2f5"));
  t.insert(QStringLiteral("scrollIdle"), QStringLiteral("#3a3a42"));
  t.insert(QStringLiteral("scrollHover"), QStringLiteral("#585864"));
  t.insert(QStringLiteral("scrollPress"), QStringLiteral("#787886"));
  t.insert(QStringLiteral("scrim"), QStringLiteral("#9608080a"));
  t.insert(QStringLiteral("sheen"), QStringLiteral("#12ffffff"));
  t.insert(QStringLiteral("sheenBright"), QStringLiteral("#16ffffff"));
  t.insert(QStringLiteral("sheenPrimary"), QStringLiteral("#26ffffff"));
  t.insert(QStringLiteral("hoverLineBand"), QStringLiteral("#0effffff"));
  t.insert(QStringLiteral("indentGuide"), QStringLiteral("#292930"));
  t.insert(QStringLiteral("indentGuideActive"), QStringLiteral("#555562"));
  return t;
}

QVariantMap buildAdaptiveTheme(const AdaptivePaletteSeed &seed) {
  const QColor bgEditor = seed.bgEditor;
  const bool dark = relativeLuminance(bgEditor) < 0.25;
  const QColor bgShell =
      seed.bgShell.isValid()
          ? seed.bgShell
          : blendColors(bgEditor,
                        dark ? QColor(0, 0, 0) : QColor(235, 238, 244), 0.25);
  QColor fgMain = seed.fgMain.isValid()
                      ? seed.fgMain
                      : (dark ? QColor("#eef1f6") : QColor("#14161a"));
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
    accentFg = (relativeLuminance(accent) > 0.35) ? QColor("#101418")
                                                  : QColor("#ffffff");
  }
  const QColor accentStrong = blendColors(accent, fgMain, 0.22);

  const QColor borderSubtle =
      seed.border.isValid() ? blendColors(bgShell, seed.border, 0.55)
                            : blendColors(bgShell, fgMain, dark ? 0.12 : 0.13);
  const QColor borderControl =
      seed.border.isValid()
          ? blendColors(bgSurface, seed.border, 0.65)
          : blendColors(bgSurface, fgMain, dark ? 0.18 : 0.18);
  const QColor borderHover = blendColors(borderControl, fgMain, 0.28);
  const QColor borderFocus = blendColors(accent, fgMain, 0.20);

  const QColor bgLineHighlight =
      blendColors(bgEditor, fgMain, dark ? 0.045 : 0.04);
  const QColor bgLineHighlightSel =
      blendColors(bgEditor, accent, dark ? 0.14 : 0.11);
  const QColor selBg = blendColors(bgEditor, accent, dark ? 0.28 : 0.24);
  const QColor danger = seed.danger.isValid() ? seed.danger : QColor("#e0554d");

  QVariantMap t;
  t.insert(QStringLiteral("dark"), dark);
  t.insert(QStringLiteral("adaptive"), true);
  t.insert(QStringLiteral("bgShell"), colorStr(bgShell));
  t.insert(QStringLiteral("bgHeader"), colorStr(bgShell));
  t.insert(QStringLiteral("bgEditor"), colorStr(bgEditor));
  t.insert(QStringLiteral("bgGutter"), colorStr(bgGutter));
  t.insert(QStringLiteral("bgDrawer"),
           colorStr(blendColors(bgShell, bgEditor, 0.35)));
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
  t.insert(QStringLiteral("fgLabel"),
           colorStr(blendColors(fgMain, bgEditor, 0.10)));
  t.insert(QStringLiteral("fgMuted"),
           colorStr(blendColors(fgMain, bgEditor, 0.36)));
  t.insert(QStringLiteral("fgSub"),
           colorStr(blendColors(fgMain, bgEditor, 0.48)));
  t.insert(QStringLiteral("fgGutter"),
           colorStr(blendColors(fgMain, bgEditor, 0.60)));
  t.insert(QStringLiteral("accent"), colorStr(accent));
  t.insert(QStringLiteral("accentStrong"), colorStr(accentStrong));
  t.insert(QStringLiteral("selBg"), colorStr(selBg));
  t.insert(QStringLiteral("danger"), colorStr(danger));
  t.insert(QStringLiteral("fgOnPrimary"), colorStr(accentFg));
  t.insert(QStringLiteral("primaryBg"), colorStr(accent));
  t.insert(QStringLiteral("primaryHover"),
           colorStr(blendColors(accent, accentFg, 0.10)));
  t.insert(QStringLiteral("primaryPress"),
           colorStr(blendColors(accent, accentFg, 0.20)));
  t.insert(QStringLiteral("btnBorderNormal"), colorStr(borderControl));
  t.insert(QStringLiteral("btnBorderHover"), colorStr(borderHover));
  t.insert(QStringLiteral("btnBorderAccent"),
           colorStr(blendColors(accent, fgMain, 0.25)));
  t.insert(QStringLiteral("btnFillHover"),
           colorStr(withAlpha(fgMain, dark ? 28 : 16)));
  t.insert(QStringLiteral("btnFillPress"),
           colorStr(withAlpha(fgMain, dark ? 44 : 28)));
  t.insert(QStringLiteral("btnFillAccent"),
           colorStr(withAlpha(accent, dark ? 34 : 24)));
  t.insert(QStringLiteral("btnFillChip"),
           colorStr(blendColors(bgSurface, accent, dark ? 0.18 : 0.16)));
  t.insert(QStringLiteral("btnFillDanger"),
           colorStr(withAlpha(danger, dark ? 52 : 34)));
  t.insert(QStringLiteral("chipBg"),
           colorStr(blendColors(bgSurface, accent, dark ? 0.20 : 0.18)));
  t.insert(QStringLiteral("chipBorder"),
           colorStr(blendColors(borderControl, accent, 0.45)));
  t.insert(QStringLiteral("chipFg"), colorStr(fgMain));
  t.insert(QStringLiteral("scrollIdle"),
           colorStr(blendColors(bgEditor, fgMain, dark ? 0.20 : 0.22)));
  t.insert(QStringLiteral("scrollHover"),
           colorStr(blendColors(bgEditor, fgMain, dark ? 0.34 : 0.38)));
  t.insert(QStringLiteral("scrollPress"),
           colorStr(blendColors(bgEditor, fgMain, dark ? 0.48 : 0.52)));
  t.insert(QStringLiteral("scrim"),
           colorStr(withAlpha(bgShell.darker(220), dark ? 155 : 105)));
  t.insert(QStringLiteral("sheen"),
           colorStr(withAlpha(QColor(255, 255, 255), dark ? 18 : 128)));
  t.insert(QStringLiteral("sheenBright"),
           colorStr(withAlpha(QColor(255, 255, 255), dark ? 22 : 144)));
  t.insert(QStringLiteral("sheenPrimary"),
           colorStr(withAlpha(QColor(255, 255, 255), dark ? 38 : 48)));
  t.insert(QStringLiteral("hoverLineBand"),
           colorStr(withAlpha(fgMain, dark ? 14 : 10)));
  t.insert(QStringLiteral("indentGuide"),
           colorStr(blendColors(bgEditor, fgMain, dark ? 0.09 : 0.10)));
  t.insert(QStringLiteral("indentGuideActive"),
           colorStr(blendColors(bgEditor, fgMain, dark ? 0.28 : 0.28)));
  return t;
}

}

namespace nulla::theme {

bool detectWindowControls() {
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

QVariantMap buildThemePalette(int themeMode, bool &effectiveDark,
                              QStringList *watchedFiles) {
  if (watchedFiles) {
    watchedFiles->clear();
  }
  if (themeMode == 2) {
    effectiveDark = true;
    return buildFixedTheme(true);
  }
  if (themeMode == 1) {
    effectiveDark = false;
    return buildFixedTheme(false);
  }
  if (themeMode == 3) {
    effectiveDark = true;
    return buildGraphiteTheme();
  }

#if defined(Q_OS_WIN)
  const AdaptivePaletteSeed seed = tryLoadWindowsPalette();
  if (seed.valid) {
    QVariantMap t = buildAdaptiveTheme(seed);
    effectiveDark = t.value(QStringLiteral("dark"), true).toBool();
    return t;
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

  if (watchedFiles) {
    *watchedFiles = watched;
  }

  if (seed.valid) {
    QVariantMap t = buildAdaptiveTheme(seed);
    effectiveDark = t.value(QStringLiteral("dark"), true).toBool();
    return t;
  }
#endif

  const bool sysDark = QGuiApplication::styleHints()
                           ? (QGuiApplication::styleHints()->colorScheme() !=
                              Qt::ColorScheme::Light)
                           : true;
  effectiveDark = sysDark;
  return buildFixedTheme(sysDark);
}

void syncThemeWatcher(QFileSystemWatcher *watcher, int themeMode,
                      const QStringList &watchedFiles) {
#if defined(Q_OS_WIN)
  Q_UNUSED(watcher);
  Q_UNUSED(themeMode);
  Q_UNUSED(watchedFiles);
  return;
#else
  if (!watcher) {
    return;
  }
  const QStringList oldFiles = watcher->files();
  if (!oldFiles.isEmpty()) {
    watcher->removePaths(oldFiles);
  }
  const QStringList oldDirs = watcher->directories();
  if (!oldDirs.isEmpty()) {
    watcher->removePaths(oldDirs);
  }

  if (themeMode != 0) {
    return;
  }

  const QString cfgRoot =
      QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
  const QString cacheRoot =
      QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);

  QSet<QString> filesToWatch;
  for (const QString &f : watchedFiles) {
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
    watcher->addPaths(filesToWatch.values());
  }

  const QStringList dirsToWatch = {
      QDir(cfgRoot).filePath(QStringLiteral("matugen")),
      QDir(cfgRoot).filePath(QStringLiteral("gtk-4.0")),
      QDir(cfgRoot).filePath(QStringLiteral("gtk-3.0")),
      QDir(cfgRoot).filePath(QStringLiteral("qt6ct")),
      QDir(cacheRoot).filePath(QStringLiteral("wal"))};
  for (const QString &d : dirsToWatch) {
    if (QFileInfo::exists(d)) {
      watcher->addPath(d);
    }
  }
#endif
}

}
