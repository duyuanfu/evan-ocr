#include "update_manager.h"
#include "../network/network_proxy_manager.h"
#include <QSettings>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVersionNumber>
#include <QProcess>
#include <QTimer>
#include <QDebug>

static const char* GITHUB_API_OFFICIAL = "https://api.github.com/repos/duyuanfu/evan-ocr/releases/latest";
static const char* GITHUB_API_MIRROR   = "https://ghfast.top/https://api.github.com/repos/duyuanfu/evan-ocr/releases/latest";
static const char* GHFAST_PREFIX       = "https://ghfast.top/";

UpdateManager& UpdateManager::instance()
{
    static UpdateManager s_mgr;
    return s_mgr;
}

UpdateManager::UpdateManager(QObject* parent)
    : QObject(parent)
{
    m_nam = std::make_unique<QNetworkAccessManager>(this);

    QSettings settings("Evan", "Evan");
    m_autoCheckOnStartup = settings.value("Update/AutoCheckOnStartup", true).toBool();
}

void UpdateManager::setAutoCheckOnStartup(bool enable)
{
    if (m_autoCheckOnStartup != enable) {
        m_autoCheckOnStartup = enable;
        QSettings settings("Evan", "Evan");
        settings.setValue("Update/AutoCheckOnStartup", enable);
    }
}

void UpdateManager::checkForUpdates(bool silent)
{
    if (m_checkReply) {
        m_checkReply->abort();
        m_checkReply->deleteLater();
        m_checkReply = nullptr;
    }

    emit checkStarted(silent);

    // 智能源路由：若配置了自定义代理，优先直连官方 API，镜像加速作为兜底
    // 若未配置代理（直连/系统），优先走国内镜像加速通道，直连官方作为兜底
    bool customProxy = (NetworkProxyManager::instance().proxyType() == ProxyType::Custom);
    QString primaryUrl = customProxy ? GITHUB_API_OFFICIAL : GITHUB_API_MIRROR;

    requestReleaseApi(primaryUrl, silent, false);
}

void UpdateManager::cancelCheck()
{
    if (m_checkReply) {
        m_checkReply->abort();
        m_checkReply->deleteLater();
        m_checkReply = nullptr;
    }
}

void UpdateManager::requestReleaseApi(const QString& url, bool silent, bool isFallback)
{
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setHeader(QNetworkRequest::UserAgentHeader,
        QString("Evan-App/%1 (Windows x64)").arg(QCoreApplication::applicationVersion()));
    request.setRawHeader("Accept", "application/vnd.github.v3+json");

    m_checkReply = m_nam->get(request);

    // 10 秒超时看门狗保护
    auto* timeoutTimer = new QTimer(m_checkReply);
    timeoutTimer->setSingleShot(true);
    connect(timeoutTimer, &QTimer::timeout, this, [this, silent, isFallback]() {
        if (m_checkReply && m_checkReply->isRunning()) {
            m_checkReply->abort();
            if (!isFallback) {
                // 切换到兜底源再试一次
                bool customProxy = (NetworkProxyManager::instance().proxyType() == ProxyType::Custom);
                QString fallbackUrl = customProxy ? GITHUB_API_MIRROR : GITHUB_API_OFFICIAL;
                qWarning() << "[Update] 主检查源请求超时，切换至备用兜底源:" << fallbackUrl;
                requestReleaseApi(fallbackUrl, silent, true);
            } else {
                emit checkFailed("检查更新超时，请检查网络或在设置中配置自定义代理", silent);
            }
        }
    });
    timeoutTimer->start(10000);

    connect(m_checkReply, &QNetworkReply::finished, this, [this, silent, isFallback, timeoutTimer]() {
        if (!timeoutTimer->isActive()) {
            return;
        }
        timeoutTimer->stop();

        if (!m_checkReply) return;
        QNetworkReply* reply = m_checkReply;
        m_checkReply = nullptr;
        reply->deleteLater();

        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError || (statusCode >= 400 && statusCode < 600)) {
            if (!isFallback) {
                bool customProxy = (NetworkProxyManager::instance().proxyType() == ProxyType::Custom);
                QString fallbackUrl = customProxy ? GITHUB_API_MIRROR : GITHUB_API_OFFICIAL;
                qWarning() << "[Update] 主检查源请求失败 (HTTP" << statusCode << ")，切换至备用源:" << fallbackUrl;
                requestReleaseApi(fallbackUrl, silent, true);
                return;
            }

            QString errStr = reply->errorString();
            if (statusCode == 404) {
                errStr = "未发现远端发布版本 (HTTP 404)";
            } else if (statusCode == 403) {
                errStr = "请求频率过高受限 (HTTP 403: API rate limit exceeded)";
            }
            emit checkFailed(QString("检查更新失败: %1").arg(errStr), silent);
            return;
        }

        QByteArray data = reply->readAll();
        parseReleaseJson(data, silent);
    });
}

