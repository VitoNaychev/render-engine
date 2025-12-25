#include "viewport.h"
#include <QPainter>


ViewportWidget::ViewportWidget(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_DontCreateNativeAncestors);
}

HWND ViewportWidget::getNativeWindowHanle() {
    return reinterpret_cast<HWND>(winId());
}

void ViewportWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.drawImage(rect(), image);
}
