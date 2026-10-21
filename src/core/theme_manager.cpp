#include "theme_manager.h"
#include <QSettings>
#include <QGuiApplication>
#include <QPalette>
#include <QWidget>
#include <QEvent>
#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>
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
        return "QDialog { background-color: #18181b; color: #f4f4f5; font-family: 'Segoe UI', 'Microsoft YaHei UI', sans-serif; }"
               "QLabel#dialogHeaderTitle { font-size: 18px; font-weight: 700; color: #f4f4f5; font-family: 'Segoe UI Variable Display', 'Segoe UI', 'Microsoft YaHei UI', sans-serif; }"
               "QLabel#dialogHeaderSubtitle { font-size: 12px; color: #a1a1aa; }"
               "QTabWidget::pane { border: none; background: transparent; top: 0px; }"
               "QTabBar { background-color: transparent; qproperty-drawBase: 0; }"
               "QTabBar::tab { background: transparent; color: #a1a1aa; font-size: 14px; font-weight: 600; padding: 8px 18px 9px 18px; margin-right: 16px; border: none; border-bottom: 3px solid transparent; }"
               "QTabBar::tab:selected { color: #ffffff; border-bottom: 3px solid #3b82f6; }"
               "QTabBar::tab:hover:!selected { color: #e4e4e7; }"
               "QScrollArea { border: none; background-color: transparent; }"
               "QScrollArea > QWidget > QWidget { background-color: transparent; }"
               "#settingsCard { background-color: #242429; border: 1px solid #3c3c46; border-radius: 10px; }"
               "#settingsCard:hover { border-color: #525260; }"
               "#settingIconBadge { background-color: #2e2e36; border: 1px solid #3e3e4a; border-radius: 8px; font-size: 18px; min-width: 36px; max-width: 36px; min-height: 36px; max-height: 36px; }"
               "#settingRowTitle { font-size: 13px; font-weight: 600; color: #f4f4f5; }"
               "QLabel { color: #e4e4e7; font-size: 13px; }"
               "QLabel#cardTitle { color: #38bdf8; font-size: 13px; font-weight: bold; }"
               "QLabel#rowDesc { color: #a1a1aa; font-size: 11px; line-height: 1.4; }"
               "QLineEdit { background-color: #18181b; color: #ffffff; border: 1px solid #3f3f46; border-radius: 6px; padding: 6px 10px; font-size: 13px; min-height: 24px; }"
               "QLineEdit:hover { border-color: #52525b; }"
               "QLineEdit:focus { border-color: #3b82f6; background-color: #1f1f23; }"
               "QKeySequenceEdit { background-color: #18181b; color: #ffffff; border: 1px solid #3f3f46; border-radius: 6px; padding: 6px 10px; font-size: 13px; font-weight: bold; min-height: 24px; }"
               "QKeySequenceEdit:hover { border-color: #52525b; }"
               "QKeySequenceEdit:focus { border-color: #3b82f6; }"
               "QComboBox { background-color: #2c2c33; color: #f4f4f5; border: 1px solid #44444e; border-radius: 6px; padding: 5px 32px 5px 12px; font-size: 12px; font-weight: 500; min-height: 24px; }"
               "QComboBox:hover { border-color: #3b82f6; background-color: #32323a; }"
               "QComboBox:focus { border-color: #3b82f6; background-color: #2c2c33; }"
               "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: top right; width: 28px; border-left: 1px solid #44444e; border-top-right-radius: 6px; border-bottom-right-radius: 6px; background-color: #32323a; }"
               "QComboBox::drop-down:hover { background-color: #3c3c46; }"
               "QComboBox::down-arrow { image: url(:/icons/chevron_down_dark.svg); width: 12px; height: 12px; }"
               "QComboBox QAbstractItemView { background-color: #222227; color: #f4f4f5; border: 1px solid #44444e; border-radius: 8px; padding: 4px; outline: none; selection-background-color: #3b82f6; selection-color: #ffffff; }"
               "QComboBox QAbstractItemView::item { background-color: #222227; color: #f4f4f5; min-height: 28px; padding: 4px 10px; border-radius: 4px; }"
               "QComboBox QAbstractItemView::item:hover { background-color: #32323a; color: #ffffff; }"
               "QComboBox QAbstractItemView::item:selected { background-color: #3b82f6; color: #ffffff; }"
               "QCheckBox { color: #e4e4e7; font-size: 13px; spacing: 10px; font-weight: 500; }"
               "QCheckBox::indicator { width: 18px; height: 18px; border: 1px solid #52525b; border-radius: 4px; background-color: #18181b; }"
               "QCheckBox::indicator:hover { border-color: #3b82f6; }"
               "QCheckBox::indicator:checked { background-color: #3b82f6; border-color: #3b82f6; image: url(:/icons/checkmark_white.png); }"
               "QPushButton { background-color: #27272a; color: #e4e4e7; border: 1px solid #3f3f46; border-radius: 6px; padding: 6px 16px; font-size: 12px; font-weight: 500; min-height: 22px; }"
               "QPushButton:hover { background-color: #3f3f46; color: #ffffff; }"
               "QPushButton#cardActionBtn, QPushButton#testProxyBtn, QPushButton#checkUpdateBtn { background-color: #2563eb; color: #ffffff; border: 1px solid #3b82f6; border-radius: 6px; padding: 6px 16px; font-size: 12px; font-weight: 600; min-height: 24px; }"
               "QPushButton#cardActionBtn:hover, QPushButton#testProxyBtn:hover, QPushButton#checkUpdateBtn:hover { background-color: #1d4ed8; border-color: #60a5fa; }"
               "QPushButton#cardActionBtn:pressed, QPushButton#testProxyBtn:pressed, QPushButton#checkUpdateBtn:pressed { background-color: #1e40af; }"
               "QPushButton#cardActionBtn:disabled, QPushButton#testProxyBtn:disabled, QPushButton#checkUpdateBtn:disabled { background-color: #2e2e34; color: #71717a; border-color: #3f3f46; }"
               "QPushButton#clearBtn { background-color: transparent; border: none; color: #71717a; font-size: 12px; padding: 0px; }"
               "QPushButton#clearBtn:hover { color: #ef4444; }"
               "QPushButton#saveBtn { background-color: #2563eb; border: 1px solid #3b82f6; color: #ffffff; font-weight: 600; min-height: 24px; padding: 6px 20px; }"
               "QPushButton#saveBtn:hover { background-color: #1d4ed8; border-color: #2563eb; }"
               "QPushButton#cancelBtn { background-color: #27272a; border: 1px solid #3f3f46; color: #a1a1aa; min-height: 24px; padding: 6px 18px; }"
               "QPushButton#cancelBtn:hover { background-color: #3f3f46; color: #ffffff; }"
               "QPushButton#resetBtn { background-color: transparent; border: 1px solid transparent; color: #71717a; min-height: 24px; padding: 6px 12px; }"
               "QPushButton#resetBtn:hover { background-color: #27272a; border-color: #3f3f46; color: #e4e4e7; }"
               "QScrollBar:vertical { background-color: transparent; width: 6px; margin: 4px 2px 4px 0px; border-radius: 3px; }"
               "QScrollBar:vertical:hover { width: 8px; }"
               "QScrollBar::handle:vertical { background-color: #52525b; min-height: 36px; border-radius: 3px; }"
               "QScrollBar::handle:vertical:hover { background-color: #71717a; }"
               "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; background: none; border: none; }"
               "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }";
    } else {
        return "QDialog { background-color: #f8fafc; color: #0f172a; font-family: 'Segoe UI', 'Microsoft YaHei UI', sans-serif; }"
               "QLabel#dialogHeaderTitle { font-size: 18px; font-weight: 700; color: #0f172a; font-family: 'Segoe UI Variable Display', 'Segoe UI', 'Microsoft YaHei UI', sans-serif; }"
               "QLabel#dialogHeaderSubtitle { font-size: 12px; color: #64748b; }"
               "QTabWidget::pane { border: none; background: transparent; top: 0px; }"
               "QTabBar { background-color: transparent; qproperty-drawBase: 0; }"
               "QTabBar::tab { background: transparent; color: #64748b; font-size: 14px; font-weight: 600; padding: 8px 18px 9px 18px; margin-right: 16px; border: none; border-bottom: 3px solid transparent; }"
               "QTabBar::tab:selected { color: #0f172a; border-bottom: 3px solid #2563eb; }"
               "QTabBar::tab:hover:!selected { color: #1e293b; }"
               "QScrollArea { border: none; background-color: transparent; }"
               "QScrollArea > QWidget > QWidget { background-color: transparent; }"
               "#settingsCard { background-color: #ffffff; border: 1px solid #e2e8f0; border-radius: 10px; }"
               "#settingsCard:hover { border-color: #cbd5e1; }"
               "#settingIconBadge { background-color: #f1f5f9; border: 1px solid #e2e8f0; border-radius: 8px; font-size: 18px; min-width: 36px; max-width: 36px; min-height: 36px; max-height: 36px; }"
               "#settingRowTitle { font-size: 13px; font-weight: 600; color: #0f172a; }"
               "QLabel { color: #1e293b; font-size: 13px; }"
               "QLabel#cardTitle { color: #2563eb; font-size: 13px; font-weight: bold; }"
               "QLabel#rowDesc { color: #64748b; font-size: 11px; line-height: 1.4; }"
               "QLineEdit { background-color: #ffffff; color: #0f172a; border: 1px solid #cbd5e1; border-radius: 6px; padding: 6px 10px; font-size: 13px; min-height: 24px; }"
               "QLineEdit:focus { border-color: #2563eb; }"
               "QKeySequenceEdit { background-color: #ffffff; color: #0f172a; border: 1px solid #cbd5e1; border-radius: 6px; padding: 6px 10px; font-size: 13px; font-weight: bold; min-height: 24px; }"
               "QKeySequenceEdit:focus { border-color: #2563eb; }"
               "QComboBox { background-color: #ffffff; color: #0f172a; border: 1px solid #cbd5e1; border-radius: 6px; padding: 5px 32px 5px 12px; font-size: 13px; font-weight: 500; min-height: 24px; }"
               "QComboBox:hover { border-color: #2563eb; background-color: #f8fafc; }"
               "QComboBox:focus { border-color: #2563eb; background-color: #ffffff; }"
               "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: top right; width: 28px; border-left: 1px solid #e2e8f0; border-top-right-radius: 6px; border-bottom-right-radius: 6px; background-color: #f8fafc; }"
               "QComboBox::drop-down:hover { background-color: #f1f5f9; }"
               "QComboBox::down-arrow { image: url(:/icons/chevron_down_light.svg); width: 12px; height: 12px; }"
               "QComboBox QAbstractItemView { background-color: #ffffff; color: #0f172a; border: 1px solid #cbd5e1; border-radius: 6px; padding: 4px; outline: none; selection-background-color: #eff6ff; selection-color: #2563eb; }"
               "QComboBox QAbstractItemView::item { background-color: #ffffff; color: #0f172a; min-height: 28px; padding: 4px 10px; border-radius: 4px; }"
               "QComboBox QAbstractItemView::item:hover { background-color: #f1f5f9; color: #0f172a; }"
               "QComboBox QAbstractItemView::item:selected { background-color: #eff6ff; color: #2563eb; }"
               "QCheckBox { color: #1e293b; font-size: 13px; spacing: 10px; font-weight: 500; }"
               "QCheckBox::indicator { width: 18px; height: 18px; border: 1px solid #cbd5e1; border-radius: 4px; background-color: #ffffff; }"
               "QCheckBox::indicator:hover { border-color: #2563eb; }"
               "QCheckBox::indicator:checked { background-color: #2563eb; border-color: #2563eb; image: url(:/icons/checkmark_white.png); }"
               "QPushButton { background-color: #ffffff; color: #334155; border: 1px solid #cbd5e1; border-radius: 6px; padding: 6px 16px; font-size: 12px; font-weight: 500; min-height: 22px; }"
               "QPushButton:hover { background-color: #f1f5f9; color: #0f172a; border-color: #94a3b8; }"
               "QPushButton#cardActionBtn { background-color: #ffffff; color: #1e293b; border: 1px solid #cbd5e1; border-radius: 6px; padding: 6px 16px; font-size: 12px; font-weight: 600; min-height: 22px; }"
               "QPushButton#cardActionBtn:hover { background-color: #2563eb; border-color: #2563eb; color: #ffffff; }"
               "QPushButton#cardActionBtn:pressed { background-color: #1d4ed8; }"
               "QPushButton#cardActionBtn:disabled { background-color: #f1f5f9; color: #94a3b8; border-color: #e2e8f0; }"
               "QPushButton#clearBtn { background-color: transparent; border: none; color: #94a3b8; font-size: 12px; padding: 0px; }"
               "QPushButton#clearBtn:hover { color: #ef4444; }"
               "QPushButton#saveBtn { background-color: #2563eb; border: 1px solid #1d4ed8; color: #ffffff; font-weight: 600; min-height: 24px; padding: 6px 20px; }"
               "QPushButton#saveBtn:hover { background-color: #1d4ed8; }"
               "QPushButton#cancelBtn { background-color: #ffffff; border: 1px solid #cbd5e1; color: #64748b; min-height: 24px; padding: 6px 18px; }"
               "QPushButton#cancelBtn:hover { background-color: #f1f5f9; color: #0f172a; border-color: #94a3b8; }"
               "QPushButton#resetBtn { background-color: transparent; border: 1px solid transparent; color: #64748b; min-height: 24px; padding: 6px 12px; }"
               "QPushButton#resetBtn:hover { background-color: #f1f5f9; border-color: #e2e8f0; color: #0f172a; }"
               "QScrollBar:vertical { background-color: transparent; width: 6px; margin: 4px 2px 4px 0px; border-radius: 3px; }"
               "QScrollBar:vertical:hover { width: 8px; }"
               "QScrollBar::handle:vertical { background-color: #cbd5e1; min-height: 36px; border-radius: 3px; }"
               "QScrollBar::handle:vertical:hover { background-color: #94a3b8; }"
               "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; background: none; border: none; }"
               "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }";
    }
}

