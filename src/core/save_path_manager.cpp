#include "save_path_manager.h"
#include <QSettings>
#include <QDir>
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
    QSettings settings("Evan", "Evan");
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
    QSettings settings("Evan", "Evan");
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
    QSettings settings("Evan", "Evan");
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

void SavePathManager::rememberSavedPath(const QString& filePath)
{
    if (!m_rememberLast) return;
    if (filePath.isEmpty()) return;

    QString dir = QFileInfo(filePath).absolutePath();
    if (dir.isEmpty() || dir == m_lastUsedDirectory) return;

    m_lastUsedDirectory = dir;
    QSettings settings("Evan", "Evan");
    settings.setValue("SavePath/LastUsedDirectory", dir);
}

void SavePathManager::resetToDefault()
{
    setCustomSaveDirectory(QString());
    setRememberLastDirectory(true);
    m_lastUsedDirectory.clear();
    QSettings settings("Evan", "Evan");
    settings.remove("SavePath/LastUsedDirectory");
}
