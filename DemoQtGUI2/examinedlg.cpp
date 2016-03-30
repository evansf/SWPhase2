#include "examinedlg.h"
#include "qtdisplay.h"
#include "qtoverlay.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "QT_CV.h"
#include <qpainter.h>
#include <qfile.h>
#include <qtextstream>
#include <QtWidgets/qmessagebox>

CExamineDlg::CExamineDlg(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	QRect ParentRect = parent->geometry();
	setGeometry(ParentRect.x(),ParentRect.height(),width(),height());

	ui.Display1->setDisplayMode(QTDisplay::FIT_SCALE);
	m_DrawMode = OFF;
	ui.Display1->SetSnap(SNAP);	// closer closes polygon

	connect(ui.ContinueButton,SIGNAL(clicked()),this,SLOT(on_ContinueButtonClicked()));
	connect(ui.AbortButton,SIGNAL(clicked()),this,SLOT(on_AbortButtonClicked()));
	connect(ui.Display1,SIGNAL(RectDrawn(QRect)),this,SLOT(on_RectDrawn(QRect)));
	connect(ui.Display1,SIGNAL(MousePress(QMouseEvent*)),this,SLOT(on_MousePress(QMouseEvent*)));
	connect(ui.Display1,SIGNAL(MouseRelease(QMouseEvent*)),this,SLOT(on_MouseRelease(QMouseEvent*)));
}

CExamineDlg::~CExamineDlg()
{

}
void CExamineDlg::setImage(QImage &img)
{
	m_Img = img;
	ui.Display1->AttachQImage(m_Img);
	ui.Display1->UpdateRoi();
}

void CExamineDlg::on_DisplayClicked(QMouseEvent *pE)
{
	QPoint P(pE->x(), pE->y());
	P = ui.Display1->DisplayToRoi(P);

}
void CExamineDlg::on_MousePress(QMouseEvent *pE)
{
	QPoint P(pE->x(), pE->y());
	P = ui.Display1->DisplayToRoi(P);

}
void CExamineDlg::on_MouseRelease(QMouseEvent *pE)
{
	QPoint P(pE->x(), pE->y());
	P = ui.Display1->DisplayToRoi(P);

}

void CExamineDlg::on_ContinueButtonClicked()
{
	accept();
}
void CExamineDlg::on_AbortButtonClicked()
{
	reject();
}
void CExamineDlg::on_RectDrawn(QRect box)
{
	m_Box = ui.Display1->DisplayToRoi(box);
}
