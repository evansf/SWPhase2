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
#define DEBUGFILE100 1
#define DEBUGFILENAME "C:/junk/Debug.csv"

class CInspectPrivate
{
public:
	typedef enum trainmode
	{
		TRAIN,
		INSPECT
	}TRAINMODE;
public:
	CInspectPrivate(SISParams &params);
	~CInspectPrivate(void);
	virtual CInspect::ERR_INSP create(cv::Mat &image, cv::Mat &mask,  CInspectData **ppData,
							int tilewidth, int tileheight, int stepcols, int steprows) = 0;
	virtual CInspect::ERR_INSP PreProc();
	virtual CInspect::ERR_INSP train(cv::Mat &image);
	virtual CInspect::ERR_INSP trainEx(cv::Mat &image);	// extend training
	virtual CInspect::ERR_INSP optimize();
	virtual CInspect::ERR_INSP inspect(cv::Mat &image);
	double getRawScore(int *pX=NULL, int *pY=NULL)
		{if(pX!=NULL) *pX = m_MaxX;
		 if(pY!=NULL) *pY = m_MaxY;
		 return m_MaxScore;};
	double getFinalScore(){return m_FinalScore;};
	double getMean(){return m_MahalaMean;};
	double getSigma(){m_MahalaSigma = sqrt(m_MahalaM2/(m_MahalaCount-1));return m_MahalaSigma;};
	void DrawRect(int x, int y, int width, int height, unsigned long ARGB);
	cv::Mat & getImageOverlay(){return m_pData->m_Overlay;};
	cv::Mat & getImageAligned(){return m_pData->m_ImgAligned;};
//	std::vector<std::array<float,2>> getHistogramRaw();
//	std::vector<std::array<float,2>> getHistogramScores();
	void setLearnThresh(float t){m_LearnThresh=t;};
	void setInspectThresh(float t){m_InspectThresh=t;};
	void setRawScoreDiv(float g){m_RawScoreDiv=g;};
	void setMustAlign(bool b){m_MustAlign = b;};

protected:
	SISParams &m_params;
	CInspect::ERR_INSP runExtract(void (*ExtractProc)(TILE &,cv::Mat &,cv::Mat &)=NULL);
	CInspect::ERR_INSP runTrain(void (*TrainProc)(TILE &,KNOWLEDGE &,cv::Mat &)=NULL);
	CInspect::ERR_INSP runInspect(void (*InspectProc)(TILE &,KNOWLEDGE &,cv::Mat &)=NULL);
	CInspectData **m_ppData;	// points to callers pointer to common data
	CInspectData *m_pData;
	cv::Mat m_Model;
	cv::Mat m_ModelBW;
	cv::Mat m_Mask;
	cv::Mat m_FeatMat;	// mat to hold features for tiles
	std::vector<TILE> m_TileVec;	// vector of tiles
	std::vector<KNOWLEDGE> m_ModelKnowledge;

	static float m_RawScoreDiv;
	float m_LearnThresh;
	float m_InspectThresh;
	TRAINMODE m_TrainMode;
	bool m_MustAlign;
	
	bool MaskOK(int row, int col, int width, int height);
	CInspect::ERR_INSP align();
	void cvtToBW(cv::Mat &img);
	void annotate(cv::Rect rect, cv::Scalar color = cvScalar(0,64,255,192), bool maskoff = false);
	void annotate(std::vector<vector<cv::Point>> contours,cv::Scalar color = cvScalar(0,64,255,192), bool maskoff = false);
	void Mask(cv::Mat &src, cv::Mat &mask, cv::Mat &dst);
	void UpdateRawScore(TILE T);

	double m_FinalScore;
	double m_MaxScore;
	int m_MaxIndex;
	int m_MaxX,m_MaxY;
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

