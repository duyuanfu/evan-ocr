#pragma once

#include <QObject>
#include <QEvent>
#include <QComboBox>

/**
 * @brief 下拉框滚轮事件守卫
 *
 * 解决两个交互痛点：
 *  1. 鼠标悬停在 QComboBox 上滚动滚轮时，Qt 默认会直接切换选中项，用户难以瞄准目标项；
 *  2. 滚轮事件会继续冒泡到外层 QScrollArea，导致整个设置页被意外滚动。
 *
 * 守卫策略：完全吞掉 QComboBox 上的 Wheel 事件。
 *  - 选中项不会再被滚轮意外改变；
 *  - 父级滚动区不会被连带滚动；
 *  - 用户必须点击下拉框展开列表后再点击目标项进行选择 (符合 Fluent 规范)。
 */
class WheelEventGuard : public QObject
{
public:
    explicit WheelEventGuard(QObject* parent = nullptr) : QObject(parent) {}

protected:
    bool eventFilter(QObject* obj, QEvent* event) override
    {
        if (event->type() == QEvent::Wheel) {
            return true;   // 吞掉滚轮事件
        }
        return QObject::eventFilter(obj, event);
    }
};

/**
 * @brief 为下拉框安装滚轮守卫 (可重复调用，内部自动去重)
 */
inline void installWheelGuard(QComboBox* combo)
{
    if (!combo) return;
    // 通过属性标记去重，避免重复叠加事件过滤器
    static const char* kGuardFlag = "evan_wheel_guard_installed";
    if (combo->property(kGuardFlag).toBool()) return;
    combo->setProperty(kGuardFlag, true);
    combo->installEventFilter(new WheelEventGuard(combo));
}
