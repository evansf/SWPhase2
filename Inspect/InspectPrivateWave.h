#pragma once
#include "Inspect.h"
#include "InspectPrivate.h"
#include <array>
#include <vector>
#include <numeric>
#include "tbb/tbb.h"

using namespace tbb;

// Isolate the symbol definitions so that different data types can
// continue to use rational names.....
namespace WAVE
{
#define WAVEDATASIZE 10
#include "tilestructs.h"

//********************************************************************************************************

class CInspectPrivateWave :
	public CInspectPrivate
{
public:
	CInspectPrivateWave(SISParams &params);
	~CInspectPrivateWave(void);
	CInspect::ERR_INSP create(cv::Mat &image, cv::Mat &mask, CInspectData **ppData,
							int tilewidth, int tileheight, int stepcols, int steprows);
	CInspect::ERR_INSP PreProc();
	CInspect::ERR_INSP train(cv::Mat &image);
	CInspect::ERR_INSP trainEx(cv::Mat &image);
	CInspect::ERR_INSP optimize();
	CInspect::ERR_INSP inspect(cv::Mat &image);
private:
	CInspect::ERR_INSP runExtract2(void (*ExtractProc)(TILE &,cv::Mat &,cv::Mat &,cv::Mat &));
	CInspect::ERR_INSP runScoring();
	cv::Mat m_Integral;
	cv::Mat m_IntegralRotated;
};

}	// namespace WAVE