#pragma once

#include <QString>
#include <functional>

class QWidget;

/**
 * @brief 统一的「保存」流程控制器
 *
 * 依据用户在「设置 → 自定义保存文件路径」中的配置决定保存行为：
 *
 *  - **已配置自定义保存目录** → 一键自动保存到该目录，**不再弹出保存对话框**，
 *    落盘后以 Toast 形式提示完整路径；
 *  - **未配置** → 维持原有「保存对话框」交互，并自动记忆上次保存位置。
 *
 * 同时负责同名文件避让 (自动保存时同一秒内可能产生重名)。
 */
class SaveHelper
{
public:
    struct Result {
        bool saved = false;      ///< 是否已成功落盘
        bool cancelled = false;  ///< 用户是否取消了保存
        bool autoSaved = false;  ///< 是否走的一键自动保存
        QString filePath;        ///< 实际落盘路径
    };

    // 实际的写盘回调: 接收完整文件路径, 返回是否成功
    using Writer = std::function<bool(const QString& filePath)>;

    /**
     * @brief 执行保存流程
     * @param parent      父窗口 (对话框用)
     * @param defaultName 默认文件名 (含扩展名, 如 Evan_20260101_120000.png)
     * @param caption     对话框标题
     * @param filter      文件类型过滤器
     * @param writer      写盘回调
     */
    static Result save(QWidget* parent,
                       const QString& defaultName,
                       const QString& caption,
                       const QString& filter,
                       const Writer& writer);
};
