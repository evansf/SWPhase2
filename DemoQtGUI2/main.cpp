#include "demoqtgui.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	DemoQtGUI w;
	w.show();
	return a.exec();
}
