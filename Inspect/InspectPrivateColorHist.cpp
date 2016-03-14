#include "stdafx.h"
#include "InspectPrivateColorHist.h"
#include "Palette.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/video/video.hpp"


using namespace COLORHIST;

// *********** forward declarations of static (this file only) functions *********************
// Proceedures for the (*proc)() of the parallel_for loops
static void ColorHistExtract(TILE &,cv::Mat &,int);
static void FloatScore(TILE &,KNOWLEDGE &);
// Proceedure to do comparison of vectors using Normalized Correlation having score range of [-1.0...0.0....1.0]
static float NCRScore(std::array<float,DATASIZE> &I, std::array<float,DATASIZE> &M, int N, float m, float i, float K1, float K2);
static void showDebugScores(TILE &T, KNOWLEDGE &K);

// *********** Methods for the specific feature implementation *******************************
#define _PI (3.141592654)
CInspectPrivateColorHist::CInspectPrivateColorHist(void)
{
	m_pPalette = new CPalette(256);
	m_BinShift = 0;
	for(int i=256/DATASIZE; i>1; i/=2)
		m_BinShift++;
	return;
}

CInspectPrivateColorHist::~CInspectPrivateColorHist(void)
{
}

CInspect::ERR_INSP CInspectPrivateColorHist::create(cv::Mat &image, cv::Mat &mask,
										   int tilewidth, int tileheight, int stepcols, int steprows)
{
	if(stepcols == 0) stepcols = tilewidth;
	if(steprows == 0) steprows = tileheight;
	// call the generic method first
	CInspect::ERR_INSP err = CInspectPrivate::create(image, mask, tilewidth, tileheight, stepcols, steprows);
	if(err != CInspect::OK)
		return err;

	cv::Mat temp;
	switch(m_Model.channels())
	{
	case 1:
		cvtColor(m_Model,temp,CV_GRAY2BGR);
		break;
	case 3:
		temp = m_Model;
		break;
	case 4:
		cvtColor(m_Model,temp,CV_RGBA2BGR);
		break;
	default:
		return CInspect::UNSUPPORTEDIMAGETYPE;
		break;
	}

	if(m_pPalette->Create(temp) != 0)
		return CInspect::UNABLETOPALETTIZE;
//	m_pPalette->SetSorted(true);
	m_pPalette->SetSorted(false);
	m_pPalette->MakePaletteTable();

	// tile the image applying the mask
	// make a vector of TILE objects and a vector of KNOWLEDGE objects
	int numrows = (temp.rows - tileheight)/steprows;
	int numcols = (temp.cols - tilewidth)/stepcols;
	m_TileVec.clear();
	TILE tile;
	tile.tileheight = tileheight;
	tile.tilewidth = tilewidth;
	m_ModelKnowledge.clear();
	KNOWLEDGE k;
	for(int row = 0; row <numrows; row++)
	{
		for(int col = 0; col <numcols; col++)
		{
			if(MaskOK(row*steprows,col*stepcols,tilewidth,tileheight))
			{
				tile.row = row*steprows; tile.col = col*stepcols;
				m_TileVec.push_back(tile);
				m_ModelKnowledge.push_back(k);
			}
		}
	}
	if(m_TileVec.size() == 0)
		return CInspect::NOTILESUNDEREMASK;

	return CInspect::OK;
}
CInspect::ERR_INSP CInspectPrivateColorHist::train(cv::Mat &image)
{
	// get the cv::Mat from the input data
	m_ImgIn = image;
	
	CInspect::ERR_INSP err;
	err = runScoring();
	if(err != CInspect::OK)
		return err;

	// for Training,  add vector to Knowledge if different
	// scan the tiles and check the scores
	err = CInspect::OK;
	for(int i=0; i<(int)m_TileVec.size(); i++)
	{
		KNOWLEDGE &K = m_ModelKnowledge[i];
		TILE &T = m_TileVec[i];
		if ( (K.kvec.size() == 0)				// first example so keep it
			|| (T.bestscore < m_LearnThresh)	)	// add if different
		{
			K.kvec.push_back(T.Features);
			K.i.push_back(T.m);
			K.i2.push_back(T.m2);
			K.score.push_back(0.0F);	// must create for later use
		}
	}

	return err;
}
CInspect::ERR_INSP CInspectPrivateColorHist::inspect(cv::Mat &image)
{
	// get the cv::Mat from the input data
	m_ImgIn = image;
	
	CInspect::ERR_INSP err;
	err = runScoring();
	if(err != CInspect::OK)
		return err;

	// for Inspecting,  return FAIL if different
	float minscore = 1.0F;
	int minindex;
	err = CInspect::OK;
	m_Overlay = cv::Mat(m_ImgIn.size(),CV_8UC4);
	m_Overlay = 0;
	for(int i=0; i<(int)m_TileVec.size(); i++)
	{
		KNOWLEDGE &K = m_ModelKnowledge[i];
		TILE &T = m_TileVec[i];
		if(minscore > T.bestscore)
		{
			minscore = T.bestscore;
			minindex = i;
		}
		T.status = CInspect::OK;
		if(T.bestscore < m_InspectThresh)	// add if different
		{
			T.status = CInspect::FAIL;
			err = CInspect::FAIL;
		}

		if(T.status == CInspect::FAIL)
		{
			annotate(cv::Rect(T.col,T.row,T.tilewidth,T.tileheight));
		}
	}
	return err;
}
CInspect::ERR_INSP CInspectPrivateColorHist::runScoring()
{
	// call the generic align method first
	CInspect::ERR_INSP err = align();
	if(err != CInspect::OK)
		return err;

	// guarantee image type
	cv::Mat temp;
	switch(m_Model.channels())
	{
	case 1:
		cvtColor(m_Model,temp,CV_GRAY2BGR);
		break;
	case 3:
		temp = m_Model;
		break;
	case 4:
		cvtColor(m_Model,temp,CV_RGBA2BGR);
		break;
	default:
		return CInspect::UNSUPPORTEDIMAGETYPE;
		break;
	}

	// calculate the color selector (Bin number)
	// palletize the color
	cv::Mat PalImg(temp.size(),CV_8U);
	if(m_pPalette->Palettize(temp,PalImg) != 0)
		return CInspect::UNABLETOPALETTIZE;
#if(0)
	m_pPalette->SavePalettizedAsPCX(PalImg,"C:/junk/PalImg.pcx");
#endif
#if(0)
	cv::imwrite("C:/junk/PalImg.png",PalImg);
#endif

	// create the struct used in the parallel_for
	TileEval tilev(m_TileVec,PalImg,m_BinShift);
	tilev.proc = ColorHistExtract;	// use this proceedure in each parallel_for
	try
	{
		parallel_for(  blocked_range<int>( 0, (int)m_TileVec.size() ) , tilev );
	}
	catch(int e)
	{
		int wait=e;	// errors are caught here (if any)
	}

	// compare with feature vectors already in DataBase
	// run a compare search for each tile object against each knowledge object
	TileScore tileS(m_TileVec, m_ModelKnowledge);
	tileS.proc = FloatScore;	// use this proceedure in each parallel_for
	try
	{
		parallel_for(  blocked_range<int>( 0, (int)m_TileVec.size() ) , tileS );
	}
	catch(int e)
	{
		int wait=e;	// errors are caught here (if any)
	}

	 return CInspect::OK;
}

