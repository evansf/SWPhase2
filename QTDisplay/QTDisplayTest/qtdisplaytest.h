#ifndef QTDISPLAYTEST_H
#define QTDISPLAYTEST_H

#include <QtWidgets/QMainWindow>
#include "ui_qtdisplaytest.h"

class QTDisplayTest : public QMainWindow
{
	Q_OBJECT

public:
	QTDisplayTest(QWidget *parent = 0);
	~QTDisplayTest();

private:
	Ui::QTDisplayTestClass ui;
};

#endif // QTDISPLAYTEST_H
