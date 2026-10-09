#pragma once

#include <QString>
#include <QList>

enum class PluginStatus {
    NotInstalled,       // 未安装
    Downloading,        // 正在下载中
    Extracting,         // 正在解压部署
    Installed,          // 已安装就绪
    Failed              // 失败异常
};

struct PluginInfo {
    QString id;                  // 唯一标识符, 如 "translation-en-zh"
    QString name;                // 插件显示名
    QString version;             // 插件版本号, 如 "v1.0.0"
    QString category;            // 插件分类 ("翻译扩展", "OCR模型", "辅助工具")
    QString iconEmoji;           // 图标表情符号 ("🌐", "🔍", "🎨")
    QString description;         // 详细功能说明
    QString sizeDisplay;         // 显示大小 ("38 MB")
    qint64 sizeBytes = 0;        // 预估字节数
    QString downloadUrl;         // 下载链接 (GitHub Releases / CDN)
    QString installSubdir;       // 安装相对目录 ("plugins/translation")
    QString checkRelativeFile;   // 核心检查文件 ("translator-engine.exe" 或 "models")

    // 运行时动态状态
    PluginStatus status = PluginStatus::NotInstalled;
    int progressPercent = 0;     // 进度百分比 (0-100)
    qint64 downloadedBytes = 0;
    qint64 totalBytes = 0;
    double speedBytesPerSec = 0.0;
    QString statusMessage;
};
