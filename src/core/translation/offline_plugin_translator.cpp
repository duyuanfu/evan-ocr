#include "offline_plugin_translator.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QThreadPool>
#include <QElapsedTimer>
#include <QDateTime>
#include <QMetaObject>
#include <QDebug>

OfflinePluginTranslator::OfflinePluginTranslator(QObject* parent)
    : QObject(parent)
{
}

QString OfflinePluginTranslator::findPluginExecutable() const
{
    QString appDir = QCoreApplication::applicationDirPath();
    QStringList candidates = {
        appDir + "/plugins/translation/translator-engine.exe",
        appDir + "/plugins/translation/offline-translator.exe",
        appDir + "/plugins/translation/ctranslate2-runner.exe",
        appDir + "/plugins/translation/marian-cli.exe",
        appDir + "/plugins/translation/main.exe",
        "plugins/translation/translator-engine.exe",
        "plugins/translation/offline-translator.exe",
        "plugins/translation/ctranslate2-runner.exe",
        "plugins/translation/main.exe"
    };

    for (const auto& path : candidates) {
        if (QFile::exists(path)) {
            return QDir::toNativeSeparators(QFileInfo(path).absoluteFilePath());
        }
    }
    return QString();
}

QString OfflinePluginTranslator::findModelsDir() const
{
    QString appDir = QCoreApplication::applicationDirPath();
    QStringList candidates = {
        appDir + "/plugins/translation/models",
        appDir + "/plugins/translation",
        "plugins/translation/models",
        "plugins/translation"
    };

    for (const auto& dir : candidates) {
        if (QDir(dir).exists()) {
            return QDir::toNativeSeparators(QFileInfo(dir).absoluteFilePath());
        }
    }
    return QString();
}

bool OfflinePluginTranslator::isAvailable() const
{
    return !findPluginExecutable().isEmpty();
}

QString OfflinePluginTranslator::componentPath() const
{
    QString exe = findPluginExecutable();
    if (!exe.isEmpty()) return exe;
    return "未安装 (扩展目录: plugins/translation/)";
}

QString OfflinePluginTranslator::modelsDirectory() const
{
    return findModelsDir();
}

QList<LanguagePair> OfflinePluginTranslator::supportedLanguagePairs() const
{
    // 如果插件目录存在特定的 config.json 描述文件，优先动态解析
    QString configPath = QCoreApplication::applicationDirPath() + "/plugins/translation/config.json";
    if (QFile::exists(configPath)) {
        QFile f(configPath);
        if (f.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
            if (!doc.isNull() && doc.isObject()) {
                QJsonArray pairsArr = doc.object().value("supported_languages").toArray();
                if (!pairsArr.isEmpty()) {
                    QList<LanguagePair> dynPairs;
                    for (const auto& item : pairsArr) {
                        QJsonObject obj = item.toObject();
                        LanguagePair lp;
                        lp.sourceLang = obj.value("from").toString("auto");
                        lp.targetLang = obj.value("to").toString("zh");
                        lp.displayName = obj.value("name").toString(QString("%1 ➔ %2").arg(lp.sourceLang, lp.targetLang));
                        dynPairs.append(lp);
                    }
                    if (!dynPairs.isEmpty()) return dynPairs;
                }
            }
        }
    }

    // 默认标准神经网络模型对
    return {
        {"auto", "zh", "自动检测 ➔ 中文 (离线神经网络)"},
        {"en", "zh", "英语 ➔ 中文 (离线神经网络)"},
        {"zh", "en", "中文 ➔ 英语 (离线神经网络)"},
        {"ja", "zh", "日语 ➔ 中文 (离线神经网络)"},
        {"ko", "zh", "韩语 ➔ 中文 (离线神经网络)"},
        {"ru", "zh", "俄语 ➔ 中文 (离线神经网络)"}
    };
}

