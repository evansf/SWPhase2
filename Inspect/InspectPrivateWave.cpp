#include "stdafx.h"
#include "InspectPrivatewave.h"
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/video/video.hpp"

using namespace WAVE;
#include "CoVariance.h"

// *********** forward declarations of static (this file only) functions *********************
// Proceedures for the (*proc)() of the parallel_for loops
// provide Score function
static void WaveExtract(TILE &,cv::Mat &,cv::Mat &,cv::Mat &feat);

static void showDebugScores(TILE &T, KNOWLEDGE &K);

// special tileEval struct
// we need two input images for the Integral and the RotatedIntegral Mats
struct TileEvalWave
{
	std::vector<TILE> &tiles;
	cv::Mat &ImgRef1;
	cv::Mat &ImgRef2;
	cv::Mat &FeatMat;
	void (*proc)(TILE &,cv::Mat &,cv::Mat &,cv::Mat &);
	void operator()( const blocked_range<int>& range ) const
	{
		for( int i=range.begin(); i!=range.end(); ++i )
		{
			(*proc)(tiles[i],ImgRef1,ImgRef2,FeatMat);
		}
	}
	TileEvalWave(std::vector<TILE> &tvec, cv::Mat &Img1, cv::Mat &Img2, cv::Mat &Feat ) : tiles(tvec), ImgRef1(Img1), ImgRef2(Img2), FeatMat(Feat) {};
};

// *********** Methods for the specific feature implementation *******************************
CInspectPrivateWave::CInspectPrivateWave(SISParams &params) :
	CInspectPrivate(params)
{
}

CInspectPrivateWave::~CInspectPrivateWave(void)
{
}

CInspect::ERR_INSP CInspectPrivateWave::create(cv::Mat &image, cv::Mat &mask, CInspectData **ppData,
										   int tilewidth, int tileheight, int stepcols, int steprows)
{
	// tile shapes of 4x4 4x8 8x4 8x8 are OK
	if( ! ((tilewidth == 4)||(tilewidth == 8)) )
		return CInspect::TILESHAPENOTSUPPORTED;
	if( ! ((tileheight == 4)||(tileheight == 8)) )
		return CInspect::TILESHAPENOTSUPPORTED;
	if(stepcols == 0) stepcols = tilewidth;
	if(steprows == 0) steprows = tileheight;
	// call the generic method first
	CInspect::ERR_INSP err = CInspectPrivate::create(image, mask, ppData,
													tilewidth, tileheight, stepcols, steprows);
	if(err != CInspect::OK)
		return err;

		// make a matrix for the feature vectors
	// rows = number of tiles ;  cols = number of features
	m_FeatMat.create((int)m_TileVec.size(), WAVEDATASIZE, CV_32F);
	m_Integral.create(mask.size(),CV_64F);
	m_IntegralRotated.create(mask.size(),CV_64F);
	return CInspect::OK;
}
// Preproc --- 
CInspect::ERR_INSP CInspectPrivateWave::PreProc()
{
	CInspect::ERR_INSP err = CInspectPrivate::PreProc();
	if(err != CInspect::OK)
		return err;

	// Run Time image data for DCT must be a float
	float scaleing = 1.0F/256.0F;
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
				pF[j] = (float)pBW[j]*scaleing;
		}
		m_pData->m_ImgRun = temp;
	}
	cv::Mat sumSq;
	cv::integral(m_pData->m_ImgRun,m_Integral,sumSq,m_IntegralRotated);
	return err;
}
CInspect::ERR_INSP CInspectPrivateWave::train(cv::Mat &image)
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
CInspect::ERR_INSP CInspectPrivateWave::trainEx(cv::Mat &image)
{
	// call generic inspect which calls PreProc() if needed
	CInspect::ERR_INSP err = CInspectPrivate::inspect(image);
	if(err != CInspect::OK)
		return err;
	err = CInspectPrivate::trainEx(image);

	return err;
}

CInspect::ERR_INSP CInspectPrivateWave::optimize()
{
	CInspect::ERR_INSP err = CInspectPrivate::optimize();

	return err;
}
CInspect::ERR_INSP CInspectPrivateWave::inspect(cv::Mat &image)
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