void UpdateManager::parseReleaseJson(const QByteArray& jsonData, bool silent)
{
    QJsonParseError parseErr;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        emit checkFailed("解析远端版本信息失败 (JSON 格式错误)", silent);
        return;
    }

    QJsonObject obj = doc.object();
    QString tagName = obj.value("tag_name").toString().trimmed();
    if (tagName.isEmpty()) {
        emit checkFailed("远端版本信息不完整 (缺少 tag_name)", silent);
        return;
    }

    QString remoteVerStr = tagName;
    if (remoteVerStr.startsWith('v', Qt::CaseInsensitive)) {
        remoteVerStr = remoteVerStr.mid(1);
    }

    QString currentVerStr = QCoreApplication::applicationVersion().trimmed();
    if (currentVerStr.startsWith('v', Qt::CaseInsensitive)) {
        currentVerStr = currentVerStr.mid(1);
    }

    QVersionNumber currentVer = QVersionNumber::fromString(currentVerStr);
    QVersionNumber remoteVer  = QVersionNumber::fromString(remoteVerStr);

    bool hasNewer = (remoteVer > currentVer);

    UpdateInfo info;
    info.version = remoteVerStr;
    info.tagName = tagName;
    info.title = obj.value("name").toString().trimmed();
    if (info.title.isEmpty()) info.title = QString("Evan %1").arg(tagName);
    info.releaseNotes = obj.value("body").toString();
    info.publishedAt = obj.value("published_at").toString();
    info.hasUpdate = hasNewer;

    // 寻找 windows-x64 zip 资产
    QJsonArray assets = obj.value("assets").toArray();
    for (const auto& aVal : assets) {
        QJsonObject aObj = aVal.toObject();
        QString name = aObj.value("name").toString();
        if (name.contains("evan-", Qt::CaseInsensitive) && name.endsWith("-windows-x64.zip", Qt::CaseInsensitive)) {
            info.assetName = name;
            info.sizeBytes = aObj.value("size").toInteger();
            info.downloadUrl = aObj.value("browser_download_url").toString();
            break;
        }
    }

    // 若未精准匹配，兜底查找任意 .zip 资产
    if (info.downloadUrl.isEmpty()) {
        for (const auto& aVal : assets) {
            QJsonObject aObj = aVal.toObject();
            QString name = aObj.value("name").toString();
            if (name.endsWith(".zip", Qt::CaseInsensitive) && !name.contains("plugin", Qt::CaseInsensitive)) {
                info.assetName = name;
                info.sizeBytes = aObj.value("size").toInteger();
                info.downloadUrl = aObj.value("browser_download_url").toString();
                break;
            }
        }
    }

    if (!hasNewer) {
        emit noUpdateAvailable(silent);
    } else {
        emit updateAvailable(info, silent);
    }
}

