#include "stdafx.h"
#include "InspectPrivatedct.h"
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/video/video.hpp"

using namespace DCT;
#include "CoVariance.h"
// *********** forward declarations of static (this file only) functions *********************
// Proceedures for the (*proc)() of the parallel_for loops
// provide Score function
static void DCTExtract(TILE &,cv::Mat &, cv::Mat &feat);

static void showDebugScores(TILE &T, KNOWLEDGE &K);

// Macro to draw crosses on cv::Mat images
#define drawCross( img, center, color, d , t ) \
	cv::line( img, cv::Point( (int)center.x - d, (int)center.y - d ), \
	cv::Point( (int)center.x + d, (int)center.y + d ), \
	color, t, 0 ); \
	cv::line( img, cv::Point( (int)center.x + d, (int)center.y - d ), \
	cv::Point( (int)center.x - d, (int)center.y + d ), \
	color, t, 0 )
#define drawCross2(img,center,color,d) drawCross(img,center,cv::Scalar(255,255,255),d, 5); drawCross(img,center,color,d, 3 );

// *********** Methods for the specific feature implementation *******************************
CInspectPrivateDCT::CInspectPrivateDCT(SISParams &params) :
	CInspectPrivate(params)
{
}

CInspectPrivateDCT::~CInspectPrivateDCT(void)
{
}

CInspect::ERR_INSP CInspectPrivateDCT::create(cv::Mat &image, cv::Mat &mask, CInspectData **ppData,
										   int tilewidth, int tileheight, int stepcols, int steprows)
{
	if(stepcols == 0) stepcols = tilewidth;
	if(steprows == 0) steprows = tileheight;
	// call the generic method first
	CInspect::ERR_INSP err = CInspectPrivate::create(image, mask, ppData,
													tilewidth, tileheight, stepcols, steprows);
	if(err != CInspect::OK)
		return err;

	// make a matrix for the feature vectors
	// rows = number of tiles ;  cols = number of features
	m_FeatMat.create((int)m_TileVec.size(), DCTDATASIZE, CV_32F);

	return CInspect::OK;
}
// Preproc --- 
CInspect::ERR_INSP CInspectPrivateDCT::PreProc()
{
	// generic preproc also aligns if necessary
	CInspect::ERR_INSP err = CInspectPrivate::PreProc();
	if(err != CInspect::OK)
		return err;

	// Run Time image data for DCT must be a float
	if(m_pData->m_ImgRun.type() != CV_32F)
	{
		float scaling = 1.0F/256.0F;
		cv::Mat temp(m_pData->m_ImgRun.size(),CV_32F);
		unsigned char* pBW;
		float* pF;
		for(int i=0; i<m_pData->m_ImgRun.rows; i++)
		{
			pBW = m_pData->m_ImgRun.ptr(i);
			pF = (float*)temp.ptr(i);
			for(int j=0; j<m_pData->m_ImgRun.cols; j++)
				pF[j] = (float)pBW[j]*scaling;
		}
		m_pData->m_ImgRun = temp;
	}
	return err;
}
CInspect::ERR_INSP CInspectPrivateDCT::train(cv::Mat &image)
{
	// call generic train which calls PreProc() if needed
	CInspect::ERR_INSP err = CInspectPrivate::train(image);
	if(err != CInspect::OK)
		return err;

	err = runScoring(DCTExtract);
	if(err != CInspect::OK)
		return err;

	GlobalLogMsg("**** DCTData ****\n");
	PrintMat(GetGlobalLog(),&m_FeatMat(cv::Rect(0,100,DCTDATASIZE,1)));
	m_ModelKnowledge[100].covar.LogCovar();	// Debug of incremental

	return err;
}
CInspect::ERR_INSP CInspectPrivateDCT::trainPass2(cv::Mat &image)
{
	// call generic train which calls PreProc() if needed
	CInspect::ERR_INSP err = CInspectPrivate::trainPass2(image);
	if(err != CInspect::OK)
		return err;

	err = runScoring(DCTExtract);
	if(err != CInspect::OK)
		return err;

	return err;
}

CInspect::ERR_INSP CInspectPrivateDCT::inspect(cv::Mat &image, int *failIndex)
{
	// call generic inspect (calls PreProc() if needed)
	CInspect::ERR_INSP err = CInspectPrivate::inspect(image,failIndex);
	if(err != CInspect::OK)
		return err;

	err = runScoring(DCTExtract);
	if(err != CInspect::OK)
		return err;

	// for Inspecting,  return FAIL if different
	err = CInspect::OK;
	m_MahalaCount = 0;


	FindNewWorstScore(failIndex);

	return err;
}


//*********** Specific Code for Specific Feature data **********************
static void DCTExtract(TILE &t, cv::Mat &Img,cv::Mat &FeatMat)
{
	t.status = CInspect::ERR_INSP::OK;
	cv::Mat temp = Img(cv::Rect(t.col,t.row,t.tilewidth,t.tileheight));

	cv::Mat DCT;
	cv::dct(temp,DCT);

	float* pDCT = (float*)DCT.ptr();

	float* pFeat = (float*)FeatMat.ptr(t.Index);
	if(DCTDATASIZE == 9)
	{
		// take 9 DCT from top left diagonal corner (Not 0,0)
		memcpy(pFeat,pDCT,4*sizeof(float));
		pFeat += 4; pDCT += 8;
		memcpy(pFeat,pDCT,3*sizeof(float));
		pFeat += 3; pDCT += 8;
		memcpy(pFeat,pDCT,2*sizeof(float));
		//pFeat += 2; pDCT += 8;
		//memcpy(pFeat,pDCT,1*sizeof(float));
	}
	else  //(size == 4)
	{
		// take [0,1];[0,2];[1,0];[2,0] DCT from top left corner
		memcpy(pFeat,pDCT+1,2*sizeof(float));
		pDCT = (float*)DCT.ptr(1);
		pFeat[2] = pDCT[0];
		pDCT = (float*)DCT.ptr(2);
		pFeat[3] = pDCT[0];
	}

	t.datasize = DCTDATASIZE;
}

// showDebugScores() is a convenience debug function to call if you are having issues....
// This code WILL BE OUT OF DATE -- Modify to see what you want to show.
static void	showDebugScores(TILE &T, KNOWLEDGE &K,cv::Mat &FeatMat)
{
	char msg[500];
	sprintf_s(msg,500,"Tile Inspect DCT (%4d, %4d) = \n",T.col,T.row);
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

