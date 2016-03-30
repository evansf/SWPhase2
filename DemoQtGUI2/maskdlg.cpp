#include "maskdlg.h"
#include "qtdisplay.h"
#include "qtoverlay.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "QT_CV.h"
#include <qpainter.h>
#include <qfile.h>
#include <qtextstream>
#include <QtWidgets/qmessagebox>
#include "SISUtility.h"
#include "Inspect.h"

CMaskDlg::CMaskDlg(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	ui.Display1->setDisplayMode(QTDisplay::FIT_SCALE);
	m_DrawMode = BOX;
	ui.Display1->SetSnap(SNAP);	// closer closes polygon
	m_AlignModels.clear();

	connect(ui.saveButton,SIGNAL(clicked()),this,SLOT(on_saveButton_Clicked()));
	connect(ui.cancelButton,SIGNAL(clicked()),this,SLOT(on_cancelButton_Clicked()));
	connect(ui.resetButton, SIGNAL(clicked()), this, SLOT(on_resetButton_Clicked()));
	connect(ui.includeButton,SIGNAL(clicked()),this,SLOT(on_includeButton_Clicked()));
	connect(ui.excludeButton,SIGNAL(clicked()),this,SLOT(on_excludeButton_Clicked()));
	connect(ui.discardButton,SIGNAL(clicked()),this,SLOT(on_discardButton_Clicked()));
	connect(ui.setCropButton,SIGNAL(clicked()),this,SLOT(on_setCropButton_Clicked()));
	connect(ui.boxButton,SIGNAL(clicked()),this,SLOT(on_boxButton_Clicked()));
	connect(ui.polyButton,SIGNAL(clicked()),this,SLOT(on_polyButton_Clicked()));
	connect(ui.closepolyButton,SIGNAL(clicked()),this,SLOT(on_closepolyButton_Clicked()));
	connect(ui.Display1,SIGNAL(PolyKnot(QPoint)),this,SLOT(on_PolyKnot(QPoint)));
	connect(ui.Display1,SIGNAL(PolyDrawn()),this,SLOT(on_PolyDrawn()));
	connect(ui.Display1,SIGNAL(RectDrawn(QRect)),this,SLOT(on_RectDrawn(QRect)));
	connect(ui.ClearAlignModelsButton,SIGNAL(clicked()),this,SLOT(on_ClearAlignModelsButton_clicked())); 
	connect(ui.Display1,SIGNAL(MousePress(QMouseEvent*)),this,SLOT(on_MousePress(QMouseEvent*)));
	connect(ui.Display1,SIGNAL(MouseRelease(QMouseEvent*)),this,SLOT(on_MouseRelease(QMouseEvent*)));
}

CMaskDlg::~CMaskDlg()
{

}
void CMaskDlg::setFiles(QString img, QString cropped, QString mask, QString params)
{
	ImgFile = img;
	MaskFile = mask;
	// model can be name of points or alignmodel file
	ParamsFile = params;
	ModelFile = cropped;

	// params file is read by calling process
	m_QCrop = QRect(m_Params.CropRect().x,m_Params.CropRect().y,m_Params.CropRect().width,m_Params.CropRect().height);

	m_fileImg = QImage(img);
	// GUI is impossibly slow if HUGE images are involved
	// Rescale image appropriately and then scale the results (mask and points)
	// when they are returned or saved.
	m_CroppedImg = m_fileImg.copy(m_QCrop);
	m_Params.m_DisplayScale = 1.0F;
	m_Img = m_CroppedImg;

	QMask = QPixmap(m_Img.size());
	QColor C = Qt::lightGray;
	C.setAlpha(TRANSLUCENCY);
	QMask.fill(C);

	ui.Display1->AttachQImage(m_Img);
	ui.Display1->SetLinesize(1);
	ui.Display1->ShowOverlay();
	ui.Display1->UpdateRoi();
}
#define ALIGNMODELSIZE 32
void CMaskDlg::on_ClearAlignMasks()
{
	m_AlignModels.clear();
}
void CMaskDlg::on_AddAlignModelsButton_clicked()
{
	if(m_RecentDraw != BOX)
		return;

	CAlignModel Align;
	// m_Box already in Model coordinates
	Align.setRect(m_Box.x(),m_Box.y(),m_Box.width(),m_Box.height());
	m_AlignModels.push_back(Align);
	ShowMask();
	m_RecentDraw = NONE;
}
void CMaskDlg::on_resetButton_Clicked()
{
	m_QCrop = QRect(0,0,m_fileImg.width(),m_fileImg.height());
	m_CroppedImg = m_fileImg;
	m_Params.m_DisplayScale = 1.0F;
	m_Img = m_CroppedImg;

	QMask = QPixmap(m_Img.size());
	QColor C = Qt::lightGray;
	C.setAlpha(TRANSLUCENCY);
	QMask.fill(C);

	ui.Display1->AttachQImage(m_Img);
	ui.Display1->SetLinesize(1);
	ui.Display1->ShowOverlay();
	ui.Display1->UpdateRoi();
}
void CMaskDlg::on_DisplayClicked(QMouseEvent *pE)
{
	QPoint P(pE->x(), pE->y());
	P = ui.Display1->DisplayToRoi(P);
	for(int i=0; i<(int)m_AlignModels.size(); i++)
	{
		if(m_AlignModels[i].contains(P.x(), P.y()))
		{
			m_AlignModels[i].setCenter(P.x(),P.y());
			break;	// only move the first one found
		}
	}
	ShowMask();
}
void CMaskDlg::on_MousePress(QMouseEvent *pE)
{
	QPoint P(pE->x(), pE->y());
	P = ui.Display1->DisplayToRoi(P);
	m_AlignMoveIndex = -1;	// if not in any align object space
	for(int i=0; i<(int)m_AlignModels.size(); i++)
	{
		if(m_AlignModels[i].contains(P.x(), P.y()))
		{
			m_AlignMoveIndex = i;
			break;	// only select the first one found
		}
	}
}
void CMaskDlg::on_MouseRelease(QMouseEvent *pE)
{
	if(m_AlignMoveIndex < 0)
		return;
	QPoint P(pE->x(), pE->y());
	P = ui.Display1->DisplayToRoi(P);
	m_AlignModels[m_AlignMoveIndex].setCenter(P.x(),P.y());
	m_AlignMoveIndex = -1;	// no longer dragging
	ShowMask();
}

