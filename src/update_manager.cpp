#include "update_manager.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QSysInfo>
#include <QTimer>
#include <QUrl>
#include <QVersionNumber>

#include <cstdio>

#ifndef NULLA_APP_VERSION
#define NULLA_APP_VERSION "0.4.0"
#endif

namespace {

constexpr qint64 kAutoCheckIntervalSecs = 86400;
constexpr int kCheckTimeoutMs = 15000;
constexpr int kDownloadTimeoutMs = 120000;

QString cleanVersionString(const QString &raw) {
  QString s = raw.trimmed();
  if (s.startsWith(QLatin1Char('v')) || s.startsWith(QLatin1Char('V'))) {
    s.remove(0, 1);
  }
  const int dash = s.indexOf(QLatin1Char('-'));
  if (dash > 0) {
    s = s.left(dash);
  }
  const int plus = s.indexOf(QLatin1Char('+'));
  if (plus > 0) {
    s = s.left(plus);
  }
  return s;
}

}

UpdateManager::UpdateManager(QObject *parent)
    : QObject(parent), m_nam(new QNetworkAccessManager(this)),
      m_startupTimer(new QTimer(this)) {
  const QString appVer = QCoreApplication::applicationVersion().trimmed();
  m_currentVersion =
      appVer.isEmpty() ? QStringLiteral(NULLA_APP_VERSION) : appVer;

  detectPlatformCapabilities();
  loadState();

  m_startupTimer->setSingleShot(true);
  m_startupTimer->setInterval(2500);
  connect(m_startupTimer, &QTimer::timeout, this,
          &UpdateManager::onStartupCheckTimer);

  if (m_autoCheck && qEnvironmentVariableIsEmpty("NULLA_DISABLE_AUTO_UPDATE") &&
      QGuiApplication::platformName() != QLatin1String("offscreen")) {
    m_startupTimer->start();
  }
}

UpdateManager::~UpdateManager() {
  abortActiveRequests();
  if (m_partFile) {
    m_partFile->close();
    delete m_partFile;
    m_partFile = nullptr;
  }
}

bool UpdateManager::bannerVisible() const {
  if (m_bannerDismissed) {
    return false;
  }
  if (m_status == Downloading || m_status == ReadyToInstall) {
    return true;
  }
  if (m_status == Error) {
    return m_bannerOnError;
  }
  if (m_status == Available) {
    return m_skippedVersion != m_availableVersion;
  }
  return false;
}

QString UpdateManager::repoUrl() const {
  return QStringLiteral("https://github.com/pzwt4cc/nulla");
}

QString UpdateManager::authorName() const { return QStringLiteral("pzwt4cc"); }

void UpdateManager::setAutoCheck(bool enabled) {
  if (m_autoCheck == enabled) {
    return;
  }
  m_autoCheck = enabled;
  saveState();
  emit autoCheckChanged();
}

void UpdateManager::detectPlatformCapabilities() {
#if defined(Q_OS_WIN)
  m_systemInstalled = false;
  m_canSelfUpdate = true;
#elif defined(Q_OS_LINUX)
  const QString exePath = QCoreApplication::applicationFilePath();
  const QFileInfo exeInfo(exePath);
  const bool dirWritable = QFileInfo(exeInfo.absolutePath()).isWritable();
  m_systemInstalled = isSystemManagedPath(exePath, dirWritable);
  const bool archOk =
      QSysInfo::currentCpuArchitecture() == QLatin1String("x86_64");
  const bool hasTar =
      !QStandardPaths::findExecutable(QStringLiteral("tar")).isEmpty();
  m_canSelfUpdate = !m_systemInstalled && archOk && hasTar;
#else
  m_systemInstalled = true;
  m_canSelfUpdate = false;
#endif
}

bool UpdateManager::isSystemManagedPath(const QString &exePath,
                                        bool dirWritable) {
  if (exePath.isEmpty() || !dirWritable) {
    return true;
  }
  const QString clean = QDir::cleanPath(exePath);
  return clean.startsWith(QLatin1String("/usr/")) ||
         clean.startsWith(QLatin1String("/bin/")) ||
         clean.startsWith(QLatin1String("/sbin/")) ||
         clean.startsWith(QLatin1String("/opt/"));
}

