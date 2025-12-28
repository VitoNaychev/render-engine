#ifndef UI_NOTEPAD_H
#define UI_NOTEPAD_H

#include "viewport.h"

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <QtWidgets/QLabel>

QT_BEGIN_NAMESPACE

class Ui_DragonMainWindow
{

public:
    QWidget* centralWidget;
    QGridLayout* mainLayout;
    ViewportWidget* viewport;
    QStatusBar* statusBar;
    QLabel* statusFPS;

    void setupUi(QMainWindow* Notepad)
    {
        if (Notepad->objectName().isEmpty())
            Notepad->setObjectName("Notepad");
        Notepad->resize(600, 600);

        // Central widget + layout
        centralWidget = new QWidget(Notepad);
        centralWidget->setObjectName("centralWidget");
        Notepad->setCentralWidget(centralWidget);

        mainLayout = new QGridLayout(centralWidget);
        mainLayout->setObjectName("mainLayout");
        mainLayout->setContentsMargins(0, 0, 0, 0);

        viewport = new ViewportWidget(centralWidget);
        viewport->setAttribute(Qt::WA_NativeWindow);
        viewport->setObjectName("viewport");
        mainLayout->addWidget(viewport, 0, 0);

        // Real status bar
        statusBar = new QStatusBar(Notepad);
        statusBar->setObjectName("statusbar");
        Notepad->setStatusBar(statusBar);

        statusFPS = new QLabel(statusBar);
        statusFPS->setObjectName("statusFPS");
        statusBar->addPermanentWidget(statusFPS); // stays on the right
        // or: statusBar->addWidget(statusFPS);    // on the left

        retranslateUi(Notepad);
        QMetaObject::connectSlotsByName(Notepad);
    }

    void retranslateUi(QMainWindow* Notepad)
    {
        Notepad->setWindowTitle(QCoreApplication::translate("Notepad", "Notepad", nullptr));
        statusFPS->setText(QCoreApplication::translate("Notepad", "FPS: 0", nullptr));
    }
};

namespace Ui {
    class DragonMainWindow: public Ui_DragonMainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_NOTEPAD_H