void CMaskDlg::on_saveButton_Clicked()
{

	if( (int)m_AlignModels.size() < NUMALIGNMODELS)
	{
		QString msg = QString("Unable to SAVE Files -- Please create %1 Alignment Points").arg(NUMALIGNMODELS);
		QMessageBox("ERROR!",msg,QMessageBox::Icon::Critical,QMessageBox::Button::Ok,0,0).exec();
		return;
	}

	// put the alignpoints into the params
	CAlignModel m;
	for(int i=0; i<(int)m_AlignModels.size(); i++)
	{
		m = m_AlignModels[i];
		// coordinates in model space
		// transform to image space
		m.m_X += m_Params.CropRect().x;
		m.m_Y += m_Params.CropRect().y;
		m_Params.m_Models[i] = m;
	}
	m_Params.m_ModelCount = NUMALIGNMODELS;
	// put the crop into the paraams
	cv::Rect crop = Rect(m_QCrop.left(),m_QCrop.top(),m_QCrop.width(),m_QCrop.height());
	m_Params.SetCrop( crop );

	// save the cropped image as the model
	cv::imwrite(ModelFile.toUtf8().constData(),qimage_to_mat_cpy(m_CroppedImg,CV_8UC1));

	cv::Mat mask = qimage_to_mat_cpy(QMask.toImage(),CV_8UC4);
	std::vector<cv::Mat> planes;
	cv::split(mask,planes);
	cv::threshold(planes[3],planes[3],TRANSLUCENCY+1,255,CV_THRESH_BINARY);
	// output mask file
	cv::imwrite(MaskFile.toUtf8().constData(),planes[3]);

		// then write the parameters file
	if(m_Params.write(ParamsFile.toUtf8().constData()))
	{
		QString msg("Unable to write parameters ");
		QMessageBox("ERROR!",msg+ParamsFile,QMessageBox::Icon::Critical,QMessageBox::Button::Ok,0,0).exec();
	}

	QDialog::accept();
}