bool UpdateManager::isNewerVersion(const QString &candidate,
                                   const QString &current) {
  const QString candClean = cleanVersionString(candidate);
  const QString currClean = cleanVersionString(current);
  if (candClean.isEmpty() || currClean.isEmpty()) {
    return false;
  }
  const QVersionNumber vCand = QVersionNumber::fromString(candClean);
  const QVersionNumber vCurr = QVersionNumber::fromString(currClean);
  if (vCand.isNull() || vCurr.isNull()) {
    return false;
  }
  return QVersionNumber::compare(vCand, vCurr) > 0;
}

QString UpdateManager::parseExpectedSha256(const QByteArray &shaFileData) {
  const QString text = QString::fromUtf8(shaFileData).trimmed();
  static const QRegularExpression hexRe(
      QStringLiteral("\\b([0-9a-fA-F]{64})\\b"));
  const QRegularExpressionMatch m = hexRe.match(text);
  return m.hasMatch() ? m.captured(1).toLower() : QString();
}

QString UpdateManager::settingsFilePath() const {
  const QString cfgDir =
      QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
  return QDir(cfgDir).filePath(QStringLiteral("nulla/settings.ini"));
}

QString UpdateManager::updatesDirPath() const {
  const QString dataDir =
      QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
  return QDir(dataDir).filePath(QStringLiteral("updates"));
}

void UpdateManager::loadState() {
  QSettings s(settingsFilePath(), QSettings::IniFormat);
  s.beginGroup(QStringLiteral("updater"));
  m_autoCheck = s.value(QStringLiteral("auto_check"), true).toBool();
  m_lastCheckEpoch =
      s.value(QStringLiteral("last_check_epoch"), 0).toLongLong();
  m_skippedVersion =
      cleanVersionString(s.value(QStringLiteral("skipped_version")).toString());
  const QString cachedVer =
      cleanVersionString(s.value(QStringLiteral("cached_version")).toString());
  const QString cachedReleaseUrl =
      s.value(QStringLiteral("cached_release_url")).toString();
  const QString cachedAssetUrl =
      s.value(QStringLiteral("cached_asset_url")).toString();
  const QString cachedAssetName =
      s.value(QStringLiteral("cached_asset_name")).toString();
  const QString cachedShaUrl =
      s.value(QStringLiteral("cached_sha256_url")).toString();
  const QString cachedFile =
      s.value(QStringLiteral("cached_downloaded_file")).toString();
  s.endGroup();

  bool dirty = false;
  if (!m_skippedVersion.isEmpty() &&
      !isNewerVersion(m_skippedVersion, m_currentVersion)) {
    m_skippedVersion.clear();
    dirty = true;
  }

  if (!cachedVer.isEmpty() && isNewerVersion(cachedVer, m_currentVersion)) {
    m_availableVersion = cachedVer;
    m_releaseUrl = cachedReleaseUrl;
    m_assetUrl = cachedAssetUrl;
    m_assetName = cachedAssetName;
    m_sha256Url = cachedShaUrl;
    if (!cachedFile.isEmpty() && QFileInfo::exists(cachedFile) &&
        m_canSelfUpdate) {
      m_downloadedFilePath = cachedFile;
      m_status = ReadyToInstall;
      m_downloadProgress = 100;
    } else if (!m_assetUrl.isEmpty() || !m_releaseUrl.isEmpty()) {
      m_status = Available;
    }
  } else if (!cachedVer.isEmpty() || !cachedFile.isEmpty()) {
    clearCachedUpdate();
    dirty = true;
  }

  if (dirty) {
    saveState();
  }
}

void UpdateManager::saveState() const {
  const QString iniPath = settingsFilePath();
  QDir().mkpath(QFileInfo(iniPath).absolutePath());
  QSettings s(iniPath, QSettings::IniFormat);
  s.beginGroup(QStringLiteral("updater"));
  s.setValue(QStringLiteral("auto_check"), m_autoCheck);
  s.setValue(QStringLiteral("last_check_epoch"), m_lastCheckEpoch);
  s.setValue(QStringLiteral("skipped_version"), m_skippedVersion);
  s.setValue(QStringLiteral("cached_version"), m_availableVersion);
  s.setValue(QStringLiteral("cached_release_url"), m_releaseUrl);
  s.setValue(QStringLiteral("cached_asset_url"), m_assetUrl);
  s.setValue(QStringLiteral("cached_asset_name"), m_assetName);
  s.setValue(QStringLiteral("cached_sha256_url"), m_sha256Url);
  s.setValue(QStringLiteral("cached_downloaded_file"), m_downloadedFilePath);
  s.endGroup();
  s.sync();
}

