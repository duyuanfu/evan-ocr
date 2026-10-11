#pragma once

#include <QString>

class AutoStartHelper
{
public:
    // 检查当前是否已在注册表设置开机自启动
    static bool isAutoStartEnabled();

    // 启用或禁用开机自启动 (写入/移除当前用户 Run 注册表键)
    static void setAutoStartEnabled(bool enabled);

    // 应用程序初次启动初始化：默认开启开机自启动
    static void ensureInitialDefaultSetup();
};
