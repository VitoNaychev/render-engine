#include "app.h"
#include "window.h"

#include <QTimer>
#include <QCoreApplication>

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

    connect(qApp, &QCoreApplication::aboutToQuit, this, &DragonApp::onQuit);

    return true;
}

void DragonApp::onIdleTick() {
    renderFrame();
}

void DragonApp::onQuit() {
    if (idleTimer != nullptr) {
        idleTimer->stop();
    }
    if (fpsTimer != nullptr) {
        fpsTimer->stop();
    }

    if (mainWindow != nullptr) {
        mainWindow->close();
        delete mainWindow;
        mainWindow = nullptr;
    }

    idleTimer = nullptr;
    fpsTimer = nullptr;
}

void DragonApp::updateRenderStats() {
    int frameIdx = engine.getFrameIdx();

    mainWindow->setFPS(frameIdx - lastFrameIdx);
    lastFrameIdx = frameIdx;
}

bool DragonApp::initWindow() {
    mainWindow = new DragonMainWindow(nullptr);
    return mainWindow != nullptr;
}

void DragonApp::renderFrame() {
    engine.renderFrame();
    mainWindow->updateViewport(engine.getQImageForFrame());
}
