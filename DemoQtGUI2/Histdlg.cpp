#include "Histdlg.h"


CHistDlg::CHistDlg(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	m_pPlotCurve = new QwtPlotCurve("Score Histogram");
	m_pSeries = NULL;
	ui.HistPlot->setAxisScale(QwtPlot::yLeft,0,1000);
	ui.HistPlot->setAxisScale(QwtPlot::xBottom,0,100);
	ui.HistPlot->setAxisTitle(QwtPlot::yLeft,QwtText("Count"));
	ui.HistPlot->setAxisTitle(QwtPlot::xBottom,QwtText("Score"));
}

CHistDlg::~CHistDlg()
{
}
void CHistDlg::setPlotData(std::vector<std::array<float,2>> histdata)
{
	if(m_pSeries != NULL)
		delete m_pSeries;

	QVector<QPointF> histVec;
	double minx = FLT_MAX, maxx=FLT_MIN;
	for(int i=0; i<(int)histdata.size(); i++)
	{
		histVec.push_back(QPointF(histdata[i][0],histdata[i][1]));
		if(histdata[i][0]<minx) minx = histdata[i][0];
		if(histdata[i][0]>maxx) maxx = histdata[i][0];
	}

	m_pSeries = new QwtPointSeriesData(histVec);

	m_pPlotCurve->setData(m_pSeries);
	ui.HistPlot->setAxisScale(QwtPlot::yLeft,0,1000);
	ui.HistPlot->setAxisScale(QwtPlot::xBottom,minx,maxx);
	m_pPlotCurve->attach(ui.HistPlot);
}
