#include "window.h"
#include "ui_app.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QString>

DragonMainWindow::DragonMainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::DragonMainWindow)
{
    ui->setupUi(this);
}

DragonMainWindow::~DragonMainWindow()
{
    delete ui;
}

void DragonMainWindow::setFPS(const int fps) {
    ui->statusFPS->setText(QString::number(fps));
}

HWND DragonMainWindow::getViewportHWND() {
    return ui->viewport->getNativeWindowHanle();
}
