#ifndef VIEWPORT_H
#define VIEWPORT_H

#include <QWidget>

class ViewportWidget : public QWidget {
    Q_OBJECT

public:
    ViewportWidget(QWidget* parent = nullptr) : QWidget(parent) {}

    void updateImage(const QImage& image);
protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QImage image;

};

#endif
