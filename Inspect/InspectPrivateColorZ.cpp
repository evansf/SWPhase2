#include "stdafx.h"
#include "InspectPrivateColorZ.h"
#include "PaletteOctree.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/video/video.hpp"
#include "SISUtility.h"
#include <math.h>


using namespace COLORZ;
#define PALETTESIZE 256

// *********** forward declarations of static (this file only) functions *********************
// Proceedures for the (*proc)() of the parallel_for loops
static void ColorZExtract(TILE &,cv::Mat &);

// *********** Methods for the specific feature implementation *******************************

CInspectPrivateColorZ::CInspectPrivateColorZ(SISParams &params) :
	CInspectPrivate(params)
{
	m_pPalette = new CPaletteOctree();
	return;
}

CInspectPrivateColorZ::~CInspectPrivateColorZ(void)
{
}

CInspect::ERR_INSP CInspectPrivateColorZ::create(cv::Mat &image, cv::Mat &mask, CInspectData **ppData, CInspect::ALIGNMODE mode, 
										   int tilewidth, int tileheight, int stepcols, int steprows)
{
	if(stepcols == 0) stepcols = tilewidth;
	if(steprows == 0) steprows = tileheight;
	// call the generic method first
	CInspect::ERR_INSP err = CInspectPrivate::create(image, mask, ppData, mode,
													tilewidth, tileheight, stepcols, steprows);
	if(err != CInspect::OK)
		return err;

	cv::Mat temp;
	switch(m_Model.channels())
	{
	case 1:
		cvtColor(m_Model,temp,CV_GRAY2BGR);
		break;
	case 3:
		temp = m_Model;
		break;
	case 4:
		cvtColor(m_Model,temp,CV_RGBA2BGR);
		break;
	default:
		return CInspect::UNSUPPORTEDIMAGETYPE;
		break;
	}

	if(m_pPalette->Create(temp,PALETTESIZE) != 0)
		return CInspect::UNABLETOPALETTIZE;
	m_pPalette->MakePaletteTable();

	// tile the image applying the mask
	// make a vector of TILE objects and a vector of KNOWLEDGE objects
	int numrows = (temp.rows - tileheight)/steprows;
	int numcols = (temp.cols - tilewidth)/stepcols;
	m_TileVec.clear();
	TILE tile;
	tile.tileheight = tileheight;
	tile.tilewidth = tilewidth;
	m_PaletteKnowledge.clear();
	KNOWLEDGE k;
	for(int i = 0; i<PALETTESIZE; i++)
	{
		m_PaletteKnowledge.push_back(k);
	}

	return CInspect::OK;
}

CInspect::ERR_INSP CInspectPrivateColorZ::train(cv::Mat &image)
{
	cv::Mat temp;
	switch(image.channels())
	{
	case 1:
		cvtColor(image,temp,CV_GRAY2BGR);
		break;
	case 3:
		temp = m_Model;
		break;
	case 4:
		cvtColor(image,temp,CV_RGBA2BGR);
		break;
	default:
		return CInspect::UNSUPPORTEDIMAGETYPE;
		break;
	}

	// get the cv::Mat from the input data
	m_pData->m_ImgIn = temp;
	
	CInspect::ERR_INSP err = CInspect::OK;
//	err = runScoring(image);
//	if(err != CInspect::OK)
//		return err;

	// for Training,  add vector to Knowledge if different
	// scan the tiles and check the scores
	err = CInspect::OK;
	if(!m_pPalette->exists())
	{
		m_pPalette->Create(image,PALETTESIZE);
	}

	m_pPalette->Palettize(image,m_Palettized);
	Learn(m_Palettized, image);	// get stats for color relative to its palette brothers

	return err;
}

CInspect::ERR_INSP CInspectPrivateColorZ::optimize()
{
	CInspect::ERR_INSP err = CInspect::OK;
	// invert the coVariance matrix in each palette color
	CCoVariance *pC;
	for(int i=0; i<PALETTESIZE; i++)
	{
		pC = &(m_PaletteKnowledge[i].covar);
		if(pC->Invert()!= 0)
		{
			err = CInspect::UNABLETOINVERTCOVAR;
		}
	}
	return err;
}

