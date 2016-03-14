#pragma warning(push)
#pragma warning( disable : 4996)	// QWT passes std::vector internally but without a DLL interface

#include "demoqtgui.h"
#include <QtWidgets/qfiledialog>
#include <QtWidgets/qmessagebox>
#include <QtWidgets/QListWidget>
#include <QtCore/qfileinfo>
#include <QtCore/qfile>
#include <QtCore/qtextstream>
#include "SISUtility.h"
#include "utility.h"
#include "Inspect.h"
#include "qtoverlay.h"
#include "Qt_CV.h"

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "MaskDlg.h"
#include "ROCdlg.h"
#include "Histdlg.h"
#include "examinedlg.h"

// forward declaration of local functions
static void DirDlg(QString &dirName);

DemoQtGUI::DemoQtGUI(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	// open logfile
	FILE *f;
	fopen_s(&f,"C:/SIS/SISLog.txt","w");
	m_Logfile.open(f,QIODevice::WriteOnly);
	m_TestDir = ui.TestFolderLineEdit->text();
	TestFolderLineEdit_Update();

	ui.Display1->setDisplayMode(QTDisplay::FIT_SCALE);
	ui.Display2->setDisplayMode(QTDisplay::FIT_SCALE);
	ui.Display2->setHidden(true);

	m_pInspect = NULL;
	m_Reset = false;

	connect(ui.TestFolderBrowseButton,SIGNAL(clicked()),this,SLOT(on_TestFolderBrowseButton_Clicked()));
	connect(ui.TestFolderLineEdit,SIGNAL(textChanged(const QString &)),
						this,SLOT(on_TestFolderLineEdit_textChanged(const QString &)));
	connect(ui.CreateButton,SIGNAL(clicked()),this,SLOT(on_CreateButton_Clicked()));
	connect(ui.ResetButton,SIGNAL(clicked()),this,SLOT(on_Reset_Clicked()));
	connect(ui.TrainButton,SIGNAL(clicked()),this,SLOT(on_TrainButton_Clicked()));
	connect(ui.InspectButton,SIGNAL(clicked()),this,SLOT(on_InspectButton_Clicked()));
	connect(ui.InspectTrainButton,SIGNAL(clicked()),this,SLOT(on_InspectTrainButton_Clicked()));
	connect(ui.RunStatsButton,SIGNAL(clicked()),this,SLOT(on_RunStatsButton_Clicked()));
	connect(ui.ShowStatsButton,SIGNAL(clicked()),this,SLOT(on_ShowStatsButton_Clicked()));
	connect(ui.OptimizeButton,SIGNAL(clicked()),this,SLOT(on_OptimizeButton_Clicked()));
	connect(ui.RadioButtonContinuous,SIGNAL(clicked()),this,SLOT(on_ModeRadioButton_Clicked()));
	connect(ui.RadioButtonPauseEach,SIGNAL(clicked()),this,SLOT(on_ModeRadioButton_Clicked()));
	connect(ui.RadioButtonPauseIncorrect,SIGNAL(clicked()),this,SLOT(on_ModeRadioButton_Clicked()));
}

DemoQtGUI::~DemoQtGUI()
{
	delete m_pInspect;
}
void DemoQtGUI::on_TestFolderBrowseButton_Clicked()
{
	if(m_TestDir.isEmpty()) m_TestDir = QCoreApplication::applicationDirPath();

	DirDlg(m_TestDir);	// get a new dir
	ui.TestFolderLineEdit->setText(m_TestDir);
	TestFolderLineEdit_Update();
	return;
}
void DemoQtGUI::TestFolderLineEdit_Update()
{
	m_TrainDir = m_TestDir + "\\Train";
	m_InspectPassDir = m_TestDir + "\\inspectPass";
	m_InspectFailDir = m_TestDir + "\\inspectFail";
}

