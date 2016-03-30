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
// include local timer even in release mode
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
	m_ZScoreMode = false;
	m_Pass2 = false;
	m_WorstInspection = 0;
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
double CInspect::getRawScore(int *pInspect)	// most recent Total inspect Score
{
	// properties for worst inspection were set durring inspect()
	int Index;
	double score = m_pPrivate[m_WorstInspection]->getRawScore(&Index);
	if(pInspect!=NULL) *pInspect = Index;
	return score;
}
double CInspect::getDScore(int *pInspect)	// most recent Total inspect Score
{
	// properties for worst inspection were set durring inspect()
	int Index;
	double score = m_pPrivate[m_WorstInspection]->getDScore(&Index);
	if(pInspect!=NULL) *pInspect = Index;
	return score;
}
double CInspect::getZScore(int *pInspect)	// most recent Total inspect Score
{
	// properties for worst inspection were set durring inspect()
	int Index;
	double score = m_pPrivate[m_WorstInspection]->getZScore(&Index);
	if(pInspect!=NULL) *pInspect = Index;
	return score;
}
double CInspect::getTrueScore()
{
	double s;
	double final = 0.0;
	for(int i=0; i<m_InspectionCount; i++)
	{
		if(m_ZScoreMode)
		{
			s = m_pPrivate[i]->getZScore();
			final = s > final ? s : final;	// big Z fails
		}
		else
		{
			s = m_pPrivate[i]->getTrueScore();
			final = s > final ? s : final;	// big Final fails
		}
	}
	return final;
}
double CInspect::getMean(int i){return m_pPrivate[i]->getMean();}
double CInspect::getSigma(int i){return m_pPrivate[i]->getSigma();}
void CInspect::setUseZScoreMode(bool b)
{
	m_ZScoreMode = b;
	for(int i=0; i<m_InspectionCount; i++)
		m_pPrivate[i]->setUseZScoreMode(b);
}

void CInspect::setPass2(bool b)
{
	m_Pass2 = b;
	if(b)
		for(int i=0; i<m_InspectionCount; i++)
			m_pPrivate[i]->clearZStats();
}

CInspect::ERR_INSP CInspect::create(DATATYPE type, CInspectImage image, CInspectImage mask,
		int tilewidth, int tileheight, int stepcols, int steprows)
{
	STARTTIMER(t0);
	if(m_InspectionCount >= NUMINSPECTIONS)
		return TOO_MANY_INSPECTIONS;

	if(m_pDataGlobal!= NULL)
		delete m_pDataGlobal;
	m_pDataGlobal = NULL;
	m_ZScoreMode = false;
	m_Pass2 = false;
	m_WorstInspection = 0;

	// NEW INSPECTION TYPES -- ADD TO THIS SWITCH AND TO THE ENUM
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
		if(m_pPrivate[i])
			if(m_Pass2)
				err = m_pPrivate[i]->trainPass2(CVImg );
			else
				err = m_pPrivate[i]->train(CVImg );

	m_isOptimized = false;
	STOPTIMER(t0);
	m_OpTime = t0;
	return err;
}
CInspect::ERR_INSP CInspect::trainEx(int index) 	// extends training for overlay marked tiles
{
	CInspect::ERR_INSP err;
	STARTTIMER(t0);
	err = m_pPrivate[m_WorstInspection]->trainEx(index);
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

CInspect::ERR_INSP CInspect::inspect( CInspectImage image, int *pInspectIndex, int *pFailIndex )
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

	m_TrueScore = 1.0;
	m_ZScore = 0.0;
	double score;
	CInspect::ERR_INSP errReturn = CInspect::OK;
	for(int i=0; i<m_InspectionCount; i++)
	{
		if(m_pPrivate[i])
		{
			err = m_pPrivate[i]->inspect(CVImg, pFailIndex );
			if(m_ZScoreMode)
			{
				score = m_pPrivate[i]->getZScore();
				if(score > m_ZScore)	// larger scores fail
				{
					m_ZScore = score;
					m_WorstInspection = i;
					if(score > m_Threshold)
						errReturn = FAIL;
				}
			}
			else
			{
				m_pPrivate[i]->getRawScore(pFailIndex);
				score = m_pPrivate[i]->getTrueScore();
				if(score < m_TrueScore)	// smaller scores fail
				{
					m_TrueScore = score;
					m_WorstInspection = i;
					if(score < m_Threshold)
						errReturn = FAIL;
				}
			}
		}
		if(errReturn != CInspect::OK)
			break;
	}

	STOPTIMER(t0);
	m_OpTime = t0;
	if(pInspectIndex!=NULL) *pInspectIndex = m_WorstInspection;
	return errReturn;
}
void CInspect::MarkTileSkip(int index)	// Find new worst score will skip this index
{
	for(int i=0; i<m_InspectionCount; i++)
		m_pPrivate[i]->MarkTileSkip(index);
}
CInspect::ERR_INSP CInspect::FindNewWorstScore(int *pfailIndex)
{
	return m_pPrivate[m_WorstInspection]->FindNewWorstScore(pfailIndex);
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
#if(0)	// unsupported types
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

void CInspect::setInspectThresh(float t)
{
	m_Threshold = t;
	for(int i=0; i<m_InspectionCount; i++)
		if(m_pPrivate[i]) m_pPrivate[i]->setInspectThresh(t);
}

void CInspect::getXY(int index, int *pX, int *pY)
{
	m_pPrivate[m_WorstInspection]->getXY(index, pX, pY);
}
