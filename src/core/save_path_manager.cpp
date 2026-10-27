#include "save_path_manager.h"
#include "app_settings.h"
#include <QSettings>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

SavePathManager& SavePathManager::instance()
{
    static SavePathManager mgr;
    return mgr;
}

SavePathManager::SavePathManager(QObject* parent)
    : QObject(parent)
{
    QSettings& settings = appSettings();
    m_customDirectory = settings.value("SavePath/CustomDirectory", "").toString();
    m_lastUsedDirectory = settings.value("SavePath/LastUsedDirectory", "").toString();
    m_rememberLast = settings.value("SavePath/RememberLastDirectory", true).toBool();
}

QString SavePathManager::customSaveDirectory() const
{
    return m_customDirectory;
}

void SavePathManager::setCustomSaveDirectory(const QString& dir)
{
    QString normalized = dir.trimmed();
    if (!normalized.isEmpty()) {
        normalized = QDir::toNativeSeparators(QDir(normalized).absolutePath());
    }
    if (m_customDirectory == normalized) return;

    m_customDirectory = normalized;
    QSettings& settings = appSettings();
    settings.setValue("SavePath/CustomDirectory", m_customDirectory);
    emit customDirectoryChanged(m_customDirectory);
}

bool SavePathManager::rememberLastDirectory() const
{
    return m_rememberLast;
}

void SavePathManager::setRememberLastDirectory(bool enable)
{
    if (m_rememberLast == enable) return;
    m_rememberLast = enable;
    QSettings& settings = appSettings();
    settings.setValue("SavePath/RememberLastDirectory", enable);
    if (!enable) {
        m_lastUsedDirectory.clear();
        settings.remove("SavePath/LastUsedDirectory");
    }
    emit rememberDirectoryChanged(enable);
}

QString SavePathManager::lastUsedDirectory() const
{
    return m_lastUsedDirectory;
}

QString SavePathManager::buildSavePath(const QString& defaultName, QString* outDir) const
{
    QString dir;

    // 优先级 1: 用户显式指定的自定义保存目录
    if (!m_customDirectory.isEmpty() && QDir(m_customDirectory).exists()) {
        dir = m_customDirectory;
    }
    // 优先级 2: 自动记忆上次保存位置
    else if (m_rememberLast && !m_lastUsedDirectory.isEmpty() && QDir(m_lastUsedDirectory).exists()) {
        dir = m_lastUsedDirectory;
    }
    // 优先级 3: 系统图片目录 (首次使用体验最佳)
    else {
        dir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
        if (dir.isEmpty()) {
            dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        }
        if (dir.isEmpty()) {
            dir = QDir::homePath();
        }
    }

    if (outDir) *outDir = dir;
    return QDir(dir).filePath(defaultName);
}

bool SavePathManager::isAutoSaveEnabled() const
{
    // 目录必须存在且可写，否则降级回退到「保存对话框」流程
    if (m_customDirectory.isEmpty()) return false;
    const QDir dir(m_customDirectory);
    if (!dir.exists()) return false;
    QFileInfo info(dir.absolutePath());
    return info.isDir() && info.isWritable();
}

QString SavePathManager::buildUniqueSavePath(const QString& defaultName, QString* outDir) const
{
    QString dir;
    const QString base = buildSavePath(defaultName, &dir);
    if (outDir) *outDir = dir;

    if (!QFile::exists(base)) return base;

    // 同名文件已存在：按 _1 / _2 递增避让
    const QFileInfo fi(base);
    const QString suffix = fi.suffix();
    const QString stem = fi.completeBaseName();
    for (int i = 1; i < 1000; ++i) {
        const QString candidate = dir + QDir::separator()
            + stem + QString("_%1").arg(i) + (suffix.isEmpty() ? QString() : "." + suffix);
        if (!QFile::exists(candidate)) return candidate;
    }
    return base;
}

void SavePathManager::rememberSavedPath(const QString& filePath)
{
    if (!m_rememberLast) return;
    if (filePath.isEmpty()) return;

    QString dir = QFileInfo(filePath).absolutePath();
    if (dir.isEmpty() || dir == m_lastUsedDirectory) return;

    m_lastUsedDirectory = dir;
    QSettings& settings = appSettings();
    settings.setValue("SavePath/LastUsedDirectory", dir);
}

void SavePathManager::resetToDefault()
{
    setCustomSaveDirectory(QString());
    setRememberLastDirectory(true);
    m_lastUsedDirectory.clear();
    QSettings& settings = appSettings();
    settings.remove("SavePath/LastUsedDirectory");
}
