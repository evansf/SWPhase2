#ifndef HISTDLG_H
#define HISTDLG_H

#include <QDialog>
#include <qpoint.h>
#include <qvector.h>
#include <qwt_plot_curve.h>
#include "ui_HistDlg.h"
#include <vector>
#include <array>
using namespace std;

class CHistDlg : public QDialog
{
	Q_OBJECT

public:
	CHistDlg(QWidget *parent = 0);
	~CHistDlg();
	Ui::HistDlg ui;

	void setPlotData(std::vector<std::array<float,2>> histdata);

private:
	QwtPlotCurve *m_pPlotCurve;
	QwtPointSeriesData *m_pSeries;
	float m_K;

};

#endif // HISTDLG_H
