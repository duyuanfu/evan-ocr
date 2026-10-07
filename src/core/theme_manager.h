#pragma once

#include <QObject>
#include <QString>
#include <QColor>

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

signals:
    void themeChanged(bool isDark);

private:
    explicit ThemeManager(QObject* parent = nullptr);
    ~ThemeManager() override = default;

    bool detectSystemIsDark() const;

    ThemeMode m_themeMode = ThemeMode::Auto;
};
