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
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QTimer>
#include <QDebug>

OfflinePluginTranslator::OfflinePluginTranslator(QObject* parent)
    : QObject(parent)
{
}

QString OfflinePluginTranslator::name() const
{
    QString localModel;
    if (checkLocalLlmAvailable(localModel)) {
        return QString("本地离线大模型 (%1)").arg(localModel);
    }
    return "本地离线神经网络 (纯本地·100%隐私)";
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

bool OfflinePluginTranslator::checkLocalLlmAvailable(QString& detectedModel) const
{
    // 轻量探测本地 Ollama (11434) 离线大模型服务
    QUrl url("http://127.0.0.1:11434/api/tags");
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);

    QNetworkAccessManager nam;
    QNetworkReply* reply = nam.get(request);

    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timeoutTimer, &QTimer::timeout, &loop, [&]() {
        if (reply->isRunning()) {
            reply->abort();
        }
        loop.quit();
    });

    timeoutTimer.start(600); // 600ms 快速探活
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        reply->deleteLater();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            QJsonArray modelsArr = doc.object().value("models").toArray();
            if (!modelsArr.isEmpty()) {
                // 优先挑选适合翻译的中文/多语言大模型 (如 qwen, deepseek, llama 等)
                QString chosenModel = modelsArr.at(0).toObject().value("name").toString();
                for (const auto& item : modelsArr) {
                    QString mName = item.toObject().value("name").toString().toLower();
                    if (mName.contains("qwen") || mName.contains("deepseek") || mName.contains("translate")) {
                        chosenModel = item.toObject().value("name").toString();
                        break;
                    }
                }
                detectedModel = chosenModel;
                return true;
            }
        }
    } else {
        reply->deleteLater();
    }

    return false;
}

TranslationResult OfflinePluginTranslator::translateWithLocalLlm(const QString& text, const QString& srcLang, const QString& targetLang, const QString& modelName)
{
    TranslationResult result;
    result.originalText = text;
    result.sourceLang = srcLang;
    result.targetLang = targetLang;
    result.engineType = TranslationEngineType::OfflinePlugin;
    result.engineName = QString("本地大模型 (%1)").arg(modelName);

    QElapsedTimer timer;
    timer.start();

    QUrl url("http://127.0.0.1:11434/api/generate");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // 针对本地大模型微调的高精度系统翻译 Prompt
    QString langFrom = (srcLang == "zh") ? "Chinese" : (srcLang == "en") ? "English" : (srcLang == "ja") ? "Japanese" : "source language";
    QString langTo = (targetLang == "zh") ? "Simplified Chinese" : (targetLang == "en") ? "English" : (targetLang == "ja") ? "Japanese" : "target language";

    QString prompt = QString(
        "You are an expert translation engine. Translate the following text from %1 to %2. "
        "Keep the original paragraph layout. Output ONLY the raw translated text with NO conversational filler, NO quotes, NO explanation:\n\n%3"
    ).arg(langFrom, langTo, text);

    QJsonObject reqObj;
    reqObj["model"] = modelName;
    reqObj["prompt"] = prompt;
    reqObj["stream"] = false;

    QNetworkAccessManager nam;
    QNetworkReply* reply = nam.post(request, QJsonDocument(reqObj).toJson());

    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timeoutTimer, &QTimer::timeout, &loop, [&]() {
        if (reply->isRunning()) {
            reply->abort();
        }
        loop.quit();
    });

    timeoutTimer.start(30000); // 大模型本地推理允许 30 秒超时
    loop.exec();

    result.elapsedMs = timer.elapsed();

    if (reply->error() != QNetworkReply::NoError) {
        result.success = false;
        result.errorMessage = QString("本地大模型服务通信失败: %1").arg(reply->errorString());
        reply->deleteLater();
        return result;
    }

    QByteArray respData = reply->readAll();
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(respData);
    if (!doc.isObject()) {
        result.success = false;
        result.errorMessage = "本地大模型返回的数据格式异常";
        return result;
    }

    QString translatedStr = doc.object().value("response").toString().trimmed();
    if (translatedStr.isEmpty()) {
        result.success = false;
        result.errorMessage = "本地大模型未生成有效译文";
        return result;
    }

    result.success = true;
    result.translatedText = translatedStr;
    return result;
}

bool OfflinePluginTranslator::isAvailable() const
{
    QString dummyModel;
    if (checkLocalLlmAvailable(dummyModel)) {
        return true;
    }
    return !findPluginExecutable().isEmpty();
}

QString OfflinePluginTranslator::componentPath() const
{
    QString localModel;
    if (checkLocalLlmAvailable(localModel)) {
        return QString("本地大模型已就绪 (模型: %1, 端口: 11434)").arg(localModel);
    }

    QString exe = findPluginExecutable();
    if (!exe.isEmpty()) return exe;
    return "未安装 (可安装本地 Ollama 大模型或在 plugins/translation/ 放入模型)";
}

QString OfflinePluginTranslator::modelsDirectory() const
{
    return findModelsDir();
}

QList<LanguagePair> OfflinePluginTranslator::supportedLanguagePairs() const
{
    return {
        {"auto", "zh", "自动检测 ➔ 中文"},
        {"auto", "en", "自动检测 ➔ 英语"},
        {"zh", "en", "中文 ➔ 英语"},
        {"en", "zh", "英语 ➔ 中文"},
        {"zh", "ja", "中文 ➔ 日语"},
        {"ja", "zh", "日语 ➔ 中文"},
        {"zh", "ko", "中文 ➔ 韩语"},
        {"ko", "zh", "韩语 ➔ 中文"},
        {"zh", "ru", "中文 ➔ 俄语"},
        {"ru", "zh", "俄语 ➔ 中文"}
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

    // 1. 优先尝试本地离线大模型服务 (Ollama / LocalAI / LM Studio)
    QString localModel;
    if (checkLocalLlmAvailable(localModel)) {
        TranslationResult llmRes = translateWithLocalLlm(text, srcLang, targetLang, localModel);
        if (llmRes.success) {
            return llmRes;
        }
    }

    // 2. 备用：调用 plugins/translation/ 独立本地模型引擎
    QString exePath = findPluginExecutable();
    if (exePath.isEmpty()) {
        result.success = false;
        result.errorMessage = "未检测到本地离线大模型或独立插件。\n请启动本地大模型服务 (如 Ollama) 或在 plugins/translation/ 目录放入模型包。";
        return result;
    }

    QElapsedTimer timer;
    timer.start();

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
