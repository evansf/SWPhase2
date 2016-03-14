#ifndef TILESTRUCTS_HEADER
#define TILESTRUCTS_HEADER
using namespace tbb;

// Each tile region that is inside mask has one of these structures
// They are kept in std::vector<TILE> m_TileVec
class Tile
{
public:
	int Index;
	int datasize;
	int row;
	int col;
	int tilewidth;
	int tileheight;
	float scoreRaw;
	float ZScore;
	CInspect::ERR_INSP status;
};
typedef Tile TILE;

// Knowledge without samples to use incremental covar method
// Each tile region that is inside mask has one of these structures
// They are kept in std::vector<KNOWLEDGE> m_ModelKnowledge
class KnowledgeM
{
public:
	CCoVariance covar;	// also contains means
	float scoreRawMean;
	float scoreSum, scoreSumSq;
	int scoreCount;
};
typedef KnowledgeM KNOWLEDGE;

// This struct is filled in with data references and (*proc)
// by the Constructor.  It is used by the parallel library
// to do the actual call for each tile in the vector tiles.
// The result is creation of the features vector for each tile
// as created by the extract Proc,  (*proc)
struct TileEval
{
	std::vector<TILE> &tiles;
	cv::Mat &ImgRef;
	cv::Mat &FeatMat;
	void (*proc)(TILE &,cv::Mat &,cv::Mat &);
	void operator()( const blocked_range<int>& range ) const
	{
		for( int i=range.begin(); i!=range.end(); ++i )
		{
			(*proc)(tiles[i],ImgRef,FeatMat);
		}
	}
	TileEval(std::vector<TILE> &tvec, cv::Mat &Img, cv::Mat &Feat ) : tiles(tvec), ImgRef(Img), FeatMat(Feat) {};
};

// This struct is filled in with data references and (*proc)
// by the Constructor.  It is used by the parallel library
// to do the actual call for each tile in the vector tiles.
// The result is creation of a score for the features vector
//  for each tile as specified by by the Scoring Proc,  (*proc)
struct TileScore
{
	std::vector<TILE> &tiles;
	std::vector<KNOWLEDGE> &KDb;
	cv::Mat &FeatMat;
	void (*proc)(TILE &,KNOWLEDGE &,cv::Mat &);
	void operator()( const blocked_range<int>& range ) const
	{
		for( int i=range.begin(); i!=range.end(); ++i )
		{
			if((int)KDb.size()==1)
				(*proc)(tiles[i],KDb[0],FeatMat);
			else
				(*proc)(tiles[i],KDb[i],FeatMat);
		}
	}
	TileScore(std::vector<TILE> &tvec, std::vector<KNOWLEDGE> &K, cv::Mat &Feat ) : tiles(tvec), KDb(K), FeatMat(Feat){};
};


#endif // TILESTRUCTS_HEADER