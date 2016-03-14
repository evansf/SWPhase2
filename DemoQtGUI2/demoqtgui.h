#ifndef DEMOQTGUI_H
#define DEMOQTGUI_H

#include <QtWidgets/QMainWindow>
#include <QtCore/qfile>
#include "ui_demoqtgui.h"
#include "SISUtility.h"
class CInspect;

class DemoQtGUI : public QMainWindow
{
	Q_OBJECT
public:
	DemoQtGUI(QWidget *parent = 0);
	~DemoQtGUI();

public slots:
	void on_TestFolderBrowseButton_Clicked();
	void on_TestFolderLineEdit_textChanged(const QString &);
	void on_CreateButton_Clicked();
	void on_TrainButton_Clicked();
	void on_InspectButton_Clicked();
	void on_RunStatsButton_Clicked();
	void on_ShowStatsButton_Clicked();
	void on_OptimizeButton_Clicked();
	void on_ModeRadioButton_Clicked();
	void on_InspectTrainButton_Clicked();

	void on_Reset_Clicked();

private:
	Ui::DemoQtGUIClass ui;
	QString m_TestDir;
	QString m_TrainDir;
	QString m_InspectPassDir;
	QString m_InspectFailDir;
	QImage m_ModelImg;
	QImage m_ModelMask;
	cv::Mat m_ModelCVImg;
	cv::Mat m_ModelCVMask;
	QImage m_ImgScaled;	// scaled down for display
	float m_Imgscale;
	bool m_Reset;
	QVector<QPointF> m_StatsPlotData;

	SISParams m_Params;
	CInspect *m_pInspect;	// pointer to keep implementation blind

	void makeMask(QString Image0, QString model0, QString Mask0, QString params0);
	void Inspect(QString dirname, int *pass, int *fail, bool expectFail);
	void LogMsg(QString s);
	QFile m_Logfile;
	bool m_Abort;
	double m_AvTime;
	int m_ImgCount;
	bool m_TrainEx;

	void setDisplayImg(QImage Ilarge);
	void setOverlayImg(QImage Ilarge);
	void TestFolderLineEdit_Update();
	void getParameters(FILE* pFile);
};

#endif // DEMOQTGUI_H
