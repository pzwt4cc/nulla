#ifndef NULLA_THEME_PALETTE_H
#define NULLA_THEME_PALETTE_H

#include <QStringList>
#include <QVariantMap>

class QFileSystemWatcher;

namespace nulla::theme {

bool detectWindowControls();

QVariantMap buildThemePalette(int themeMode, bool &effectiveDark,
                              QStringList *watchedFiles = nullptr);

void syncThemeWatcher(QFileSystemWatcher *watcher, int themeMode,
                      const QStringList &watchedFiles);

}

#endif