QString ThemeManager::getOcrDialogStyle() const
{
    if (isDarkMode()) {
        return "QDialog { background-color: #18181b; color: #f4f4f5; font-family: 'Segoe UI', 'Microsoft YaHei', sans-serif; }"
               "QSplitter::handle { background-color: #3f3f46; width: 3px; }"
               "QSplitter::handle:hover { background-color: #3b82f6; }"
               "QScrollArea { background-color: #27272a; border: 1px solid #3f3f46; border-radius: 8px; }"
               "QPlainTextEdit { background-color: #27272a; color: #f4f4f5; border: 1px solid #3f3f46; border-radius: 8px; padding: 10px 12px; font-family: 'Segoe UI', 'Microsoft YaHei', sans-serif; font-size: 13px; line-height: 1.5; selection-background-color: #1e3a8a; selection-color: #ffffff; }"
               "QPlainTextEdit:focus { border-color: #3b82f6; }"
               "QPushButton { background-color: #27272a; color: #e4e4e7; border: 1px solid #3f3f46; border-radius: 6px; padding: 5px 12px; font-size: 12px; font-weight: 500; }"
               "QPushButton:hover { background-color: #3f3f46; color: #ffffff; }"
               "QPushButton:checked { background-color: #1e3a8a; color: #93c5fd; border-color: #3b82f6; font-weight: 600; }"
               "QComboBox { background-color: #1f1f23; color: #f4f4f5; border: 1px solid #3f3f46; border-radius: 6px; padding: 4px 30px 4px 10px; font-size: 12px; font-weight: 500; min-height: 22px; }"
               "QComboBox:focus, QComboBox:hover { border-color: #3b82f6; background-color: #27272a; }"
               "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: top right; width: 24px; border-left: 1px solid #3f3f46; border-top-right-radius: 6px; border-bottom-right-radius: 6px; background-color: #27272a; }"
               "QComboBox::drop-down:hover { background-color: #3f3f46; }"
               "QComboBox::down-arrow { image: url(:/icons/chevron_down_dark.svg); width: 12px; height: 12px; }"
               "QComboBox QAbstractItemView { background-color: #1f1f23; color: #f4f4f5; border: 1px solid #3f3f46; border-radius: 6px; padding: 4px; outline: none; selection-background-color: #3b82f6; selection-color: #ffffff; }"
               "QComboBox QAbstractItemView::item { background-color: #1f1f23; color: #f4f4f5; min-height: 26px; padding: 4px 10px; border-radius: 4px; }"
               "QComboBox QAbstractItemView::item:hover { background-color: #3f3f46; color: #ffffff; }"
               "QComboBox QAbstractItemView::item:selected { background-color: #3b82f6; color: #ffffff; }"
               "QLabel { color: #e4e4e7; font-size: 12px; }"
               "QScrollBar:vertical { background-color: transparent; width: 6px; margin: 0px; }"
               "QScrollBar::handle:vertical { background-color: #52525b; min-height: 24px; border-radius: 3px; }"
               "QScrollBar::handle:vertical:hover { background-color: #71717a; }"
               "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; background: none; border: none; }"
               "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }";
    } else {
        return "QDialog { background-color: #f8fafc; color: #0f172a; font-family: 'Segoe UI', 'Microsoft YaHei', sans-serif; }"
               "QSplitter::handle { background-color: #e2e8f0; width: 3px; }"
               "QSplitter::handle:hover { background-color: #3b82f6; }"
               "QScrollArea { background-color: #ffffff; border: 1px solid #e2e8f0; border-radius: 8px; }"
               "QPlainTextEdit { background-color: #ffffff; color: #0f172a; border: 1px solid #e2e8f0; border-radius: 8px; padding: 10px 12px; font-family: 'Segoe UI', 'Microsoft YaHei', sans-serif; font-size: 13px; line-height: 1.5; selection-background-color: #bfdbfe; selection-color: #1e3a8a; }"
               "QPlainTextEdit:focus { border-color: #3b82f6; }"
               "QPushButton { background-color: #ffffff; color: #334155; border: 1px solid #cbd5e1; border-radius: 6px; padding: 5px 12px; font-size: 12px; font-weight: 500; }"
               "QPushButton:hover { background-color: #f1f5f9; color: #0f172a; border-color: #94a3b8; }"
               "QPushButton:checked { background-color: #eff6ff; color: #2563eb; border-color: #3b82f6; font-weight: 600; }"
               "QComboBox { background-color: #ffffff; color: #0f172a; border: 1px solid #cbd5e1; border-radius: 6px; padding: 4px 30px 4px 10px; font-size: 12px; font-weight: 500; min-height: 22px; }"
               "QComboBox:focus, QComboBox:hover { border-color: #2563eb; background-color: #f8fafc; }"
               "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: top right; width: 24px; border-left: 1px solid #e2e8f0; border-top-right-radius: 6px; border-bottom-right-radius: 6px; background-color: #f8fafc; }"
               "QComboBox::drop-down:hover { background-color: #f1f5f9; }"
               "QComboBox::down-arrow { image: url(:/icons/chevron_down_light.svg); width: 12px; height: 12px; }"
               "QComboBox QAbstractItemView { background-color: #ffffff; color: #0f172a; border: 1px solid #cbd5e1; border-radius: 6px; padding: 4px; outline: none; selection-background-color: #eff6ff; selection-color: #2563eb; }"
               "QComboBox QAbstractItemView::item { background-color: #ffffff; color: #0f172a; min-height: 26px; padding: 4px 10px; border-radius: 4px; }"
               "QComboBox QAbstractItemView::item:hover { background-color: #f1f5f9; color: #0f172a; }"
               "QComboBox QAbstractItemView::item:selected { background-color: #eff6ff; color: #2563eb; }"
               "QLabel { color: #1e293b; font-size: 12px; }";
    }
}

