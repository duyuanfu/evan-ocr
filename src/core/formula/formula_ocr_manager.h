#pragma once

#include <QObject>
#include <QImage>
#include <QString>
#include <QByteArray>
#include <functional>
#include <memory>

struct FormulaOcrResult
{
    bool success = false;
    bool isAuthError = false;     // 是否因缺少或错误的授权 Token 导致失败 (HTTP 401 / 204)
    QString errorMessage;
    QString latex;                // 核心 LaTeX 源码
    QString mathml;               // 适用于 Word 的 MathML
    QString blockMarkdown;        // $$ ... $$ 块级 Markdown
    QString inlineMarkdown;       // $ ... $ 行内 Markdown
    QByteArray previewSvgBytes;   // 实时渲染的高清矢量 SVG / PNG 图片数据
    QString engineName;           // 执行识别的引擎通道名称
    qint64 elapsedMs = 0;         // 识别耗时 (毫秒)
};

class QNetworkAccessManager;

class FormulaOcrManager : public QObject
{
    Q_OBJECT
public:
    static FormulaOcrManager& instance();

    // 检查本地离线公式识别插件是否已就绪
    bool isOfflinePluginReady() const;

    // 获取离线插件的可执行文件物理路径
    QString findOfflinePluginExecutable() const;

    // 当前通道模式偏好 ("Online" 默认 / "Offline")
    QString preferredMode() const;
    void setPreferredMode(const QString& mode);

    // SimpleTeX 在线授权 Token 管理 (simpletex.cn 每月免费赠送上千次调用)
    QString simpleTexToken() const;
    void setSimpleTexToken(const QString& token);

    // 异步发起公式识别
    void recognizeAsync(const QImage& formulaImage, qreal dpr,
                        std::function<void(const FormulaOcrResult&)> callback);

    // 请求在线公式矢量渲染图 (返回 SVG / PNG 字节流)
    void renderFormulaAsync(const QString& latex,
                            std::function<void(bool success, const QByteArray& data, const QString& mimeType)> callback);

private:
    explicit FormulaOcrManager(QObject* parent = nullptr);
    ~FormulaOcrManager() override = default;

    void recognizeOnlineAsync(const QByteArray& imageBytes, qint64 startTime,
                              std::function<void(const FormulaOcrResult&)> callback);

    void recognizeOfflineAsync(const QImage& image, qint64 startTime,
                               std::function<void(const FormulaOcrResult&)> callback);

    void ensureDaemonRunning();
    void stopDaemon();

    std::unique_ptr<QNetworkAccessManager> m_nam;
    QString m_preferredMode = "Online";

    class QProcess* m_daemonProc = nullptr;
    bool m_daemonReady = false;
};
