#include "ROCdlg.h"


CROCDlg::CROCDlg(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	m_pPlotCurve = new QwtPlotCurve("Detector");
	m_pSeries = NULL;
	ui.ROCPlot->setAxisScale(QwtPlot::yLeft,0.0,1.0);
	ui.ROCPlot->setAxisScale(QwtPlot::xBottom,0.0,1.0);
	ui.ROCPlot->setAxisTitle(QwtPlot::yLeft,QwtText("True Positive Rate (TPR)"));
	ui.ROCPlot->setAxisTitle(QwtPlot::xBottom,QwtText("False Positive Rate (FPR)"));
}

CROCDlg::~CROCDlg()
{
}
void CROCDlg::setPlotData(QVector<QPointF> &data)
{
	fitData = false;
	if(m_pSeries != NULL)
		delete m_pSeries;
	m_pSeries = new QwtPointSeriesData(data);

	m_pPlotCurve->setData(m_pSeries);
	m_pPlotCurve->attach(ui.ROCPlot);
}
void CROCDlg::fitPlotData(QVector<QPointF> &data)
{
	fitData = true;
	if(m_pSeries != NULL)
		delete m_pSeries;

	// get mean of hyperbola constant
	m_K = 0.0F;
	int count = 0;
	for(int i=0; i<(int)data.size(); i++)
	{
		if( (data[i].x() > FLT_EPSILON) && (data[i].x() < 1.0F) )
		{
			m_K += data[i].x()*data[i].y();
			count++;
		}
	}
	m_K /= (float)count;

	// create 100 Point plot of hyperbola
	QVector<QPointF> regression;
	QPointF P(0.0F,0.0F);
	regression.push_back(P);	// plot the 0,0 point
	for(float x=0.0F; x<1.0F; x += 0.01F)
	{
		P.setX(x);
		P.setY(1.0F - m_K / x);
		if(P.y()>0.0F)
			regression.push_back(P);
	}
	m_pSeries = new QwtPointSeriesData(regression);

	m_pPlotCurve->setData(m_pSeries);
	m_pPlotCurve->attach(ui.ROCPlot);
}
