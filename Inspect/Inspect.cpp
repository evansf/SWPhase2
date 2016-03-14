// Inspect.cpp : Defines the exported functions for the DLL application.
//

#include "Inspect.h"

#include "InspectPrivate.h"
#include "InspectPrivateDCT.h"
#include "InspectPrivateIntensity.h"
#include "InspectPrivateWave.h"
#include "InspectPrivateMOM.h"

// define error strings
#define INSPECTSOURCE 1
#include "inspecterrors.h"
#define _LOCALTIMER 1
#include "timer.h"
#include "SISUtility.h"

CInspect::CInspect(SISParams &params) :
	m_params(params)
{
	for(int i=0; i<NUMINSPECTIONS; i++)
		m_pPrivate[i] = NULL;
	m_InspectionCount = 0;
	m_pDataGlobal = NULL;
}

CInspect::~CInspect(void)
{
	for(int i=0; i<NUMINSPECTIONS; i++)
		if(m_pPrivate[i] != NULL)
			delete m_pPrivate[i];
	if(m_pDataGlobal != NULL) delete m_pDataGlobal;
}
void CInspect::setMustAlign(bool b)
{
	for(int i=0; i<NUMINSPECTIONS; i++)
		if(m_pPrivate[i]) m_pPrivate[i]->setMustAlign(b);
}
void CInspect::setRawScoreDiv(float g)
{
	for(int i=0; i<NUMINSPECTIONS; i++)
		if(m_pPrivate[i]) m_pPrivate[i]->setRawScoreDiv(g);
}
double CInspect::getRawScore(int *pInspect, int *pX, int *pY)	// most recent Total inspect Score
{
	// properties for worst inspection were set durring inspect()
	if(pX!=NULL) *pX=m_FailX;
	if(pY!=NULL) *pY=m_FailY;
	if(pInspect!=NULL) *pInspect = m_WorstInspection;
	return m_TotalScore;
}
double CInspect::getFinalScore()
{
	double s;
	double final = 1.0;
	for(int i=0; i<m_InspectionCount; i++)
	{
		s = m_pPrivate[i]->getFinalScore();
		final = s < final ? s : final;
	}
	return final;
}
double CInspect::getMean(int i){return m_pPrivate[i]->getMean();}
double CInspect::getSigma(int i){return m_pPrivate[i]->getSigma();}

CInspect::ERR_INSP CInspect::create(DATATYPE type, CInspectImage image, CInspectImage mask,
		int tilewidth, int tileheight, int stepcols, int steprows)
{
	STARTTIMER(t0);
	if(m_InspectionCount >= NUMINSPECTIONS)
		return TOO_MANY_INSPECTIONS;

	switch(type)
	{
	case DCT:
		m_pPrivate[m_InspectionCount] = new DCT::CInspectPrivateDCT(m_params);
		break;
	case INTENSITY:
		m_pPrivate[m_InspectionCount] = new INTENSITY::CInspectPrivateIntensity(m_params);
		break;
	case WAVE:
		m_pPrivate[m_InspectionCount] = new WAVE::CInspectPrivateWave(m_params);
		break;
	case MOM:
		m_pPrivate[m_InspectionCount] = new MOM::CInspectPrivateMOM(m_params);
		break;
	default:
		return BADDATATYPE;
	}
	cv::Mat CVImg;
	InspectImageToMat(image, CVImg);
	cv::Mat CVMask;
	InspectImageToMat(mask, CVMask);


	CInspect::ERR_INSP err = m_pPrivate[m_InspectionCount]->create( CVImg, CVMask, &m_pDataGlobal,
		tilewidth, tileheight, stepcols, steprows);

	m_isOptimized = false;
	STOPTIMER(t0);
	m_OpTime = t0;

	if(err != OK)
	{
		m_pPrivate[m_InspectionCount] = NULL;
		return err;
	}
	m_InspectionCount ++;
	return OK;
}

