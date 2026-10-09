#include "translation_plugin_manager.h"
#include <QSettings>
#include <QCoreApplication>
#include <QDir>
#include <QDebug>

TranslationPluginManager::TranslationPluginManager(QObject* parent)
    : QObject(parent)
{
    m_offlineTranslator = std::make_unique<OfflinePluginTranslator>();
    m_onlineTranslator = std::make_unique<OnlineFallbackTranslator>();
}

TranslationPluginManager& TranslationPluginManager::instance()
{
    static TranslationPluginManager mgr;
    return mgr;
}

bool TranslationPluginManager::isOfflinePluginReady() const
{
    return m_offlineTranslator && m_offlineTranslator->isAvailable();
}

QString TranslationPluginManager::preferredEnginePreference() const
{
    QSettings settings("Evan", "Evan");
    // 默认首选离线插件；未安装时自动降级在线
    return settings.value("Translation/Engine", "Offline").toString();
}

void TranslationPluginManager::setPreferredEnginePreference(const QString& pref)
{
    QSettings settings("Evan", "Evan");
    settings.setValue("Translation/Engine", pref);
    emit engineChanged(activeEngineName(), activeEngineType() == TranslationEngineType::OfflinePlugin);
}

void TranslationPluginManager::refreshPlugins()
{
    m_offlineTranslator = std::make_unique<OfflinePluginTranslator>();
    emit engineChanged(activeEngineName(), activeEngineType() == TranslationEngineType::OfflinePlugin);
}

ITranslator* TranslationPluginManager::activeTranslator()
{
    QString pref = preferredEnginePreference();
    if (pref == "Offline" && isOfflinePluginReady()) {
        return m_offlineTranslator.get();
    }
    if (pref == "Offline" && !isOfflinePluginReady()) {
        // 自动平滑回退到在线直连
        return m_onlineTranslator.get();
    }
    return m_onlineTranslator.get();
}

QString TranslationPluginManager::activeEngineName() const
{
    if (preferredEnginePreference() == "Offline" && isOfflinePluginReady()) {
        return m_offlineTranslator->name();
    }
    if (preferredEnginePreference() == "Offline" && !isOfflinePluginReady()) {
        return "在线直连 (未检测到离线模型，已自动降级)";
    }
    return m_onlineTranslator->name();
}

TranslationEngineType TranslationPluginManager::activeEngineType() const
{
    if (preferredEnginePreference() == "Offline" && isOfflinePluginReady()) {
        return TranslationEngineType::OfflinePlugin;
    }
    return TranslationEngineType::OnlineFallback;
}

QList<LanguagePair> TranslationPluginManager::currentSupportedLanguagePairs() const
{
    if (preferredEnginePreference() == "Offline" && isOfflinePluginReady()) {
        return m_offlineTranslator->supportedLanguagePairs();
    }
    return m_onlineTranslator->supportedLanguagePairs();
}

void TranslationPluginManager::translateAsync(const QString& text, const QString& srcLang, const QString& targetLang,
                                             std::function<void(const TranslationResult&)> callback)
{
    ITranslator* translator = activeTranslator();
    if (!translator) {
        TranslationResult err;
        err.success = false;
        err.errorMessage = "没有可用的翻译引擎驱动";
        if (callback) callback(err);
        return;
    }

    translator->translateAsync(text, srcLang, targetLang, callback);
}

QString TranslationPluginManager::detectLanguageHeuristic(const QString& text)
{
    if (text.trimmed().isEmpty()) return "auto";

    int cjkCount = 0;
    int kanaCount = 0;
    int hangulCount = 0;
    int cyrillicCount = 0;
    int latinCount = 0;
    int totalLetters = 0;

    for (int i = 0; i < text.size(); ++i) {
        ushort u = text.at(i).unicode();
        if (text.at(i).isLetter()) {
            totalLetters++;
            if ((u >= 0x3040 && u <= 0x309F) || (u >= 0x30A0 && u <= 0x30FF)) {
                kanaCount++;
            } else if ((u >= 0xAC00 && u <= 0xD7AF) || (u >= 0x1100 && u <= 0x11FF)) {
                hangulCount++;
            } else if (u >= 0x0400 && u <= 0x04FF) {
                cyrillicCount++;
            } else if ((u >= 0x4E00 && u <= 0x9FFF) || (u >= 0x3400 && u <= 0x4DBF)) {
                cjkCount++;
            } else if ((u >= 'a' && u <= 'z') || (u >= 'A' && u <= 'Z')) {
                latinCount++;
            }
        }
    }

    if (totalLetters == 0) return "auto";

    if (kanaCount > 0) return "ja";
    if (hangulCount > 0) return "ko";
    if (cyrillicCount > totalLetters * 0.3) return "ru";
    if (cjkCount > totalLetters * 0.2) return "zh";
    if (latinCount > totalLetters * 0.4) return "en";

    return "auto";
}

QString TranslationPluginManager::pluginDirectory() const
{
    QString dir = QCoreApplication::applicationDirPath() + "/plugins/translation";
    return QDir::toNativeSeparators(dir);
}

QString TranslationPluginManager::pluginDownloadUrl() const
{
    return "https://github.com/duyuanfu/evan-ocr/releases";
}
