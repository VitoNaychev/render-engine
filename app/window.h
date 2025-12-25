#ifndef WINDOW_H
#define WINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class DragonMainWindow;
}
QT_END_NAMESPACE

class DragonMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    DragonMainWindow(QWidget *parent = nullptr);
    ~DragonMainWindow();

    void setFPS(const int fps);

    HWND getViewportHWND();

    // void closeEvent(QCloseEvent* event);

private:
    Ui::DragonMainWindow *ui;
};
#endif // WINDOW_H
