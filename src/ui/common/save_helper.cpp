#include "save_helper.h"
#include "save_toast.h"

#include "../../core/save_path_manager.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QDir>

SaveHelper::Result SaveHelper::save(QWidget* parent,
                                     const QString& defaultName,
                                     const QString& caption,
                                     const QString& filter,
                                     const Writer& writer)
{
    Result result;
    if (defaultName.isEmpty() || !writer) return result;

    SavePathManager& spm = SavePathManager::instance();

    // ============ 模式一：已配置自定义保存目录 → 一键自动保存 ============
    if (spm.isAutoSaveEnabled()) {
        result.autoSaved = true;

        const QString target = spm.buildUniqueSavePath(defaultName);
        if (target.isEmpty()) return result;

        if (writer(target)) {
            result.saved = true;
            result.filePath = target;
            spm.rememberSavedPath(target);
            SaveToast::showToast(QStringLiteral("已保存到"), QDir::toNativeSeparators(target));
        } else {
            QMessageBox::warning(parent, QStringLiteral("保存失败"),
                                 QStringLiteral("无法写入文件：\n%1").arg(target));
        }
        return result;
    }

    // ============ 模式二：未配置 → 弹出保存对话框 ============
    const QString defaultPath = spm.buildSavePath(defaultName);
    const QString path = QFileDialog::getSaveFileName(parent, caption, defaultPath, filter);
    if (path.isEmpty()) {
        result.cancelled = true;
        return result;
    }

    if (writer(path)) {
        result.saved = true;
        result.filePath = path;
        spm.rememberSavedPath(path);
    } else {
        QMessageBox::warning(parent, QStringLiteral("保存失败"),
                             QStringLiteral("无法写入文件：\n%1").arg(path));
    }
    return result;
}
