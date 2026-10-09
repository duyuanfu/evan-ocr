#include "plugin_manager.h"
#include "../translation/translation_plugin_manager.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QNetworkRequest>
#include <QThreadPool>
#include <QMetaObject>
#include <QDebug>

#include <QNetworkProxyFactory>

PluginManager& PluginManager::instance()
{
    static PluginManager mgr;
    return mgr;
}

PluginManager::PluginManager(QObject* parent)
    : QObject(parent)
{
    QNetworkProxyFactory::setUseSystemConfiguration(true);
    m_nam = std::make_unique<QNetworkAccessManager>(this);
    initCatalog();
    refreshAllStatus();
}

void PluginManager::initCatalog()
{
    m_plugins.clear();

    // 1. RapidOCR 高精度离线文字识别核心引擎
    PluginInfo ocrCore;
    ocrCore.id = "rapidocr-engine";
    ocrCore.name = "RapidOCR 高精度离线文字识别引擎";
    ocrCore.category = "文字识别";
    ocrCore.iconEmoji = "🔍";
    ocrCore.description = "基于 ONNX Runtime 与 PaddleOCR 的纯本地高精度离线文字识别引擎，支持中英文排版、旋转方向纠正与精准坐标提取。";
    ocrCore.sizeDisplay = "74 MB";
    ocrCore.sizeBytes = 77700000;
    // 使用国内 CDN 高速加速镜像直链，确保免翻墙秒级建立连接并极速下载
    ocrCore.downloadUrl = "https://ghfast.top/https://github.com/duyuanfu/evan-ocr/releases/download/plugins/evan-plugin-rapidocr-engine.zip";
    ocrCore.installSubdir = "plugins/ocr";
    ocrCore.checkRelativeFile = "RapidOCR-json.exe";
    m_plugins.append(ocrCore);

    // 2. 核心离线中英神经网络机器翻译扩展包
    PluginInfo transZhEn;
    transZhEn.id = "translation-en-zh";
    transZhEn.name = "离线中英双向神经网络翻译扩展包";
    transZhEn.category = "离线翻译";
    transZhEn.iconEmoji = "🌐";
    transZhEn.description = "基于轻量级神经网络翻译模型，提供中英双向高精度、毫秒级出字的纯本地翻译能力。0 联网请求，商业机密与私有代码 100% 隐私安全。";
    transZhEn.sizeDisplay = "38 MB";
    transZhEn.sizeBytes = 39845888;
    // 使用国内 CDN 高速加速镜像直链
    transZhEn.downloadUrl = "https://ghfast.top/https://github.com/duyuanfu/evan-ocr/releases/download/plugins/evan-plugin-translation-en-zh.zip";
    transZhEn.installSubdir = "plugins/translation";
    transZhEn.checkRelativeFile = "translator-engine.exe";
    m_plugins.append(transZhEn);
}

QList<PluginInfo> PluginManager::plugins() const
{
    return m_plugins;
}

PluginInfo PluginManager::pluginInfo(const QString& id) const
{
    for (const auto& item : m_plugins) {
        if (item.id == id) return item;
    }
    return PluginInfo();
}

QString PluginManager::pluginRootDir() const
{
    QString appDir = QCoreApplication::applicationDirPath();
    return QDir::toNativeSeparators(appDir + "/plugins");
}

void PluginManager::refreshAllStatus()
{
    for (auto& item : m_plugins) {
        // 如果当前正在下载中或解压中，不重置状态
        if (item.status != PluginStatus::Downloading && item.status != PluginStatus::Extracting) {
            updateSinglePluginStatus(item);
        }
    }
    emit pluginsRefreshed();
}

