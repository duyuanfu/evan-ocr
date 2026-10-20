#pragma once

#include <QObject>
#include <QString>
#include <QNetworkProxy>
#include <functional>

enum class ProxyType {
    System = 0,   // 跟随系统代理设置 (默认)
    Custom = 1,   // 自定义代理 (HTTP / SOCKS5)
    Direct = 2    // 直连 (不走任何代理)
};

enum class ProxyProtocol {
    Http = 0,     // HTTP / HTTPS 代理
    Socks5 = 1    // SOCKS5 代理
};

class NetworkProxyManager : public QObject
{
    Q_OBJECT
public:
    static NetworkProxyManager& instance();

    // 属性获取与修改
    ProxyType proxyType() const { return m_type; }
    void setProxyType(ProxyType type);

    ProxyProtocol protocol() const { return m_protocol; }
    void setProtocol(ProxyProtocol proto);

    QString host() const { return m_host; }
    void setHost(const QString& host);

    quint16 port() const { return m_port; }
    void setPort(quint16 port);

    // 持久化与全局生效
    void load();
    void save();
    void apply();

    // 格式智能解析：支持直接粘贴 http://127.0.0.1:7890、127.0.0.1:7890、socks5://127.0.0.1:1080 等
    static bool parseProxyString(const QString& input, QString& outHost, quint16& outPort, ProxyProtocol& outProtocol);

    // 异步连通性与测速探针 (超时 5 秒)
    void testConnection(const QString& targetUrl, std::function<void(bool success, qint64 elapsedMs, const QString& errorMsg)> callback);

signals:
    void proxyConfigChanged();

private:
    explicit NetworkProxyManager(QObject* parent = nullptr);
    ~NetworkProxyManager() override = default;

    ProxyType m_type = ProxyType::System;
    ProxyProtocol m_protocol = ProxyProtocol::Http;
    QString m_host = "127.0.0.1";
    quint16 m_port = 7890;
};
