#include "qtdisplaytest.h"
#include "..\qtdisplay.h"
#include "..\Qt_CV.h"
#include <QtGui\QImage>

QTDisplayTest::QTDisplayTest(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	QImage img("C:\\SIS\\Test\\CCameraFilesImageFiles\\Set1\\taiwan2clean.jpg");
	cv::Mat MatImg = qimage_to_mat_ref(img,CV_8UC4);
	ui.Display1->AttachMat(&MatImg);
	ui.Display1->update();
}

QTDisplayTest::~QTDisplayTest()
{

}
