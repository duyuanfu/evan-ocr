#include "app_settings.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {

// 目录可写性探测（避免直接假定程序目录可写）
bool dirWritable(const QString& dir)
{
    if (dir.isEmpty()) return false;
    QFile probe(QDir(dir).filePath(QStringLiteral(".evan_write_probe")));
    if (!probe.open(QIODevice::WriteOnly)) return false;
    probe.write("ok");
    probe.close();
    probe.remove();
    return true;
}

/**
 * @brief 清理旧版本写入系统的痕迹
 *
 * - 删除 `HKCU\...\CurrentVersion\Run\Evan`（旧版本默认开启的开机自启）
 * - 删除 `HKCU\Software\Evan`（旧版本的注册表配置树）
 *
 * 仅在设置成功迁移到 INI 之后调用，避免误删用户数据。
 */
void cleanupLegacyRegistry()
{
#ifdef Q_OS_WIN
    HKEY hRun = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                      0, KEY_SET_VALUE, &hRun) == ERROR_SUCCESS) {
        RegDeleteValueW(hRun, L"Evan");
        RegCloseKey(hRun);
    }
    RegDeleteTreeW(HKEY_CURRENT_USER, L"Software\\Evan");
#endif
}

/**
 * @brief 一次性把旧版本注册表或旧位置配置迁移到便携 INI
 */
void migrateLegacySettingsOnce()
{
    const QString iniPath = portableConfigPath();

    {
        QSettings ini(iniPath, QSettings::IniFormat);

        // 如果 INI 已存在且不仅包含迁移标记，说明已有完整便携配置
        const QStringList curKeys = ini.allKeys();
        const bool alreadyConfigured = !curKeys.isEmpty() && 
            (curKeys.size() > 1 || curKeys.first() != QStringLiteral("General/PortableMigrated"));

        if (!alreadyConfigured) {
            // 1. 优先尝试从旧版注册表迁移 (HKCU\Software\Evan\Evan)
            {
                QSettings legacy(QStringLiteral("Evan"), QStringLiteral("Evan"));
                const QStringList regKeys = legacy.allKeys();
                for (const QString& key : regKeys) {
                    ini.setValue(key, legacy.value(key));
                }
            }

            // 2. 若注册表为空，尝试从 AppData 遗留的旧 INI 文件迁移
            if (ini.allKeys().isEmpty() || 
                (ini.allKeys().size() == 1 && ini.contains(QStringLiteral("General/PortableMigrated")))) {
                const QStringList candidatePaths = {
                    QDir::homePath() + QStringLiteral("/AppData/Roaming/config/evan.ini"),
                    QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/config/evan.ini"),
                    QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/Evan/config/evan.ini"),
                };
                for (const QString& cand : candidatePaths) {
                    if (cand != iniPath && QFile::exists(cand)) {
                        QSettings oldIni(cand, QSettings::IniFormat);
                        for (const QString& key : oldIni.allKeys()) {
                            ini.setValue(key, oldIni.value(key));
                        }
                        QFile::remove(cand);
                        // 顺便清理空的父目录
                        QDir(QFileInfo(cand).absolutePath()).rmdir(QString());
                        break;
                    }
                }
            }

            // 开机自启已彻底移除，相关遗留键不再迁移
            ini.remove(QStringLiteral("General/AutoStart"));
            ini.remove(QStringLiteral("General/AutoStartInitialized"));
            ini.setValue(QStringLiteral("General/PortableMigrated"), true);
            ini.sync();
        }
    }

    // 迁移落盘成功后清理注册表，避免旧痕迹残留
    if (QFile::exists(iniPath)) {
        cleanupLegacyRegistry();
    }
}

} // namespace

QString portableDataRoot()
{
    static const QString root = []() -> QString {
        QString appDir;
#ifdef Q_OS_WIN
        wchar_t buf[MAX_PATH] = {0};
        if (GetModuleFileNameW(nullptr, buf, MAX_PATH) > 0) {
            appDir = QFileInfo(QString::fromWCharArray(buf)).absolutePath();
        }
#endif
        if (appDir.isEmpty()) {
            appDir = QCoreApplication::applicationDirPath();
        }
        if (dirWritable(appDir)) return appDir;

        // 回退：程序目录不可写（如放在 Program Files）
        const QString fallback = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (!fallback.isEmpty()) {
            QDir().mkpath(fallback);
            if (dirWritable(fallback)) return fallback;
        }
        return appDir;
    }();
    return root;
}

QString portableConfigPath()
{
    const QString dir = QDir(portableDataRoot()).filePath(QStringLiteral("config"));
    QDir().mkpath(dir);
    return QDir(dir).filePath(QStringLiteral("evan.ini"));
}

QString portableLogPath()
{
    const QString dir = QDir(portableDataRoot()).filePath(QStringLiteral("logs"));
    QDir().mkpath(dir);
    return QDir(dir).filePath(QStringLiteral("evan.log"));
}

QSettings& appSettings()
{
    static QSettings* settings = []() -> QSettings* {
        migrateLegacySettingsOnce();
        return new QSettings(portableConfigPath(), QSettings::IniFormat);
    }();
    return *settings;
}
