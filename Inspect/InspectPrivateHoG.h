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
namespace HOG
{
#define DATASIZE 16

typedef Tile<unsigned char,DATASIZE> TILE;
typedef Knowledge<unsigned char,DATASIZE> KNOWLEDGE;

struct TileEval
{
	std::vector<TILE> &tiles;
	cv::Mat &ImgRef;
	void (*proc)(TILE &,cv::Mat &);
	void operator()( const blocked_range<int>& range ) const
	{
		for( int i=range.begin(); i!=range.end(); ++i )
		{
			(*proc)(tiles[i],ImgRef);
		}
	}
	TileEval(std::vector<TILE> &tvec, cv::Mat &Img ) : tiles(tvec), ImgRef(Img) {};
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

class CInspectPrivateHoG :
	public CInspectPrivate
{
public:
	CInspectPrivateHoG(void);
	~CInspectPrivateHoG(void);
	CInspect::ERR_INSP create(cv::Mat &image, cv::Mat &mask,
							int tilewidth, int tileheight, int stepcols, int steprows);
	CInspect::ERR_INSP train(cv::Mat &image);
	CInspect::ERR_INSP inspect(cv::Mat &image);
private:
	std::vector<KNOWLEDGE> m_ModelKnowledge;
	std::vector<TILE> m_TileVec;
	cv::Mat m_BinTable;

	CInspect::ERR_INSP runScoring();
	inline unsigned char GetBin(short gH, short gV);
};

}	// namespace HOG