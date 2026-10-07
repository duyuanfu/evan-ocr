#include "theme_manager.h"
#include <QSettings>
#include <QGuiApplication>
#include <QPalette>
#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

ThemeManager::ThemeManager(QObject* parent)
    : QObject(parent)
{
    // 从用户配置读取
    QSettings settings("Evan", "Evan");
    QString modeStr = settings.value("Theme/Mode", "Auto").toString();
    if (modeStr == "Light") {
        m_themeMode = ThemeMode::Light;
    } else if (modeStr == "Dark") {
        m_themeMode = ThemeMode::Dark;
    } else {
        m_themeMode = ThemeMode::Auto;
    }
}

ThemeManager& ThemeManager::instance()
{
    static ThemeManager mgr;
    return mgr;
}

void ThemeManager::setThemeMode(ThemeMode mode)
{
    if (m_themeMode != mode) {
        m_themeMode = mode;
        QSettings settings("Evan", "Evan");
        QString str = (mode == ThemeMode::Light) ? "Light" :
                      (mode == ThemeMode::Dark)  ? "Dark" : "Auto";
        settings.setValue("Theme/Mode", str);
        emit themeChanged(isDarkMode());
    }
}

bool ThemeManager::isDarkMode() const
{
    if (m_themeMode == ThemeMode::Light) return false;
    if (m_themeMode == ThemeMode::Dark)  return true;
    return detectSystemIsDark();
}

bool ThemeManager::detectSystemIsDark() const
{
#ifdef Q_OS_WIN
    // 查询 Windows 注册表判断当前系统是否为浅色应用模式
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD value = 1;
        DWORD size = sizeof(value);
        if (RegQueryValueExW(hKey, L"AppsUseLightTheme", nullptr, nullptr,
                             reinterpret_cast<LPBYTE>(&value), &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return (value == 0); // 0 为深色，1 为浅色
        }
        RegCloseKey(hKey);
    }
#endif

    // 回退机制：根据 Qt 系统调色板亮度判定
    return QGuiApplication::palette().color(QPalette::Window).lightness() < 128;
}

QString ThemeManager::getTrayMenuStyle() const
{
    if (isDarkMode()) {
        return "QMenu { background-color: #242424; color: #ffffff; border: 1px solid #3c3c3c; padding: 4px; }"
               "QMenu::item { padding: 6px 24px; border-radius: 4px; font-size: 12px; }"
               "QMenu::item:selected { background-color: #0078d7; color: #ffffff; }"
               "QMenu::separator { height: 1px; background: #383838; margin: 4px 6px; }";
    } else {
        return "QMenu { background-color: #ffffff; color: #1e293b; border: 1px solid #e2e8f0; padding: 4px; border-radius: 6px; }"
               "QMenu::item { padding: 6px 24px; border-radius: 4px; font-size: 12px; }"
               "QMenu::item:selected { background-color: #2563eb; color: #ffffff; }"
               "QMenu::separator { height: 1px; background: #f1f5f9; margin: 4px 6px; }";
    }
}

QString ThemeManager::getPinMenuStyle() const
{
    if (isDarkMode()) {
        return "QMenu { background-color: #242424; color: #ffffff; border: 1px solid #3c3c3c; padding: 4px; }"
               "QMenu::item { padding: 5px 20px; border-radius: 3px; }"
               "QMenu::item:selected { background-color: #0078d7; color: #ffffff; }"
               "QMenu::separator { height: 1px; background: #383838; margin: 4px 6px; }";
    } else {
        return "QMenu { background-color: #ffffff; color: #1e293b; border: 1px solid #e2e8f0; padding: 4px; border-radius: 6px; }"
               "QMenu::item { padding: 5px 20px; border-radius: 3px; }"
               "QMenu::item:selected { background-color: #2563eb; color: #ffffff; }"
               "QMenu::separator { height: 1px; background: #f1f5f9; margin: 4px 6px; }";
    }
}

QString ThemeManager::getFloatingToolbarStyle() const
{
    if (isDarkMode()) {
        return "QWidget { background-color: #1f1f23; border: 1px solid #383838; border-radius: 6px; }";
    } else {
        return "QWidget { background-color: #ffffff; border: 1px solid #e2e8f0; border-radius: 6px; }";
    }
}