void PluginManager::updateSinglePluginStatus(PluginInfo& info)
{
    QString appDir = QCoreApplication::applicationDirPath();
    QString targetDirPath = appDir + "/" + info.installSubdir;
    QString targetFilePath = targetDirPath + "/" + info.checkRelativeFile;

    bool isInstalled = false;

    if (info.id == "translation-en-zh") {
        // 对翻译核心插件，检查是否有引擎 exe 或 models 目录
        if (QFile::exists(targetFilePath) ||
            QFile::exists(targetDirPath + "/offline-translator.exe") ||
            QFile::exists(targetDirPath + "/ctranslate2-runner.exe") ||
            QDir(targetDirPath + "/models").exists()) {
            isInstalled = true;
        }
    } else if (info.id == "rapidocr-engine") {
        if (QFile::exists(targetFilePath) ||
            QFile::exists(appDir + "/plugins/ocr/RapidOCR-json.exe") ||
            QFile::exists(appDir + "/plugins/ocr/rapidocr.exe") ||
            QFile::exists(appDir + "/rapidocr/RapidOCR-json.exe") ||
            QFile::exists(appDir + "/rapidocr/rapidocr.exe")) {
            isInstalled = true;
        }
    } else {
        if (QFile::exists(targetFilePath) || QDir(targetFilePath).exists()) {
            isInstalled = true;
        }
    }

    if (isInstalled) {
        info.status = PluginStatus::Installed;
        info.statusMessage = "已安装并就绪";
        info.progressPercent = 100;
    } else {
        info.status = PluginStatus::NotInstalled;
        info.statusMessage = "未安装";
        info.progressPercent = 0;
    }
}

void PluginManager::installPlugin(const QString& id)
{
    int index = -1;
    for (int i = 0; i < m_plugins.size(); ++i) {
        if (m_plugins[i].id == id) {
            index = i;
            break;
        }
    }
    if (index == -1) return;

    PluginInfo& info = m_plugins[index];
    if (info.status == PluginStatus::Downloading || info.status == PluginStatus::Extracting) {
        return;
    }

    info.status = PluginStatus::Downloading;
    info.progressPercent = 0;
    info.downloadedBytes = 0;
    info.totalBytes = info.sizeBytes;
    info.statusMessage = "正在建立高速连接...";
    emit pluginStatusChanged(id, PluginStatus::Downloading, info.statusMessage);

    QString tempZip = QDir::tempPath() + QString("/evan_plugin_%1.zip").arg(id);
    if (QFile::exists(tempZip)) {
        QFile::remove(tempZip);
    }

    auto* file = new QFile(tempZip, this);
    if (!file->open(QIODevice::WriteOnly)) {
        info.status = PluginStatus::Failed;
        info.statusMessage = "无法创建临时下载文件";
        emit pluginStatusChanged(id, PluginStatus::Failed, info.statusMessage);
        delete file;
        return;
    }

    m_activeFiles[id] = file;

    QNetworkRequest request(QUrl(info.downloadUrl));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setHeader(QNetworkRequest::UserAgentHeader, "Evan-App/1.0.0 (Windows x64)");

    QNetworkReply* reply = m_nam->get(request);
    m_activeReplies[id] = reply;

    QElapsedTimer timer;
    timer.start();
    m_speedTimers[id] = timer;
    m_lastBytes[id] = 0;

    connect(reply, &QNetworkReply::readyRead, this, [this, id, reply, file]() {
        if (file && file->isOpen()) {
            file->write(reply->readAll());
        }
    });

    connect(reply, &QNetworkReply::downloadProgress, this, [this, id](qint64 bytesReceived, qint64 bytesTotal) {
        int idx = -1;
        for (int i = 0; i < m_plugins.size(); ++i) {
            if (m_plugins[i].id == id) {
                idx = i;
                break;
            }
        }
        if (idx == -1) return;

        PluginInfo& p = m_plugins[idx];
        p.downloadedBytes = bytesReceived;
        if (bytesTotal > 0) {
            p.totalBytes = bytesTotal;
            p.progressPercent = static_cast<int>((bytesReceived * 100) / bytesTotal);
        } else if (p.sizeBytes > 0) {
            p.progressPercent = qBound(0, static_cast<int>((bytesReceived * 100) / p.sizeBytes), 99);
        }

        double speedMBs = 0.0;
        if (m_speedTimers.contains(id)) {
            qint64 elapsedMs = m_speedTimers[id].elapsed();
            if (elapsedMs > 500) {
                qint64 delta = bytesReceived - m_lastBytes[id];
                speedMBs = (delta / (1024.0 * 1024.0)) / (elapsedMs / 1000.0);
                m_speedTimers[id].restart();
                m_lastBytes[id] = bytesReceived;
                p.speedBytesPerSec = speedMBs;
            }
        }

        emit pluginProgressChanged(id, p.progressPercent, bytesReceived, p.totalBytes, p.speedBytesPerSec);
    });

    connect(reply, &QNetworkReply::finished, this, [this, id, reply, file, tempZip]() {
        reply->deleteLater();
        m_activeReplies.remove(id);
        m_speedTimers.remove(id);
        m_lastBytes.remove(id);

        if (file) {
            file->flush();
            file->close();
            delete file;
            m_activeFiles.remove(id);
        }

        int idx = -1;
        for (int i = 0; i < m_plugins.size(); ++i) {
            if (m_plugins[i].id == id) {
                idx = i;
                break;
            }
        }
        if (idx == -1) return;
        PluginInfo& p = m_plugins[idx];

        int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError || (httpCode >= 400 && httpCode < 600)) {
            QFile::remove(tempZip);
            p.status = PluginStatus::Failed;
            if (httpCode == 404 || reply->error() == QNetworkReply::ContentNotFoundError) {
                p.statusMessage = "下载失败 (HTTP 404): 远端 Releases 尚未上传该扩展包，请先发布资产或在本地放入 plugins/ 目录";
            } else if (reply->error() == QNetworkReply::TimeoutError || reply->error() == QNetworkReply::HostNotFoundError || reply->error() == QNetworkReply::ConnectionRefusedError) {
                p.statusMessage = QString("网络连接受限: %1 (国内访问 GitHub 资产易受阻，建议配置镜像或手动下载)").arg(reply->errorString());
            } else {
                p.statusMessage = QString("下载异常: HTTP %1 - %2").arg(httpCode).arg(reply->errorString());
            }
            emit pluginStatusChanged(id, PluginStatus::Failed, p.statusMessage);
            return;
        }

        // 下载完成，进入解压阶段
        p.status = PluginStatus::Extracting;
        p.statusMessage = "下载完成，正在自动解压部署中...";
        emit pluginStatusChanged(id, PluginStatus::Extracting, p.statusMessage);

        QString appDir = QCoreApplication::applicationDirPath();
        QString destDir = appDir + "/" + p.installSubdir;

        QThreadPool::globalInstance()->start([this, id, tempZip, destDir]() {
            bool ok = extractZipArchive(tempZip, destDir);
            QFile::remove(tempZip);

            QMetaObject::invokeMethod(this, [this, id, ok]() {
                int i = -1;
                for (int k = 0; k < m_plugins.size(); ++k) {
                    if (m_plugins[k].id == id) {
                        i = k;
                        break;
                    }
                }
                if (i == -1) return;

                PluginInfo& info = m_plugins[i];
                if (ok) {
                    info.status = PluginStatus::Installed;
                    info.progressPercent = 100;
                    info.statusMessage = "✓ 已安装并热加载就绪";
                    emit pluginStatusChanged(id, PluginStatus::Installed, info.statusMessage);

                    // 自动热重载翻译插件管理器
                    TranslationPluginManager::instance().refreshPlugins();
                } else {
                    info.status = PluginStatus::Failed;
                    info.statusMessage = "自动解压失败，请检查目录权限或手动解压。";
                    emit pluginStatusChanged(id, PluginStatus::Failed, info.statusMessage);
                }
            }, Qt::QueuedConnection);
        });
    });
}