void DemoQtGUI::on_TestFolderLineEdit_textChanged(const QString &str)
{
	m_TestDir = str;
	TestFolderLineEdit_Update();
}
void DemoQtGUI::on_Reset_Clicked()
{
	m_Reset = true;
	m_Params = SISParams();
}
void DemoQtGUI::on_CreateButton_Clicked()
{
	QString maskname = m_TrainDir + QString("\\mask.png");
	QString modelname = m_TrainDir + QString("\\modelimg.png");
	QString paramsname = m_TrainDir + QString("\\params.txt");

	ui.statusBar->showMessage("Create Model",0);

	if(  m_Reset  )
	{
		QString cmd = QString("del %1").arg(maskname);
		int status = std::system(cmd.toUtf8().constData());
		cmd = QString("del %1").arg(modelname);
		status = std::system(cmd.toUtf8().constData());
		cmd = QString("del %1").arg(paramsname);
		status = std::system(cmd.toUtf8().constData());

		QString filename = QFileDialog::getOpenFileName(this,
						tr("Select Model Image"), m_TrainDir);
		QFile img(filename);
		if(!img.exists())
			return;
		makeMask(filename,modelname,maskname,paramsname);
		img.close();
		LogMsg(QString("Model Created Using Dialog"));
	}
	else
	{
		LogMsg(QString("Model Exists"));
	}

	m_Params = SISParams();	// reset to defaults then read if exists
	if(QFile::exists(paramsname))
	{
		if(m_Params.read(paramsname.toUtf8().constData()))
		{
			LogMsg(QString("Error Reading Parameters file"));
			m_Params = SISParams();
		}
	}
	else
	{
		QString msg = QString("Model Does Not Exist -- Please press Reset\n");
		QMessageBox("ERROR!",msg,QMessageBox::Icon::Critical,QMessageBox::Button::Ok,0,0).exec();
		return;
	}

	// Load the data from the valid files
	m_ModelImg.load(modelname);
	LogMsg(QString("Model Image Loaded from file ") + modelname);
	m_ModelMask.load(maskname);
	LogMsg(QString("Model Mask Loaded from file ") + maskname);

	if(ui.InspectDataTypeListWidget->currentRow() < 0) 
		ui.InspectDataTypeListWidget->setCurrentRow(0);
	QString classname = ui.InspectDataTypeListWidget->currentItem()->text();
	if(m_pInspect != NULL)
		delete m_pInspect;
	int tilewidth=8; int tileheight=8; int stepcols=8; int steprows=8;
	CInspect::DATATYPE datatype = CInspect::DCT;
	if(classname == QString("CInspectDCT"))
	{
		tilewidth=16; tileheight=16; stepcols=12; steprows=12;
		datatype = CInspect::DCT;
		ui.RawGainSpinBox->setValue(100000.0);
	}
	else if(classname == QString("CInspectIntensity"))
	{
		tilewidth=3; tileheight=3; stepcols=3; steprows=3;
		datatype = CInspect::INTENSITY;
		ui.RawGainSpinBox->setValue(1000000.0);
	}
	else if(classname == QString("CInspectWave"))
	{
		tilewidth=4; tileheight=4; stepcols=4; steprows=4;
		datatype = CInspect::WAVE;
		ui.RawGainSpinBox->setValue(10000000.0);
	}
	else if(classname == QString("CInspectMOM"))
	{
		tilewidth=8; tileheight=8; stepcols=8; steprows=8;
		datatype = CInspect::MOM;
		ui.RawGainSpinBox->setValue(5000.0);
	}
	else
		return;

	setDisplayImg(m_ModelImg);
	ui.Display1->SetLinesize(3);
	ui.Display1->ShowOverlay();
	ui.Display1->UpdateRoi();
	ui.Display1->EnableRubberBand(true);

	INSPECTIMAGE InsImg = QImageToInspectImage(m_ModelImg);
	INSPECTIMAGE InsMask = QImageToInspectImage(m_ModelMask);

	int AlignModelSize = 25;	// debug
	m_pInspect = new CInspect(m_Params);
	m_pInspect->create(datatype,InsImg,InsMask,
					tilewidth, tileheight, stepcols, steprows);
	ui.TimeCreateSpinBox->setValue(m_pInspect->getOpTime());
	ui.statusBar->showMessage("Ready",0);

}
void DemoQtGUI::on_TrainButton_Clicked()
{
	m_pInspect->setLearnThresh(ui.LearnThreshSpinBox->value());
	m_pInspect->setMustAlign(ui.AlignCheckBox->isChecked());
	CInspect::ERR_INSP err = CInspect::OK;
	QString msg;
	QDir dir(m_TrainDir);
	QStringList filters;
    filters << "*.png" << "*.tiff" ;
	QStringList list = dir.entryList(filters);
	QString imgfilename;
	QString loadName;
	m_AvTime = 0.0;	m_ImgCount = 0;
	foreach(imgfilename,list)
	{
		if(imgfilename.left(4) == QString("mask"))
			continue;
		if(imgfilename.left(5) == QString("model"))
			continue;
		loadName = m_TrainDir + QString("\\") + imgfilename;
		QImage img(loadName);
		if(img.isNull())
		{
			msg = QString("Load of " + loadName + " FAILED" );
			if(ui.RadioButtonPauseIncorrect->isChecked())
			{
				QMessageBox("ERROR!",msg,QMessageBox::Icon::Critical,QMessageBox::Button::Ok,0,0).exec();
			}
			continue;
		}
		ui.statusBar->showMessage("Training file "+loadName,20);

		INSPECTIMAGE InsImg = QImageToInspectImage(img);

		err = m_pInspect->train(InsImg);
		m_AvTime += m_pInspect->getOpTime(); m_ImgCount++;

		INSPECTIMAGE Aligned = m_pInspect->getAlignedImage();
		QImage alignedQImage;
		InspectImageToQImage(Aligned,alignedQImage);
		setDisplayImg(alignedQImage);

		loadName = m_TrainDir + QString("\\Aligned");
		if(QDir(loadName).exists())
		{
			loadName = loadName + QString("\\aligned-") + imgfilename;
			cv::Mat mat = cv::Mat(Aligned.height,Aligned.width,CV_8UC1,Aligned.pPix,Aligned.imgStride);
			cv::imwrite(loadName.toUtf8().constData(),mat);
		}

		if(err != CInspect::OK)
		{
			msg = QString(imgfilename + " error: " 
							+ m_pInspect->getErrorString(err).c_str());
			if(ui.RadioButtonPauseIncorrect->isChecked())
			{
				QMessageBox("ERROR!",msg,QMessageBox::Icon::Critical,QMessageBox::Button::Ok,0,0).exec();
			}
		}
		if(ui.RadioButtonPauseEach->isChecked())
		{
			msg = QString("Train  of " + imgfilename + " Complete status: " + m_pInspect->getErrorString(err).c_str());
			QMessageBox mBox("Train Process!",msg +  "\n    Wish to Continue? ",QMessageBox::Icon::Question,
						QMessageBox::Button::Yes, QMessageBox::Button::No,QMessageBox::Button::NoButton);
			int escape = mBox.exec();
			if(escape == QMessageBox::Button::No)
			{
				LogMsg(msg);
				LogMsg(QString("Selected Do Not Continue"));
				break;
			}
		}
		LogMsg(msg);
	}

	m_AvTime /= m_ImgCount;
	ui.TimeTrainSpinBox->setValue(m_AvTime);
	ui.MpixTrainSpinBox->setValue((m_ModelImg.width() * m_ModelImg.height())/(1000000 * m_AvTime));
	ui.statusBar->showMessage("Ready",0);

}
void DemoQtGUI::on_InspectTrainButton_Clicked()
{
	if(!m_pInspect->isOptimized())
	{
		QString msg = QString("Inspect Object Trained but not Optimized\n");
		LogMsg(msg);
		QMessageBox("ERROR!",msg,QMessageBox::Icon::Critical,QMessageBox::Button::Ok,0,0).exec();
		return;
	}
	ui.fpSpinBox->setValue(0);
	ui.tnSpinBox->setValue(0);
	ui.tpSpinBox->setValue(0);
	ui.fnSpinBox->setValue(0);
	m_pInspect->setInspectThresh(ui.InspectThreshSpinBox->value());
	m_pInspect->setMustAlign(ui.AlignCheckBox->isChecked());
	int pass, fail;
	ui.DirActiveLabel->setText("TrainDir Should PASS ");
	Inspect(m_TrainDir, &pass,&fail, false);
	ui.TimePassSpinBox->setValue(m_AvTime);
	ui.MpixPassSpinBox->setValue((m_ModelImg.width() * m_ModelImg.height())/(1000000 * m_AvTime));
	ui.tpSpinBox->setValue(pass);
	ui.fnSpinBox->setValue(fail);
	ui.statusBar->showMessage("Ready",0);
	ui.MeanLabel->setText(QString("%1").arg(m_pInspect->getMean(0)));
	ui.SigmaLabel->setText(QString("%1").arg(m_pInspect->getSigma(0)));

}
void DemoQtGUI::on_InspectButton_Clicked()
{
	if(!m_pInspect->isOptimized())
	{
		QString msg = QString("Inspect Object Trained but not Optimized\n");
		LogMsg(msg);
		QMessageBox("ERROR!",msg,QMessageBox::Icon::Critical,QMessageBox::Button::Ok,0,0).exec();
		return;
	}
	m_pInspect->setInspectThresh(ui.InspectThreshSpinBox->value());
	m_pInspect->setMustAlign(ui.AlignCheckBox->isChecked());
	int pass, fail;
	ui.DirActiveLabel->setText("Should FAIL ");
	Inspect(m_InspectFailDir, &pass,&fail,true);
	ui.TimeFailSpinBox->setValue(m_AvTime);
	ui.MpixFailSpinBox->setValue((m_ModelImg.width() * m_ModelImg.height())/(1000000 * m_AvTime));
	ui.fpSpinBox->setValue(pass);
	ui.tnSpinBox->setValue(fail);
	ui.MeanLabel->setText(QString("%1").arg(m_pInspect->getMean(0)));
	ui.SigmaLabel->setText(QString("%1").arg(m_pInspect->getSigma(0)));
	if(m_Abort)
		return;
	ui.DirActiveLabel->setText("Should PASS ");
	Inspect(m_InspectPassDir, &pass,&fail, false);
	ui.TimePassSpinBox->setValue(m_AvTime);
	ui.MpixPassSpinBox->setValue((m_ModelImg.width() * m_ModelImg.height())/(1000000 * m_AvTime));
	ui.tpSpinBox->setValue(pass);
	ui.fnSpinBox->setValue(fail);
	ui.MeanLabel->setText(QString("%1").arg(m_pInspect->getMean(0)));
	ui.SigmaLabel->setText(QString("%1").arg(m_pInspect->getSigma(0)));
	ui.statusBar->showMessage("Ready",0);

}
void DemoQtGUI::Inspect(QString dirname, int *p, int *f, bool expectFail)
{
	m_Abort = false;
	m_AvTime = 0.0;	m_ImgCount = 0;
	int pass=0, fail=0;
	QString msg;
	QDir dir(dirname);
	QStringList filters;
    filters << "*.png" << "*.tiff" ;
	QStringList list = dir.entryList(filters);
	CInspect::ERR_INSP err = CInspect::OK;
	QString imgfilename;
	QString loadName;
	QImage img;
	if(ui.ExtendedTrainBox->isChecked() && !expectFail)
		m_TrainEx = true;
	else
		m_TrainEx = false;
	foreach(imgfilename,list)
	{
		if(imgfilename.left(4) == QString("mask"))
			continue;
		if(imgfilename.left(5) == QString("model"))
			continue;

		loadName = dirname + QString("\\") + imgfilename;
		ui.statusBar->showMessage("Inspecting file "+loadName,10);
		img.load(loadName);
		if(img.isNull())
		{
			msg = QString("Load of " + loadName + " FAILED" );
			if(ui.RadioButtonPauseIncorrect->isChecked())
			{
				QMessageBox("ERROR!",msg,QMessageBox::Icon::Critical,QMessageBox::Button::Ok,0,0).exec();
			}
			continue;
		}

		INSPECTIMAGE InsImg = QImageToInspectImage(img);

		m_pInspect->setRawScoreDiv(ui.RawGainSpinBox->value());
		int failX,failY,failIndex;
		err = m_pInspect->inspect(InsImg);
		m_AvTime += m_pInspect->getOpTime(); m_ImgCount++;
		ui.Score_Value->setText(QString("%1").arg(m_pInspect->getRawScore(&failIndex,&failX,&failY)));
		ui.FinalScoreLabel->setText(QString("%1").arg(m_pInspect->getFinalScore()));


		INSPECTIMAGE Aligned = m_pInspect->getAlignedImage();
		QImage alignedQImage;
		InspectImageToQImage(Aligned,alignedQImage);
		setDisplayImg(alignedQImage);

		loadName = dirname + QString("\\Aligned");
		if(QDir(loadName).exists())
		{
			loadName = loadName + QString("\\aligned-") + imgfilename;
			cv::Mat mat = cv::Mat(Aligned.height,Aligned.width,CV_8UC1,Aligned.pPix,Aligned.imgStride);
			cv::imwrite(loadName.toUtf8().constData(),mat);
		}

		int escape;
		if(err != CInspect::OK)
		{
			fail++;
			ui.Display1->ClearOverlay();
			CInspectImage A = m_pInspect->getAlignedImage();
			QImage AlignedImage;
			InspectImageToQImage(A,AlignedImage);
			setDisplayImg(AlignedImage);
			cv::Mat temp;
			QImage Overlay;
			if(err == CInspect::FAIL) 
			{
				// for FAIL images, show failed tiles on overlay
				ui.Display2->setHidden(false);

				m_pInspect->DrawRect(failX-20,failY-20,40,40, 0xFFFFFF00);
				CInspectImage O = m_pInspect->getOverlayImage();
				InspectImageToQImage(O,Overlay);
				setOverlayImg(Overlay);
				ui.Display1->ShowOverlay();

				InspectImageToMatGui(A,temp);
				cv::copyMakeBorder(temp,temp,20,20,20,20,BORDER_CONSTANT,92);
				// make 40X40 rect that centers on fail point
				// boundary provides -20 to coordinates
				cv::Rect rect = cv::Rect(failX,failY,40,40);
				temp = temp(rect);
				temp *= 10;
				ui.Display2->AttachMat(&temp);
				ui.Display2->UpdateRoi();
				if(m_TrainEx)
				{
					// retrain images that should pass
					err = m_pInspect->trainEx(InsImg);
				}
			}
			ui.Display1->UpdateRoi();

			msg = QString(imgfilename + " status: "
							+ m_pInspect->getErrorString(err).c_str());
			ui.statusBar->showMessage(msg,10);

			if(!expectFail && ui.RadioButtonPauseIncorrect->isChecked() || ui.RadioButtonPauseEach->isChecked())
			{
				QString dirtype = dirname.mid(dirname.lastIndexOf(QChar('\\')));
				QMessageBox mBox(dirtype,msg + "\n   Wish to Continue? ",QMessageBox::Icon::Question,
					QMessageBox::Button::Yes, QMessageBox::Button::No,QMessageBox::Button::NoButton);
				escape = mBox.exec();
				if( escape == QMessageBox::Button::No )
				{
					LogMsg(msg);
					LogMsg(QString("Selected Do Not Continue"));
					m_Abort = true;
					break;
				}
			}
		}
		else
		{
			pass++;
			CInspectImage A = m_pInspect->getAlignedImage();
			QImage AlignedImage;
			InspectImageToQImage(A,AlignedImage);
			setDisplayImg(AlignedImage);
			ui.Display1->UpdateRoi();
			msg = QString(imgfilename + " status: " + m_pInspect->getErrorString(err).c_str());
			ui.statusBar->showMessage(msg,10);

			if(expectFail && ui.RadioButtonPauseIncorrect->isChecked() || ui.RadioButtonPauseEach->isChecked())
			{

				CExamineDlg dlg;
				dlg.setImage(AlignedImage);
				escape = dlg.exec();

				if( escape == QDialog::DialogCode::Rejected ) // only continues if Continue is selected
				{
					LogMsg(msg);
					LogMsg(QString("Examine Dialog Selected ABORT or Close(X)"));
					m_Abort = true;
					break;
				}
			}
		}

		LogMsg(msg);
	}
	m_AvTime /= m_ImgCount;
	*p = pass; *f = fail;
}

