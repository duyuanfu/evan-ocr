#pragma once

#include "translator_interface.h"
#include <QObject>
#include <QNetworkAccessManager>

class OnlineFallbackTranslator : public QObject, public ITranslator
{
    Q_OBJECT
public:
    explicit OnlineFallbackTranslator(QObject* parent = nullptr);
    ~OnlineFallbackTranslator() override = default;

    QString name() const override { return "在线极速直连 (轻量免配置)"; }
    TranslationEngineType type() const override { return TranslationEngineType::OnlineFallback; }
    bool isAvailable() const override { return true; }

    QList<LanguagePair> supportedLanguagePairs() const override;

    TranslationResult translate(const QString& text, const QString& srcLang = "auto", const QString& targetLang = "zh") override;
    void translateAsync(const QString& text, const QString& srcLang, const QString& targetLang,
                        std::function<void(const TranslationResult&)> callback) override;

private:
    QString mapLangCode(const QString& lang) const;
};
