#pragma once

#include "translator_interface.h"
#include <QObject>
#include <QString>

class OfflinePluginTranslator : public QObject, public ITranslator
{
    Q_OBJECT
public:
    explicit OfflinePluginTranslator(QObject* parent = nullptr);
    ~OfflinePluginTranslator() override = default;

    QString name() const override { return "本地离线神经网络 (纯本地·100%隐私)"; }
    TranslationEngineType type() const override { return TranslationEngineType::OfflinePlugin; }

    bool isAvailable() const override;
    QString componentPath() const;
    QString modelsDirectory() const;

    QList<LanguagePair> supportedLanguagePairs() const override;

    TranslationResult translate(const QString& text, const QString& srcLang = "auto", const QString& targetLang = "zh") override;
    void translateAsync(const QString& text, const QString& srcLang, const QString& targetLang,
                        std::function<void(const TranslationResult&)> callback) override;

private:
    QString findPluginExecutable() const;
    QString findModelsDir() const;
};
