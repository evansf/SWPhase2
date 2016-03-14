//#include <algorithm>
#include "PaletteOctree.h"
#include <vector>

// useful macros
static unsigned char MASK[COLORBITS] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
#define BIT(b,n)    (((b)&MASK[n])>>n)
#define LEVEL(c,d)  ((BIT((c)->r,(d)))<<2 | BIT((c)->g,(d))<<1 | BIT((c)->b,(d)))

//
// OctnodeType is a generic octree node (Data Class)
//
class COctreeNode {
public:
	int     level;                      // Level for this node
	bool    isleaf;                     // TRUE if this is a leaf node
	unsigned char    index;                      // Color table index
	long   npixels;                    // Total pixels that have this color
	long   redsum, greensum, bluesum;  // Sum of the color components
	long   redSumSq, greenSumSq, blueSumSq;	// Sum of the colors squared
	ColorData *color;                     // Color at this (leaf) node
	COctreeNode  *child[8];          // Tree pointers
	COctreeNode  *nextnode;          // Reducible list pointer
} ;


CPaletteOctree::CPaletteOctree()  : CPalette()
{
	m_pOctree = CreateOctNode(0);
	globalLeafLevel = TREEDEPTH;
	globalTotalLeaves = 0;
	for(int i=0; i<TREEDEPTH; i++)
		reducelist[i] = (COctreeNode *)NULL;
	m_Sorted = false;
}

CPaletteOctree::~CPaletteOctree(void)
{
	Clear(m_pOctree);
	delete m_pOctree;
}
int CPaletteOctree::Create(cv::Mat& src,int size)
{
	if(src.channels() != 3) return -1;
	// Call Parent
	CPalette::Create(src,size);
	int step = src.channels();
	int width, height;
	width = src.cols; height = src.rows;
	ColorData C;
	unsigned char *pSrc;
	int i,j;
	for(i=0; i<height; i++)
	{
		pSrc = src.ptr(i);
		for(j=0; j<width; j++)
		{
			C.color.b = pSrc[step*j]; C.color.g = pSrc[step*j+1]; C.color.r = pSrc[step*j+2];
			InsertTree(&m_pOctree,&C,0);
			if(globalTotalLeaves > m_PaletteSize) ReduceTree();
		}
	}
	return 0;	// OK
}
int CPaletteOctree::Palettize(cv::Mat& src, cv::Mat& dst)
{
	if((dst.rows != src.rows) || (dst.cols != src.cols) || (dst.type() != CV_8UC1) )
	{	// re-allocate destination
		dst.create(src.rows,src.cols,CV_8UC1);
	}
	int height, width;
	height = src.rows; width = src.cols;
	int step = src.channels();
	ColorData C;
	unsigned char *pSrc, *pDst;
	int i,j;
	for(i=0; i<height; i++)
	{
		pSrc = (unsigned char*)src.ptr(i);
		pDst = (unsigned char*)dst.ptr(i);
		for(j=0; j<width; j++)
		{
			C.color.b = pSrc[step*j]; C.color.g = pSrc[step*j+1]; C.color.r = pSrc[step*j+2];
			pDst[j] = QuantizeColor(C.color);
		}
	}
	return 0;	// OK
}

void CPaletteOctree::Clear(COctreeNode *node)
{
	int i;
	for(i=0; i<8; i++)
		if(node->child[i] != (COctreeNode *)NULL)
			Clear(node->child[i]);
}

//--------------------------------------------------------------------------
//
// CreateOctNode
//
// Allocates and initializes a new octree node.  The level of the node is
// determined by the caller.
//
// Arguments:
//  level   int     Tree level where the node will be inserted.
//
// Returns:
//  Pointer to newly allocated node.  Does not return on failure.
//
COctreeNode *CPaletteOctree::CreateOctNode(int level)
{
	COctreeNode  *newnode = new COctreeNode;
    int                 i;

    newnode->level = level;
	newnode->isleaf = (level == globalLeafLevel);
    newnode->npixels = 0;
	newnode->index = 0;
	newnode->redsum = newnode->greensum = newnode->bluesum = 0L;
	newnode->redSumSq = newnode->greenSumSq = newnode->blueSumSq = 0L;

    for (i = 0; i < 8; i++) {
        newnode->child[i] = (COctreeNode *)NULL;
	}
    if (newnode->isleaf) { 
        globalTotalLeaves++;
    }
    else {
        MakeReducible(level, newnode);
    }
	return newnode;
}

//-----------------------------------------------------------------------
//
// MakeReducible
//
// Adds a node to the reducible list for the specified level
//
void CPaletteOctree::MakeReducible(int level, COctreeNode *node)
{
    node->nextnode = reducelist[level];
    reducelist[level] = node;
}


//----------------------------------------------------------------------------
//
// InsertTree
//
// Insert a color into the octree
//
void CPaletteOctree::InsertTree(COctreeNode **tree, ColorData *pC, int depth)
{

    if (*tree == (COctreeNode *)NULL) {
        *tree = CreateOctNode(depth);
	}
    if ((*tree)->isleaf) {
        (*tree)->npixels++;
        (*tree)->redsum += pC->color.r;
        (*tree)->greensum += pC->color.g;
        (*tree)->bluesum += pC->color.b;
        (*tree)->redSumSq += (pC->color.r)*(pC->color.r);
        (*tree)->greenSumSq += (pC->color.g)*(pC->color.g);
        (*tree)->blueSumSq += (pC->color.b)*(pC->color.b);
	}
	else {
		InsertTree(&((*tree)->child[LEVEL(&pC->color, COLORBITS-depth)]),
				   pC,
				   depth+1);
	}
}

