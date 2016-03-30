#pragma once
#include "Inspect.h"
#include "CoVariance.h"
#include <array>
#include "opencv2/core/core.hpp"
#include "tbb/tbb.h"
using namespace tbb;
#include "tilestructs.h"
#include "InspectData.h"

// enable code to debug math for tile[100]
//#define DEBUGFILE100 1
//#define DEBUGFILENAME "C:/junk/Debug.csv"

class CInspectPrivate
{
public:
	typedef enum trainmode
	{
		TRAIN,
		TRAINEX,
		TRAINPASS2,
		INSPECT,
		NONE
	}TRAINMODE;
public:
	CInspectPrivate(SISParams &params);
	~CInspectPrivate(void);
	virtual CInspect::ERR_INSP create(cv::Mat &image, cv::Mat &mask,  CInspectData **ppData,
							int tilewidth, int tileheight, int stepcols, int steprows) = 0;
	virtual CInspect::ERR_INSP PreProc();
	virtual CInspect::ERR_INSP train(cv::Mat &image);
	virtual CInspect::ERR_INSP trainPass2(cv::Mat &image);
	        CInspect::ERR_INSP trainEx(int failindex);	// extend training
	        CInspect::ERR_INSP optimize();
	        CInspect::ERR_INSP optimize(int index);
	virtual CInspect::ERR_INSP inspect(cv::Mat &image, int *failIndex);

	void MarkTileSkip(int index)
				{m_TileVec[index].status = CInspect::ERR_INSP::SKIP;m_MahalaCount = 0;};
	CInspect::ERR_INSP FindNewWorstScore(int *pfailIndex=NULL);	// failIndex = -1 if no failures

	double getRawScore(int *pIndex=NULL)
		{if(pIndex!=NULL) *pIndex = m_WorstIndex;
		 return m_WorstRawScore;};
	double getZScore(int *pIndex=NULL)
		{if(pIndex!=NULL) *pIndex = m_WorstIndex;
		 return m_WorstZScore;};
	double getDScore(int *pIndex=NULL)
		{if(pIndex!=NULL) *pIndex = m_WorstIndex;
		 return m_WorstDScore;};
	double getTrueScore(int *pIndex=NULL);
	void   getXY(int index, int *pX, int *pY);
	double getMean(){return m_MahalaMean;};
	double getSigma(){m_MahalaSigma = sqrt(m_MahalaM2/(m_MahalaCount-1));return m_MahalaSigma;};
	cv::Mat & getImageOverlay(){return m_pData->m_Overlay;};
	cv::Mat & getImageAligned(){return m_pData->m_ImgAligned;};

	void setInspectThresh(float t){m_InspectThresh=t;};
	void setRawScoreDiv(double g){m_RawScoreDiv=g;};
	void setMustAlign(bool b){m_MustAlign = b;};
	void setUseZScoreMode(bool b){m_ZScoreMode = b;};

	void clearZStats();	// call before doing TrainPass2
	void DrawRect(int x, int y, int width, int height, unsigned long ARGB);

protected:
	SISParams &m_params;
	// the following are the Parallel run loop code to which runtime functions must be supplied
	CInspect::ERR_INSP runExtract(void (*ExtractProc)(TILE &,cv::Mat &,cv::Mat &)=NULL);
	CInspect::ERR_INSP runScoring(void (*ExtractProc)(TILE &,cv::Mat &,cv::Mat &)=NULL);
	CInspect::ERR_INSP runTrain(void (*TrainProc)(TILE &,KNOWLEDGE &,cv::Mat &)=NULL);
	CInspect::ERR_INSP runInspect(void (*InspectProc)(TILE &,KNOWLEDGE &,cv::Mat &)=NULL);
	CInspectData **m_ppData;	// points to callers pointer to common data
	CInspectData *m_pData;
	cv::Mat m_Model;
	cv::Mat m_ModelBW;
	cv::Mat m_Mask;
	cv::Mat m_MaskColor;
	cv::Mat m_FeatMat;	// mat to hold features for tiles
	std::vector<TILE> m_TileVec;	// vector of tiles
	std::vector<KNOWLEDGE> m_ModelKnowledge;	// vector of knowledge

	double m_RawScoreDiv;
	float m_InspectThresh;
	TRAINMODE m_TrainMode;
	bool m_MustAlign;
	bool m_ZScoreMode;
	
	bool MaskOK(int row, int col, int width, int height);
	CInspect::ERR_INSP align();
	void cvtToBW(cv::Mat &img);
	void annotate(cv::Rect rect, cv::Scalar color = cvScalar(255,64,0,255), bool maskoff = false);
	void annotate(std::vector<vector<cv::Point>> contours,cv::Scalar color = cvScalar(255,64,0,255), bool maskoff = false);
	void Mask(cv::Mat &src, cv::Mat &mask, cv::Mat &dst);
	bool UpdateScore(TILE &T);	// returns true if this tile has become the worst so far
	double MahalanobisScore(double raw, double expDiv = 200.0)
	{
		// need a score of 1.0 for perfect and 0.0 for worst
		double factor = -1.0 / expDiv;
		double score = 	 exp( factor * raw );
		return score;
	}

	double m_WorstScore;
	int m_WorstIndex;
	double m_WorstRawScore;
	double m_WorstDScore;
	double m_WorstZScore;
	double m_MahalaM2;
	double m_MahalaMean;
	double m_MahalaSigma;
	double m_MahalaCount;
	void KeepStats(float v);
	FILE *m_DebugFile;
	void putFloatMat(FILE* pFile, cv::Mat *pMat, char *prepend = "", char *append = "");

	cv::Mat m_VarMat;
	cv::Mat m_CoMat;
	cv::Mat m_DebugMeans;

};

