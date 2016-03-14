/********************************************************************************
** Form generated from reading UI file 'qtdisplaytest.ui'
**
** Created by: Qt User Interface Compiler version 5.2.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_QTDISPLAYTEST_H
#define UI_QTDISPLAYTEST_H

#include <..\qtdisplay.h>
#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_QTDisplayTestClass
{
public:
    QWidget *centralWidget;
    QTDisplay *Display1;
    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *QTDisplayTestClass)
    {
        if (QTDisplayTestClass->objectName().isEmpty())
            QTDisplayTestClass->setObjectName(QStringLiteral("QTDisplayTestClass"));
        QTDisplayTestClass->resize(600, 400);
        centralWidget = new QWidget(QTDisplayTestClass);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        Display1 = new QTDisplay(centralWidget);
        Display1->setObjectName(QStringLiteral("Display1"));
        Display1->setGeometry(QRect(10, 10, 371, 341));
        QTDisplayTestClass->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(QTDisplayTestClass);
        menuBar->setObjectName(QStringLiteral("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 600, 21));
        QTDisplayTestClass->setMenuBar(menuBar);
        mainToolBar = new QToolBar(QTDisplayTestClass);
        mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
        QTDisplayTestClass->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(QTDisplayTestClass);
        statusBar->setObjectName(QStringLiteral("statusBar"));
        QTDisplayTestClass->setStatusBar(statusBar);

        retranslateUi(QTDisplayTestClass);

        QMetaObject::connectSlotsByName(QTDisplayTestClass);
    } // setupUi

    void retranslateUi(QMainWindow *QTDisplayTestClass)
    {
        QTDisplayTestClass->setWindowTitle(QApplication::translate("QTDisplayTestClass", "QTDisplayTest", 0));
    } // retranslateUi

};

namespace Ui {
    class QTDisplayTestClass: public Ui_QTDisplayTestClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_QTDISPLAYTEST_H
