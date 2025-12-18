#include "viewport.h"
#include <QPainter>

void ViewportWidget::updateImage(const QImage& image) {
    this->image = image;
    update();
}

void ViewportWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.drawImage(rect(), image);
}