void DemoQtGUI::on_RunStatsButton_Clicked()
{
	QPointF P;
	int tp,fn,fp,tn;
	float Tpr,Fpr;

	// don't do retrain when we are inspecting at ALL thresholds
	ui.ExtendedTrainBox->setChecked(false);

	QString statsfilename = m_TestDir + QString("/stats.txt");
	QFile statsfile(statsfilename);
    if (!statsfile.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
	QTextStream stats(&statsfile);
	QString comma(",");
	QString endln("\n");
	stats << QString("Thresh,tp,fn,fp,tn,Fpr,Tpr\n");

	m_StatsPlotData.clear();
	for(float i=0.1f; i<1.0f; i+=0.1f)
	{
		ui.InspectThreshSpinBox->setValue(i);
		on_InspectButton_Clicked();
		tp = ui.tpSpinBox->value();
		fn = ui.fnSpinBox->value();
		fp = ui.fpSpinBox->value();
		tn = ui.tnSpinBox->value();
		Fpr =  (float)fp / (fp+tn);
		Tpr =  (float)tp / (tp+fn);
		P.setX(Fpr);
		P.setY(Tpr);
		m_StatsPlotData.push_back(P);
		stats << i << comma << tp << comma << fn << comma << fp << comma << tn << comma;
		stats << Fpr << comma << Tpr << endln; 
	}
	stats.flush();
	statsfile.close();
}
void DemoQtGUI::on_ShowStatsButton_Clicked()
{
	CROCDlg dlg(this);

	if(m_StatsPlotData.size() > 1)
	{
		dlg.setPlotData(m_StatsPlotData);;
//		dlg.fitPlotData(m_StatsPlotData);
		dlg.exec();
	}
}
void DemoQtGUI::on_OptimizeButton_Clicked()
{
	CInspect::ERR_INSP err = CInspect::OK;
	ui.statusBar->showMessage("Optimizing ",10);
	if((err = m_pInspect->optimize()) !=  CInspect::OK)
	{
		QString msg;
		ui.Display1->ClearOverlay();
		CInspectImage A = m_pInspect->getAlignedImage();
		QImage AlignedImage;
		InspectImageToQImage(A,AlignedImage);
		setDisplayImg(AlignedImage);
		CInspectImage O = m_pInspect->getOverlayImage();
		QImage Overlay;
		InspectImageToQImage(O,Overlay);
		setOverlayImg(Overlay);
		ui.Display1->ShowOverlay();
		ui.Display1->UpdateRoi();
		msg = QString("Optimize Failed at RED TILES   Complete status: ") + m_pInspect->getErrorString(err).c_str();
		QMessageBox mBox("Optimize ** ",msg + "\n   Wish to Continue? ",QMessageBox::Icon::Question,
			QMessageBox::Button::Yes, QMessageBox::Button::No,QMessageBox::Button::NoButton);
		if( mBox.exec() == QMessageBox::Button::No )
		{
			LogMsg(msg);
			LogMsg(QString("Selected Do Not Continue"));
			m_Abort = true;
		}
	}
	ui.statusBar->showMessage("Ready ",0);
}
void DemoQtGUI::on_ModeRadioButton_Clicked()
{}

void DemoQtGUI::makeMask(QString imgname, QString modelname, QString maskname, QString params)
{
	bool ModelExists = false;
	CMaskDlg dlg(this);

	dlg.setParams(m_Params);
	dlg.setFiles(imgname,modelname,maskname,params);

	while(!ModelExists)
	{
		if(!dlg.exec())
		{
			QMessageBox mbox(QMessageBox::Warning,"Verify Default Mask",
				"Mask was not created by Dialog. \n    Press OK to use an all inclusive mask.\n    Press Cancel to Re-Define Mask.",
				QMessageBox::Ok | QMessageBox::Cancel);
			if(mbox.exec())
			{
				QImage img(imgname);
				cv::imwrite(modelname.toUtf8().constData(),qimage_to_mat_ref(img,CV_8U));
				cv::Mat maskMat = qimage_to_mat_cpy(img,CV_8U);
				maskMat = 255;	// create a mask that includes all
				cv::imwrite(maskname.toUtf8().constData(),maskMat);

				// create model points
				cv::vector<cv::Point2f> points;
				cv::Mat temp = qimage_to_mat_ref(img,CV_8UC3);
				cv::cvtColor(temp,temp,CV_BGR2GRAY);
				points.clear();
				cv::goodFeaturesToTrack(temp,points,4,0.2,50,noArray(),5);
				CAlignModel m;
				for(int i=0; i<points.size(); i++)
				{
					m.setPos(points[i].x,points[i].y);
					m_Params.m_Models[i] = m;
				}
				m_Params.SetCrop(Rect(0,0,0,0));
			}
		}
		else
		{
			ModelExists = true;
			dlg.getParams(&m_Params);
		}
	}

}
void DemoQtGUI::setOverlayImg(QImage Ilarge)
{
	// GUI is impossibly slow if HUGE images are involved
	// Rescale image appropriately and then scale the results (mask and points)
	// when they are returned or saved.
	QImage overlay;
	cv::Mat temp;
	if(m_Imgscale != 1.0)
	{
		temp = qimage_to_mat_ref(Ilarge,CV_8UC4);
		cv::resize(temp,temp, cvSize(0, 0), m_Imgscale,m_Imgscale);
		overlay = mat_to_qimage_ref(temp,QImage::Format_ARGB32_Premultiplied);
	}
	else
	{
		overlay = Ilarge;
	}

	ui.Display1->SetOverlay(overlay);
}
void DemoQtGUI::setDisplayImg(QImage Ilarge)
{
	// GUI is impossibly slow if HUGE images are involved
	// Rescale image appropriately and then scale the results (mask and points)
	// when they are returned or saved.
	if(Ilarge.size().width() > 512)
	{
		m_Imgscale = 512.0F / Ilarge.size().width();
		m_ImgScaled = Ilarge.scaledToWidth(512);
	}
	else
	{
		m_Imgscale = 1.0F;
		m_ImgScaled = Ilarge;
	}
	ui.Display1->AttachQImage(m_ImgScaled);
	// clear the failImage on Display2
	ui.Display2->setHidden(true);
}
void DemoQtGUI::LogMsg(QString msg)
{
	QTextStream LogStream(&m_Logfile);
	LogStream << msg << "\n";
}


//********************* static Local Functions ***************************
static void DirDlg(QString &dirName)
{
	QFileDialog FileDlg;
	FileDlg.setFileMode(QFileDialog::Directory);
	FileDlg.setViewMode(QFileDialog::List);
	FileDlg.setDirectory(dirName);
	FileDlg.setOption(QFileDialog::ShowDirsOnly,true);
	if(FileDlg.exec())
	{
		QStringList list = FileDlg.selectedFiles();
		if((int)list.size() > 0)
		{
			QString DirName = list[0];
			if(!DirName.isEmpty())
			{
				dirName = DirName;
			}
		}
		else
			QMessageBox::critical(0, "Select Directory",
				QString("No Image Directory Selected \n"));
	}
}

#pragma warning(pop)