void UpdateManager::clearCachedUpdate() {
  m_availableVersion.clear();
  m_releaseUrl.clear();
  m_assetUrl.clear();
  m_assetName.clear();
  m_sha256Url.clear();
  m_downloadedFilePath.clear();
  m_downloadProgress = 0;
}

void UpdateManager::abortActiveRequests() {
  if (m_checkReply) {
    m_checkReply->disconnect(this);
    m_checkReply->abort();
    m_checkReply->deleteLater();
    m_checkReply = nullptr;
  }
  if (m_downloadReply) {
    m_downloadReply->disconnect(this);
    m_downloadReply->abort();
    m_downloadReply->deleteLater();
    m_downloadReply = nullptr;
  }
  if (m_shaReply) {
    m_shaReply->disconnect(this);
    m_shaReply->abort();
    m_shaReply->deleteLater();
    m_shaReply = nullptr;
  }
}

void UpdateManager::onStartupCheckTimer() {
  if (!m_autoCheck) {
    return;
  }
  const qint64 now = QDateTime::currentSecsSinceEpoch();
  if (m_lastCheckEpoch > 0 &&
      (now - m_lastCheckEpoch) < kAutoCheckIntervalSecs) {
    return;
  }
  checkForUpdates(false);
}

void UpdateManager::checkForUpdates(bool manual) {
  if (m_status == Checking || m_status == Downloading ||
      m_status == Installing) {
    return;
  }
  m_manualCheck = manual;
  m_bannerOnError = false;
  if (manual) {
    m_errorMessage.clear();
    m_status = Checking;
    emit stateChanged();
  }

  QNetworkRequest req(QUrl(QStringLiteral(
      "https://api.github.com/repos/pzwt4cc/nulla/releases/latest")));
  req.setHeader(QNetworkRequest::UserAgentHeader,
                QStringLiteral("Nulla/") + m_currentVersion);
  req.setRawHeader("Accept", "application/vnd.github+json");
  req.setRawHeader("X-GitHub-Api-Version", "2022-11-28");
  req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                   QNetworkRequest::NoLessSafeRedirectPolicy);
  req.setTransferTimeout(kCheckTimeoutMs);

  m_checkReply = m_nam->get(req);
  connect(m_checkReply, &QNetworkReply::finished, this,
          &UpdateManager::onCheckFinished);
}

void UpdateManager::onCheckFinished() {
  QNetworkReply *reply = m_checkReply;
  m_checkReply = nullptr;
  if (!reply) {
    return;
  }

  const int httpCode =
      reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  const QByteArray body = reply->readAll();
  const QNetworkReply::NetworkError netErr = reply->error();
  const QString errStr = reply->errorString();
  reply->deleteLater();

  if (netErr != QNetworkReply::NoError || httpCode < 200 || httpCode >= 300) {
    if (m_manualCheck) {
      m_errorMessage =
          errStr.isEmpty() ? QStringLiteral("HTTP %1").arg(httpCode) : errStr;
      m_status = Error;
      emit stateChanged();
    }
    return;
  }

  if (!applyReleaseMetadata(body)) {
    if (m_manualCheck) {
      m_errorMessage = QStringLiteral("Invalid release metadata");
      m_status = Error;
      emit stateChanged();
    }
    return;
  }

  m_lastCheckEpoch = QDateTime::currentSecsSinceEpoch();
  saveState();
  emit stateChanged();
}

