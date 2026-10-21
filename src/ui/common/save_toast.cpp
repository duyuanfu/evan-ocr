#include "save_toast.h"

#include "../../core/theme_manager.h"

#include <QPainter>
#include <QPainterPath>
#include <QGuiApplication>
#include <QScreen>
#include <QCursor>
#include <QTimer>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QFontMetrics>
#include <QDir>

namespace {
constexpr int kMarginBottom = 96;      // 距屏幕底部的留白
constexpr int kPaddingH = 18;
constexpr int kPaddingV = 14;
constexpr int kIconSize = 26;
constexpr int kGap = 12;
constexpr int kMaxPathWidth = 520;     // 路径过长时省略
}

SaveToast::SaveToast(QWidget* parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint
                   | Qt::WindowStaysOnTopHint
                   | Qt::Tool
                   | Qt::WindowTransparentForInput);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    setFocusPolicy(Qt::NoFocus);
}

SaveToast::~SaveToast() = default;

void SaveToast::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    const bool dark = ThemeManager::instance().isDarkMode();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    // ---- 投影 ----
    for (int i = 6; i >= 1; --i) {
        QColor shadow(0, 0, 0, dark ? 90 : 45);
        shadow.setAlpha(shadow.alpha() / 6);
        painter.setPen(Qt::NoPen);
        painter.setBrush(shadow);
        painter.drawRoundedRect(QRectF(1, i, width() - 2, height() - 1), 12, 12);
    }

    // ---- 卡片 ----
    const QColor bg = dark ? QColor(45, 45, 45, 238) : QColor(255, 255, 255, 250);
    const QColor border = dark ? QColor(255, 255, 255, 30) : QColor(0, 0, 0, 26);
    painter.setPen(QPen(border, 1));
    painter.setBrush(bg);
    painter.drawRoundedRect(QRectF(0.5, 0.5, width() - 1, height() - 1), 12, 12);

    // ---- 成功图标 (圆形对勾) ----
    const int cy = height() / 2;
    const QRectF iconRect(kPaddingH, cy - kIconSize / 2, kIconSize, kIconSize);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(16, 124, 16, dark ? 210 : 235));
    painter.drawEllipse(iconRect);

    QPen checkPen(Qt::white, 2.0);
    checkPen.setCapStyle(Qt::RoundCap);
    checkPen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(checkPen);
    painter.drawPolyline(QPolygonF({
        QPointF(iconRect.left() + iconRect.width() * 0.28, iconRect.top() + iconRect.height() * 0.52),
        QPointF(iconRect.left() + iconRect.width() * 0.44, iconRect.top() + iconRect.height() * 0.68),
        QPointF(iconRect.left() + iconRect.width() * 0.73, iconRect.top() + iconRect.height() * 0.34)
    }));

    // ---- 文本 ----
    const int textLeft = kPaddingH + kIconSize + kGap;
    const int textWidth = qMax(0, width() - textLeft - kPaddingH);

    QFont titleFont = font();
    titleFont.setPointSizeF(9.5);
    titleFont.setBold(true);
    QFontMetrics titleFm(titleFont);

    QFont subFont = font();
    subFont.setPointSizeF(8.0);
    QFontMetrics subFm(subFont);

    if (m_subtitle.isEmpty()) {
        painter.setPen(dark ? QColor(255, 255, 255) : QColor(31, 31, 31));
        painter.setFont(titleFont);
        painter.drawText(QRect(textLeft, 0, textWidth, height()),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         titleFm.elidedText(m_title, Qt::ElideRight, textWidth));
        return;
    }

    const int titleH = titleFm.height();
    const int subH = subFm.height();
    const int totalH = titleH + subH + 2;
    const int top = (height() - totalH) / 2;

    painter.setFont(titleFont);
    painter.setPen(dark ? QColor(255, 255, 255) : QColor(31, 31, 31));
    painter.drawText(QRect(textLeft, top, textWidth, titleH),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     titleFm.elidedText(m_title, Qt::ElideRight, textWidth));

    painter.setFont(subFont);
    painter.setPen(dark ? QColor(191, 191, 191) : QColor(90, 90, 90));
    painter.drawText(QRect(textLeft, top + titleH + 2, textWidth, subH),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     subFm.elidedText(m_subtitle, Qt::ElideMiddle, textWidth));
}

void SaveToast::showToast(const QString& title, const QString& subtitle)
{
    auto* toast = new SaveToast();
    toast->m_title = title;
    toast->m_subtitle = subtitle;

    QFont titleFont = toast->font();
    titleFont.setPointSizeF(9.5);
    titleFont.setBold(true);
    const QFontMetrics titleFm(titleFont);

    QFont subFont = toast->font();
    subFont.setPointSizeF(8.0);
    const QFontMetrics subFm(subFont);

    int textWidth = titleFm.horizontalAdvance(title);
    if (!subtitle.isEmpty()) {
        textWidth = qMax(textWidth, qMin(kMaxPathWidth, subFm.horizontalAdvance(subtitle)));
    }

    const int textHeight = subtitle.isEmpty()
        ? titleFm.height()
        : titleFm.height() + subFm.height() + 2;

    // 文本可用宽度 = 控件宽度 - 左侧图标区 - 右侧留白
    const int textLeft = kPaddingH + kIconSize + kGap;
    toast->setFixedSize(textLeft + textWidth + kPaddingH,
                        qMax(kPaddingV * 2 + kIconSize, kPaddingV * 2 + textHeight));

    // 优先显示在鼠标所在屏幕，其次主屏
    QScreen* screen = QGuiApplication::screenAt(QCursor::pos());
    if (!screen) screen = QGuiApplication::primaryScreen();
    const QRect avail = screen->availableGeometry();
    toast->move(avail.center().x() - toast->width() / 2,
                avail.bottom() - toast->height() - kMarginBottom);

    toast->setWindowOpacity(0.0);
    toast->show();

    QPropertyAnimation* fade = new QPropertyAnimation(toast, "windowOpacity", toast);
    fade->setDuration(160);
    fade->setStartValue(0.0);
    fade->setEndValue(1.0);
    fade->setEasingCurve(QEasingCurve::OutCubic);
    fade->start(QAbstractAnimation::DeleteWhenStopped);

    // 2.6 秒后淡出并销毁
    QTimer::singleShot(2600, toast, [toast]() {
        QPropertyAnimation* out = new QPropertyAnimation(toast, "windowOpacity", toast);
        out->setDuration(320);
        out->setStartValue(toast->windowOpacity());
        out->setEndValue(0.0);
        out->setEasingCurve(QEasingCurve::InCubic);
        out->start(QAbstractAnimation::DeleteWhenStopped);
        QObject::connect(out, &QPropertyAnimation::finished, toast, &QObject::deleteLater);
    });
}
