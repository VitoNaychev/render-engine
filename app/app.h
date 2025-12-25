#ifndef APP_H
#define APP_H

#include "../engine/engine.h"
#include "window.h"

#include <QObject>

class DragonApp : public QObject {
    Q_OBJECT

public:
    bool init();

public slots:
    void onIdleTick();

    void onQuit();

private:
    bool initWindow();

    void renderFrame();

    void updateRenderStats();

private:
    Engine* engine;
    DragonMainWindow* mainWindow;

    QTimer* idleTimer = nullptr;
    QTimer* fpsTimer = nullptr;
    int lastFrameIdx = 0;
};

#endif