CInspect::ERR_INSP CInspectPrivateWave::runScoring()
{
	CInspect::ERR_INSP err = CInspect::OK;
	err = runExtract2(WaveExtract);

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
CInspect::ERR_INSP CInspectPrivateWave::runExtract2(void (*ExtractProc)(TILE &,cv::Mat &,cv::Mat &,cv::Mat &))
{
	CInspect::ERR_INSP err = CInspect::OK;

	// create the struct used in the parallel_for
	TileEvalWave tilev(m_TileVec,m_Integral,m_IntegralRotated, m_FeatMat);
	tilev.proc = ExtractProc;	// use this proceedure in each parallel_for
	try
	{
		parallel_for(  blocked_range<int>( 0, (int)m_TileVec.size() ) , tilev );
	}
	catch(int e)
	{
		int wait=e;	// errors are caught here (if any)
		err = CInspect::PARALLEL_FOR_EXTRACT;
	}
	return err;
}
//*********** Specific Code for Specific Feature data **********************
// macros to define the sub-rectangles
#define TOTAL  (*pBR - *pTR + *pTL - *pBL)
#define RIGHT  (*pBR - *pTR + *pTC - *pBC)
#define BOTTOM (*pBR - *pTC + *pTL - *pBL)
#define COLUMB (*pBMR - *pTMR + *pTML - *pBML)
#define MIDDLE (*pBRM - *pTRM + *pTLM - *pBLM)
#define CENTER (*(pBRM-Sx) - *(pTRM-Sx) + *(pTLM+Sx) - *(pBLM+Sx))
static void WaveExtract(TILE &t, cv::Mat &Integral, cv::Mat &IntegtralRotated, cv::Mat &FeatMat)
{
	// m_ImgRun already transformed by wavelet xfrm
	float* pFeat = (float*)FeatMat.ptr(t.Index);
	cv::Mat temp;
	// row pointers for integral rectangles
	int Sx = t.tilewidth/4;
	int Sy = Sx;
	// pointers for integral rectangles
	double *pTL,*pTML,*pTC,*pTMR,*pTR;
	double      *pTLM,     *pTRM;
	double *pLC;
	double      *pBLM,     *pBRM;
	double *pBL,*pBML,*pBC,*pBMR,*pBR;

	double right,bottom,columb,middle,center;
	int w = t.tilewidth;
	int h = t.tileheight;
	int row = t.row;
	int col = t.col;
	double TotalBox;
	if(WAVEDATASIZE == 10)
	{
		// take 10 Haar like features from integral images
		pTL  = ((double*)Integral.ptr(row))+col;      pTML = pTL + Sx; pTC = pTL + Sx*2; pTMR = pTL + Sx*3; pTR  = pTL  + w;
		pTLM = ((double*)Integral.ptr(row+Sy))+col;                                                         pTRM = pTLM + w;
		pLC  = ((double*)Integral.ptr(row+Sy*2))+col;
		pBLM = ((double*)Integral.ptr(row+Sy*3))+col;  								                        pBRM = pBLM + w;
		pBL  = ((double*)Integral.ptr(row+h))+col;    pBML = pTL + Sx; pBC = pBL + Sx*2; pBMR = pBL + Sx*3; pBR  = pBL  + w;

		TotalBox = TOTAL;
		right  = RIGHT;
		bottom = BOTTOM;
		columb = COLUMB;
		middle = MIDDLE;
		center = CENTER;
		// left-right
		*pFeat++ = (float)(TotalBox - 2 * RIGHT); 
		// top-bottom
		*pFeat++ = (float)(TotalBox - 2 * BOTTOM); 
		// left+right-middle
		*pFeat++ = (float)(TotalBox - 2 * COLUMB); 
		// top+bottom-middle
		*pFeat++ = (float)(TotalBox - 2 * MIDDLE); 
		// total-center
		*pFeat++ = (float)(TotalBox - 4 * CENTER); 

		// take 10 Haar like features from IntegralRotated images
		pTL  = ((double*)IntegtralRotated.ptr(row))+col;      pTML = pTL + Sx; pTC = pTL + Sx*2; pTMR = pTL + Sx*3; pTR  = pTL  + w;
		pTLM = ((double*)IntegtralRotated.ptr(row+Sy))+col;                                                         pTRM = pTLM + w;
		pLC  = ((double*)IntegtralRotated.ptr(row+Sy*2))+col;
		pBLM = ((double*)IntegtralRotated.ptr(row+Sy*3))+col;  								                        pBRM = pBLM + w;
		pBL  = ((double*)IntegtralRotated.ptr(row+h))+col;    pBML = pTL + Sx; pBC = pBL + Sx*2; pBMR = pBL + Sx*3; pBR  = pBL  + w;

		TotalBox = TOTAL;
		// left-right
		*pFeat++ = (float)(TotalBox - 2 * RIGHT); 
		// top-bottom
		*pFeat++ = (float)(TotalBox - 2 * BOTTOM); 
		// left+right-middle
		*pFeat++ = (float)(TotalBox - 2 * COLUMB); 
		// top+bottom-middle
		*pFeat++ = (float)(TotalBox - 2 * MIDDLE); 
		// total-center
		*pFeat++ = (float)(TotalBox - 4 * CENTER); 

	}
		if( (t.Index == 100) )
		{
			cv::Mat temp = FeatMat(cv::Rect(0,100,FeatMat.cols,1));
			//putFloatMat(m_DebugFile, &temp);
		}
	t.datasize = WAVEDATASIZE;
}

// showDebugScores() is a convenience debug function to call if you are having issues....
// This codew WILL BE OUT OF DATE -- Modify to show what you need to view.
static void	showDebugScores(TILE &T, KNOWLEDGE &K,cv::Mat &FeatMat)
{
	char msg[500];
	sprintf_s(msg,500,"Tile Inspect Wavelet (%4d, %4d) = \n",T.col,T.row);
	OutputDebugStringA(msg);
	float *pF = (float*)FeatMat.ptr(T.Index);

	char temp[100];
	cv::Mat CvInv = K.covar.InverseMatrix();
	if(CvInv.empty())
		OutputDebugStringA("Knowledge Empty\n");
	else
	{
		sprintf_s(msg,500,"Tile Knowledge  \n");
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

