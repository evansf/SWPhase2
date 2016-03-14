#include "stdafx.h"
#include "PaletteMedian.h"
#include <array>

//
// CVoxTreeNode is a generic octree node (Data Class)
// each non-leaf volume nodes points to 8 children nodes.
// This bifurcates each non-leaf volume in all 3 dimensions
//
class CVoxTreeNode {
public:
	int     level;                      // Level for this node
	bool    isleaf;                     // TRUE if this is a leaf node
	ColorData minColors;	// r,g,b of the minimum edge of this volume
	ColorData medianColors;	// where each dimension is split for the children
	ColorData maxColors;	// r,g,b of the maximum edge of this volume
	unsigned char    index;                      // Color table index
	long   npixels;                    // Total pixels that have this color
	long   redsum, greensum, bluesum;  // Sum of the color components
	long   redSumSq, greenSumSq, blueSumSq;	// Sum of the colors squared
	ColorData *color;                     // Color at this (leaf) node
	CVoxTreeNode  *child[8];          // Tree pointers
	CVoxTreeNode(int l) : level(l), isleaf(true), npixels(0), redsum(0), greensum(0), bluesum(0), redSumSq(0), greenSumSq(0), blueSumSq(0) {};
	void Split8();
	void ClearHist();
	std::array<unsigned long,256> Hist[3];	// histograms

} ;


CPaletteMedian::CPaletteMedian(int size)
{
	m_PaletteSize = size;
	m_table = new ColorData[size]; 
	m_pVoxTree = new CVoxTreeNode(0);
}

#define HISTSAMPLESIZE 50000

CPaletteMedian::~CPaletteMedian(void)
{
}
int CPaletteMedian::Create(cv::Mat& src)
{
	if(src.channels() != 3) return -1;


	return 0;	// OK
}

void CPaletteMedian::MedianSplit(CVoxTreeNode & node, cv::Mat &img)
{
	m_pVoxTree->ClearHist();
	RandomHist(img, HISTSAMPLESIZE);
	CVoxTreeNode *big = m_pVoxTree;
	big->Split8();			// tree has 8 limbs

	m_pVoxTree->ClearHist();
	RandomHist(img, HISTSAMPLESIZE);
	for(int i=0; i<8; i++)	// then tree has 64 limbs
		m_pVoxTree->child[i]->Split8();

	// final splits to 512 bins
		m_pVoxTree->ClearHist();
	RandomHist(img, 4*HISTSAMPLESIZE);
	for(int i=0; i<8; i++)	// then tree has 512 limbs
		m_pVoxTree->child[i]->Split8();

	// Reduce(256);	// merge close bins to reduce palette to use a UCHAR for the index

}

int CPaletteMedian::whichchild(ColorData thisColor,ColorData medianColors)
{
	int A;
	_asm
	{
	movd MM0,thisColor
	movd MM1,medianColors
	pmaxub	MM0,MM1	;must do unsigned compare
	pcmpeqb MM0,MM1	;by finding max and then back compare
	pmovmskb eax,mm0	;moves highbit of 8 bytes to low byte
	movd A,eax
	emms
	}
	return A;
}
//---------------------------------------------------------------------------
//
// QuantizeColor
//
// Returns the palette table index of an RGB color by traversing the
// octree to the leaf level
//
int CPaletteMedian::QuantizeColor(COctreeNode *tree, ColorData *color)
{
	if (tree->isleaf) {
		return tree->index;
	}
    return QuantizeColor(tree->child[LEVEL(color,COLORBITS-tree->level)],color);
}

//***********************************************************************************************************//
void CVoxTreeNode::Split8(CVoxTreeNode* n)
{
	if( !isleaf )
		return;
	isleaf = false;
	ColorData minC[8];
	ColorData maxC[8];
	for(i=0; i<8; i++)
	{
		child[i] = new CVoxTreeNode(level+1);
		// give all children the parent outer boundary
		child[i]->minColors = minColors;
		child[i]->maxColors = maxColors;
	}
	// children 0,1,2,3 and 4,5,6,7 split the red
	unsigned long M = Median[0];
	medianColors.cRED = (unsigned char)M;
	maxC[0].cRED = minC[1].cRED = minC[2].cRED = minC[3].cRED = M;
	maxC[4].cRED = minC[5].cRED = minC[6].cRED = minC[7].cRED = M+1;
	// children 0,1,4,5 and 2,3,6,7 split the green
	M = Median[1];
	medianColors.cGREEN = (unsigned char)M;
	maxC[0].cGREEN = minC[1].cGREEN = minC[4].cGREEN = minC[5].cGREEN = M;
	maxC[2].cGREEN = minC[3].cGREEN = minC[6].cGREEN = minC[7].cGREEN = M+1;
	// children 0,2,4,6 and 1,3,5,7 split the blue
	M = Median[2];
	medianColors.cRED = (unsigned char)M;
	maxC[4].cBLUE = minC[5].cBLUE = minC[6].cBLUE = minC[7].cBLUE = M+1;

	for(i=0; i<8; i++)
	{
		child[i]->minColors = minC[i];
		child[i]->maxColors = maxC[i];
	}
}

unsigned long CVoxTreeNode::Median(int chan)
{
	unsigned long index = Hist[chan].size()-1;
	int count;
	for(int i=0; i<index; i++)
		count += Hist[i];
	count /=2;	// find the middle
	while( (count -= Hist[index--]) > 0) ;
	return index;	// the largest value that is in the low group
}

void CVoxTreeNode::ClearHist()
{
	if(isleaf)
	{
		for(int i=0; i<3; i++)
			Hist[i].clear();
	}
	else
		for(int i=0; i<8; i++)
			child[i]->ClearHist();
}