void PluginManager::cancelDownload(const QString& id)
{
    if (m_activeReplies.contains(id)) {
        QNetworkReply* reply = m_activeReplies.take(id);
        reply->abort();
        reply->deleteLater();
    }
    if (m_activeFiles.contains(id)) {
        QFile* file = m_activeFiles.take(id);
        file->close();
        file->remove();
        delete file;
    }
    m_speedTimers.remove(id);
    m_lastBytes.remove(id);

    for (auto& item : m_plugins) {
        if (item.id == id) {
            updateSinglePluginStatus(item);
            emit pluginStatusChanged(id, item.status, "已取消下载");
            break;
        }
    }
}

bool PluginManager::installFromLocalZip(const QString& zipPath, const QString& targetPluginId)
{
    if (!QFile::exists(zipPath)) return false;

    int idx = -1;
    for (int i = 0; i < m_plugins.size(); ++i) {
        if (m_plugins[i].id == targetPluginId) {
            idx = i;
            break;
        }
    }
    if (idx == -1 && !m_plugins.isEmpty()) idx = 0;
    if (idx == -1) return false;

    PluginInfo& p = m_plugins[idx];
    p.status = PluginStatus::Extracting;
    p.statusMessage = "正在解压本地 ZIP 扩展包...";
    emit pluginStatusChanged(p.id, PluginStatus::Extracting, p.statusMessage);

    QString appDir = QCoreApplication::applicationDirPath();
    QString destDir = appDir + "/" + p.installSubdir;

    bool ok = extractZipArchive(zipPath, destDir);
    if (ok) {
        updateSinglePluginStatus(p);
        p.status = PluginStatus::Installed;
        p.statusMessage = "✓ 已成功导入本地插件并热加载";
        p.progressPercent = 100;
        emit pluginStatusChanged(p.id, PluginStatus::Installed, p.statusMessage);
        TranslationPluginManager::instance().refreshPlugins();
        return true;
    } else {
        p.status = PluginStatus::Failed;
        p.statusMessage = "本地 ZIP 解压失败，请检查压缩包是否完整";
        emit pluginStatusChanged(p.id, PluginStatus::Failed, p.statusMessage);
        return false;
    }
}