void UpdateManager::startDownload(const UpdateInfo& info)
{
    cancelDownload();

    if (info.downloadUrl.isEmpty()) {
        emit downloadFailed("未找到有效的更新包下载地址");
        return;
    }

    QString finalDownloadUrl = info.downloadUrl;
    // 若未配置自定义代理，自动启用国内加速镜像
    if (NetworkProxyManager::instance().proxyType() != ProxyType::Custom) {
        if (!finalDownloadUrl.startsWith(GHFAST_PREFIX)) {
            finalDownloadUrl = GHFAST_PREFIX + finalDownloadUrl;
        }
    }

    m_currentDownloadingZip = QDir::tempPath() + QString("/evan_update_%1.zip").arg(info.version);
    if (QFile::exists(m_currentDownloadingZip)) {
        QFile::remove(m_currentDownloadingZip);
    }

    m_downloadFile = new QFile(m_currentDownloadingZip, this);
    if (!m_downloadFile->open(QIODevice::WriteOnly)) {
        emit downloadFailed("无法在临时目录创建更新文件");
        delete m_downloadFile;
        m_downloadFile = nullptr;
        return;
    }

    QNetworkRequest request;
    request.setUrl(QUrl(finalDownloadUrl));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setHeader(QNetworkRequest::UserAgentHeader,
        QString("Evan-App/%1 (Windows x64)").arg(QCoreApplication::applicationVersion()));

    m_downloadReply = m_nam->get(request);
    m_speedTimer.start();
    m_lastBytes = 0;
    m_currentSpeedMBs = 0.0;

    connect(m_downloadReply, &QNetworkReply::readyRead, this, [this]() {
        if (m_downloadFile && m_downloadFile->isOpen() && m_downloadReply) {
            m_downloadFile->write(m_downloadReply->readAll());
        }
    });

    connect(m_downloadReply, &QNetworkReply::downloadProgress, this, [this](qint64 bytesReceived, qint64 bytesTotal) {
        qint64 elapsedMs = m_speedTimer.elapsed();
        if (elapsedMs >= 500) {
            qint64 delta = bytesReceived - m_lastBytes;
            m_currentSpeedMBs = (delta / (1024.0 * 1024.0)) / (elapsedMs / 1000.0);
            m_speedTimer.restart();
            m_lastBytes = bytesReceived;
        }
        emit downloadProgress(bytesReceived, bytesTotal, m_currentSpeedMBs);
    });

    connect(m_downloadReply, &QNetworkReply::finished, this, [this]() {
        if (!m_downloadReply) return;
        QNetworkReply* reply = m_downloadReply;
        m_downloadReply = nullptr;
        reply->deleteLater();

        if (m_downloadFile) {
            m_downloadFile->flush();
            m_downloadFile->close();
            delete m_downloadFile;
            m_downloadFile = nullptr;
        }

        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError || (statusCode >= 400 && statusCode < 600)) {
            QFile::remove(m_currentDownloadingZip);
            emit downloadFailed(QString("下载更新失败: HTTP %1 - %2").arg(statusCode).arg(reply->errorString()));
            return;
        }

        emit downloadFinished(m_currentDownloadingZip);
    });
}

void UpdateManager::cancelDownload()
{
    if (m_downloadReply) {
        m_downloadReply->abort();
        m_downloadReply->deleteLater();
        m_downloadReply = nullptr;
    }
    if (m_downloadFile) {
        m_downloadFile->close();
        delete m_downloadFile;
        m_downloadFile = nullptr;
    }
    if (!m_currentDownloadingZip.isEmpty() && QFile::exists(m_currentDownloadingZip)) {
        QFile::remove(m_currentDownloadingZip);
    }
}

bool UpdateManager::checkInstallDirWritePermission(QString* errorMsg) const
{
    QString appDir = QCoreApplication::applicationDirPath();
    QString testFilePath = appDir + "/.evan_write_test.tmp";

    QFile testFile(testFilePath);
    if (!testFile.open(QIODevice::WriteOnly)) {
        if (errorMsg) {
            *errorMsg = QString("程序安装目录无写入权限:\n%1\n\n可能是安装在系统保护目录（如 Program Files），需要管理员权限。").arg(appDir);
        }
        return false;
    }
    testFile.close();
    testFile.remove();
    return true;
}