CInspect::ERR_INSP CInspectPrivateColorZ::inspect(cv::Mat &image)
{
	cv::Mat temp;
	switch(image.channels())
	{
	case 1:
		cvtColor(image,temp,CV_GRAY2BGR);
		break;
	case 3:
		temp = m_Model;
		break;
	case 4:
		cvtColor(image,temp,CV_RGBA2BGR);
		break;
	default:
		return CInspect::UNSUPPORTEDIMAGETYPE;
		break;
	}

	// get the cv::Mat from the input data
	m_pData->m_ImgIn = temp;
	
	CInspect::ERR_INSP err;
	err = runScoring(image);
	if(err != CInspect::OK)
		return err;

	// for Inspecting,  return FAIL if different
	float minscore = 1.0F;

	err = CInspect::OK;

	runScoring(image);

	cv::Mat Reject;
	cv::threshold(m_ScoreImg,Reject,m_InspectThresh,255,CV_THRESH_BINARY);

	cv::Mat Reject8U;
	Reject.convertTo(Reject8U,CV_8U);
	// erode to eliminate noise
	cv::erode(Reject8U,Reject8U,cv::Mat(),cv::Point(-1,-1),3);
	// identify remaining blobs of adequate size
	std::vector<vector<Point>> contours;
	cv::findContours(Reject8U,contours,CV_RETR_LIST,CV_CHAIN_APPROX_SIMPLE);
	filterContours(contours);
	if(!contours.empty())
	{
		annotate(contours);
		return CInspect::FAIL;
	}

	m_MaxScore = 0.0;
	m_MaxX = m_MaxY = 0;
	return err;
}
CInspect::ERR_INSP CInspectPrivateColorZ::runScoring(cv::Mat &image)
{
	// call the generic align method first
	CInspect::ERR_INSP err = align();
	if(err != CInspect::OK)
		return err;

	// guarantee image type
	cv::Mat temp;
	switch(m_Model.channels())
	{
	case 1:
		cvtColor(m_Model,temp,CV_GRAY2BGR);
		break;
	case 3:
		temp = m_Model;
		break;
	case 4:
		cvtColor(m_Model,temp,CV_RGBA2BGR);
		break;
	default:
		return CInspect::UNSUPPORTEDIMAGETYPE;
		break;
	}

	// calculate the color selector (Bin number)
	// palletize the color
	m_pPalette->Palettize(image,m_Palettized);
	Score(m_Palettized, image, m_ScoreImg);	// get score for color relative to its palette brothers

	return CInspect::OK;
}
void CInspectPrivateColorZ::Score(cv::Mat &Palettized, cv::Mat &image, cv::Mat &ScoreImg)
{
	ScoreImg.create(Palettized.size(),CV_32F);
	// Use knowledge for each palette color to get Mahalnobis score translated to [0.0....1.0]
	unsigned char *pPix;
	unsigned char *pBGR;
	float *pOut;
//	cv::Mat CInverse;
//	cv::Mat Means;
	cv::Mat BGRFloat(1,3,CV_32F);
	float *pF = (float*)BGRFloat.ptr();
	float raw;
//	double Det;
	for(int i=0; i<Palettized.rows; i++)
	{
		pPix = m_Palettized.ptr(i);
		pBGR = image.ptr(i);
		pOut = (float*)ScoreImg.ptr(i);
		for(int j=0; j<m_Palettized.cols; j++)
		{
			if(m_PaletteKnowledge[pPix[j]].covar.m_pM12 == NULL)
			{
				pOut[j] = 0.0F;
				continue;
			}
//			CInverse = m_PaletteKnowledge[pPix[j]].covar.InverseMatrix();
//			Det = m_PaletteKnowledge[pPix[j]].covar.m_detC; 
//			Means = m_PaletteKnowledge[pPix[j]].covar.Means();
			pF[0] = (float)pBGR[j]; pF[1] = (float)pBGR[j+1]; pF[2] = (float)pBGR[j+2];
			raw = m_PaletteKnowledge[pPix[j]].covar.MahalanobisRaw(BGRFloat);
			pOut[j] = m_PaletteKnowledge[pPix[j]].covar.MahalanobisScore(raw, m_ScaleMahala);
		}
	}
}

#define SMALLSIZE 5
void CInspectPrivateColorZ::filterContours(std::vector<vector<cv::Point>> &contours)
{
	std::vector<vector<cv::Point>> temp;
	for(int i=0; i<(int)contours.size(); i++)
	{
		// eliminate small area
		if((int)contours[i].size() < SMALLSIZE)
			continue;
		// eliminate 'not under mask'
		std::vector<cv::Point> contour = contours[i];
		for(int j=0; j<(int)contour.size(); j++)
		{
			if(!m_Mask.ptr(contour[j].y)[contour[j].x])	// at least one pixel of contour outside
				continue;
		}
		// put survivors onto output
		temp.push_back(contour);
	}
	contours = temp;
}

void CInspectPrivateColorZ::Learn(cv::Mat &m_Palettized, cv::Mat &image)
{
	int row,col;
	CCoVariance *pC;
	unsigned char *pPix;
	unsigned char * pMask;
	unsigned char * pImg;
	int ColorIdx;
	float ColorFloats[3];
	for(row = 0; row < m_Palettized.rows; row++)
	{
		pMask = m_Mask.ptr(row);
		pPix  = m_Palettized.ptr(row);
		pImg = image.ptr(row);
		for(col = 0; col < m_Palettized.cols; col++)
		{
			if(!pMask[col]) continue;

			ColorFloats[0] = (float)pImg[col]; ColorFloats[1] =  (float)pImg[col+1];	ColorFloats[2] =  (float)pImg[col+2];

			ColorIdx = pPix[col];	// get covar from Knowledge of this palette color
			pC = &(m_PaletteKnowledge[ColorIdx].covar);
			if(pC->Samples() == 0)
				pC->Create(ColorFloats,3);
			else
				pC->IncrementalUpdate(ColorFloats,3);
		}
	}
}