QString ThemeManager::getSettingsDialogStyle() const
{
    if (isDarkMode()) {
        return "QDialog { background-color: #18181b; color: #f4f4f5; }"
               "QTabWidget::pane { border: 1px solid #3f3f46; border-radius: 8px; background-color: #1f1f23; top: -1px; }"
               "QTabBar::tab { background-color: #27272a; color: #a1a1aa; padding: 8px 16px; margin-right: 4px; border-top-left-radius: 6px; border-top-right-radius: 6px; font-size: 13px; font-weight: 500; }"
               "QTabBar::tab:selected { background-color: #1f1f23; color: #ffffff; border-top: 2px solid #3b82f6; }"
               "QTabBar::tab:hover:!selected { background-color: #3f3f46; color: #e4e4e7; }"
               "QLabel { color: #e4e4e7; font-size: 13px; }"
               "QLabel#rowDesc { color: #71717a; font-size: 11px; }"
               "QKeySequenceEdit { background-color: #27272a; color: #ffffff; border: 1px solid #3f3f46; border-radius: 6px; padding: 6px 10px; font-size: 13px; font-weight: bold; }"
               "QKeySequenceEdit:focus { border-color: #3b82f6; }"
               "QComboBox { background-color: #27272a; color: #ffffff; border: 1px solid #3f3f46; border-radius: 6px; padding: 6px 10px; font-size: 13px; font-weight: 500; }"
               "QCheckBox { color: #e4e4e7; font-size: 13px; spacing: 8px; }"
               "QPushButton { background-color: #27272a; color: #e4e4e7; border: 1px solid #3f3f46; border-radius: 6px; padding: 6px 14px; font-size: 12px; font-weight: 500; }"
               "QPushButton:hover { background-color: #3f3f46; color: #ffffff; }"
               "QPushButton#clearBtn { background-color: transparent; border: none; color: #71717a; font-size: 12px; }"
               "QPushButton#clearBtn:hover { color: #ef4444; }"
               "QPushButton#saveBtn { background-color: #2563eb; border-color: #3b82f6; color: #ffffff; font-weight: 600; }"
               "QPushButton#saveBtn:hover { background-color: #1d4ed8; }"
               "QScrollArea { border: none; background: transparent; }";
    } else {
        return "QDialog { background-color: #f8fafc; color: #0f172a; }"
               "QTabWidget::pane { border: 1px solid #e2e8f0; border-radius: 8px; background-color: #ffffff; top: -1px; }"
               "QTabBar::tab { background-color: #f1f5f9; color: #64748b; padding: 8px 16px; margin-right: 4px; border-top-left-radius: 6px; border-top-right-radius: 6px; font-size: 13px; font-weight: 500; }"
               "QTabBar::tab:selected { background-color: #ffffff; color: #0f172a; border-top: 2px solid #2563eb; }"
               "QTabBar::tab:hover:!selected { background-color: #e2e8f0; color: #1e293b; }"
               "QLabel { color: #1e293b; font-size: 13px; }"
               "QLabel#rowDesc { color: #64748b; font-size: 11px; }"
               "QKeySequenceEdit { background-color: #ffffff; color: #0f172a; border: 1px solid #cbd5e1; border-radius: 6px; padding: 6px 10px; font-size: 13px; font-weight: bold; }"
               "QKeySequenceEdit:focus { border-color: #2563eb; }"
               "QComboBox { background-color: #ffffff; color: #0f172a; border: 1px solid #cbd5e1; border-radius: 6px; padding: 6px 10px; font-size: 13px; font-weight: 500; }"
               "QCheckBox { color: #1e293b; font-size: 13px; spacing: 8px; }"
               "QPushButton { background-color: #ffffff; color: #334155; border: 1px solid #cbd5e1; border-radius: 6px; padding: 6px 14px; font-size: 12px; font-weight: 500; }"
               "QPushButton:hover { background-color: #f1f5f9; color: #0f172a; border-color: #94a3b8; }"
               "QPushButton#clearBtn { background-color: transparent; border: none; color: #94a3b8; font-size: 12px; }"
               "QPushButton#clearBtn:hover { color: #ef4444; }"
               "QPushButton#saveBtn { background-color: #2563eb; border-color: #1d4ed8; color: #ffffff; font-weight: 600; }"
               "QPushButton#saveBtn:hover { background-color: #1d4ed8; }"
               "QScrollArea { border: none; background: transparent; }";
    }
}