CInspect::ERR_INSP CInspect::train( CInspectImage image )
{
	STARTTIMER(t0);
	CInspect::ERR_INSP err;
	if(m_pDataGlobal != NULL)	// must start with new data class
	{
		delete m_pDataGlobal;
		m_pDataGlobal = NULL;
	}
	cv::Mat CVImg;
	InspectImageToMat(image, CVImg);

	for(int i=0; i<m_InspectionCount; i++)
		if(m_pPrivate[i]) err = m_pPrivate[i]->train(CVImg );

	m_isOptimized = false;
	STOPTIMER(t0);
	m_OpTime = t0;
	return err;
}
CInspect::ERR_INSP CInspect::trainEx(CInspectImage image) 	// extends training for overlay marked tiles
{
	STARTTIMER(t0);
	CInspect::ERR_INSP err;
	if(m_pDataGlobal != NULL)	// must start with new data class
	{
		delete m_pDataGlobal;
		m_pDataGlobal = NULL;
	}
	cv::Mat CVImg;
	InspectImageToMat(image, CVImg);
	for(int i=0; i<m_InspectionCount; i++)
		if(m_pPrivate[i]) err = m_pPrivate[i]->trainEx(CVImg);

	m_isOptimized = false;
	STOPTIMER(t0);
	m_OpTime = t0;
	return err;
}

CInspect::ERR_INSP CInspect::optimize()
{
	STARTTIMER(t0);
	CInspect::ERR_INSP err;
	for(int i=0; i<m_InspectionCount; i++)
		if(m_pPrivate[i]) err = m_pPrivate[i]->optimize();

	m_isOptimized = true;
	STOPTIMER(t0);
	m_OpTime = t0;
	return err;
}

CInspect::ERR_INSP CInspect::inspect( CInspectImage image, int *pFailIndex )
{
	STARTTIMER(t0);
	if(m_pDataGlobal != NULL)
	{
		delete m_pDataGlobal;
		m_pDataGlobal = NULL;
	}
	CInspect::ERR_INSP err;
	cv::Mat CVImg;
	InspectImageToMat(image, CVImg);

	m_TotalScore = 0;
	double score;
	for(int i=0; i<m_InspectionCount; i++)
	{
		if(m_pPrivate[i])
		{
			err = m_pPrivate[i]->inspect(CVImg );
			score = m_pPrivate[i]->getRawScore();
			if(score > m_TotalScore)
			{
				m_TotalScore = score;
				m_pPrivate[i]->getRawScore(&m_FailX,&m_FailY);
				if(pFailIndex!=NULL) *pFailIndex = i;
			}
		}
	}

	STOPTIMER(t0);
	m_OpTime = t0;
	return err;
}

std::string & CInspect::getErrorString(ERR_INSP err)
{
	return errorString[(int)err];
}
void CInspect::DrawRect( int x, int y, int width, int height, unsigned long xRGB)
{
	// only one overlay for all inspections so any private can draw for us
	m_pPrivate[0]->DrawRect(x,y,width,height,xRGB);
}

CInspectImage CInspect::getOverlayImage()
{
	INSPECTIMAGE Img;
	cv::Mat temp = m_pDataGlobal->m_Overlay;
	Img.height    = temp.rows;
	Img.width     = temp.cols;
	Img.pPix      = temp.ptr();
	Img.imgStride = temp.step;
	Img.Imagetype = imgT::UCHAR_FOURPLANE;
	return Img;
}
CInspectImage CInspect::getAlignedImage()
{
	INSPECTIMAGE Img;
	cv::Mat temp = m_pDataGlobal->m_ImgAligned;
	Img.height    = temp.rows;
	Img.width     = temp.cols;
	Img.pPix      = temp.ptr();
	Img.imgStride = temp.step;
	switch(temp.type())
	{
	case CV_8UC1:
		Img.Imagetype = imgT::UCHAR_ONEPLANE;
		break;
	case CV_8UC3:
		Img.Imagetype = imgT::UCHAR_THREEPLANE;
		break;
	case CV_8UC4:
		Img.Imagetype = imgT::UCHAR_FOURPLANE;
		break;
#if(0)
	case CV_16SC1:
		Img.Imagetype = imgT::SHORT_ONEPLANE;
		break;
	case CV_16SC3:
		Img.Imagetype = imgT::SHORT_THREEPLANE;
		break;
	case CV_16SC4:
		Img.Imagetype = imgT::SHORT_FOURPLANE;
		break;
	case CV_32F:
		Img.Imagetype = imgT::FLOAT32_FOURPLANE;
		break;
#endif
	default:
		Img.Imagetype = imgT::UCHAR_ONEPLANE;
		break;
	}
	return Img;
}

void CInspect::setLearnThresh(float t)
{
	for(int i=0; i<m_InspectionCount; i++)
		if(m_pPrivate[i]) m_pPrivate[i]->setLearnThresh(t);
}
void CInspect::setInspectThresh(float t)
{
	for(int i=0; i<m_InspectionCount; i++)
		if(m_pPrivate[i]) m_pPrivate[i]->setInspectThresh(t);
}