void CMaskDlg::on_cancelButton_Clicked()
{
	done( -1);
}
void CMaskDlg::ShowMask()
{
	ui.Display1->ClearOverlay();

	ui.Display1->DrawPixmap(QMask        ,QPoint(0,0));

	// draw the model boxes
	CAlignModel A;
	for(int i=0; i<(int)m_AlignModels.size(); i++)
	{
		A = m_AlignModels[i];
		ui.Display1->DrawRect(QRect(A.m_X,A.m_Y,A.m_Width,A.m_Height), Qt::yellow);
	}

	ui.Display1->UpdateRoi();
}
void CMaskDlg::on_includeButton_Clicked()
{
	QPainter painter(&QMask);
	painter.initFrom(&QMask);
	painter.setCompositionMode(QPainter::CompositionMode_Source);
	QPainterPath path;
	QColor C = Qt::green;
	C.setAlpha(TRANSLUCENCY+10);	// transparent Green
	switch(m_RecentDraw)
	{
	case BOX:
		painter.fillRect(m_Box,C);
		break;
	case POLY:
		path.moveTo(m_PolyVec[0]);
		for(int i=0; i<m_PolyVec.size(); i++)
			path.lineTo(m_PolyVec[i]);
		path.lineTo(m_PolyVec[0]);
		painter.fillPath(path,C);
		break;
	default:
		break;
	}
	painter.end();
	// create cvMask for point filter
	Mask = qimage_to_mat_cpy(QMask.toImage(),CV_8UC4);
	std::vector<cv::Mat> planes;
	cv::split(Mask,planes);
	cv::threshold(planes[3],Mask,TRANSLUCENCY+1,255,CV_THRESH_BINARY);
	ShowMask();
	m_RecentDraw = NONE;
}
void CMaskDlg::on_excludeButton_Clicked()
{
	QPainter painter(&QMask);
	painter.initFrom(&QMask);
	painter.setCompositionMode(QPainter::CompositionMode_Source);
	QPainterPath path;
	QColor C = Qt::lightGray;
	C.setAlpha(TRANSLUCENCY);	// transparent lightGray
	switch(m_RecentDraw)
	{
	case BOX:
		painter.fillRect(m_Box,C);
		break;
	case POLY:
		path.moveTo(m_PolyVec[0]);
		for(int i=0; i<m_PolyVec.size(); i++)
			path.lineTo(m_PolyVec[i]);
		path.lineTo(m_PolyVec[0]);
		painter.fillPath(path,C);
		break;
	default:
		break;
	}
	painter.end();
	// create cvMask for point filter
	Mask = qimage_to_mat_cpy(QMask.toImage(),CV_8UC4);
	std::vector<cv::Mat> planes;
	cv::split(Mask,planes);
	cv::threshold(planes[3],Mask,TRANSLUCENCY+1,255,CV_THRESH_BINARY);
	ShowMask();
	m_RecentDraw = NONE;

}

void CMaskDlg::on_discardButton_Clicked()
{
	QFont f = ui.polyButton->font();
	f.setBold(false);
	ui.boxButton->setFont(f);
	ui.polyButton->setFont(f);
	m_PolyVec.clear();
	m_RecentDraw = NONE;
	ShowMask();
}
void CMaskDlg::on_setCropButton_Clicked()
{
	// remove offsets from old crop and add in new
	CAlignModel model;
	for(int i=0; i<(int)m_AlignModels.size(); i++)
	{
		model = m_AlignModels[i];
		model.setPos(model.m_X+m_Params.CropRect().x-m_Box.left(), model.m_Y+m_Params.CropRect().y-m_Box.top());
		m_AlignModels[i] = model;
	}
	m_QCrop = QRect(m_Box.left(),m_Box.top(),m_Box.width(),m_Box.height());
	m_CroppedImg = m_fileImg.copy(m_QCrop);
	m_Img = m_CroppedImg;
	QMask = QPixmap(m_Img.size());
	QColor C = Qt::lightGray;
	C.setAlpha(TRANSLUCENCY);
	QMask.fill(C);
	m_Img = m_CroppedImg;
	m_Mask = QMask;
	ui.Display1->AttachQImage(m_Img);
	ShowMask();
	m_RecentDraw = NONE;
}
void CMaskDlg::on_boxButton_Clicked()
{
	m_DrawMode = BOX;
	QFont f = ui.boxButton->font();
	f.setBold(true);
	ui.boxButton->setFont(f);
	ui.Display1->SetRubberBandMode(QTDisplay::BOX);
}
void CMaskDlg::on_polyButton_Clicked()
{
	m_PolyVec.clear();
	m_DrawMode = POLY;
	QFont f = ui.polyButton->font();
	f.setBold(true);
	ui.polyButton->setFont(f);
	ui.Display1->SetRubberBandMode(QTDisplay::POLY);
}
void CMaskDlg::on_closepolyButton_Clicked()
{
	if(m_PolyVec.size() < 2) return;
	on_PolyKnot(ui.Display1->RoiToDisplay(m_PolyVec[0]));	// close the polygon
	on_PolyDrawn();	// signal new polygon
}

void CMaskDlg::on_PolyKnot(QPoint p)
{
	p = ui.Display1->DisplayToRoi(p);
	if(m_PolyVec.size() > 0)
	{
		QLine l(m_PolyVec.back(),p);
		ui.Display1->DrawLine(l);
		ui.Display1->UpdateRoi();
	}
	m_PolyVec.push_back(p);
}
void CMaskDlg::on_PolyDrawn()
{
	QFont f = ui.polyButton->font();
	f.setBold(false);
	ui.polyButton->setFont(f);
	ui.polyButton->setEnabled(true);
	m_RecentDraw = POLY;
}
void CMaskDlg::on_RectDrawn(QRect box)
{
	QFont f = ui.boxButton->font();
	f.setBold(false);
	ui.boxButton->setFont(f);
	ui.boxButton->setEnabled(true);
	m_Box = ui.Display1->DisplayToRoi(box);
	ui.Display1->DrawRect(m_Box);
	ui.Display1->UpdateRoi();
	m_RecentDraw = BOX;
}

bool inline CMaskDlg::InMask(std::pair<float,float> p)
{
	unsigned char* pC = (unsigned char*)Mask.ptr((int)p.second);
	return ( pC[(int)p.first] != 0);
}
