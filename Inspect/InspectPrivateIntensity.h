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
namespace INTENSITY
{
#define INTENSITYDATASIZE 9

#include "tilestructs.h"

//********************************************************************************************************

class CInspectPrivateIntensity :
	public CInspectPrivate
{
public:
	CInspectPrivateIntensity(SISParams &params);
	~CInspectPrivateIntensity(void);
	CInspect::ERR_INSP create(cv::Mat &image, cv::Mat &mask, CInspectData **ppData,
							int tilewidth, int tileheight, int stepcols, int steprows);
	CInspect::ERR_INSP PreProc();
	CInspect::ERR_INSP train(cv::Mat &image);
	CInspect::ERR_INSP trainPass2(cv::Mat &image);
	CInspect::ERR_INSP optimize();
	CInspect::ERR_INSP inspect(cv::Mat &image, int *failIndex);
private:

};

}	// namespace INTENSITY