bool UpdateManager::applyReleaseMetadata(const QByteArray &jsonPayload) {
  QJsonParseError parseErr;
  const QJsonDocument doc = QJsonDocument::fromJson(jsonPayload, &parseErr);
  if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
    return false;
  }

  const QJsonObject obj = doc.object();
  if (obj.value(QStringLiteral("draft")).toBool() ||
      obj.value(QStringLiteral("prerelease")).toBool()) {
    clearCachedUpdate();
    m_errorMessage.clear();
    m_bannerOnError = false;
    m_status = UpToDate;
    return true;
  }

  const QString tag = obj.value(QStringLiteral("tag_name")).toString();
  const QString ver = cleanVersionString(tag);
  if (ver.isEmpty()) {
    return false;
  }

  if (!isNewerVersion(ver, m_currentVersion)) {
    clearCachedUpdate();
    m_errorMessage.clear();
    m_bannerOnError = false;
    m_status = UpToDate;
    return true;
  }

  const QString htmlUrl = obj.value(QStringLiteral("html_url")).toString();
  QString pickedUrl;
  QString pickedName;
  QString pickedShaUrl;

  const QJsonArray assets = obj.value(QStringLiteral("assets")).toArray();
  QHash<QString, QString> assetUrls;
  for (const QJsonValue &v : assets) {
    if (!v.isObject()) {
      continue;
    }
    const QJsonObject a = v.toObject();
    const QString name = a.value(QStringLiteral("name")).toString();
    const QString url =
        a.value(QStringLiteral("browser_download_url")).toString();
    if (!name.isEmpty() && !url.isEmpty()) {
      assetUrls.insert(name, url);
    }
  }

  for (auto it = assetUrls.constBegin(); it != assetUrls.constEnd(); ++it) {
    const QString &name = it.key();
#if defined(Q_OS_WIN)
    if (name.endsWith(QLatin1String("-win64.msi"), Qt::CaseInsensitive) ||
        (pickedName.isEmpty() &&
         name.endsWith(QLatin1String(".msi"), Qt::CaseInsensitive))) {
      pickedName = name;
      pickedUrl = it.value();
    }
#else
    if (name.endsWith(QLatin1String("-linux-x86_64.tar.gz"),
                      Qt::CaseInsensitive) ||
        (pickedName.isEmpty() &&
         name.endsWith(QLatin1String(".tar.gz"), Qt::CaseInsensitive))) {
      pickedName = name;
      pickedUrl = it.value();
    }
#endif
  }

  if (!pickedName.isEmpty()) {
    const QString shaKey = pickedName + QLatin1String(".sha256");
    if (assetUrls.contains(shaKey)) {
      pickedShaUrl = assetUrls.value(shaKey);
    }
  }

  if (m_availableVersion != ver) {
    m_bannerDismissed = false;
    m_downloadedFilePath.clear();
    m_downloadProgress = 0;
  }

  m_availableVersion = ver;
  m_releaseUrl = htmlUrl.isEmpty()
                     ? repoUrl() + QStringLiteral("/releases/latest")
                     : htmlUrl;
  m_assetName = pickedName;
  m_assetUrl = pickedUrl;
  m_sha256Url = pickedShaUrl;
  m_errorMessage.clear();
  m_bannerOnError = false;

  if (!m_downloadedFilePath.isEmpty() &&
      QFileInfo::exists(m_downloadedFilePath) && m_canSelfUpdate) {
    m_status = ReadyToInstall;
    m_downloadProgress = 100;
  } else {
    m_status = Available;
  }
  return true;
}

void UpdateManager::startDownload() {
  if (!m_canSelfUpdate || m_assetUrl.isEmpty() || m_assetName.isEmpty()) {
    openReleasePage();
    return;
  }
  if (m_status == Downloading || m_status == Installing) {
    return;
  }
  if (m_status == ReadyToInstall && !m_downloadedFilePath.isEmpty() &&
      QFileInfo::exists(m_downloadedFilePath)) {
    return;
  }

  const QString dirPath = updatesDirPath();
  if (!QDir().mkpath(dirPath)) {
    m_bannerOnError = true;
    failDownload(QStringLiteral("Cannot create updates directory"));
    return;
  }

  QDir dir(dirPath);
  const QStringList oldFiles = dir.entryList(QDir::Files);
  for (const QString &f : oldFiles) {
    QFile::remove(dir.filePath(f));
  }

  m_partFilePath = dir.filePath(m_assetName + QLatin1String(".part"));
  if (m_partFile) {
    m_partFile->close();
    delete m_partFile;
    m_partFile = nullptr;
  }
  m_partFile = new QFile(m_partFilePath, this);
  if (!m_partFile->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    delete m_partFile;
    m_partFile = nullptr;
    m_bannerOnError = true;
    failDownload(QStringLiteral("Cannot open temporary update file"));
    return;
  }

  m_errorMessage.clear();
  m_bannerOnError = true;
  m_downloadProgress = 0;
  m_status = Downloading;
  emit downloadProgressChanged();
  emit stateChanged();

  QNetworkRequest req((QUrl(m_assetUrl)));
  req.setHeader(QNetworkRequest::UserAgentHeader,
                QStringLiteral("Nulla/") + m_currentVersion);
  req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                   QNetworkRequest::NoLessSafeRedirectPolicy);
  req.setTransferTimeout(kDownloadTimeoutMs);

  m_downloadReply = m_nam->get(req);
  connect(m_downloadReply, &QNetworkReply::readyRead, this,
          &UpdateManager::onDownloadReadyRead);
  connect(m_downloadReply, &QNetworkReply::downloadProgress, this,
          &UpdateManager::onDownloadProgress);
  connect(m_downloadReply, &QNetworkReply::finished, this,
          &UpdateManager::onDownloadFinished);
}

