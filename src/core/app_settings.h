#pragma once

#include <QString>

class QSettings;

/**
 * @brief 便携式（真绿色）数据目录
 *
 * Evan 采用完全便携的数据存储策略，**不写注册表、不留系统残留**：
 *
 * | 数据 | 位置 |
 * | :--- | :--- |
 * | 配置 | `<程序目录>/config/evan.ini` |
 * | 日志 | `<程序目录>/logs/evan.log` |
 *
 * 若程序目录不可写（例如被放在 `C:\Program Files`），自动回退到
 * `%APPDATA%/Evan/Evan/`，保证功能可用而不是静默失败。
 */
QString portableDataRoot();

// 配置文件完整路径 `<数据根目录>/config/evan.ini`（目录会自动创建）
QString portableConfigPath();

// 日志文件完整路径 `<数据根目录>/logs/evan.log`（目录会自动创建）
QString portableLogPath();

/**
 * @brief 全局配置对象（单例，IniFormat）
 *
 * 首次调用时会自动完成两件事：
 *  1. 从旧版本的注册表 `HKCU\Software\Evan\Evan` 一次性迁移全部设置；
 *  2. 清理旧版本遗留的注册表配置树与「开机自启」启动项，保证真绿色无残留。
 */
QSettings& appSettings();
