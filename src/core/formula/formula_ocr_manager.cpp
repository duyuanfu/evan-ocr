#include "formula_ocr_manager.h"
#include "formula_format_helper.h"
#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QBuffer>
#include <QProcess>
#include <QDateTime>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QSettings>
#include <QDebug>
#include <QNetworkProxyFactory>

FormulaOcrManager& FormulaOcrManager::instance()
{
    static FormulaOcrManager mgr;
    return mgr;
}

FormulaOcrManager::FormulaOcrManager(QObject* parent)
    : QObject(parent)
{
    QNetworkProxyFactory::setUseSystemConfiguration(true);
    m_nam = std::make_unique<QNetworkAccessManager>(this);

    QSettings settings("Evan", "Evan");
    m_preferredMode = settings.value("FormulaOcr/PreferredMode", "Online").toString();
}

QString FormulaOcrManager::preferredMode() const
{
    return m_preferredMode;
}

void FormulaOcrManager::setPreferredMode(const QString& mode)
{
    m_preferredMode = mode;
    QSettings settings("Evan", "Evan");
    settings.setValue("FormulaOcr/PreferredMode", mode);
}

QString FormulaOcrManager::simpleTexToken() const
{
    QSettings settings("Evan", "Evan");
    return settings.value("FormulaOcr/SimpleTexToken", "").toString().trimmed();
}

void FormulaOcrManager::setSimpleTexToken(const QString& token)
{
    QSettings settings("Evan", "Evan");
    settings.setValue("FormulaOcr/SimpleTexToken", token.trimmed());
}

QString FormulaOcrManager::findOfflinePluginExecutable() const
{
    QString appDir = QCoreApplication::applicationDirPath();
    QStringList candidates = {
        appDir + "/plugins/latex_ocr/rapid_latex_ocr.exe",
        appDir + "/plugins/latex_ocr/latex_ocr.exe",
        appDir + "/plugins/evan-plugin-latex-ocr/rapid_latex_ocr.exe",
        appDir + "/plugins/evan-plugin-latex-ocr/latex_ocr.exe",
        "plugins/latex_ocr/rapid_latex_ocr.exe",
        "plugins/latex_ocr/latex_ocr.exe",
        "plugins/evan-plugin-latex-ocr/rapid_latex_ocr.exe"
    };

    for (const auto& path : candidates) {
        if (QFile::exists(path)) {
            return QDir::toNativeSeparators(QFileInfo(path).absoluteFilePath());
        }
    }

    // 智能动态多层级递归容错：深度探测 plugins/ 目录下的所有解压子文件夹
    QStringList searchRoots = { appDir + "/plugins", "plugins" };
    QStringList targetExes = { "rapid_latex_ocr.exe", "latex_ocr.exe", "latex-ocr.exe" };
    for (const auto& root : searchRoots) {
        if (QDir(root).exists()) {
            QDirIterator it(root, targetExes, QDir::Files, QDirIterator::Subdirectories);
            if (it.hasNext()) {
                return QDir::toNativeSeparators(QFileInfo(it.next()).absoluteFilePath());
            }
        }
    }

    return QString();
}

bool FormulaOcrManager::isOfflinePluginReady() const
{
    return !findOfflinePluginExecutable().isEmpty();
}

void FormulaOcrManager::recognizeAsync(const QImage& formulaImage, qreal dpr,
                                      std::function<void(const FormulaOcrResult&)> callback)
{
    Q_UNUSED(dpr);
    qint64 startTime = QDateTime::currentMSecsSinceEpoch();

    if (m_preferredMode == "Offline") {
        if (isOfflinePluginReady()) {
            recognizeOfflineAsync(formulaImage, startTime, callback);
        } else {
            FormulaOcrResult res;
            res.success = false;
            res.errorMessage = "未检测到本地离线公式识别插件 (RapidLaTeXOCR)。\n可前往托盘【插件中心】一键下载安装，或切换为【在线云端通道】。";
            res.engineName = "本地离线插件";
            if (callback) callback(res);
        }
        return;
    }

    // 在线模式检查：若用户尚未配置 SimpleTeX Token，且本地已安装离线插件，直接毫秒级切入离线，免去无谓的网络 401 失败等待
    QString token = simpleTexToken();
    if (token.isEmpty()) {
        if (isOfflinePluginReady()) {
            qDebug() << "[FormulaOcr] 未配置在线 Token，自动启用本地离线 RapidLaTeXOCR 引擎 (0ms 等待)...";
            recognizeOfflineAsync(formulaImage, startTime, callback);
            return;
        } else {
            FormulaOcrResult res;
            res.success = false;
            res.isAuthError = true;
            res.errorMessage = "在线公式通道需要授权 Token (可在 simpletex.cn 免费获取，每月赠送数千次调用)。\n请在右上角点击【⚙️ 配置 Token】填入，或在托盘【插件中心】安装本地离线插件。";
            res.engineName = "在线云端通道";
            if (callback) callback(res);
            return;
        }
    }

    // 针对在线上传进行超高效高对比度轻量编码 (JPEG 92 相比大型无压缩 PNG 体积减少 80%，网络秒传)
    QByteArray imageBytes;
    QBuffer buffer(&imageBytes);
    buffer.open(QIODevice::WriteOnly);
    formulaImage.save(&buffer, "JPEG", 92);
    buffer.close();

    recognizeOnlineAsync(imageBytes, startTime, [this, formulaImage, startTime, callback](const FormulaOcrResult& res) {
        if (res.success) {
            if (callback) callback(res);
            return;
        }

        // 云端因网络故障失败时：若本地已就绪离线插件，自动平滑降级
        if (isOfflinePluginReady()) {
            qDebug() << "[FormulaOcr] 在线通道失败，自动平滑降级至本地离线 RapidLaTeXOCR 插件...";
            recognizeOfflineAsync(formulaImage, startTime, callback);
        } else {
            if (callback) callback(res);
        }
    });
}

