#include "rapid_ocr_engine.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QThreadPool>
#include <QMetaObject>
#include <QDebug>
#include <algorithm>

RapidOcrEngine::RapidOcrEngine(QObject* parent)
    : QObject(parent)
{
}

QString RapidOcrEngine::findExecutablePath() const
{
    QString appDir = QCoreApplication::applicationDirPath();
    QStringList candidates = {
        appDir + "/rapidocr/RapidOCR-json.exe",
        appDir + "/rapidocr/rapidocr.exe",
        appDir + "/RapidOCR-json.exe",
        appDir + "/rapidocr.exe",
        "rapidocr/RapidOCR-json.exe",
        "rapidocr/rapidocr.exe",
        "RapidOCR-json.exe"
    };

    for (const auto& path : candidates) {
        if (QFile::exists(path)) {
            return QDir::toNativeSeparators(QFileInfo(path).absoluteFilePath());
        }
    }
    return QString();
}

QString RapidOcrEngine::findModelsDir() const
{
    QString appDir = QCoreApplication::applicationDirPath();
    QStringList candidates = {
        appDir + "/rapidocr/models",
        appDir + "/models",
        "rapidocr/models",
        "models"
    };

    for (const auto& dir : candidates) {
        if (QDir(dir).exists()) {
            return QDir::toNativeSeparators(QFileInfo(dir).absoluteFilePath());
        }
    }
    return QString();
}

bool RapidOcrEngine::isAvailable() const
{
    QString exe = findExecutablePath();
    return !exe.isEmpty();
}

QString RapidOcrEngine::componentPath() const
{
    QString exe = findExecutablePath();
    if (!exe.isEmpty()) return exe;
    return "未安装 (检测路径: rapidocr/RapidOCR-json.exe)";
}

