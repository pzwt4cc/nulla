#ifndef NULLA_UPDATE_MANAGER_H
#define NULLA_UPDATE_MANAGER_H

#include <QObject>
#include <QPointer>
#include <QString>
#include <QtGlobal>
#include <QtQml/qqmlregistration.h>

class QFile;
class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

class UpdateManager : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("Managed by EditorBackend")

  Q_PROPERTY(int status READ status NOTIFY stateChanged)
  Q_PROPERTY(QString currentVersion READ currentVersion CONSTANT)
  Q_PROPERTY(QString availableVersion READ availableVersion NOTIFY stateChanged)
  Q_PROPERTY(QString releaseUrl READ releaseUrl NOTIFY stateChanged)
  Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY stateChanged)
  Q_PROPERTY(
      int downloadProgress READ downloadProgress NOTIFY downloadProgressChanged)
  Q_PROPERTY(bool canSelfUpdate READ canSelfUpdate CONSTANT)
  Q_PROPERTY(bool systemInstalled READ systemInstalled CONSTANT)
  Q_PROPERTY(bool bannerVisible READ bannerVisible NOTIFY stateChanged)
  Q_PROPERTY(
      bool autoCheck READ autoCheck WRITE setAutoCheck NOTIFY autoCheckChanged)
  Q_PROPERTY(qint64 lastCheckEpoch READ lastCheckEpoch NOTIFY stateChanged)
  Q_PROPERTY(QString repoUrl READ repoUrl CONSTANT)
  Q_PROPERTY(QString authorName READ authorName CONSTANT)

public:
  enum Status {
    Idle = 0,
    Checking = 1,
    UpToDate = 2,
    Available = 3,
    Downloading = 4,
    ReadyToInstall = 5,
    Installing = 6,
    Error = 7
  };
  Q_ENUM(Status)

  explicit UpdateManager(QObject *parent = nullptr);
  ~UpdateManager() override;

  int status() const { return m_status; }
  QString currentVersion() const { return m_currentVersion; }
  QString availableVersion() const { return m_availableVersion; }
  QString releaseUrl() const { return m_releaseUrl; }
  QString errorMessage() const { return m_errorMessage; }
  int downloadProgress() const { return m_downloadProgress; }
  bool canSelfUpdate() const { return m_canSelfUpdate; }
  bool systemInstalled() const { return m_systemInstalled; }
  bool bannerVisible() const;
  bool autoCheck() const { return m_autoCheck; }
  qint64 lastCheckEpoch() const { return m_lastCheckEpoch; }
  QString repoUrl() const;
  QString authorName() const;

  void setAutoCheck(bool enabled);

  Q_INVOKABLE void checkForUpdates(bool manual = true);
  Q_INVOKABLE void startDownload();
  Q_INVOKABLE bool installAndRestart();
  Q_INVOKABLE void dismissBanner();
  Q_INVOKABLE void skipVersion();
  Q_INVOKABLE void openReleasePage();
  Q_INVOKABLE void openRepoPage();

  bool applyReleaseMetadata(const QByteArray &jsonPayload);
  static bool isNewerVersion(const QString &candidate, const QString &current);
  static QString parseExpectedSha256(const QByteArray &shaFileData);
  static bool isSystemManagedPath(const QString &exePath, bool dirWritable);

signals:
  void stateChanged();
  void downloadProgressChanged();
  void autoCheckChanged();

private slots:
  void onStartupCheckTimer();
  void onCheckFinished();
  void onDownloadReadyRead();
  void onDownloadProgress(qint64 received, qint64 total);
  void onDownloadFinished();
  void onSha256Finished();

private:
  void detectPlatformCapabilities();
  void loadState();
  void saveState() const;
  void clearCachedUpdate();
  void abortActiveRequests();
  QString updatesDirPath() const;
  QString settingsFilePath() const;
  void failDownload(const QString &message);

  QNetworkAccessManager *m_nam = nullptr;
  QPointer<QNetworkReply> m_checkReply;
  QPointer<QNetworkReply> m_downloadReply;
  QPointer<QNetworkReply> m_shaReply;
  QTimer *m_startupTimer = nullptr;
  QFile *m_partFile = nullptr;

  int m_status = Idle;
  QString m_currentVersion;
  QString m_availableVersion;
  QString m_releaseUrl;
  QString m_assetUrl;
  QString m_assetName;
  QString m_sha256Url;
  QString m_downloadedFilePath;
  QString m_partFilePath;
  QString m_errorMessage;
  QString m_skippedVersion;
  int m_downloadProgress = 0;
  bool m_canSelfUpdate = false;
  bool m_systemInstalled = false;
  bool m_autoCheck = true;
  bool m_bannerDismissed = false;
  bool m_bannerOnError = false;
  bool m_manualCheck = false;
  qint64 m_lastCheckEpoch = 0;
};

#endif