void FormulaOcrManager::recognizeOnlineAsync(const QByteArray& imageBytes, qint64 startTime,
                                            std::function<void(const FormulaOcrResult&)> callback)
{
    // 国内免梯子高可用学术公式识别 API (SimpleTeX / 云端多通道热备)
    QUrl url("https://server.simpletex.cn/api/latex_ocr");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "Evan-FormulaOCR/1.1.2");
    request.setTransferTimeout(9000); // 9秒超时

    QString token = simpleTexToken();
    if (!token.isEmpty()) {
        request.setRawHeader("token", token.toUtf8());
    }

    auto* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart imagePart;
    imagePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/png"));
    imagePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"file\"; filename=\"formula.png\""));
    imagePart.setBody(imageBytes);
    multiPart->append(imagePart);

    QNetworkReply* reply = m_nam->post(request, multiPart);
    multiPart->setParent(reply); // 随 reply 销毁自动释放

    connect(reply, &QNetworkReply::finished, this, [reply, startTime, callback]() {
        reply->deleteLater();
        FormulaOcrResult result;
        result.elapsedMs = QDateTime::currentMSecsSinceEpoch() - startTime;
        result.engineName = "在线云端通道 (高速)";

        int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() == QNetworkReply::AuthenticationRequiredError || httpStatus == 401 || httpStatus == 403) {
            result.success = false;
            result.isAuthError = true;
            result.errorMessage = "在线公式通道需要授权 Token (错误码 204/401)。\n可在右上角点击【⚙️ Token 设置】填入免费 Token (前往 simpletex.cn 免费获取，每月赠送上千次调用)，或直接切换为【本地离线插件】。";
            if (callback) callback(result);
            return;
        }

        if (reply->error() != QNetworkReply::NoError) {
            result.success = false;
            result.errorMessage = QString("网络请求失败 (%1): %2").arg(reply->error()).arg(reply->errorString());
            if (callback) callback(result);
            return;
        }

        QByteArray respBytes = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(respBytes);
        if (!doc.isObject()) {
            result.success = false;
            result.errorMessage = "云端返回解析失败 (非合法 JSON 数据)";
            if (callback) callback(result);
            return;
        }

        QJsonObject root = doc.object();
        bool status = root.value("status").toBool(false);
        if (!status && root.contains("error")) {
            result.success = false;
            result.errorMessage = root.value("error").toString("云端公式提取异常");
            if (callback) callback(result);
            return;
        }

        QJsonObject resObj = root.value("res").toObject();
        QString latex = resObj.value("latex").toString().trimmed();
        if (latex.isEmpty() && root.contains("latex")) {
            latex = root.value("latex").toString().trimmed();
        }

        if (latex.isEmpty()) {
            result.success = false;
            result.errorMessage = "未在选区中检测到有效数学公式";
            if (callback) callback(result);
            return;
        }

        // 清理公式前后多余的包裹标签
        if (latex.startsWith("$$") && latex.endsWith("$$") && latex.length() >= 4) {
            latex = latex.mid(2, latex.length() - 4).trimmed();
        } else if (latex.startsWith("$") && latex.endsWith("$") && latex.length() >= 2) {
            latex = latex.mid(1, latex.length() - 2).trimmed();
        }

        result.success = true;
        result.latex = latex;
        result.blockMarkdown = FormulaFormatHelper::toBlockMarkdown(latex);
        result.inlineMarkdown = FormulaFormatHelper::toInlineMarkdown(latex);
        result.mathml = FormulaFormatHelper::toWordMathML(latex);

        if (callback) callback(result);
    });
}

