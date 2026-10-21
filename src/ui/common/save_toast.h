#pragma once

#include <QWidget>
#include <QString>
#include <functional>

/**
 * @brief 保存提示浮层 (Toast)
 *
 * 在「一键自动保存」模式下告知用户文件已落盘的位置。
 * 特点：
 *  - 无边框、始终置顶、鼠标穿透，不会打断当前操作；
 *  - 跟随当前主题 (深/浅) 自动切换配色；
 *  - 定时自动淡出销毁，无需手动关闭。
 */
class SaveToast : public QWidget
{
    Q_OBJECT
public:
    explicit SaveToast(QWidget* parent = nullptr);
    ~SaveToast() override;

    /**
     * @brief 弹出一条保存提示
     * @param title    标题 (如「已保存到」)
     * @param subtitle 详细路径 (可为空)
     */
    static void showToast(const QString& title, const QString& subtitle = QString());

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QString m_title;
    QString m_subtitle;
};
