#pragma once

#include <QObject>
#include <QString>
#include <QKeyEvent>
#include <QKeySequence>

struct HotkeyConfigData {
    // 1. 全局热键
    QString globalSnipping = "F1";
    QString globalPin = "F3";

    // 2. 截屏操作快捷键
    QString snippingUndo = "Ctrl+Z";
    QString snippingPin = "F3";
    QString snippingConfirm = "Return";
    QString snippingSave = "Ctrl+S";
    QString snippingOcr = "Ctrl+O";
    QString snippingCancel = "Esc";

    // 3. 截图标注工具快捷键
    QString toolRect = "R";
    QString toolArrow = "A";
    QString toolPencil = "P";
    QString toolText = "T";
    QString toolMosaic = "M";
    QString toolCharEdit = "E";

    // 4. 独立贴图窗口快捷键
    QString pinClose = "Esc";
    QString pinOcr = "Ctrl+O";
    QString pinCopy = "Ctrl+C";
    QString pinSave = "Ctrl+S";
};

class HotkeyConfig : public QObject
{
    Q_OBJECT
public:
    static HotkeyConfig& instance();

    const HotkeyConfigData& data() const { return m_data; }
    void setData(const HotkeyConfigData& data);

    void load();
    void save();
    void resetToDefaults();

    // 判定 QKeyEvent 是否命中目标快捷键字符串
    static bool matches(QKeyEvent* event, const QString& keySeqStr);

signals:
    void configChanged();

private:
    explicit HotkeyConfig(QObject* parent = nullptr);
    ~HotkeyConfig() override = default;

    HotkeyConfigData m_data;
};