void UpdateManager::onDownloadReadyRead() {
  if (!m_downloadReply || !m_partFile) {
    return;
  }
  const QByteArray chunk = m_downloadReply->readAll();
  if (!chunk.isEmpty()) {
    if (m_partFile->write(chunk) != chunk.size()) {
      m_downloadReply->abort();
    }
  }
}

void UpdateManager::onDownloadProgress(qint64 received, qint64 total) {
  int pct = 0;
  if (total > 0) {
    pct = qBound(0, static_cast<int>((received * 100) / total), 100);
  }
  if (pct != m_downloadProgress) {
    m_downloadProgress = pct;
    emit downloadProgressChanged();
  }
}

void UpdateManager::onDownloadFinished() {
  QNetworkReply *reply = m_downloadReply;
  m_downloadReply = nullptr;
  if (!reply) {
    return;
  }

  if (m_partFile && reply->bytesAvailable() > 0) {
    m_partFile->write(reply->readAll());
  }
  if (m_partFile) {
    m_partFile->flush();
    m_partFile->close();
    delete m_partFile;
    m_partFile = nullptr;
  }

  const int httpCode =
      reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  const QNetworkReply::NetworkError netErr = reply->error();
  const QString errStr = reply->errorString();
  reply->deleteLater();

  if (netErr != QNetworkReply::NoError || httpCode < 200 || httpCode >= 300 ||
      !QFileInfo::exists(m_partFilePath) ||
      QFileInfo(m_partFilePath).size() <= 0) {
    QFile::remove(m_partFilePath);
    failDownload(errStr.isEmpty() ? QStringLiteral("Download failed") : errStr);
    return;
  }

  if (m_sha256Url.isEmpty()) {
    const QString finalPath = QDir(updatesDirPath()).filePath(m_assetName);
    QFile::remove(finalPath);
    if (!QFile::rename(m_partFilePath, finalPath)) {
      QFile::remove(m_partFilePath);
      failDownload(QStringLiteral("Failed to stage downloaded file"));
      return;
    }
    m_downloadedFilePath = finalPath;
    m_downloadProgress = 100;
    m_bannerOnError = false;
    m_status = ReadyToInstall;
    saveState();
    emit downloadProgressChanged();
    emit stateChanged();
    return;
  }

  QNetworkRequest req((QUrl(m_sha256Url)));
  req.setHeader(QNetworkRequest::UserAgentHeader,
                QStringLiteral("Nulla/") + m_currentVersion);
  req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                   QNetworkRequest::NoLessSafeRedirectPolicy);
  req.setTransferTimeout(kCheckTimeoutMs);

  m_shaReply = m_nam->get(req);
  connect(m_shaReply, &QNetworkReply::finished, this,
          &UpdateManager::onSha256Finished);
}

