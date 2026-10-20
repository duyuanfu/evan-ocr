#include "network_proxy_manager.h"
#include <QSettings>
#include <QNetworkProxyFactory>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QElapsedTimer>
#include <QTimer>
#include <QUrl>
#include <QDebug>

NetworkProxyManager& NetworkProxyManager::instance()
{
    static NetworkProxyManager s_instance;
    return s_instance;
}

NetworkProxyManager::NetworkProxyManager(QObject* parent)
    : QObject(parent)
{
    load();
}

void NetworkProxyManager::setProxyType(ProxyType type)
{
    if (m_type != type) {
        m_type = type;
        apply();
        save();
    }
}

void NetworkProxyManager::setProtocol(ProxyProtocol proto)
{
    if (m_protocol != proto) {
        m_protocol = proto;
        if (m_type == ProxyType::Custom) {
            apply();
        }
        save();
    }
}

void NetworkProxyManager::setHost(const QString& host)
{
    QString h = host.trimmed();
    QString parsedHost;
    quint16 parsedPort = 0;
    ProxyProtocol parsedProto = ProxyProtocol::Http;
    if (parseProxyString(h, parsedHost, parsedPort, parsedProto)) {
        h = parsedHost;
        m_port = parsedPort;
        m_protocol = parsedProto;
    } else {
        if (h.startsWith("http://", Qt::CaseInsensitive)) h = h.mid(7);
        else if (h.startsWith("https://", Qt::CaseInsensitive)) h = h.mid(8);
        else if (h.startsWith("socks5://", Qt::CaseInsensitive)) h = h.mid(9);
        int colon = h.indexOf(':');
        if (colon > 0) h = h.left(colon);
        while (h.endsWith('/')) h.chop(1);
    }

    if (m_host != h) {
        m_host = h;
        if (m_type == ProxyType::Custom) {
            apply();
        }
        save();
    }
}

void NetworkProxyManager::setPort(quint16 port)
{
    if (m_port != port) {
        m_port = port;
        if (m_type == ProxyType::Custom) {
            apply();
        }
        save();
    }
}

void NetworkProxyManager::load()
{
    QSettings settings("Evan", "Evan");
    settings.beginGroup("NetworkProxy");

    int typeVal = settings.value("type", static_cast<int>(ProxyType::System)).toInt();
    m_type = static_cast<ProxyType>(qBound(0, typeVal, 2));

    int protoVal = settings.value("protocol", static_cast<int>(ProxyProtocol::Http)).toInt();
    m_protocol = static_cast<ProxyProtocol>(qBound(0, protoVal, 1));

    m_host = settings.value("host", "127.0.0.1").toString().trimmed();
    if (m_host.isEmpty()) m_host = "127.0.0.1";

    int portVal = settings.value("port", 7890).toInt();
    m_port = static_cast<quint16>(qBound(1, portVal, 65535));

    settings.endGroup();
}

void NetworkProxyManager::save()
{
    QSettings settings("Evan", "Evan");
    settings.beginGroup("NetworkProxy");

    settings.setValue("type", static_cast<int>(m_type));
    settings.setValue("protocol", static_cast<int>(m_protocol));
    settings.setValue("host", m_host);
    settings.setValue("port", m_port);

    settings.endGroup();
}

void NetworkProxyManager::apply()
{
    if (m_type == ProxyType::System) {
        // 跟随系统代理
        QNetworkProxyFactory::setUseSystemConfiguration(true);
        qDebug() << "[NetworkProxy] 已配置为: 跟随系统代理设置";
    } else if (m_type == ProxyType::Direct) {
        // 直连，禁用系统代理和全局代理
        QNetworkProxyFactory::setUseSystemConfiguration(false);
        QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
        qDebug() << "[NetworkProxy] 已配置为: 直连 (禁用任何代理)";
    } else if (m_type == ProxyType::Custom) {
        // 自定义代理 (HTTP / SOCKS5)
        QNetworkProxyFactory::setUseSystemConfiguration(false);
        QNetworkProxy::ProxyType qtProto = (m_protocol == ProxyProtocol::Socks5)
                                              ? QNetworkProxy::Socks5Proxy
                                              : QNetworkProxy::HttpProxy;
        QNetworkProxy proxy(qtProto, m_host, m_port);
        QNetworkProxy::setApplicationProxy(proxy);
        qDebug() << "[NetworkProxy] 已配置为: 自定义代理"
                 << (m_protocol == ProxyProtocol::Socks5 ? "SOCKS5" : "HTTP")
                 << QString("%1:%2").arg(m_host).arg(m_port);
    }

    emit proxyConfigChanged();
}

