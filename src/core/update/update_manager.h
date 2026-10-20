#pragma once

#include <QObject>
#include <QString>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QElapsedTimer>
#include <memory>

struct UpdateInfo {
    QString version;
    QString tagName;
    QString title;
    QString releaseNotes;
    QString publishedAt;
    QString downloadUrl;
    QString assetName;
    qint64 sizeBytes = 0;
    bool hasUpdate = false;
};

class UpdateManager : public QObject
{
    Q_OBJECT
public:
    static UpdateManager& instance();

    // 配置
    bool autoCheckOnStartup() const { return m_autoCheckOnStartup; }
    void setAutoCheckOnStartup(bool enable);

    // 检查更新
    void checkForUpdates(bool silent = false);
    void cancelCheck();

    // 下载更新
    void startDownload(const UpdateInfo& info);
    void cancelDownload();

    // 目录写权限检测
    bool checkInstallDirWritePermission(QString* errorMsg = nullptr) const;

    // 启动外部自更新 Helper 批处理并重启
    bool executeUpdaterAndRestart(const QString& zipFilePath, QString* errorMsg = nullptr);

signals:
    void checkStarted(bool silent);
    void updateAvailable(const UpdateInfo& info, bool silent);
    void noUpdateAvailable(bool silent);
    void checkFailed(const QString& errorMsg, bool silent);

    void downloadProgress(qint64 receivedBytes, qint64 totalBytes, double speedMBs);
    void downloadFinished(const QString& tempZipPath);
    void downloadFailed(const QString& errorMsg);

private:
    explicit UpdateManager(QObject* parent = nullptr);
    ~UpdateManager() override = default;

    void requestReleaseApi(const QString& url, bool silent, bool isFallback);
    void parseReleaseJson(const QByteArray& jsonData, bool silent);

    bool m_autoCheckOnStartup = true;

    std::unique_ptr<QNetworkAccessManager> m_nam;
    QNetworkReply* m_checkReply = nullptr;
    QNetworkReply* m_downloadReply = nullptr;
    QFile* m_downloadFile = nullptr;
    QString m_currentDownloadingZip;

    QElapsedTimer m_speedTimer;
    qint64 m_lastBytes = 0;
    double m_currentSpeedMBs = 0.0;
};
