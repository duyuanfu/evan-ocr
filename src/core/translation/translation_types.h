#pragma once

#include <QString>
#include <QList>
#include <QPair>

enum class TranslationEngineType {
    OfflinePlugin,   // 本地离线神经网络插件 (0MB 外部模型、100% 本地隐私)
    OnlineFallback   // 轻量在线直连备用引擎 (免下载即开即用)
};

struct LanguagePair {
    QString sourceLang;      // 如 "auto", "zh", "en", "ja", "ko"
    QString targetLang;      // 如 "zh", "en", "ja", "ko"
    QString displayName;     // 如 "自动检测 ➔ 中文"
};

struct TranslationResult {
    bool success = false;
    QString errorMessage;
    QString originalText;
    QString translatedText;
    QString sourceLang = "auto";
    QString targetLang = "zh";
    QString detectedSourceLang;
    TranslationEngineType engineType = TranslationEngineType::OnlineFallback;
    QString engineName;
    qint64 elapsedMs = 0;
};