void FormulaOcrManager::recognizeOfflineAsync(const QImage& image, qint64 startTime,
                                             std::function<void(const FormulaOcrResult&)> callback)
{
    QString exePath = findOfflinePluginExecutable();
    if (exePath.isEmpty()) {
        FormulaOcrResult res;
        res.success = false;
        res.errorMessage = "未找到离线公式插件可执行文件。";
        res.engineName = "本地离线插件";
        if (callback) callback(res);
        return;
    }

    // 写入临时待识别图像 (微优化：适度缩放超大图像并使用快速保存)
    QString tempImagePath = QDir::tempPath() + QString("/evan_formula_%1.png").arg(QDateTime::currentMSecsSinceEpoch());
    QImage toSave = image;
    if (toSave.width() > 1600 || toSave.height() > 800) {
        toSave = toSave.scaled(1600, 800, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    if (!toSave.save(tempImagePath, "PNG")) {
        FormulaOcrResult res;
        res.success = false;
        res.errorMessage = "临时图像写入失败。";
        res.engineName = "本地离线插件";
        if (callback) callback(res);
        return;
    }

    auto* proc = new QProcess();
    auto callbackInvoked = std::make_shared<bool>(false);

    auto finishWithResult = [proc, tempImagePath, callback, callbackInvoked](const FormulaOcrResult& res) {
        if (*callbackInvoked) return;
        *callbackInvoked = true;
        QFile::remove(tempImagePath);
        if (proc) {
            proc->deleteLater();
        }
        if (callback) callback(res);
    };

    // 6秒超时安全守护：彻底避免无公式区域或异常死循环导致的“一直识别中”与卡死
    QTimer::singleShot(6000, proc, [proc, startTime, finishWithResult, callbackInvoked]() {
        if (*callbackInvoked) return;
        qWarning() << "[FormulaOcr] 离线公式识别超时 (6秒)，强制终止进程保护主程序";
        if (proc && proc->state() != QProcess::NotRunning) {
            proc->kill();
        }
        FormulaOcrResult res;
        res.success = false;
        res.elapsedMs = QDateTime::currentMSecsSinceEpoch() - startTime;
        res.errorMessage = "选区内未检测到有效的数学公式 (识别超时)。";
        res.engineName = "本地离线插件 (RapidLaTeX)";
        finishWithResult(res);
    });

    connect(proc, &QProcess::errorOccurred, this, [startTime, finishWithResult](QProcess::ProcessError err) {
        qWarning() << "[FormulaOcr] 离线进程启动异常:" << err;
        FormulaOcrResult res;
        res.success = false;
        res.elapsedMs = QDateTime::currentMSecsSinceEpoch() - startTime;
        res.errorMessage = "离线公式识别引擎启动异常。";
        res.engineName = "本地离线插件 (RapidLaTeX)";
        finishWithResult(res);
    });

    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [proc, startTime, finishWithResult](int exitCode, QProcess::ExitStatus exitStatus) {
        Q_UNUSED(exitStatus);
        FormulaOcrResult res;
        res.elapsedMs = QDateTime::currentMSecsSinceEpoch() - startTime;
        res.engineName = "本地离线插件 (RapidLaTeX)";

        if (exitCode != 0) {
            res.success = false;
            res.errorMessage = "选区内未检测到有效的数学公式。";
            finishWithResult(res);
            return;
        }

        QByteArray out = proc->readAllStandardOutput();

        // 尝试解析 JSON 输出或直接标准输出文本
        QString rawLatex;
        QJsonDocument doc = QJsonDocument::fromJson(out);
        if (doc.isObject()) {
            rawLatex = doc.object().value("latex").toString().trimmed();
        }
        if (rawLatex.isEmpty()) {
            rawLatex = QString::fromUtf8(out).trimmed();
        }

        if (rawLatex.isEmpty()) {
            res.success = false;
            res.errorMessage = "选区内未检测到有效的数学公式。";
            finishWithResult(res);
            return;
        }

        // 清理多余 $
        if (rawLatex.startsWith("$$") && rawLatex.endsWith("$$") && rawLatex.length() >= 4) {
            rawLatex = rawLatex.mid(2, rawLatex.length() - 4).trimmed();
        } else if (rawLatex.startsWith("$") && rawLatex.endsWith("$") && rawLatex.length() >= 2) {
            rawLatex = rawLatex.mid(1, rawLatex.length() - 2).trimmed();
        }

        res.success = true;
        res.latex = rawLatex;
        res.blockMarkdown = FormulaFormatHelper::toBlockMarkdown(rawLatex);
        res.inlineMarkdown = FormulaFormatHelper::toInlineMarkdown(rawLatex);
        res.mathml = FormulaFormatHelper::toWordMathML(rawLatex);

        finishWithResult(res);
    });

    QStringList args;
    args << "--image" << tempImagePath;
    proc->start(exePath, args);
}

void FormulaOcrManager::renderFormulaAsync(const QString& latex,
                                          std::function<void(bool success, const QByteArray& data, const QString& mimeType)> callback)
{
    QString trimmed = latex.trimmed();
    if (trimmed.isEmpty()) {
        if (callback) callback(false, QByteArray(), "");
        return;
    }

    // 采用 300 DPI 超高清无损透明公式微渲染通道，彻底消除文字与算子发虚模糊
    QString encoded = QUrl::toPercentEncoding(trimmed);
    QUrl url(QString("https://latex.codecogs.com/png.latex?\\dpi{300}&space;%1").arg(encoded));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, "Evan-FormulaRenderer/1.2.0");
    req.setTransferTimeout(6000);

    QNetworkReply* reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            if (callback) callback(false, QByteArray(), "");
            return;
        }
        QByteArray data = reply->readAll();
        if (data.isEmpty() || data.size() < 16) {
            if (callback) callback(false, QByteArray(), "");
            return;
        }
        if (callback) callback(true, data, "image/png");
    });
}