QString ThemeManager::getFormulaDialogStyle() const
{
    if (isDarkMode()) {
        return "QDialog { background-color: #18181b; color: #f4f4f5; }"
               "QSplitter::handle { background-color: #3f3f46; height: 3px; width: 3px; }"
               "QSplitter::handle:hover { background-color: #3b82f6; }"
               "QScrollArea { background-color: #27272a; border: 1px solid #3f3f46; border-radius: 8px; }"
               "QPlainTextEdit { background-color: #27272a; color: #38bdf8; border: 1px solid #3f3f46; border-radius: 8px; padding: 10px 14px; font-family: 'Consolas', 'Cascadia Code', 'Segoe UI', monospace; font-size: 13px; line-height: 1.6; selection-background-color: #1e3a8a; selection-color: #ffffff; }"
               "QPlainTextEdit:focus { border-color: #3b82f6; }"
               "QPushButton { background-color: #27272a; color: #e4e4e7; border: 1px solid #3f3f46; border-radius: 6px; padding: 6px 14px; font-size: 12px; font-weight: 500; }"
               "QPushButton:hover { background-color: #3f3f46; color: #ffffff; }"
               "QPushButton:pressed { background-color: #52525b; }"
               "QPushButton#wordBtn { background-color: #2563eb; border-color: #3b82f6; color: #ffffff; font-weight: bold; }"
               "QPushButton#wordBtn:hover { background-color: #1d4ed8; }"
               "QComboBox { background-color: #1f1f23; color: #f4f4f5; border: 1px solid #3f3f46; border-radius: 6px; padding: 5px 30px 5px 12px; font-size: 12px; font-weight: 500; min-height: 22px; }"
               "QComboBox:hover { border-color: #3b82f6; background-color: #27272a; }"
               "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: top right; width: 24px; border-left: 1px solid #3f3f46; border-top-right-radius: 6px; border-bottom-right-radius: 6px; background-color: #27272a; }"
               "QComboBox::drop-down:hover { background-color: #3f3f46; }"
               "QComboBox::down-arrow { image: url(:/icons/chevron_down_dark.svg); width: 12px; height: 12px; }"
               "QComboBox QAbstractItemView { background-color: #1f1f23; color: #f4f4f5; border: 1px solid #3f3f46; border-radius: 6px; padding: 4px; outline: none; selection-background-color: #3b82f6; selection-color: #ffffff; }"
               "QComboBox QAbstractItemView::item { background-color: #1f1f23; color: #f4f4f5; min-height: 26px; padding: 4px 10px; border-radius: 4px; }"
               "QComboBox QAbstractItemView::item:hover { background-color: #3f3f46; color: #ffffff; }"
               "QComboBox QAbstractItemView::item:selected { background-color: #3b82f6; color: #ffffff; }"
               "QLabel { color: #e4e4e7; }"
               "QScrollBar:vertical { background-color: transparent; width: 6px; margin: 0px; }"
               "QScrollBar::handle:vertical { background-color: #52525b; min-height: 24px; border-radius: 3px; }"
               "QScrollBar::handle:vertical:hover { background-color: #71717a; }"
               "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; background: none; border: none; }"
               "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }";
    } else {
        return "QDialog { background-color: #f8fafc; color: #0f172a; }"
               "QSplitter::handle { background-color: #e2e8f0; height: 3px; width: 3px; }"
               "QSplitter::handle:hover { background-color: #3b82f6; }"
               "QScrollArea { background-color: #ffffff; border: 1px solid #e2e8f0; border-radius: 8px; }"
               "QPlainTextEdit { background-color: #ffffff; color: #0f172a; border: 1px solid #e2e8f0; border-radius: 8px; padding: 10px 14px; font-family: 'Consolas', 'Cascadia Code', 'Segoe UI', monospace; font-size: 13px; line-height: 1.6; selection-background-color: #bfdbfe; selection-color: #1e3a8a; }"
               "QPlainTextEdit:focus { border-color: #3b82f6; }"
               "QPushButton { background-color: #ffffff; color: #334155; border: 1px solid #cbd5e1; border-radius: 6px; padding: 6px 14px; font-size: 12px; font-weight: 500; }"
               "QPushButton:hover { background-color: #f1f5f9; color: #0f172a; border-color: #94a3b8; }"
               "QPushButton:pressed { background-color: #e2e8f0; }"
               "QPushButton#wordBtn { background-color: #2563eb; border-color: #1d4ed8; color: #ffffff; font-weight: bold; }"
               "QPushButton#wordBtn:hover { background-color: #1d4ed8; }"
               "QComboBox { background-color: #ffffff; color: #0f172a; border: 1px solid #cbd5e1; border-radius: 6px; padding: 5px 30px 5px 12px; font-size: 12px; font-weight: 500; min-height: 22px; }"
               "QComboBox:hover { border-color: #2563eb; background-color: #f8fafc; }"
               "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: top right; width: 24px; border-left: 1px solid #e2e8f0; border-top-right-radius: 6px; border-bottom-right-radius: 6px; background-color: #f8fafc; }"
               "QComboBox::drop-down:hover { background-color: #f1f5f9; }"
               "QComboBox::down-arrow { image: url(:/icons/chevron_down_light.svg); width: 12px; height: 12px; }"
               "QComboBox QAbstractItemView { background-color: #ffffff; color: #0f172a; border: 1px solid #cbd5e1; border-radius: 6px; padding: 4px; outline: none; selection-background-color: #eff6ff; selection-color: #2563eb; }"
               "QComboBox QAbstractItemView::item { background-color: #ffffff; color: #0f172a; min-height: 26px; padding: 4px 10px; border-radius: 4px; }"
               "QComboBox QAbstractItemView::item:hover { background-color: #f1f5f9; color: #0f172a; }"
               "QComboBox QAbstractItemView::item:selected { background-color: #eff6ff; color: #2563eb; }"
               "QLabel { color: #1e293b; }";
    }
}

