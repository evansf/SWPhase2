#include "stdafx.h"
#include "InspectPrivateIntensity.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/video/video.hpp"
#define _USE_MATH_DEFINES
#include <math.h>


using namespace INTENSITY;
// *********** forward declarations of static (this file only) functions *********************
// Proceedures for the (*proc)() of the parallel_for loops
static void RelativeIntensityExtract(TILE &,cv::Mat &, cv::Mat &);
static void IntensityExtract(TILE &,cv::Mat &, cv::Mat &);
static void showDebugScores(TILE &T, KNOWLEDGE &K, cv::Mat &feat);

// *********** Methods for the specific feature implementation *******************************
CInspectPrivateIntensity::CInspectPrivateIntensity(SISParams &params) :
	CInspectPrivate(params)
{
}

CInspectPrivateIntensity::~CInspectPrivateIntensity(void)
{
}

CInspect::ERR_INSP CInspectPrivateIntensity::create(cv::Mat &image, cv::Mat &mask, CInspectData **ppData,
										   int tilewidth, int tileheight, int stepcols, int steprows)
{
	if(stepcols == 0) stepcols = tilewidth;
	if(steprows == 0) steprows = tileheight;
	// call the generic method first
	CInspect::ERR_INSP err = CInspectPrivate::create(image, mask, ppData,
													tilewidth, tileheight, stepcols, steprows);
	if(err != CInspect::OK)
		return err;

	setRawScoreDiv(100.0f / (tilewidth * tileheight));

	cvtToBW(m_Model);	// Initially Intensity is only on BW images

	// make a matrix for the feature vectors
	// rows = number of tiles ;  cols = number of features
	m_FeatMat.create((int)m_TileVec.size(), tilewidth*tileheight, CV_32F);

	return CInspect::OK;
}
// Preproc --- 
CInspect::ERR_INSP CInspectPrivateIntensity::PreProc()
{
	CInspect::ERR_INSP err = CInspectPrivate::PreProc();
	if(err != CInspect::OK)
		return err;

	// Run Time image data for Intensity must be a float
	if(m_pData->m_ImgRun.type() != CV_32F)
	{
		cv::Mat temp(m_pData->m_ImgRun.size(),CV_32F);
		unsigned char* pBW;
		float* pF;
		for(int i=0; i<m_pData->m_ImgRun.rows; i++)
		{
			pBW = m_pData->m_ImgRun.ptr(i);
			pF = (float*)temp.ptr(i);
			for(int j=0; j<m_pData->m_ImgRun.cols; j++)
				pF[j] = (float)pBW[j];
		}
		m_pData->m_ImgRun = temp;
	}
	return err;
}

CInspect::ERR_INSP CInspectPrivateIntensity::train(cv::Mat &image)
{
	m_TrainMode = TRAIN;
	// call generic train which calls PreProc() if needed
	CInspect::ERR_INSP err = CInspectPrivate::train(image);
	if(err != CInspect::OK)
		return err;
	
	err = runScoring();
	if(err != CInspect::OK)
		return err;

	return err;

}
CInspect::ERR_INSP CInspectPrivateIntensity::trainEx(cv::Mat &image)
{
	// call generic inspect which calls PreProc() if needed
	CInspect::ERR_INSP err = CInspectPrivate::inspect(image);
	if(err != CInspect::OK)
		return err;
	err = CInspectPrivate::trainEx(image);

	return err;
}

#define ZEROGAIN (0.05F)
CInspect::ERR_INSP CInspectPrivateIntensity::optimize()
{
	CInspect::ERR_INSP err = CInspectPrivate::optimize();

	return err;
}
CInspect::ERR_INSP CInspectPrivateIntensity::inspect(cv::Mat &image)
{
	m_TrainMode = INSPECT;
	// call generic inspect (calls PreProc() if needed)
	CInspect::ERR_INSP err = CInspectPrivate::inspect(image);
	if(err != CInspect::OK)
		return err;

	err = runScoring();
	if(err != CInspect::OK)
		return err;

	// for Inspecting,  return FAIL if different
	m_MaxScore = 0.0f;
	err = CInspect::OK;
	m_MahalaCount = 0;
	m_FinalScore = 1.0F;
	float Score;
	for(int i=0; i<(int)m_TileVec.size(); i++)
	{
		TILE &T = m_TileVec[i];
		Score = m_ModelKnowledge[i].covar.MahalanobisScore(T.scoreRaw, m_RawScoreDiv);
		m_FinalScore = Score < m_FinalScore ? Score : m_FinalScore; 
		UpdateRawScore(T);
		T.status = CInspect::OK;
		if(Score < m_InspectThresh)	// reject if different
		{
			T.status = CInspect::FAIL;
			err = CInspect::FAIL;
		}
		cv::Rect roi;
		cv::Mat temp;
		if(T.status == CInspect::FAIL)
		{
			annotate(cv::Rect(T.col,T.row,T.tilewidth,T.tileheight));
		}
	}
	return err;
}

