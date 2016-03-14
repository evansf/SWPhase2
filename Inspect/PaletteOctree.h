#ifndef PALETTEOCTREEHEADER
#define PALETTEOCTREEHEADER

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "opencv2/core/core.hpp"
#include "Palette.h"

#define COLORBITS	8
#define TREEDEPTH	6

class COctreeNode;

class CPaletteOctree : public CPalette
{
public:
	CPaletteOctree();
	~CPaletteOctree(void);
	int Create(cv::Mat& src,int size);	// returns 0 on success
	int Palettize(cv::Mat& src, cv::Mat& dst);	// returns 0 on success
	void MakePaletteTable();
	void SetSorted(bool b){m_Sorted = b;};

protected:
	unsigned char QuantizeColor(BGRType color);
	unsigned char QuantizeColor(COctreeNode *tree, BGRType *color);
	COctreeNode *m_pOctree;
	void InsertTree(COctreeNode **tree, ColorData *color, int depth);
	void MakePaletteTable(COctreeNode *tree, int *index, vector<unsigned long> &countsort,vector<void*> &pTree );
	bool m_Sorted;

private:
	int globalLeafLevel;
	int globalTotalLeaves;
	COctreeNode *reducelist[TREEDEPTH];
	void MakeReducible(int level, COctreeNode *node);
	COctreeNode *CreateOctNode(int level);
	void ReduceTree();
	COctreeNode *GetReducible(void);
	void Clear(COctreeNode *node);

};

#endif // PALETTEOCTREEHEADER