bool NetworkProxyManager::parseProxyString(const QString& input, QString& outHost, quint16& outPort, ProxyProtocol& outProtocol)
{
    QString s = input.trimmed();
    if (s.isEmpty()) return false;

    if (s.startsWith("socks5://", Qt::CaseInsensitive)) {
        outProtocol = ProxyProtocol::Socks5;
        s = s.mid(9);
    } else if (s.startsWith("http://", Qt::CaseInsensitive)) {
        outProtocol = ProxyProtocol::Http;
        s = s.mid(7);
    } else if (s.startsWith("https://", Qt::CaseInsensitive)) {
        outProtocol = ProxyProtocol::Http;
        s = s.mid(8);
    }

    // 移除尾部斜杠
    while (s.endsWith('/')) {
        s.chop(1);
    }

    int colonIdx = s.lastIndexOf(':');
    if (colonIdx > 0 && colonIdx < s.length() - 1) {
        QString hostCandidate = s.left(colonIdx).trimmed();
        bool ok = false;
        int portCandidate = s.mid(colonIdx + 1).trimmed().toInt(&ok);
        if (ok && portCandidate >= 1 && portCandidate <= 65535) {
            outHost = hostCandidate;
            outPort = static_cast<quint16>(portCandidate);
            return !outHost.isEmpty();
        }
    }

    return false;
}

void NetworkProxyManager::testConnection(const QString& targetUrl, std::function<void(bool, qint64, const QString&)> callback)
{
    QString urlStr = targetUrl.trimmed().isEmpty() ? "https://api.github.com" : targetUrl.trimmed();
    auto* nam = new QNetworkAccessManager();

    // 构造测试专用的代理对象
    if (m_type == ProxyType::System) {
        QNetworkProxyFactory::setUseSystemConfiguration(true);
    } else if (m_type == ProxyType::Direct) {
        nam->setProxy(QNetworkProxy::NoProxy);
    } else {
        QNetworkProxy testProxy(
            (m_protocol == ProxyProtocol::Socks5 ? QNetworkProxy::Socks5Proxy : QNetworkProxy::HttpProxy),
            m_host,
            m_port
        );
        nam->setProxy(testProxy);
    }

    QNetworkRequest req;
    req.setUrl(QUrl(urlStr));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setHeader(QNetworkRequest::UserAgentHeader, "Evan-Proxy-Test/1.0");

    auto* timer = new QElapsedTimer();
    timer->start();

    auto* reply = nam->get(req);

    // 5 秒超时保护
    auto* timeoutTimer = new QTimer();
    timeoutTimer->setSingleShot(true);
    QObject::connect(timeoutTimer, &QTimer::timeout, reply, [reply, nam, timer, timeoutTimer, callback]() {
        if (reply->isRunning()) {
            reply->abort();
            if (callback) {
                callback(false, timer->elapsed(), "连接超时 (超过 5 秒未收到响应)");
            }
        }
        timeoutTimer->deleteLater();
        delete timer;
        nam->deleteLater();
    });
    timeoutTimer->start(5000);

    QObject::connect(reply, &QNetworkReply::finished, reply, [reply, nam, timer, timeoutTimer, callback]() {
        if (!timeoutTimer->isActive()) {
            reply->deleteLater();
            return;
        }
        timeoutTimer->stop();
        timeoutTimer->deleteLater();

        qint64 ms = timer->elapsed();
        delete timer;

        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() == QNetworkReply::NoError || (statusCode >= 200 && statusCode < 400)) {
            if (callback) {
                callback(true, ms, QString("连接成功 (耗时 %1 ms)").arg(ms));
            }
        } else {
            QString err = reply->errorString();
            if (statusCode > 0) {
                err = QString("HTTP %1 (%2)").arg(statusCode).arg(err);
            }
            if (callback) {
                callback(false, ms, QString("连接失败: %1").arg(err));
            }
        }

        reply->deleteLater();
        nam->deleteLater();
    });
}
