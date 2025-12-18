#include "app.h"
#include "window.h"

#include <QTimer>

bool DragonApp::init() {
    if (false == initWindow()) {
        return false;
    }
    mainWindow->show();

    idleTimer = new QTimer(mainWindow);
    connect(idleTimer, &QTimer::timeout, this, &DragonApp::onIdleTick);
    idleTimer->start(0);

    fpsTimer = new QTimer(mainWindow);
    connect(fpsTimer, &QTimer::timeout, this, &DragonApp::updateRenderStats);
    fpsTimer->start(1'000);

    return true;
}

void DragonApp::onIdleTick() {
    renderFrame();
}

void DragonApp::updateRenderStats() {
    int frameIdx = engine.getFrameIdx();

    mainWindow->setFPS(frameIdx - lastFrameIdx);
    lastFrameIdx = frameIdx;
}

bool DragonApp::initWindow() {
    mainWindow = new DragonMainWindow(nullptr);
    return true;
}

void DragonApp::renderFrame() {
    engine.renderFrame();
    mainWindow->updateViewport(engine.getQImageForFrame());
}
