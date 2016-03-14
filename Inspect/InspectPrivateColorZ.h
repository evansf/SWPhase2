#pragma once
#include "Inspect.h"
#include "InspectPrivate.h"
#include <array>
#include <vector>
#include <numeric>
#include "tbb/tbb.h"

class CPalette;

using namespace tbb;

// Isolate the symbol definitions so that different data types can
// continue to use rational names.....
namespace COLORZ
{
#define DATASIZE 1

struct TileEval
{
	std::vector<TILE> &tiles;
	cv::Mat &ImgRef;	// contains Palette ZScores
	void (*proc)(TILE &,cv::Mat &);
	void operator()( const blocked_range<int>& range ) const
	{
		for( int i=range.begin(); i!=range.end(); ++i )
		{
			(*proc)(tiles[i],ImgRef);
		}
	}
	TileEval(std::vector<TILE> &tvec, cv::Mat &Img) : tiles(tvec), ImgRef(Img) {};
};


struct TileScore
{
	std::vector<TILE> &tiles;
	std::vector<KNOWLEDGE> &KDb;
	void (*proc)(TILE &,KNOWLEDGE &);
	void operator()( const blocked_range<int>& range ) const
	{
		for( int i=range.begin(); i!=range.end(); ++i )
		{
			(*proc)(tiles[i],KDb[i]);
		}
	}
	TileScore(std::vector<TILE> &tvec, std::vector<KNOWLEDGE> &K ) : tiles(tvec), KDb(K){};
};


//********************************************************************************************************

class CInspectPrivateColorZ :
	public CInspectPrivate
{
public:
	CInspectPrivateColorZ(SISParams &params);
	~CInspectPrivateColorZ(void);
	CInspect::ERR_INSP create(cv::Mat &image, cv::Mat &mask, CInspectData **ppData,
							CInspect::ALIGNMODE mode, 
							int tilewidth, int tileheight, int stepcols, int steprows);
	CInspect::ERR_INSP train(cv::Mat &image);
	CInspect::ERR_INSP optimize();
	CInspect::ERR_INSP inspect(cv::Mat &image);
protected:
	CInspect::ERR_INSP PreProc(cv::Mat src, cv::Mat dst);

private:
	std::vector<KNOWLEDGE> m_PaletteKnowledge;
	cv::Mat m_Palettized;
	CPalette *m_pPalette;
	cv::Mat m_ScoreImg;
	CInspect::ERR_INSP runScoring(cv::Mat &image);
	void filterContours(std::vector<vector<cv::Point>> &contours);
	void Learn(cv::Mat &m_Palettized, cv::Mat &image);
	void Score(cv::Mat &m_Palettized, cv::Mat &image, cv::Mat &m_ScoreImg);
};

}	// namespace COLORZ