QString ThemeManager::getPluginMarketStyle() const
{
    if (isDarkMode()) {
        return "QDialog { background-color: #18181b; color: #f4f4f5; font-family: 'Segoe UI', 'Microsoft YaHei', sans-serif; }"
               "QLabel { color: #e4e4e7; font-size: 13px; }"
               "QLabel#marketTitle { font-size: 17px; font-weight: 700; color: #f4f4f5; }"
               "QLabel#marketHint { font-size: 11px; color: #a1a1aa; }"
               "QLineEdit { background-color: #27272a; border: 1px solid #3f3f46; border-radius: 6px; padding: 6px 12px; font-size: 13px; color: #f4f4f5; }"
               "QLineEdit:hover { border-color: #52525b; }"
               "QLineEdit:focus { border-color: #3b82f6; background-color: #1f1f23; }"
               "QScrollArea { border: none; background-color: transparent; }"
               "QScrollArea > QWidget > QWidget { background-color: transparent; }"
               "QScrollBar:vertical { background-color: transparent; width: 6px; margin: 4px 2px 4px 0px; border-radius: 3px; }"
               "QScrollBar::handle:vertical { background-color: #52525b; min-height: 36px; border-radius: 3px; }"
               "QScrollBar::handle:vertical:hover { background-color: #71717a; }"
               "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; background: none; border: none; }"
               "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }"
               "QPushButton { background-color: #27272a; color: #e4e4e7; border: 1px solid #3f3f46; border-radius: 6px; padding: 6px 14px; font-size: 12px; font-weight: 500; min-height: 22px; }"
               "QPushButton:hover { background-color: #3f3f46; color: #ffffff; }"
               "QPushButton:checked { background-color: #1e3a8a; color: #93c5fd; border-color: #3b82f6; }"
               "QPushButton#marketPrimaryBtn { background-color: #2563eb; border-color: #3b82f6; color: #ffffff; font-weight: 600; }"
               "QPushButton#marketPrimaryBtn:hover { background-color: #1d4ed8; }";
    } else {
        return "QDialog { background-color: #f8fafc; color: #0f172a; font-family: 'Segoe UI', 'Microsoft YaHei', sans-serif; }"
               "QLabel { color: #1e293b; font-size: 13px; }"
               "QLabel#marketTitle { font-size: 17px; font-weight: 700; color: #0f172a; }"
               "QLabel#marketHint { font-size: 11px; color: #64748b; }"
               "QLineEdit { background-color: #ffffff; border: 1px solid #cbd5e1; border-radius: 6px; padding: 6px 12px; font-size: 13px; color: #0f172a; }"
               "QLineEdit:focus { border-color: #3b82f6; }"
               "QScrollArea { border: none; background-color: transparent; }"
               "QScrollArea > QWidget > QWidget { background-color: transparent; }"
               "QScrollBar:vertical { background-color: transparent; width: 6px; margin: 4px 2px 4px 0px; border-radius: 3px; }"
               "QScrollBar::handle:vertical { background-color: #cbd5e1; min-height: 36px; border-radius: 3px; }"
               "QScrollBar::handle:vertical:hover { background-color: #94a3b8; }"
               "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; background: none; border: none; }"
               "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }"
               "QPushButton { background-color: #ffffff; color: #334155; border: 1px solid #cbd5e1; border-radius: 6px; padding: 6px 14px; font-size: 12px; font-weight: 500; min-height: 22px; }"
               "QPushButton:hover { background-color: #f1f5f9; color: #0f172a; border-color: #94a3b8; }"
               "QPushButton:checked { background-color: #eff6ff; color: #2563eb; border-color: #3b82f6; }"
               "QPushButton#marketPrimaryBtn { background-color: #2563eb; border-color: #1d4ed8; color: #ffffff; font-weight: 600; }"
               "QPushButton#marketPrimaryBtn:hover { background-color: #1d4ed8; }";
    }
}

