#ifndef ROCDLG_H
#define ROCDLG_H

#include <QDialog>
#include <qpoint.h>
#include <qvector.h>
#include <qwt_plot_curve.h>
#include "ui_ROCDlg.h"

class CROCDlg : public QDialog
{
	Q_OBJECT

public:
	CROCDlg(QWidget *parent = 0);
	~CROCDlg();
	Ui::ROCDlg ui;

	void setPlotData(QVector<QPointF> &data);
	void fitPlotData(QVector<QPointF> &data);

private:
	QwtPlotCurve *m_pPlotCurve;
	QwtPointSeriesData *m_pSeries;
	bool fitData;
	float m_K;

};

#endif // ROCDLG_H
