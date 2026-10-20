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
        return "QDialog { background-color: #18181b; color: #f4f4f5; font-family: 'Segoe UI', 'Microsoft YaHei UI', sans-serif; }"
               "QLabel#dialogHeaderTitle { font-size: 18px; font-weight: 700; color: #f4f4f5; font-family: 'Segoe UI Variable Display', 'Segoe UI', 'Microsoft YaHei UI', sans-serif; }"
               "QLabel#dialogHeaderSubtitle { font-size: 12px; color: #a1a1aa; }"
               "QTabWidget::pane { border: 1px solid #3f3f46; border-radius: 10px; background-color: #18181b; top: -1px; }"
               "QTabBar { background-color: transparent; qproperty-drawBase: 0; margin-bottom: 8px; }"
               "QTabBar::tab { background-color: #27272a; color: #a1a1aa; padding: 8px 24px; margin-right: 6px; border-radius: 8px; font-size: 13px; font-weight: 600; border: 1px solid #3f3f46; }"
               "QTabBar::tab:selected { background-color: #3b82f6; color: #ffffff; border: 1px solid #3b82f6; }"
               "QTabBar::tab:hover:!selected { background-color: #3f3f46; color: #f4f4f5; border-color: #52525b; }"
               "QScrollArea { border: none; background-color: transparent; }"
               "QScrollArea > QWidget > QWidget#settingsContentWidget { background-color: transparent; }"
               "QFrame#settingsCard { background-color: #27272a; border: 1px solid #3f3f46; border-radius: 10px; }"
               "QLabel { color: #e4e4e7; font-size: 13px; }"
               "QLabel#cardTitle { color: #38bdf8; font-size: 13px; font-weight: bold; }"
               "QLabel#rowDesc { color: #a1a1aa; font-size: 11px; line-height: 1.4; }"
               "QLineEdit { background-color: #18181b; color: #ffffff; border: 1px solid #3f3f46; border-radius: 6px; padding: 6px 10px; font-size: 13px; min-height: 24px; }"
               "QLineEdit:focus { border-color: #3b82f6; background-color: #1f1f23; }"
               "QKeySequenceEdit { background-color: #18181b; color: #ffffff; border: 1px solid #3f3f46; border-radius: 6px; padding: 6px 10px; font-size: 13px; font-weight: bold; min-height: 24px; }"
               "QKeySequenceEdit:focus { border-color: #3b82f6; }"
               "QComboBox { background-color: #1f1f23; color: #f4f4f5; border: 1px solid #3f3f46; border-radius: 6px; padding: 5px 32px 5px 12px; font-size: 13px; font-weight: 500; min-height: 24px; }"
               "QComboBox:hover { border-color: #3b82f6; background-color: #27272a; }"
               "QComboBox:focus { border-color: #3b82f6; background-color: #1f1f23; }"
               "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: top right; width: 28px; border-left: 1px solid #3f3f46; border-top-right-radius: 6px; border-bottom-right-radius: 6px; background-color: #27272a; }"
               "QComboBox::drop-down:hover { background-color: #3f3f46; }"
               "QComboBox::down-arrow { image: url(:/icons/chevron_down_dark.png); width: 10px; height: 10px; }"
               "QComboBox QAbstractItemView { background-color: #1f1f23; color: #f4f4f5; border: 1px solid #3f3f46; border-radius: 6px; padding: 4px; outline: none; selection-background-color: #3b82f6; selection-color: #ffffff; }"
               "QComboBox QAbstractItemView::item { background-color: #1f1f23; color: #f4f4f5; min-height: 28px; padding: 4px 10px; border-radius: 4px; }"
               "QComboBox QAbstractItemView::item:hover { background-color: #3f3f46; color: #ffffff; }"
               "QComboBox QAbstractItemView::item:selected { background-color: #3b82f6; color: #ffffff; }"
               "QCheckBox { color: #e4e4e7; font-size: 13px; spacing: 10px; font-weight: 500; }"
               "QCheckBox::indicator { width: 18px; height: 18px; border: 1px solid #52525b; border-radius: 4px; background-color: #18181b; }"
               "QCheckBox::indicator:hover { border-color: #3b82f6; }"
               "QCheckBox::indicator:checked { background-color: #3b82f6; border-color: #3b82f6; image: url(:/icons/checkmark_white.png); }"
               "QPushButton { background-color: #27272a; color: #e4e4e7; border: 1px solid #3f3f46; border-radius: 6px; padding: 6px 16px; font-size: 12px; font-weight: 500; min-height: 22px; }"
               "QPushButton:hover { background-color: #3f3f46; color: #ffffff; }"
               "QPushButton#cardActionBtn { background-color: #38383e; color: #f4f4f5; border: 1px solid #52525b; border-radius: 6px; padding: 6px 16px; font-size: 12px; font-weight: 600; min-height: 22px; }"
               "QPushButton#cardActionBtn:hover { background-color: #3b82f6; border-color: #60a5fa; color: #ffffff; }"
               "QPushButton#cardActionBtn:pressed { background-color: #1d4ed8; }"
               "QPushButton#cardActionBtn:disabled { background-color: #27272a; color: #71717a; border-color: #3f3f46; }"
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
               "QTabWidget::pane { border: 1px solid #e2e8f0; border-radius: 10px; background-color: #ffffff; top: -1px; }"
               "QTabBar { background-color: transparent; qproperty-drawBase: 0; margin-bottom: 8px; }"
               "QTabBar::tab { background-color: #f1f5f9; color: #64748b; padding: 8px 24px; margin-right: 6px; border-radius: 8px; font-size: 13px; font-weight: 600; border: 1px solid #e2e8f0; }"
               "QTabBar::tab:selected { background-color: #2563eb; color: #ffffff; border: 1px solid #2563eb; }"
               "QTabBar::tab:hover:!selected { background-color: #e2e8f0; color: #0f172a; }"
               "QScrollArea { border: none; background-color: transparent; }"
               "QScrollArea > QWidget > QWidget#settingsContentWidget { background-color: transparent; }"
               "QFrame#settingsCard { background-color: #f8fafc; border: 1px solid #e2e8f0; border-radius: 10px; }"
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
               "QComboBox::down-arrow { image: url(:/icons/chevron_down_light.png); width: 10px; height: 10px; }"
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
               "QComboBox::down-arrow { image: url(:/icons/chevron_down_dark.png); width: 10px; height: 10px; }"
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
               "QComboBox::down-arrow { image: url(:/icons/chevron_down_light.png); width: 10px; height: 10px; }"
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
               "QComboBox::down-arrow { image: url(:/icons/chevron_down_dark.png); width: 10px; height: 10px; }"
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
               "QComboBox::down-arrow { image: url(:/icons/chevron_down_light.png); width: 10px; height: 10px; }"
               "QComboBox QAbstractItemView { background-color: #ffffff; color: #0f172a; border: 1px solid #cbd5e1; border-radius: 6px; padding: 4px; outline: none; selection-background-color: #eff6ff; selection-color: #2563eb; }"
               "QComboBox QAbstractItemView::item { background-color: #ffffff; color: #0f172a; min-height: 26px; padding: 4px 10px; border-radius: 4px; }"
               "QComboBox QAbstractItemView::item:hover { background-color: #f1f5f9; color: #0f172a; }"
               "QComboBox QAbstractItemView::item:selected { background-color: #eff6ff; color: #2563eb; }"
               "QLabel { color: #1e293b; }";
    }
}
