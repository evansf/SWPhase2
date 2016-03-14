#ifndef EXAMINEDLG_H
#define EXAMINEDLG_H

#include <QDialog>
#include "ui_ExamineDlg.h"
#include "SISUtility.h"
#include <QtGui\QMouseEvent>

#define SNAP 10
#define TRANSLUCENCY 64

class CExamineDlg : public QDialog
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
	CExamineDlg(QWidget *parent = 0);
	~CExamineDlg();
	Ui::ExamineDlg ui;

public slots:
	void on_ContinueButtonClicked();
	void on_AbortButtonClicked();
	void on_RectDrawn(QRect box);
	void on_DisplayClicked(QMouseEvent*);
	void on_MousePress(QMouseEvent *);
	void on_MouseRelease(QMouseEvent *);
	void setImage(QImage &img);

signals:

private:
	MODE m_DrawMode;
	MODE m_RecentDraw;
	QRect m_Box;
	QImage m_Img;
	QPixmap OverlayPixmap;

};

#endif // EXAMINEDLG_H