bool PluginManager::uninstallPlugin(const QString& id)
{
    int index = -1;
    for (int i = 0; i < m_plugins.size(); ++i) {
        if (m_plugins[i].id == id) {
            index = i;
            break;
        }
    }
    if (index == -1) return false;

    PluginInfo& info = m_plugins[index];
    QString appDir = QCoreApplication::applicationDirPath();
    QString targetDir = appDir + "/" + info.installSubdir;

    bool removed = false;
    QDir dir(targetDir);

    if (info.id == "translation-en-zh") {
        // 清理 translation 目录下的可执行文件和模型文件
        if (dir.exists()) {
            QDir modelsDir(targetDir + "/models");
            if (modelsDir.exists()) modelsDir.removeRecursively();
            QFile::remove(targetDir + "/translator-engine.exe");
            QFile::remove(targetDir + "/offline-translator.exe");
            QFile::remove(targetDir + "/ctranslate2-runner.exe");
            QFile::remove(targetDir + "/config.json");
            removed = true;
        }
    } else if (info.id == "rapidocr-engine") {
        if (dir.exists()) {
            QFile::remove(targetDir + "/RapidOCR-json.exe");
            QFile::remove(targetDir + "/rapidocr.exe");
            removed = true;
        }
        QDir oldDir(appDir + "/rapidocr");
        if (oldDir.exists()) {
            QFile::remove(appDir + "/rapidocr/RapidOCR-json.exe");
            QFile::remove(appDir + "/rapidocr/rapidocr.exe");
            removed = true;
        }
    } else {
        QString checkFile = targetDir + "/" + info.checkRelativeFile;
        if (QFile::exists(checkFile)) {
            QFile::remove(checkFile);
            removed = true;
        } else if (QDir(checkFile).exists()) {
            QDir(checkFile).removeRecursively();
            removed = true;
        }
    }

    updateSinglePluginStatus(info);
    info.status = PluginStatus::NotInstalled;
    info.statusMessage = "已卸载";
    info.progressPercent = 0;
    emit pluginStatusChanged(id, PluginStatus::NotInstalled, info.statusMessage);

    // 热重载翻译插件管理器
    TranslationPluginManager::instance().refreshPlugins();

    return removed;
}

bool PluginManager::extractZipArchive(const QString& zipPath, const QString& destDir)
{
    QDir().mkpath(destDir);

    // 方案 1: Windows 10/11 内置的高性能 bsdtar (无任何外部依赖，极速秒解)
    QProcess process;
    QStringList args;
    args << "-xf" << QDir::toNativeSeparators(zipPath)
         << "-C" << QDir::toNativeSeparators(destDir);

    process.start("tar.exe", args);
    if (process.waitForFinished(60000) && process.exitCode() == 0) {
        return true;
    }

    // 方案 2: 若 tar 异常，优雅降级为 PowerShell Expand-Archive
    QString psCmd = QString("Expand-Archive -Path '%1' -DestinationPath '%2' -Force")
                    .arg(QDir::toNativeSeparators(zipPath), QDir::toNativeSeparators(destDir));
    QProcess ps;
    ps.start("powershell.exe", {"-NoProfile", "-NonInteractive", "-Command", psCmd});
    if (ps.waitForFinished(90000) && ps.exitCode() == 0) {
        return true;
    }

    return false;
}