//*********** Specific Code for Specific Feature data **********************
static void ColorHistExtract(TILE &t, cv::Mat &Img,int binshift)
{
	cv::Mat temp = Img(cv::Rect(t.col,t.row,t.tilewidth,t.tileheight));

	unsigned char* pU = t.Features.data();
	memset(pU,0,DATASIZE);	// clear to zero
	unsigned char* pBin;
	for(int i=0; i<t.tileheight; i++)
	{
		pBin = temp.ptr(i);
		for(int j=0; j<t.tilewidth; j++)
		{
			pU[pBin[j]>>binshift] ++;
		}
	}
	t.N = DATASIZE;
	t.m = std::accumulate(t.Features.begin(),t.Features.end(),0.0F);
	t.m2 = std::inner_product(t.Features.begin(),t.Features.end(),t.Features.begin(),0.0F);
}

// NCRScore defined inside each feature namespace so it can use the specific array<> datatype
static float NCRScore(std::array<unsigned char,DATASIZE> &I, std::array<unsigned char,DATASIZE> &M, int N, float m, float i, float K1, float K2)
{
	// some components are precalculated in the model or knowledge data
	// Precalculated lines are commented out but left here or understanding
//	float i = std::accumulate(I.begin(),I.end(),0.0F);
//	float i2 = std::inner_product(I.begin(),I.end(),I.begin(),0.0F);
	float im = std::inner_product(I.begin(),I.end(),M.begin(),0.0F);

	float K0 = N*im - i*m;
//	float K1 = N*i2 - i*i;
//	int N = (int)M.size();
//	float K2 = 1.0F / (N*m2 - m*m);

	float score = (K1> 0.0) ? (K0*K0*K2/K1) : 0.0F;
	return score;
}