CInspect::ERR_INSP CInspectPrivateIntensity::runScoring()
{
	CInspect::ERR_INSP err = CInspect::OK;
	err = runExtract(        IntensityExtract);
//	err = runExtract(RelativeIntensityExtract);

	switch(m_TrainMode)
	{
		case TRAIN:
			err = runTrain();	// use default trainproc
			break;
		case INSPECT:
			err = runInspect();	// use default Inspectproc
			break;
		default:
			break;
	}

	if( (m_DebugFile != NULL) )
	{
		cv::Mat temp = m_FeatMat(cv::Rect(0,100,m_FeatMat.cols,1));
		putFloatMat(m_DebugFile, &temp,"", "\n");
		putFloatMat(m_DebugFile, m_ModelKnowledge[100].covar.m_pM12,",,,,,","\n");
	}

	return err;
}

//*********** Specific Code for Specific Feature data **********************
static void RelativeIntensityExtract(TILE &t, cv::Mat &Img, cv::Mat &FeatMat)
{
	cv::Mat temp = Img(cv::Rect(t.col,t.row,t.tilewidth,t.tileheight));
	cv::Scalar sum = cv::sum(temp);
	float inverse = (float)(1.0 / sum[0]);
	float* pU = (float*)FeatMat.ptr(t.Index);
	float* pPix;
	for(int i=0; i<t.tileheight; i++)
	{
		pPix = (float*)temp.ptr(i);
		for(int j=0; j<t.tilewidth; j++)
		{
			pU[j] = inverse * pPix[j];
		}
		pU += t.tilewidth;
	}
	t.datasize = INTENSITYDATASIZE;
}
static void IntensityExtract(TILE &t, cv::Mat &Img, cv::Mat &FeatMat)
{
	cv::Mat temp = Img(cv::Rect(t.col,t.row,t.tilewidth,t.tileheight));
//	cv::Scalar sum = cv::sum(temp);	// keep sum to normalize
//	float inverse = (float)(1.0 / sum[0]);
	float* pU = (float*)FeatMat.ptr(t.Index);
	float* pPix;
	for(int i=0; i<t.tileheight; i++)
	{
		pPix = (float*)temp.ptr(i);
		for(int j=0; j<t.tilewidth; j++)
		{
//			pU[j] = inverse * pPix[j];
			pU[j] =           pPix[j];
		}
		pU += t.tilewidth;
	}
	t.datasize = INTENSITYDATASIZE;
}

// showDebugScores() is a convenience debug function to call if you are having issues....
// This code WILL BE OUT OF DATE -- Modify to show what you need to view.
static void	showDebugScores(TILE &T, KNOWLEDGE &K, cv::Mat &FeatMat)
{
	char msg[500];
	sprintf_s(msg,500,"Tile Inspect (%4d, %4d) = \n",T.col,T.row);
	OutputDebugStringA(msg);
	float *pF = (float*)FeatMat.ptr(T.Index);

	char temp[100];
	cv::Mat CvInv = K.covar.InverseMatrix();
	if(CvInv.empty())
		OutputDebugStringA("Knowledge Empty\n");
	else
	{
		sprintf_s(msg,500,"Tile Knowledge \n");
		OutputDebugStringA(msg);
		sprintf_s(msg,500,"Tile Feature = \n");
		OutputDebugStringA(msg);

		sprintf_s(msg,500,"%6.2f",pF[0]);
		for( int j=1; j<4; j++)
		{
			sprintf_s(temp,100,", %6.2f",pF[j]);
			strncat_s(msg,500,temp,100);
		}
		strncat_s(msg,500,"\n",1);
		OutputDebugStringA(msg);
		pF += 4;
		sprintf_s(msg,500,"%6.2f",pF[0]);
		for( int j=1; j<3; j++)
		{
			sprintf_s(temp,100,", %6.2f",pF[j]);
			strncat_s(msg,500,temp,100);
		}
		strncat_s(msg,500,"\n",1);
		OutputDebugStringA(msg);
		pF += 3;
		sprintf_s(msg,500,"%6.2f",pF[0]);
		for( int j=1; j<2; j++)
		{
			sprintf_s(temp,100,", %6.2f",pF[j]);
			strncat_s(msg,500,temp,100);
		}
		strncat_s(msg,500,"\n",1);
		OutputDebugStringA(msg);
		pF += 2;
		sprintf_s(msg,500,"%6.2f\n",pF[0]);
		OutputDebugStringA(msg);

		for(int i=0; i<CvInv.rows; i++)
		{
			pF = (float*)CvInv.ptr(i);
			sprintf_s(msg,500,"%11.2f",pF[0]);
			for( int j=1; j<CvInv.cols; j++)
			{
				sprintf_s(temp,100,", %11.2f",pF[j]);
				strncat_s(msg,500,temp,100);
			}
			strncat_s(msg,500,"\n",1);
			OutputDebugStringA(msg);
		}
					pF = (float*)CvInv.ptr(0);
					if(pF[0] == 0.0F)
						pF = (float*)CvInv.ptr(1) + 1;
	}
}