void UpdateManager::onSha256Finished() {
  QNetworkReply *reply = m_shaReply;
  m_shaReply = nullptr;
  if (!reply) {
    return;
  }

  const int httpCode =
      reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  const QByteArray shaBody = reply->readAll();
  const QNetworkReply::NetworkError netErr = reply->error();
  reply->deleteLater();

  if (netErr != QNetworkReply::NoError || httpCode < 200 || httpCode >= 300) {
    QFile::remove(m_partFilePath);
    failDownload(QStringLiteral("Failed to verify update checksum"));
    return;
  }

  const QString expected = parseExpectedSha256(shaBody);
  if (expected.isEmpty()) {
    QFile::remove(m_partFilePath);
    failDownload(QStringLiteral("Invalid checksum file"));
    return;
  }

  QFile f(m_partFilePath);
  if (!f.open(QIODevice::ReadOnly)) {
    QFile::remove(m_partFilePath);
    failDownload(QStringLiteral("Cannot read downloaded update"));
    return;
  }
  QCryptographicHash hasher(QCryptographicHash::Sha256);
  if (!hasher.addData(&f)) {
    f.close();
    QFile::remove(m_partFilePath);
    failDownload(QStringLiteral("Failed to hash downloaded update"));
    return;
  }
  f.close();

  const QString actual = QString::fromLatin1(hasher.result().toHex()).toLower();
  if (actual != expected) {
    QFile::remove(m_partFilePath);
    failDownload(QStringLiteral("Checksum verification failed"));
    return;
  }

  const QString finalPath = QDir(updatesDirPath()).filePath(m_assetName);
  QFile::remove(finalPath);
  if (!QFile::rename(m_partFilePath, finalPath)) {
    QFile::remove(m_partFilePath);
    failDownload(QStringLiteral("Failed to stage downloaded file"));
    return;
  }

  m_downloadedFilePath = finalPath;
  m_downloadProgress = 100;
  m_bannerOnError = false;
  m_status = ReadyToInstall;
  saveState();
  emit downloadProgressChanged();
  emit stateChanged();
}

void UpdateManager::failDownload(const QString &message) {
  m_downloadedFilePath.clear();
  m_partFilePath.clear();
  m_downloadProgress = 0;
  m_errorMessage = message;
  m_status = Error;
  emit downloadProgressChanged();
  emit stateChanged();
}