// FloatScore defined inside each feature namespace so it can use the specific NCRScore() method
static void FloatScore(TILE & t,KNOWLEDGE & k)
{
	t.bestscore = 0.0F;
	t.bestindex = 0;
	float K1;
	float K2 = 1.0F / (t.N*t.m2 - t.m*t.m);

	for(int j=0; j<(int)k.kvec.size(); j++)
	{
		K1 = t.N*k.i2[j] - k.i[j]*k.i[j];
		k.score[j] = NCRScore(k.kvec[j],t.Features,t.N, t.m, k.i[j], K1, K2);
		if(k.score[j] > t.bestscore)
		{
			t.bestscore = k.score[j];
			t.bestindex = j;
			k.indexOfMaxScore = j;
			k.indexOfMaxScore = j;
		}
		if( (t.row > 100) && (t.col > 100 ) && (t.row < 110) && (t.col < 110 ) )
		{
			showDebugScores(t, k);
		}
	}
}

// showDebugScores() is a convenience debug function to call if you are having issues....
static void	showDebugScores(TILE &T, KNOWLEDGE &K)
{
	char msg[100];
	sprintf_s(msg,100,"Tile Inspect (%4d, %4d) = ",T.col,T.row);
	OutputDebugStringA(msg);
	unsigned char *pF = T.Features.data();
		sprintf_s(msg,100,"       %2d, %2d, %2d, %2d, %2d, %2d, %2d, %2d, %2d, %2d, %2d, %2d, %2d, %2d, %2d, %2d\n",
			pF[0],pF[1],pF[2],pF[3],pF[4],pF[5],pF[6],pF[7],pF[8],pF[9],pF[10],pF[11],pF[12],pF[13],pF[14],pF[15]);
		OutputDebugStringA(msg);

	if(K.kvec.size() == 0)
		OutputDebugStringA("KnowledgeVector Empty\n");
	else
	{
		for(int i=0; i<(int)K.kvec.size(); i++)
		{
			pF = K.kvec[i].data();
			sprintf_s(msg,100,"Tile Knowledge    (score) = ");
			OutputDebugStringA(msg);
			sprintf_s(msg,100,"(%4.3f)%2d, %2d, %2d, %2d, %2d, %2d, %2d, %2d, %2d, %2d, %2d, %2d, %2d, %2d, %2d, %2d\n",
				K.score[i],pF[0],pF[1],pF[2],pF[3],pF[4],pF[5],pF[6],pF[7],pF[8],pF[9],pF[10],pF[11],pF[12],pF[13],pF[14],pF[15]);
			OutputDebugStringA(msg);
		}
	}
}

