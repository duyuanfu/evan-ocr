#pragma once

#include <QObject>
#include <QString>

/**
 * @brief 保存路径管理器
 *
 * 提供两项能力：
 *  1. **自定义保存目录**：用户可在「设置 → 保存路径」中指定固定的保存目录，
 *     所有截图 / 长截图 / 贴图 / GIF 的保存动作都将默认落到该目录；
 *  2. **自动记忆上次路径**：未指定自定义目录时，自动记忆用户最近一次成功保存的
 *     所在目录，下次打开保存对话框直接定位到该目录。
 */
class SavePathManager : public QObject
{
    Q_OBJECT
public:
    static SavePathManager& instance();

    // 自定义保存目录 (为空表示使用默认策略)
    QString customSaveDirectory() const;
    void setCustomSaveDirectory(const QString& dir);

    // 是否自动记忆上次保存位置
    bool rememberLastDirectory() const;
    void setRememberLastDirectory(bool enable);

    // 最近一次成功保存的目录
    QString lastUsedDirectory() const;

    /**
     * @brief 生成默认保存文件的完整路径
     * @param defaultName  默认文件名 (如 Evan_20260101_120000.png)
     * @param outDir       实际使用的目录
     * @return 拼接完成的完整文件路径
     */
    QString buildSavePath(const QString& defaultName, QString* outDir = nullptr) const;

    /**
     * @brief 是否启用「一键自动保存」模式
     *
     * 用户在「设置 → 保存路径」中指定了自定义保存目录后，截图 / 长截图 /
     * GIF 录制 / 贴图 将直接落盘到该目录，**不再弹出保存对话框**。
     * 目录不存在或不可写时自动降级回退到对话框流程。
     */
    bool isAutoSaveEnabled() const;

    /**
     * @brief 生成一个在目标目录内不重复的完整文件路径
     *
     * 自动保存时同一秒内可能产生同名文件，此处按 `_1` / `_2` 递增避让。
     */
    QString buildUniqueSavePath(const QString& defaultName, QString* outDir = nullptr) const;

    /**
     * @brief 用户成功保存后调用，记录目录供下次记忆
     */
    void rememberSavedPath(const QString& filePath);

    /**
     * @brief 恢复默认 (清除自定义目录与记忆记录)
     */
    void resetToDefault();

signals:
    void customDirectoryChanged(const QString& dir);
    void rememberDirectoryChanged(bool enable);

private:
    explicit SavePathManager(QObject* parent = nullptr);

    QString m_customDirectory;
    QString m_lastUsedDirectory;
    bool m_rememberLast = true;
};