#ifdef Q_OS_WIN
// 内部：标题栏主题跟随器 (在窗口显示 / 句柄重建时自动重刷 DWM 标题栏颜色)
class TitleBarThemeWatcher : public QObject
{
public:
    explicit TitleBarThemeWatcher(QWidget* target) : QObject(target), m_target(target) {}

protected:
    bool eventFilter(QObject* obj, QEvent* event) override
    {
        if (event->type() == QEvent::Show || event->type() == QEvent::WinIdChange) {
            QMetaObject::invokeMethod(this, []() {}, Qt::QueuedConnection);
            ThemeManager::applyDarkTitleBar(m_target);
        }
        return QObject::eventFilter(obj, event);
    }

private:
    QWidget* m_target = nullptr;
};
#endif

void ThemeManager::applyDarkTitleBar(QWidget* widget)
{
#ifdef Q_OS_WIN
    if (!widget) return;
    HWND hwnd = reinterpret_cast<HWND>(widget->winId());
    if (!hwnd) return;

    const bool dark = instance().isDarkMode();

    // 1) 沉浸式深色模式 (Windows 10 20H2+ = 19, Windows 11 = 20)
    const BOOL enabled = dark ? TRUE : FALSE;
    DwmSetWindowAttribute(hwnd, 20, &enabled, sizeof(enabled));  // DWMWA_USE_IMMERSIVE_DARK_MODE
    DwmSetWindowAttribute(hwnd, 19, &enabled, sizeof(enabled));  // 旧版本兼容

    // 2) Windows 11 22H2+ 自定义标题栏颜色 (DWMWA_CAPTION_COLOR = 35 / DWMWA_TEXT_COLOR = 36)
    //    COLORREF 为 0x00BBGGRR
    const COLORREF captionColor = dark ? 0x001B1818 : 0x00FCFAF8;   // #18181b / #f8fafc
    const COLORREF textColor    = dark ? 0x00FFFFFF : 0x001A1A1A;   // #ffffff / #1a1a1a
    DwmSetWindowAttribute(hwnd, 35, &captionColor, sizeof(captionColor));
    DwmSetWindowAttribute(hwnd, 36, &textColor, sizeof(textColor));
#else
    Q_UNUSED(widget);
#endif
}

void ThemeManager::installDarkTitleBar(QWidget* widget)
{
#ifdef Q_OS_WIN
    if (!widget) return;
    // 立即应用一次 (窗口句柄可能已存在)
    applyDarkTitleBar(widget);

    auto* watcher = new TitleBarThemeWatcher(widget);
    widget->installEventFilter(watcher);

    // 主题切换时同步刷新所有已打开窗口的标题栏
    ThemeManager* mgr = &instance();
    mgr->connect(mgr, &ThemeManager::themeChanged, widget, [widget](bool) {
        applyDarkTitleBar(widget);
    });
#else
    Q_UNUSED(widget);
#endif
}