TranslationResult OfflinePluginTranslator::translate(const QString& text, const QString& srcLang, const QString& targetLang)
{
    TranslationResult result;
    result.originalText = text;
    result.sourceLang = srcLang;
    result.targetLang = targetLang;
    result.engineType = TranslationEngineType::OfflinePlugin;
    result.engineName = name();

    if (text.trimmed().isEmpty()) {
        result.success = true;
        result.translatedText = "";
        return result;
    }

    QString exePath = findPluginExecutable();
    if (exePath.isEmpty()) {
        result.success = false;
        result.errorMessage = "未检测到离线翻译插件组件。\n请在 plugins/translation/ 目录放入离线翻译插件及模型文件。";
        return result;
    }

    QElapsedTimer timer;
    timer.start();

    // 将待翻译文本写入临时输入文件，避免命令行过长截断或中文编码转义错误
    QString tempInput = QDir::tempPath() + QString("/evan_trans_in_%1_%2.txt")
                        .arg(QCoreApplication::applicationPid())
                        .arg(QDateTime::currentMSecsSinceEpoch());
    QString tempOutput = QDir::tempPath() + QString("/evan_trans_out_%1_%2.txt")
                         .arg(QCoreApplication::applicationPid())
                         .arg(QDateTime::currentMSecsSinceEpoch());

    QFile inFile(tempInput);
    if (!inFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        result.success = false;
        result.errorMessage = "无法创建临时待翻译文本文件";
        return result;
    }
    inFile.write(text.toUtf8());
    inFile.close();

    QProcess process;
    process.setWorkingDirectory(QFileInfo(exePath).absolutePath());

    QStringList args;
    args << "--from" << srcLang
         << "--to" << targetLang
         << "--input" << tempInput
         << "--output" << tempOutput;

    QString modelsDir = findModelsDir();
    if (!modelsDir.isEmpty()) {
        args << "--models" << modelsDir;
    }

    process.start(exePath, args);

    if (!process.waitForStarted(3000)) {
        QFile::remove(tempInput);
        QFile::remove(tempOutput);
        result.success = false;
        result.errorMessage = QString("启动离线翻译引擎进程失败: %1").arg(process.errorString());
        return result;
    }

    if (!process.waitForFinished(15000)) {
        process.kill();
        QFile::remove(tempInput);
        QFile::remove(tempOutput);
        result.success = false;
        result.errorMessage = "离线翻译处理超时 (15秒)";
        return result;
    }

    result.elapsedMs = timer.elapsed();

    // 优先从输出文件读取结果，若文件未生成则从标准输出读取
    QString translatedStr;
    if (QFile::exists(tempOutput)) {
        QFile outFile(tempOutput);
        if (outFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            translatedStr = QString::fromUtf8(outFile.readAll()).trimmed();
            outFile.close();
        }
    }

    if (translatedStr.isEmpty()) {
        QByteArray stdOut = process.readAllStandardOutput();
        // 尝试解析 JSON 或取纯文本
        QJsonDocument doc = QJsonDocument::fromJson(stdOut);
        if (!doc.isNull() && doc.isObject()) {
            translatedStr = doc.object().value("result").toString();
        } else {
            translatedStr = QString::fromUtf8(stdOut).trimmed();
        }
    }

    QFile::remove(tempInput);
    QFile::remove(tempOutput);

    if (translatedStr.isEmpty()) {
        QByteArray errOut = process.readAllStandardError();
        result.success = false;
        result.errorMessage = errOut.isEmpty() ? "离线翻译进程未返回有效译文" : QString::fromUtf8(errOut);
        return result;
    }

    result.success = true;
    result.translatedText = translatedStr;
    return result;
}

void OfflinePluginTranslator::translateAsync(const QString& text, const QString& srcLang, const QString& targetLang,
                                            std::function<void(const TranslationResult&)> callback)
{
    QString txt = text;
    QString src = srcLang;
    QString tgt = targetLang;

    QThreadPool::globalInstance()->start([this, txt, src, tgt, callback]() {
        TranslationResult res = translate(txt, src, tgt);
        QMetaObject::invokeMethod(this, [callback, res]() {
            if (callback) {
                callback(res);
            }
        }, Qt::QueuedConnection);
    });
}
