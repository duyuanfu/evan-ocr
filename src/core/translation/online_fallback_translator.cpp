#include "online_fallback_translator.h"
#include <QThreadPool>
#include <QEventLoop>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonArray>
#include <QElapsedTimer>
#include <QTimer>
#include <QMetaObject>
#include <QDebug>

OnlineFallbackTranslator::OnlineFallbackTranslator(QObject* parent)
    : QObject(parent)
{
}

QList<LanguagePair> OnlineFallbackTranslator::supportedLanguagePairs() const
{
    return {
        {"auto", "zh", "自动检测 ➔ 中文 (简体)"},
        {"en", "zh", "英语 ➔ 中文 (简体)"},
        {"zh", "en", "中文 (简体) ➔ 英语"},
        {"ja", "zh", "日语 ➔ 中文 (简体)"},
        {"ko", "zh", "韩语 ➔ 中文 (简体)"},
        {"ru", "zh", "俄语 ➔ 中文 (简体)"},
        {"fr", "zh", "法语 ➔ 中文 (简体)"},
        {"de", "zh", "德语 ➔ 中文 (简体)"},
        {"auto", "en", "自动检测 ➔ 英语"}
    };
}

QString OnlineFallbackTranslator::mapLangCode(const QString& lang) const
{
    if (lang.isEmpty() || lang == "auto") return "auto";
    if (lang.startsWith("zh")) return "zh-CN";
    if (lang.startsWith("en")) return "en";
    if (lang.startsWith("ja")) return "ja";
    if (lang.startsWith("ko")) return "ko";
    if (lang.startsWith("ru")) return "ru";
    if (lang.startsWith("fr")) return "fr";
    if (lang.startsWith("de")) return "de";
    return lang.toLower();
}

TranslationResult OnlineFallbackTranslator::translate(const QString& text, const QString& srcLang, const QString& targetLang)
{
    TranslationResult result;
    result.originalText = text;
    result.sourceLang = srcLang;
    result.targetLang = targetLang;
    result.engineType = TranslationEngineType::OnlineFallback;
    result.engineName = name();

    if (text.trimmed().isEmpty()) {
        result.success = true;
        result.translatedText = "";
        return result;
    }

    QElapsedTimer timer;
    timer.start();

    // 构造请求 URL (Google Translate 免费极速接口)
    QUrl url("https://translate.googleapis.com/translate_a/single");
    QUrlQuery query;
    query.addQueryItem("client", "gtx");
    query.addQueryItem("sl", mapLangCode(srcLang));
    query.addQueryItem("tl", mapLangCode(targetLang));
    query.addQueryItem("dt", "t");
    query.addQueryItem("q", text);
    url.setQuery(query);

    QNetworkAccessManager nam;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, true);

    QNetworkReply* reply = nam.get(request);

    // 超时控制 (5秒超时)
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

    timeoutTimer.start(5000);
    loop.exec();

    result.elapsedMs = timer.elapsed();

    if (reply->error() != QNetworkReply::NoError) {
        result.success = false;
        result.errorMessage = QString("在线直连翻译失败: %1 (可配置离线插件使用纯本地翻译)").arg(reply->errorString());
        reply->deleteLater();
        return result;
    }

    QByteArray responseData = reply->readAll();
    reply->deleteLater();

    // 解析 JSON 响应结构: [[["译文片段1", "原文片段1", ...], ["译文片段2", "原文片段2", ...]], ...]
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
    if (doc.isNull() || !doc.isArray()) {
        result.success = false;
        result.errorMessage = "解析在线翻译结果失败";
        return result;
    }

    QJsonArray rootArr = doc.array();
    if (rootArr.isEmpty() || !rootArr.at(0).isArray()) {
        result.success = false;
        result.errorMessage = "返回的翻译数据结构异常";
        return result;
    }

    QJsonArray sentencesArr = rootArr.at(0).toArray();
    QString fullTranslation;
    for (int i = 0; i < sentencesArr.size(); ++i) {
        QJsonArray sentenceItem = sentencesArr.at(i).toArray();
        if (!sentenceItem.isEmpty() && sentenceItem.at(0).isString()) {
            fullTranslation += sentenceItem.at(0).toString();
        }
    }

    // 嗅探到的实际源语言
    if (rootArr.size() > 2 && rootArr.at(2).isString()) {
        result.detectedSourceLang = rootArr.at(2).toString();
    }

    result.success = true;
    result.translatedText = fullTranslation;
    return result;
}

void OnlineFallbackTranslator::translateAsync(const QString& text, const QString& srcLang, const QString& targetLang,
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