bool UpdateManager::installAndRestart() {
  if (!m_canSelfUpdate || m_status != ReadyToInstall ||
      m_downloadedFilePath.isEmpty() ||
      !QFileInfo::exists(m_downloadedFilePath)) {
    return false;
  }

#if defined(Q_OS_WIN)
  const QString dirPath = updatesDirPath();
  QDir().mkpath(dirPath);
  const QString scriptPath =
      QDir(dirPath).filePath(QStringLiteral("apply_update.ps1"));
  QFile script(scriptPath);
  if (!script.open(QIODevice::WriteOnly | QIODevice::Truncate |
                   QIODevice::Text)) {
    m_bannerOnError = true;
    failDownload(QStringLiteral("Cannot create installer launcher script"));
    return false;
  }
  const QByteArray ps =
      "param([int]$ParentPid, [string]$MsiPath, [string]$ExePath)\n"
      "try { Wait-Process -Id $ParentPid -Timeout 15 -ErrorAction "
      "SilentlyContinue } catch {}\n"
      "Start-Sleep -Milliseconds 300\n"
      "$arg = '/i \"' + $MsiPath + '\" /passive'\n"
      "$p = Start-Process -FilePath 'msiexec.exe' -ArgumentList $arg -Wait "
      "-PassThru\n"
      "if ($p.ExitCode -eq 0 -or $p.ExitCode -eq 3010) {\n"
      "  if (Test-Path -LiteralPath $ExePath) { Start-Process -FilePath "
      "$ExePath }\n"
      "  elseif (Test-Path -LiteralPath "
      "\"$env:ProgramFiles\\Nulla\\nulla.exe\") { Start-Process -FilePath "
      "\"$env:ProgramFiles\\Nulla\\nulla.exe\" }\n"
      "}\n"
      "Remove-Item -LiteralPath $MsiPath -Force -ErrorAction SilentlyContinue\n"
      "Remove-Item -LiteralPath $PSCommandPath -Force -ErrorAction "
      "SilentlyContinue\n";
  if (script.write(ps) != ps.size()) {
    script.close();
    m_bannerOnError = true;
    failDownload(QStringLiteral("Failed to write installer launcher script"));
    return false;
  }
  script.close();

  const QStringList args = {
      QStringLiteral("-NoProfile"),
      QStringLiteral("-ExecutionPolicy"),
      QStringLiteral("Bypass"),
      QStringLiteral("-WindowStyle"),
      QStringLiteral("Hidden"),
      QStringLiteral("-File"),
      QDir::toNativeSeparators(scriptPath),
      QString::number(QCoreApplication::applicationPid()),
      QDir::toNativeSeparators(m_downloadedFilePath),
      QDir::toNativeSeparators(QCoreApplication::applicationFilePath())};
  if (!QProcess::startDetached(QStringLiteral("powershell.exe"), args)) {
    m_bannerOnError = true;
    failDownload(QStringLiteral("Failed to launch installer"));
    return false;
  }
  m_status = Installing;
  emit stateChanged();
  return true;
#elif defined(Q_OS_LINUX)
  const QString dirPath = updatesDirPath();
  const QString stagingDir = QDir(dirPath).filePath(QStringLiteral("staging"));
  QDir(stagingDir).removeRecursively();
  if (!QDir().mkpath(stagingDir)) {
    m_bannerOnError = true;
    failDownload(QStringLiteral("Cannot create staging directory"));
    return false;
  }

  const int tarRc = QProcess::execute(
      QStringLiteral("tar"), {QStringLiteral("-xzf"), m_downloadedFilePath,
                              QStringLiteral("-C"), stagingDir});
  if (tarRc != 0) {
    QDir(stagingDir).removeRecursively();
    m_bannerOnError = true;
    failDownload(QStringLiteral("Failed to extract update archive"));
    return false;
  }

  QString extractedBin;
  QDirIterator it(stagingDir, QStringList() << QStringLiteral("nulla"),
                  QDir::Files, QDirIterator::Subdirectories);
  if (it.hasNext()) {
    extractedBin = it.next();
  }
  if (extractedBin.isEmpty()) {
    QDir(stagingDir).removeRecursively();
    m_bannerOnError = true;
    failDownload(QStringLiteral("Updated binary not found in archive"));
    return false;
  }

  const QString exePath = QCoreApplication::applicationFilePath();
  const QString exeDir = QFileInfo(exePath).absolutePath();
  const QString tmpTarget =
      QDir(exeDir).filePath(QStringLiteral(".nulla-update-%1")
                                .arg(QCoreApplication::applicationPid()));
  QFile::remove(tmpTarget);
  if (!QFile::copy(extractedBin, tmpTarget)) {
    QDir(stagingDir).removeRecursively();
    m_bannerOnError = true;
    failDownload(QStringLiteral("Cannot write updated binary"));
    return false;
  }
  QFile::setPermissions(tmpTarget,
                        QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                            QFileDevice::ExeOwner | QFileDevice::ReadGroup |
                            QFileDevice::ExeGroup | QFileDevice::ReadOther |
                            QFileDevice::ExeOther);

  const QByteArray srcEnc = QFile::encodeName(tmpTarget);
  const QByteArray dstEnc = QFile::encodeName(exePath);
  if (std::rename(srcEnc.constData(), dstEnc.constData()) != 0) {
    QFile::remove(tmpTarget);
    QDir(stagingDir).removeRecursively();
    m_bannerOnError = true;
    failDownload(QStringLiteral("Failed to replace binary"));
    return false;
  }

  QDir(stagingDir).removeRecursively();
  QFile::remove(m_downloadedFilePath);
  clearCachedUpdate();
  saveState();

  if (!QProcess::startDetached(exePath, {})) {
    m_bannerOnError = true;
    failDownload(QStringLiteral("Updated, please restart Nulla manually"));
    return false;
  }

  m_status = Installing;
  emit stateChanged();
  return true;
#else
  return false;
#endif
}

void UpdateManager::dismissBanner() {
  if (m_bannerDismissed && !m_bannerOnError) {
    return;
  }
  m_bannerDismissed = true;
  m_bannerOnError = false;
  emit stateChanged();
}

void UpdateManager::skipVersion() {
  if (m_availableVersion.isEmpty()) {
    return;
  }
  m_skippedVersion = m_availableVersion;
  m_bannerDismissed = true;
  m_bannerOnError = false;
  saveState();
  emit stateChanged();
}

void UpdateManager::openReleasePage() {
  const QUrl url(m_releaseUrl.isEmpty()
                     ? repoUrl() + QStringLiteral("/releases/latest")
                     : m_releaseUrl);
  QDesktopServices::openUrl(url);
}

void UpdateManager::openRepoPage() {
  QDesktopServices::openUrl(QUrl(repoUrl()));
}
