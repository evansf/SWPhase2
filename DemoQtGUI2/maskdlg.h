#ifndef MASKDLG_H
#define MASKDLG_H

#include <QDialog>
#include "ui_MaskDlg.h"
#include "SISUtility.h"
#include <QtGui\QMouseEvent>

#define SNAP 10
#define TRANSLUCENCY 64

class CAlignModel;

class CMaskDlg : public QDialog
{
	Q_OBJECT
public:
	typedef enum drawmodes
	{
		OFF,
		BOX,
		POLY,
		DISPLAY,
		SAVE,
		NONE
	} MODE;

public:
	CMaskDlg(QWidget *parent = 0);
	~CMaskDlg();
	Ui::MaskDlg ui;
	void setFiles(QString img, QString cropped, QString mask, QString paramsname);
	void setParams(SISParams &params){m_Params = params;};
	void getParams(SISParams *params){*params = m_Params;};
	std::vector<std::pair<float,float>> & CMaskDlg::getTrackPoints();

public slots:
	void on_saveButton_Clicked();
	void on_cancelButton_Clicked();
	void on_resetButton_Clicked();
	void on_includeButton_Clicked();
	void on_excludeButton_Clicked();
	void on_discardButton_Clicked();
	void on_setCropButton_Clicked();
	void on_boxButton_Clicked();
	void on_polyButton_Clicked();
	void on_closepolyButton_Clicked();
	void on_PolyKnot(QPoint p);
	void on_PolyDrawn();
	void on_RectDrawn(QRect box);
	void on_ClearAlignMasks();
	void on_AddAlignModelsButton_clicked();
	void on_DisplayClicked(QMouseEvent*);
	void on_MousePress(QMouseEvent *);
	void on_MouseRelease(QMouseEvent *);

signals:

private:
	MODE m_DrawMode;
	MODE m_RecentDraw;
	QRect m_Box;
	QString ImgFile;
	QImage m_fileImg;	// as is from file
	QImage m_CroppedImg;	// cropped
	QImage m_Img;	// scaled down for display
	float m_Imgscale;
	QString MaskFile;
	cv::Mat Mask;
	QPixmap QMask;
	QPixmap m_Mask;	// scaled down for display
	QPixmap OverlayPixmap;
	QString ParamsFile;
//	std::vector<std::pair<float,float>> TrackPoints;
//	bool m_CreateTrackPoints;
	QString ModelFile;
	int m_AlignMoveIndex;
	SISParams m_Params;
	std::vector<CAlignModel> m_AlignModels;
	QRect m_QCrop;

	std::vector<QPoint> m_PolyVec;
	void ShowMask();
	bool inline InMask(std::pair<float,float> p);
};

#endif // MASKDLG_H