bool UpdateManager::executeUpdaterAndRestart(const QString& zipFilePath, QString* errorMsg)
{
    QString appDir = QCoreApplication::applicationDirPath();
    qint64 currentPid = QCoreApplication::applicationPid();

    // 1. 生成并写入无依赖的 Windows 原生批处理脚本
    QString batPath = QDir::tempPath() + "/evan_updater.bat";
    QFile batFile(batPath);
    if (!batFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        if (errorMsg) *errorMsg = "无法创建升级辅助脚本: " + batPath;
        return false;
    }

    // 注意：使用 UTF-8 编码并显式指定 chcp 65001
    QString batContent = QString(
        "@echo off\r\n"
        "chcp 65001 >nul\r\n"
        "title Evan Auto Updater\r\n"
        "\r\n"
        "set OLD_PID=%~1\r\n"
        "set ZIP_FILE=%~2\r\n"
        "set TARGET_DIR=%~3\r\n"
        "set EXTRACT_DIR=%TEMP%\\evan_update_unpack\r\n"
        "\r\n"
        "echo ======================================================\r\n"
        "echo   Evan 正在自动升级部署，请稍候...\r\n"
        "echo ======================================================\r\n"
        "\r\n"
        ":: 1. 等待主程序旧进程完全释放退出\r\n"
        "echo [*] 等待原程序退出 (PID: %OLD_PID%)...\r\n"
        ":wait_pid\r\n"
        "timeout /t 1 /nobreak >nul\r\n"
        "tasklist /fi \"PID eq %OLD_PID%\" 2>nul | findstr /i \"%OLD_PID%\" >nul\r\n"
        "if %ERRORLEVEL% equ 0 (\r\n"
        "    goto wait_pid\r\n"
        ")\r\n"
        "timeout /t 1 /nobreak >nul\r\n"
        "\r\n"
        ":: 2. 解压新版本安装包到临时解压目录\r\n"
        "echo [*] 解压新版本安装包...\r\n"
        "if exist \"%EXTRACT_DIR%\" rmdir /s /q \"%EXTRACT_DIR%\"\r\n"
        "mkdir \"%EXTRACT_DIR%\"\r\n"
        "tar.exe -xf \"%ZIP_FILE%\" -C \"%EXTRACT_DIR%\"\r\n"
        "if %ERRORLEVEL% neq 0 (\r\n"
        "    powershell.exe -NoProfile -NonInteractive -Command \"Expand-Archive -Path '%ZIP_FILE%' -DestinationPath '%EXTRACT_DIR%' -Force\"\r\n"
        ")\r\n"
        "\r\n"
        ":: 3. 检查是否有嵌套根目录并自适应提取\r\n"
        "set SOURCE_DIR=%EXTRACT_DIR%\r\n"
        "for /d %%%%D in (\"%EXTRACT_DIR%\\*\") do (\r\n"
        "    if exist \"%%%%D\\evan.exe\" set SOURCE_DIR=%%%%D\r\n"
        ")\r\n"
        "\r\n"
        "if not exist \"%SOURCE_DIR%\\evan.exe\" (\r\n"
        "    echo [!] 错误: 未能在更新包中找到 evan.exe 可执行文件\r\n"
        "    pause\r\n"
        "    exit /b 1\r\n"
        ")\r\n"
        "\r\n"
        ":: 4. 使用 robocopy 增量覆盖程序目录，严格排除 plugins 目录！\r\n"
        "echo [*] 覆盖程序核心文件 (已保护 plugins/ 插件目录)...\r\n"
        "robocopy \"%SOURCE_DIR%\" \"%TARGET_DIR%\" /E /XD \"plugins\" /R:3 /W:1 >nul\r\n"
        "\r\n"
        ":: 5. 重新启动新版 Evan\r\n"
        "echo [*] 正在拉起新版本 Evan...\r\n"
        "start \"\" \"%TARGET_DIR%\\evan.exe\"\r\n"
        "\r\n"
        ":: 6. 清理临时解压包与自删批处理脚本\r\n"
        "echo [*] 清理临时升级缓存...\r\n"
        "rmdir /s /q \"%EXTRACT_DIR%\" 2>nul\r\n"
        "del \"%ZIP_FILE%\" 2>nul\r\n"
        "(goto) 2>nul & del \"%%~f0\"\r\n"
    );

    batFile.write(batContent.toUtf8());
    batFile.close();

    // 2. 以独立分离进程拉起 cmd 执行批处理
    QStringList args;
    args << "/c" << QDir::toNativeSeparators(batPath)
         << QString::number(currentPid)
         << QDir::toNativeSeparators(zipFilePath)
         << QDir::toNativeSeparators(appDir);

    bool started = QProcess::startDetached("cmd.exe", args);
    if (!started) {
        if (errorMsg) *errorMsg = "无法启动外部升级进程 (cmd.exe)";
        return false;
    }

    qDebug() << "[Update] 自升级批处理已成功启动，准备退出旧程序";
    return true;
}