//--------------------------------------------------------------------------
//
// ReduceTree
//
// Combines all the children of a node into the parent, makes the parent
// into a leaf
//
void CPaletteOctree::ReduceTree()
{
	COctreeNode  *node;
	unsigned char    i, nchild=0;

	node = GetReducible();
    for (i = 0; i < 8; i++) {
		if (node->child[i] != (COctreeNode *)NULL)
		{
			nchild++;
			node->redsum += node->child[i]->redsum;
			node->greensum += node->child[i]->greensum;
			node->bluesum += node->child[i]->bluesum;
			node->redSumSq += node->child[i]->redSumSq;
			node->greenSumSq += node->child[i]->greenSumSq;
			node->blueSumSq += node->child[i]->blueSumSq;
			node->npixels += node->child[i]->npixels;
			delete node->child[i];
			node->child[i] = (COctreeNode *)NULL;
		}
	}
	node->isleaf = true;
    globalTotalLeaves -= (nchild - 1);
}

//-----------------------------------------------------------------------
//
// GetReducible
//
// Returns the next available reducible node at the tree's leaf level
//
COctreeNode *CPaletteOctree::GetReducible(void)
{
    COctreeNode *node;
    while (reducelist[globalLeafLevel-1] == NULL) {
        globalLeafLevel--;
    }
    node = reducelist[globalLeafLevel-1];
    reducelist[globalLeafLevel-1] = reducelist[globalLeafLevel-1]->nextnode;
	return node;
}

//---------------------------------------------------------------------------
//
// MakePalettteTable
//
// Given a color octree, traverse the tree and do the following:
//	- Add the averaged RGB leaf color to the color palette table;
//	- Store the palette table index in the tree;
//
// When this recursive function finally returns, 'index' will contain
// the total number of colors in the palette table.
//
// If m_Sorted is true, Table will be sorted with most popular colors at low indexes
//
static float sigma(long s, long s2, int N)
{
	float sigma = sqrt((float)N * s2 - s*s)/N;
	return sigma;
}
void CPaletteOctree::MakePaletteTable()
{
	int integerindex = 0;
	int *index = &integerindex;
	std::vector<unsigned long> countsort(256,0);
	std::vector<void*> treepointers(256,NULL);
	MakePaletteTable(m_pOctree, index, countsort, treepointers );
	if(m_Sorted)
	{
		// now sort the table to put common colors at low indexes
		sort(countsort.begin(), countsort.end());
		int oldindex;
		COctreeNode *pTree;
		int i;
		int count = countsort.size();
		for(i = 0; i<count; i++)
		{
			oldindex = countsort[i] & 0x000000ff;
			pTree = (COctreeNode*)treepointers[oldindex];
			pTree->index = i;
			// output the mean
			m_table[i].color.r = (unsigned char)(pTree->redsum / pTree->npixels);
			m_table[i].color.g = (unsigned char)(pTree->greensum / pTree->npixels);
			m_table[i].color.b = (unsigned char)(pTree->bluesum / pTree->npixels);
			m_palette[i] = m_table[i].color;
		}
	}
}
void CPaletteOctree::MakePaletteTable(COctreeNode *tree, int *index, vector<unsigned long> &countsort,vector<void*> &pTree )
{
	int	i;
	if (tree->isleaf)
	{
		// output the mean
		m_table[*index].color.r = (unsigned char)(tree->redsum / tree->npixels);
        m_table[*index].color.g = (unsigned char)(tree->greensum / tree->npixels);
        m_table[*index].color.b = (unsigned char)(tree->bluesum / tree->npixels);
		m_palette[*index] = m_table[*index].color;
		countsort[*index] = ((unsigned long)tree->npixels<<8) + *index;	// kept for sort
		pTree[*index] = (void*)tree;	// kept for sort
		tree->index = *index;
		(*index)++;
	}
	else {
        for (i = 0; i < 8; i++) {
			if (tree->child[i]) {
				MakePaletteTable(tree->child[i], index, countsort, pTree);
			}
		}
	}
}

//---------------------------------------------------------------------------
//
// QuantizeColor
//
// Returns the palette table index of the nearwst BGR color
// Call this if the image is not the one that was used to generate the tree.
unsigned char CPaletteOctree::QuantizeColor(BGRType color)
{
	int bestIdx = 0;
	int bestdist = INT_MAX;
	int dist;
	for(int i=0; i<m_PaletteSize; i++)
	{
		dist = (color.b - m_palette[i].b)*(color.b - m_palette[i].b)
			+  (color.g - m_palette[i].g)*(color.g - m_palette[i].g)
			+  (color.r - m_palette[i].r)*(color.r - m_palette[i].r);
		if(dist < bestdist)
		{
			bestdist = dist;
			bestIdx = i;
		}
	}

	return (unsigned char)bestIdx;
}
// Returns the palette table index of an BGR color by traversing the
// octree to the leaf level.  Call this only if the image was used to create the tree
unsigned char CPaletteOctree::QuantizeColor(COctreeNode *tree, BGRType *color)
{
	if (tree->isleaf) {
		return tree->index;
	}
    return QuantizeColor(tree->child[LEVEL(color,COLORBITS-tree->level)],color);
}

