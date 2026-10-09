#pragma once

#include "translation_types.h"
#include <functional>

class ITranslator {
public:
    virtual ~ITranslator() = default;

    // 引擎名称
    virtual QString name() const = 0;

    // 引擎类型 (离线插件 / 在线备选)
    virtual TranslationEngineType type() const = 0;

    // 当前是否就绪可用
    virtual bool isAvailable() const = 0;

    // 支持的语言对列表
    virtual QList<LanguagePair> supportedLanguagePairs() const = 0;

    // 同步翻译接口 (阻塞，建议在工作线程执行)
    virtual TranslationResult translate(const QString& text, const QString& srcLang = "auto", const QString& targetLang = "zh") = 0;

    // 异步翻译接口
    virtual void translateAsync(const QString& text, const QString& srcLang, const QString& targetLang,
                                std::function<void(const TranslationResult&)> callback) = 0;
};
