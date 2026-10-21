#pragma once

#include <QObject>
#include <QString>
#include <QColor>

class QWidget;

enum class ThemeMode {
    Auto,   // 跟随系统设置 (默认)
    Light,  // 明亮浅色模式 (Fluent Light)
    Dark    // 沉浸深色模式 (Fluent Dark)
};

class ThemeManager : public QObject
{
    Q_OBJECT
public:
    static ThemeManager& instance();

    // 当前主题模式与实际生效的暗色判定
    ThemeMode themeMode() const { return m_themeMode; }
    void setThemeMode(ThemeMode mode);

    bool isDarkMode() const;

    // 样式表获取
    QString getTrayMenuStyle() const;
    QString getPinMenuStyle() const;
    QString getFloatingToolbarStyle() const;
    QString getSettingsDialogStyle() const;
    QString getOcrDialogStyle() const;
    QString getFormulaDialogStyle() const;
    QString getPluginMarketStyle() const;

    // Windows 11 沉浸式深色标题栏 (DWM 特性)，使系统原生标题栏同步深色
    static void applyDarkTitleBar(QWidget* widget);
    // 安装标题栏主题跟随器 (窗口显示/句柄变化时自动重刷)
    static void installDarkTitleBar(QWidget* widget);

signals:
    void themeChanged(bool isDark);

private:
    explicit ThemeManager(QObject* parent = nullptr);
    ~ThemeManager() override = default;

    bool detectSystemIsDark() const;

    ThemeMode m_themeMode = ThemeMode::Auto;
};
