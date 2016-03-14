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
namespace COLORHIST
{
#define DATASIZE 16

typedef Tile<unsigned char,DATASIZE> TILE;
typedef Knowledge<unsigned char,DATASIZE> KNOWLEDGE;

struct TileEval
{
	std::vector<TILE> &tiles;
	cv::Mat &ImgRef;
	int binshift;
	void (*proc)(TILE &,cv::Mat &,int);
	void operator()( const blocked_range<int>& range ) const
	{
		for( int i=range.begin(); i!=range.end(); ++i )
		{
			(*proc)(tiles[i],ImgRef,binshift);
		}
	}
	TileEval(std::vector<TILE> &tvec, cv::Mat &Img ,int shift ) : tiles(tvec), ImgRef(Img), binshift(shift) {};
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

class CInspectPrivateColorHist :
	public CInspectPrivate
{
public:
	CInspectPrivateColorHist(void);
	~CInspectPrivateColorHist(void);
	CInspect::ERR_INSP create(cv::Mat &image, cv::Mat &mask,
							int tilewidth, int tileheight, int stepcols, int steprows);
	CInspect::ERR_INSP train(cv::Mat &image);
	CInspect::ERR_INSP inspect(cv::Mat &image);
private:
	std::vector<KNOWLEDGE> m_ModelKnowledge;
	std::vector<TILE> m_TileVec;
	CPalette *m_pPalette;
	int m_BinShift;	// bin# = colorindex << m_BinShift

	CInspect::ERR_INSP runScoring();
};

}	// namespace COLORHIST