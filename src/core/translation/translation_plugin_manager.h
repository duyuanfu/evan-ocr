#pragma once

#include "translator_interface.h"
#include "online_fallback_translator.h"
#include "offline_plugin_translator.h"
#include <QObject>
#include <memory>

class TranslationPluginManager : public QObject
{
    Q_OBJECT
public:
    static TranslationPluginManager& instance();

    // 当前处于激活状态的翻译引擎 (首选离线；若未安装则自动平滑降级为在线直连)
    ITranslator* activeTranslator();

    OfflinePluginTranslator* offlineTranslator() const { return m_offlineTranslator.get(); }
    OnlineFallbackTranslator* onlineTranslator() const { return m_onlineTranslator.get(); }

    // 离线翻译插件是否已在本地安装就绪
    bool isOfflinePluginReady() const;

    // 当前生效引擎的名称
    QString activeEngineName() const;

    // 当前生效引擎类型
    TranslationEngineType activeEngineType() const;

    // 用户首选引擎偏好 ("Offline" 或 "Online")
    QString preferredEnginePreference() const;
    void setPreferredEnginePreference(const QString& pref);

    // 重新扫描 plugins/ 目录刷新插件状态
    void refreshPlugins();

    // 当前支持的互译语言对列表
    QList<LanguagePair> currentSupportedLanguagePairs() const;

    // 统一异步翻译入口
    void translateAsync(const QString& text, const QString& srcLang, const QString& targetLang,
                        std::function<void(const TranslationResult&)> callback);

    // 语言自动嗅探辅助函数 (基于字形特征粗判源语言: "zh", "en", "ja", "ko", "ru" 等)
    static QString detectLanguageHeuristic(const QString& text);

    // 获取离线插件目录说明与官方下载页面地址
    QString pluginDirectory() const;
    QString pluginDownloadUrl() const;

signals:
    void engineChanged(const QString& engineName, bool isOffline);

private:
    explicit TranslationPluginManager(QObject* parent = nullptr);
    ~TranslationPluginManager() override = default;

    std::unique_ptr<OfflinePluginTranslator> m_offlineTranslator;
    std::unique_ptr<OnlineFallbackTranslator> m_onlineTranslator;
};
