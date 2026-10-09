#pragma once

#include "plugin_types.h"
#include <QObject>
#include <QList>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QElapsedTimer>
#include <memory>

class PluginManager : public QObject
{
    Q_OBJECT
public:
    static PluginManager& instance();

    // 获取所有可用插件清单
    QList<PluginInfo> plugins() const;

    // 获取单个插件信息
    PluginInfo pluginInfo(const QString& id) const;

    // 重新扫描并更新本地已安装状态
    void refreshAllStatus();

    // 一键下载并自动解压安装插件
    void installPlugin(const QString& id);

    // 从本地下载的 ZIP 压缩包手动安装插件 (类似 VSCode 的 Install from VSIX...)
    bool installFromLocalZip(const QString& zipPath, const QString& targetPluginId = "translation-en-zh");

    // 取消正在进行的下载
    void cancelDownload(const QString& id);

    // 一键卸载已安装的插件
    bool uninstallPlugin(const QString& id);

    // 获取插件根目录绝对路径 (如 plugins/)
    QString pluginRootDir() const;

signals:
    void pluginsRefreshed();
    void pluginStatusChanged(const QString& id, PluginStatus status, const QString& message);
    void pluginProgressChanged(const QString& id, int percent, qint64 downloaded, qint64 total, double speedMBs);

private:
    explicit PluginManager(QObject* parent = nullptr);
    ~PluginManager() override = default;

    void initCatalog();
    void updateSinglePluginStatus(PluginInfo& info);
    bool extractZipArchive(const QString& zipPath, const QString& destDir);

    QList<PluginInfo> m_plugins;
    QMap<QString, QNetworkReply*> m_activeReplies;
    QMap<QString, QFile*> m_activeFiles;
    QMap<QString, QElapsedTimer> m_speedTimers;
    QMap<QString, qint64> m_lastBytes;
    std::unique_ptr<QNetworkAccessManager> m_nam;
};
