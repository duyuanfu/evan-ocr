#pragma once

#include <QObject>
#include <QList>
#include "pin_window.h"

class PinManager : public QObject
{
    Q_OBJECT
public:
    static PinManager& instance();

    // 创建并管理一个新的贴图窗口
    PinWindow* createPin(const QPixmap& pixmap, const QRect& initialGeometry);

    // 清理所有贴图
    void closeAll();

private:
    explicit PinManager(QObject* parent = nullptr) : QObject(parent) {}
    ~PinManager() override = default;

    QList<PinWindow*> m_pins;
};