OcrResult RapidOcrEngine::recognize(const QImage& image, qreal dpr)
{
    OcrResult result;
    result.dpr = dpr;

    if (image.isNull() || image.width() <= 0 || image.height() <= 0) {
        result.success = false;
        result.errorMessage = "待识别图像数据为空";
        return result;
    }

    QString exePath = findExecutablePath();
    if (exePath.isEmpty()) {
        result.success = false;
        result.errorMessage = "未检测到 RapidOCR 运行组件。\n请在程序目录 rapidocr/ 放置 RapidOCR-json.exe 及模型文件。";
        return result;
    }

    // 将图像写入临时高速缓存文件
    QString tempFile = QDir::tempPath() + QString("/evan_rapidocr_%1_%2.png")
                       .arg(QCoreApplication::applicationPid())
                       .arg(QDateTime::currentMSecsSinceEpoch());

    if (!image.save(tempFile, "PNG")) {
        result.success = false;
        result.errorMessage = "临时图像写入失败";
        return result;
    }

    // 组装命令行参数并调用 RapidOCR 进程
    QProcess process;
    process.setProgram(exePath);
    process.setWorkingDirectory(QFileInfo(exePath).absolutePath());

    QStringList args;
    args << "--ensureAscii=0";
    args << "--ensureLogger=0";
    args << QString("--image_path=%1").arg(QDir::toNativeSeparators(tempFile));

    process.setArguments(args);
    process.start();

    if (!process.waitForStarted(3000)) {
        QFile::remove(tempFile);
        result.success = false;
        result.errorMessage = "启动 RapidOCR 推理进程超时失败";
        return result;
    }

    if (!process.waitForFinished(15000)) {
        process.kill();
        QFile::remove(tempFile);
        result.success = false;
        result.errorMessage = "RapidOCR 识别处理超时";
        return result;
    }

    QByteArray output = process.readAllStandardOutput();
    QFile::remove(tempFile);

    if (output.isEmpty()) {
        QByteArray errOutput = process.readAllStandardError();
        result.success = false;
        result.errorMessage = errOutput.isEmpty() ? "RapidOCR 未返回识别数据" : QString::fromUtf8(errOutput);
        return result;
    }

    // 截取纯净的 JSON 文本内容 (过滤掉进程启动输出的 Banner 与日志行)
    int startIdx = output.indexOf('{');
    int endIdx = output.lastIndexOf('}');
    QByteArray jsonBytes;
    if (startIdx >= 0 && endIdx >= startIdx) {
        jsonBytes = output.mid(startIdx, endIdx - startIdx + 1);
    } else {
        jsonBytes = output;
    }

    // 解析 JSON 识别结果
    QJsonParseError parseErr;
    QJsonDocument doc = QJsonDocument::fromJson(jsonBytes, &parseErr);
    if (doc.isNull() || !doc.isObject()) {
        // 如果不是标准 json，尝试按纯文本行处理
        QString rawStr = QString::fromUtf8(output).trimmed();
        if (!rawStr.isEmpty()) {
            result.success = true;
            result.fullText = rawStr;
            OcrLine line;
            line.text = rawStr;
            line.logicalBox = QRect(0, 0, static_cast<int>(image.width() / dpr), static_cast<int>(image.height() / dpr));
            result.lines.append(line);
            return result;
        }

        result.success = false;
        result.errorMessage = QString("解析 RapidOCR 结果失败: %1").arg(parseErr.errorString());
        return result;
    }

    QJsonObject root = doc.object();
    int code = root.value("code").toInt(0);

    // code 101 表示图中未发现可识别文字
    if (code == 101) {
        result.success = true;
        result.fullText = "";
        return result;
    }

    // 标准 RapidOCR-json: code 100 表示成功
    if (code != 100 && root.contains("code")) {
        result.success = false;
        result.errorMessage = root.value("msg").toString("RapidOCR 推理异常");
        return result;
    }

    QJsonArray dataArr = root.value("data").toArray();
    result.success = true;
    QString fullText;

    for (const auto& val : dataArr) {
        if (!val.isObject()) continue;
        QJsonObject block = val.toObject();

        QString text = block.value("text").toString().trimmed();
        if (text.isEmpty()) continue;

        double score = block.value("score").toDouble(1.0);

        // 提取 4 个顶点坐标 [[x1, y1], [x2, y2], [x3, y3], [x4, y4]]
        QJsonArray boxArr = block.value("box").toArray();
        int minX = 999999, minY = 999999, maxX = -1, maxY = -1;

        for (const auto& ptVal : boxArr) {
            QJsonArray pt = ptVal.toArray();
            if (pt.size() >= 2) {
                int px = pt.at(0).toInt();
                int py = pt.at(1).toInt();
                minX = (std::min)(minX, px);
                minY = (std::min)(minY, py);
                maxX = (std::max)(maxX, px);
                maxY = (std::max)(maxY, py);
            }
        }

        QRect physBox;
        if (minX <= maxX && minY <= maxY) {
            physBox = QRect(minX, minY, maxX - minX, maxY - minY);
        } else {
            physBox = QRect(0, 0, image.width(), image.height());
        }

        OcrLine lineObj;
        lineObj.text = text;
        lineObj.confidence = static_cast<float>(score);
        lineObj.boundingBox = physBox;
        lineObj.logicalBox = QRect(
            static_cast<int>(std::round(physBox.x() / dpr)),
            static_cast<int>(std::round(physBox.y() / dpr)),
            static_cast<int>(std::round(physBox.width() / dpr)),
            static_cast<int>(std::round(physBox.height() / dpr))
        );

        // 模拟词级别包围盒
        OcrWord wordObj;
        wordObj.text = text;
        wordObj.confidence = static_cast<float>(score);
        wordObj.boundingBox = lineObj.boundingBox;
        wordObj.logicalBox = lineObj.logicalBox;
        lineObj.words.append(wordObj);

        result.lines.append(lineObj);

        if (!fullText.isEmpty()) {
            fullText += "\n";
        }
        fullText += text;
    }

    result.fullText = fullText;
    return result;
}

void RapidOcrEngine::recognizeAsync(const QImage& image, qreal dpr, std::function<void(const OcrResult&)> callback)
{
    QImage imgCopy = image.copy();
    QThreadPool::globalInstance()->start([this, imgCopy, dpr, callback]() {
        OcrResult res = recognize(imgCopy, dpr);
        QMetaObject::invokeMethod(this, [callback, res]() {
            if (callback) {
                callback(res);
            }
        });
    });
}
