#pragma once


#include "opencv2/core/core.hpp"

#define COLORBITS	8
#define TREEDEPTH	6
using namespace cv;

//
// RGBType is a simple 8-bit color triple
//
typedef struct {
	unsigned char    r,g,b;                      // The color
} RGBType;

class ColorData
{
public:
	union colorunion
	{ 
		unsigned long RGB;
		unsigned char Chan[4];
	} color;
	float sigmaR, sigmaG, sigmaB;
};
// defines alowing reference to variable of type ColorData
// as variablename.cRED or variablename.cBLUE or variablename.cRGB
#define cRED color.Chan[0]
#define cGREEN color.Chan[1]
#define cBLUE color.Chan[2]
#define cRGB color.RGB

class CVoxTreeNode;

class CPaletteMedian
{
public:
	CPaletteMedian(int size);
	~CPaletteMedian(void);
	int Create(cv::Mat& src);	// returns 0 on success
	int Palettize(cv::Mat& src, cv::Mat& dst);	// returns 0 on success
	int QuantizeColor(CVoxTreeNode *tree, ColorData *color);
	void MakePaletteTable();
	void DePalettize(cv::Mat& src8U, cv::Mat& dstBGR);
	int ZScore(cv::Mat& PalImg, cv::Mat& Img, cv::Mat& ZImg);	// get zscore for color relative to its palette brothers
	int SavePalettizedAsPCX( cv::Mat& image, char* filename);	// matrix is Palettized pixels
	ColorData *m_table;

protected:
	CVoxTreeNode *m_pVoxTree;
	CVoxTreeNode *m_thisNode;
	int m_PaletteSize;
	int m_numNodes;
	void RandomHist(cv::Mat &img, int numsamples);
	void MedianSplit(CVoxTreeNode & node,cv::Mat &img);
	void InsertTree(CVoxTreeNode **tree, ColorData *color, int depth);
	void MakePaletteTable(CVoxTreeNode *tree, int *index, vector<unsigned long> &countsort,vector<void*> &pTree );
	void reduce(int targetsize);
	bool m_Sorted;

private:
	int globalTotalLeaves;
	CVoxTreeNode *CreateVoxNode(int level);
	void Clear(CVoxTreeNode *node);
	int	whichchild(ColorData thisColor,ColorData medianColors);
};

