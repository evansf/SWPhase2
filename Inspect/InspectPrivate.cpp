#include "stdafx.h"
#include "InspectPrivate.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/video/video.hpp"
#define _USE_MATH_DEFINES
#include <math.h>

// Forward Declarations
static void ExtractDefault(TILE &t, cv::Mat &Img, cv::Mat &FeatMat);
static void InspectDefault(TILE & t,KNOWLEDGE &k, cv::Mat &feat);
static void TrainDefault(TILE & t,KNOWLEDGE &k, cv::Mat &feat);
#define DEFAULTDATASIZE 9

// Macro to draw crosses on cv::Mat images
#define drawCross( img, center, color, d , t ) \
	cv::line( img, cv::Point( (int)center.x - d, (int)center.y - d ), \
	cv::Point( (int)center.x + d, (int)center.y + d ), \
	color, t, 0 ); \
	cv::line( img, cv::Point( (int)center.x + d, (int)center.y - d ), \
	cv::Point( (int)center.x - d, (int)center.y + d ), \
	color, t, 0 )
#define drawCross2(img,center,color,d) drawCross(img,center,cv::Scalar(255,255,255),d, 5); drawCross(img,center,color,d, 3 );

// storage for static variables of class CInspectPrivate
float CInspectPrivate::m_RawScoreDiv;

CInspectPrivate::CInspectPrivate(SISParams &params) :
	m_params(params)
{
	m_LearnThresh   = 0.5F;
	m_InspectThresh = 0.7F;
	m_RawScoreDiv = 200.0F;
	m_MustAlign = false;
	m_MahalaCount = 0;
}

CInspectPrivate::~CInspectPrivate(void)
{
}
CInspect::ERR_INSP CInspectPrivate::create(cv::Mat &image, cv::Mat &mask, CInspectData **ppData,
										   int tilewidth, int tileheight, int stepcols, int steprows)
{
	if(DEBUGFILE100)
	{
		fopen_s(&m_DebugFile, DEBUGFILENAME, "w");
	}
	else
		m_DebugFile = NULL;

	// this data stays the same for lifetime
	m_ppData = ppData;
	m_Model = image.clone();
	m_ModelBW = m_Model;
	cvtToBW(m_ModelBW);	// get BW version of model

	if(mask.channels() != 1)
		cv::cvtColor(mask,m_Mask,CV_BGR2GRAY);
	else
		m_Mask = mask.clone();

	// this data changed for each input image
	// we need this class to exist now just for symetry
	if(*m_ppData == NULL)	// creator has no common data (yet)
	{
		m_pData = new CInspectData;
		*m_ppData = m_pData;	// creator is pointing now to new data
	}
	else
		m_pData = *m_ppData;

	// tile the image applying the mask
	// make a vector of TILE objects and a vector of KNOWLEDGE objects
	int numrows = (m_Model.rows - tileheight)/steprows;
	int numcols = (m_Model.cols - tilewidth)/stepcols;
	m_TileVec.clear();
	TILE tile;
	tile.tileheight = tileheight;
	tile.tilewidth = tilewidth;
	m_ModelKnowledge.clear();
	KNOWLEDGE k;
	int tilecount = 0;
	for(int row = 0; row <numrows; row++)
	{
		for(int col = 0; col <numcols; col++)
		{
			if(MaskOK(row*steprows,col*stepcols,tilewidth,tileheight))
			{
				tile.row = row*steprows; tile.col = col*stepcols;
				tile.Index = tilecount++;
				m_TileVec.push_back(tile);
				m_ModelKnowledge.push_back(k);
			}
		}
	}
	if(m_TileVec.size() == 0)
		return CInspect::NOTILESUNDEREMASK;

	return CInspect::OK;
}
CInspect::ERR_INSP CInspectPrivate::train(cv::Mat &image)
{
	if(*m_ppData == NULL)	// creator has no common data (yet)
	{
		m_pData = new CInspectData;
		*m_ppData = m_pData;	// creator is pointing now to new data
		m_pData->create( image);
		PreProc();	// this fills in some more of m_pData
	}
	else
		m_pData = *m_ppData;
	return CInspect::OK;
}
CInspect::ERR_INSP CInspectPrivate::trainEx(cv::Mat &image)
{
	CInspect::ERR_INSP err = CInspect::OK;
	// check for failed tiles and train them as normal
	for(int i=0; i<(int)m_TileVec.size(); i++)
	{
		if(m_TileVec[i].status == CInspect::FAIL)
		{
			TrainDefault(m_TileVec[i], m_ModelKnowledge[i], m_FeatMat);
		}
	}
	optimize();	// calls the derived class version
	return err;
}

CInspect::ERR_INSP CInspectPrivate::optimize()
{
	CInspect::ERR_INSP err = CInspect::OK;
	for(int i=0; i<(int)m_TileVec.size(); i++)
	{
		KNOWLEDGE &K = m_ModelKnowledge[i];
		TILE &T = m_TileVec[i];
		K.covar.Means().copyTo(m_DebugMeans);
		K.covar.m_pM12->copyTo(m_VarMat);
		if(K.covar.Invert()!= 0)
		{
			err = CInspect::UNABLETOINVERTCOVAR;
			T.status = CInspect::SKIP;
			annotate(cv::Rect(T.col,T.row,T.tilewidth,T.tileheight),cvScalar(0,0,255,255),true);
		}
		K.covar.InverseMatrix().copyTo(m_CoMat);
		if( (m_DebugFile != NULL) && (i == 100) )
		{
			fprintf(m_DebugFile,"\n");
			putFloatMat(m_DebugFile, &m_DebugMeans,"","\n\n");
			putFloatMat(m_DebugFile, &m_VarMat,"","\n\n");
			putFloatMat(m_DebugFile, &m_CoMat,"","\n\n");
			fclose(m_DebugFile);
		}
	}
	return CInspect::OK;
}
CInspect::ERR_INSP CInspectPrivate::inspect(cv::Mat &image)
{
	if(*m_ppData == NULL)	// creator has no common data (yet)
	{
		m_pData = new CInspectData;
		*m_ppData = m_pData;	// creator is pointing now to new data
		m_pData->create(image);
		PreProc();
	}
	else
		m_pData = *m_ppData;
	return CInspect::OK;
}
CInspect::ERR_INSP CInspectPrivate::PreProc()
{
	CInspect::ERR_INSP err = CInspect::OK;
	// Get a BW version for alignment
	if(m_pData->m_ImgIn.channels() == 1)
		m_pData->m_ImgInBW = m_pData->m_ImgIn;
	else
		cv::cvtColor(m_pData->m_ImgIn,m_pData->m_ImgInBW,CV_BGR2GRAY);
	if(m_MustAlign)
	{
		// call the generic align method
		err = align();
		if(err != CInspect::OK)
			return err;
	}
	else
	{
		m_pData->m_ImgAligned = m_pData->m_ImgIn(m_params.CropRect());	// for align not used
		m_pData->m_ImgRun = m_pData->m_ImgAligned;
	}
	m_pData->m_Overlay.create(m_pData->m_ImgIn.size(),CV_8UC4);
	m_pData->m_Overlay = 0;

	return err;
}
CInspect::ERR_INSP CInspectPrivate::align()
{
	std::vector<cv::Point2f> corners;
	std::vector<cv::Point2f> copyPoints;

	cv::Mat result;
	cv::Point Pmin,Pmax;
	cv::Point2f P1,P2;
#if(1)
	double minVal,maxVal;
	corners.clear();
	cv::Rect Rc = m_params.CropRect();
	for(int i=0; i<m_params.m_ModelCount; i++)
	{
		// Using the BW images, find the match of each model in the inspect image
		cv::Rect searchRect = cv::Rect(m_params.m_Models[i].m_X-50+Rc.x,m_params.m_Models[i].m_Y-50+Rc.y,
			m_params.m_Models[i].m_Width+100,m_params.m_Models[i].m_Height+100);
		cv::Mat searchRoi = m_pData->m_ImgInBW(searchRect);
		cv::Rect modelRect = cv::Rect(m_params.m_Models[i].m_X,m_params.m_Models[i].m_Y,
			m_params.m_Models[i].m_Width,m_params.m_Models[i].m_Height);
		P1.x = (float)modelRect.x+Rc.x; P1.y = (float)modelRect.y+Rc.y;
		copyPoints.push_back(P1);
		cv::Mat modelRoi  = m_ModelBW(modelRect);
		cv::matchTemplate(searchRoi,modelRoi,result,CV_TM_CCORR_NORMED);
		cv::minMaxLoc(result,&minVal,&maxVal,&Pmin,&Pmax);
		P2.x = (float)Pmax.x + searchRect.x; P2.y = (float)Pmax.y + searchRect.y;
		corners.push_back(P2);
	}

	cv::Mat Transform = cv::estimateRigidTransform(corners,copyPoints,false);
#else
	cv::Mat Transform = cv::estimateRigidTransform(m_pData->m_ImgInBW(m_params.CropRect()),m_Model,true);
#endif
	if(Transform.empty())
		return CInspect::ALIGNTRANSFORMNOTFOUND;

	// do warp needed to re-align image
	// warp the input image (BW or Color)
	cv::warpAffine(m_pData->m_ImgIn,m_pData->m_Warped,Transform,m_pData->m_ImgIn.size());

	// create the aligned image (for GUI) and the RunTime image (for Inspection)
	// for some derived inspections, ImgRun may need further processing.
	m_pData->m_ImgAligned = m_pData->m_Warped(m_params.CropRect());
	m_pData->m_ImgRun = m_pData->m_ImgAligned;

	return CInspect::OK;
}

// This is where the parallel FOR() call is made to Extract the data for the Tile
CInspect::ERR_INSP CInspectPrivate::runExtract(void (*ExtractProc)(TILE &,cv::Mat &,cv::Mat &))
{
	CInspect::ERR_INSP err = CInspect::OK;

	// create the struct used in the parallel_for
	TileEval tilev(m_TileVec,m_pData->m_ImgRun, m_FeatMat);
	if(ExtractProc == NULL)
		ExtractProc = ExtractDefault;
	tilev.proc = ExtractProc;	// use this proceedure in each parallel_for
	try
	{
		parallel_for(  blocked_range<int>( 0, (int)m_TileVec.size() ) , tilev );
	}
	catch(int e)
	{
		int wait=e;	// errors are caught here (if any)
		err = CInspect::PARALLEL_FOR_EXTRACT;
	}
	return err;
}

// This is where the parallel FOR() call is made to Train the data for the Tile
CInspect::ERR_INSP CInspectPrivate::runTrain(void (*TrainProc)(TILE & t,KNOWLEDGE &k, cv::Mat &feat))
{
	CInspect::ERR_INSP err = CInspect::OK;

	// create the struct used in the parallel_for
	TileScore tileS(m_TileVec, m_ModelKnowledge, m_FeatMat);
	if(TrainProc == NULL)
		TrainProc = TrainDefault;
	tileS.proc = TrainProc;	// use this proceedure in each parallel_for
	try
	{
		parallel_for(  blocked_range<int>( 0, (int)m_TileVec.size() ) , tileS );
	}
	catch(int e)
	{
		int wait=e;	// errors are caught here (if any)
		err = CInspect::PARALLEL_FOR_TRAIN;
	}
	return err;
}

// This is where the parallel FOR() call is made to Inspect the data for the Tile
CInspect::ERR_INSP CInspectPrivate::runInspect(void (*InspectProc)(TILE & t,KNOWLEDGE &k, cv::Mat &feat))
{
	CInspect::ERR_INSP err = CInspect::OK;

	m_MaxScore = 0.0;
	m_MahalaCount = 0;

	// create the struct used in the parallel_for
	TileScore tileS(m_TileVec, m_ModelKnowledge, m_FeatMat);
	if(InspectProc == NULL)
		InspectProc = InspectDefault;
	tileS.proc = InspectProc;	// use this proceedure in each parallel_for
	try
	{
		parallel_for(  blocked_range<int>( 0, (int)m_TileVec.size() ) , tileS );
	}
	catch(int e)
	{
		int wait=e;	// errors are caught here (if any)
		err = CInspect::PARALLEL_FOR_INSPECT;
	}

	return err;
}

// Find out if tile (x,y,w,h) is under the mask
bool CInspectPrivate::MaskOK(int row, int col, int width, int height)
{
	bool offmask = false;// start with no off mask detected
	unsigned char* pC;
	for(int i=row; i<row+height; i++)
	{
		pC = (unsigned char*)m_Mask.ptr(i);
		for(int j=col; j<col+width; j++)
			offmask |= (pC[j] == 0);	// is pixel off mask?
	}
	return !offmask;	// return true if nobody was off mask
}

void CInspectPrivate::cvtToBW(cv::Mat &img)
{
	switch(img.type())
	{
	case CV_8UC3:
		cv::cvtColor(img,img,CV_BGR2GRAY);
		break;
	case CV_8UC4:
		cv::cvtColor(img,img,CV_BGRA2GRAY);
		break;
	case CV_8U:
	default:
		break;
	}
}

void CInspectPrivate::annotate(cv::Rect rect, cv::Scalar color, bool maskoff)
{
	cv::rectangle(m_pData->m_Overlay,rect,color,-1);
	if(maskoff)
		cv::rectangle(m_Mask,rect,cvScalar(0),-1);
}
void CInspectPrivate::annotate(std::vector<vector<cv::Point>> contours,cv::Scalar color, bool maskoff)
{
	cv::drawContours(m_pData->m_Overlay, contours, -1, color, CV_FILLED);
}

void CInspectPrivate::Mask(cv::Mat &src, cv::Mat &mask, cv::Mat &dst)
{
	unsigned char *pSrc;
	unsigned char *pDst;
	unsigned char *pMask;
	int step = src.channels();
	dst.create(src.size(),src.type());
	for(int i=0; i<src.rows; i++)
	{
		pMask = mask.ptr(i);
		pSrc = src.ptr(i);
		pDst = dst.ptr(i);
		for(int j=0; j<src.cols; j++)
		{
			if(pMask[j])
			{
				for(int k=0; k<step; k++)
					pDst[j*step+k] = pSrc[j*step+k];
			}
			else
			{
				for(int k=0; k<step; k++)
					pDst[j*step+k] = 0;
			}
		}
	}
}
// There is only one overlay for all inspections.  Any derived class can draw on it.
void CInspectPrivate::DrawRect(int x, int y, int w, int h, unsigned long ARGB)
{
	cv::rectangle(m_pData->m_Overlay,cv::Rect(x,y,w,h),cv::Scalar(ARGB&0xFF,(ARGB&0xff00)>>8,(ARGB&0xff0000)>>16,(ARGB&0xFF000000)>>24),4);
}

// Track the Max Score of tiles
void CInspectPrivate::UpdateRawScore(TILE T)
{
	if(m_MahalaCount++ == 0)
	{
		m_MaxScore = 0.0;
		m_MahalaMean = m_MahalaSigma = m_MahalaM2 = 0.0;
	}
	if(m_MaxScore < T.scoreRaw)
	{
		m_MaxScore = T.scoreRaw;
		m_MaxIndex = T.Index;
		m_MaxX = T.col; m_MaxY = T.row;
	}
	// from method by Knuth
	double delta = T.scoreRaw - m_MahalaMean;
	m_MahalaMean += delta/m_MahalaCount;
	m_MahalaM2 += delta * (T.scoreRaw - m_MahalaMean);
	// m_MahalaSigma is calculated from m_MahalaM2 when accessed
}

//*********** Defauolt Code for Feature data **********************
static void ExtractDefault(TILE &t, cv::Mat &Img, cv::Mat &FeatMat)
{
	// Get a ROI of the tile
	cv::Mat temp = Img(cv::Rect(t.col,t.row,t.tilewidth,t.tileheight));

	// point to the features in the array
	float* pU = (float*)FeatMat.ptr(t.Index);
	float* pPix;
	// just put out out the intensity as the default
	for(int i=0; i<t.tileheight; i++)
	{
		pPix = (float*)temp.ptr(i);
		memcpy(pU,pPix,t.tilewidth*sizeof(float));
		pU += t.tilewidth;
	}
	t.datasize = t.tilewidth * t.tileheight;
}

// InspectDefault defined here 
static void InspectDefault(TILE & t, KNOWLEDGE &k, cv::Mat &FeatMat)
{
	if(t.status == CInspect::SKIP)
	{
		return;
	}
	else
	{
		// Use the features in the array
		cv::Mat feat = FeatMat(cvRect(0,t.Index,FeatMat.cols,1));
		// get the raw score from the covariance
		t.scoreRaw = k.covar.MahalanobisRaw(feat);
	}
}

// TrainDefault defined to use new Incremental Math
static void TrainDefault(TILE & t,KNOWLEDGE &k, cv::Mat &FeatMat)
{
	if(t.status == CInspect::SKIP)
		return;
	if( t.Index == 100 )	// DEBUG -- put breakpoint here and view the output screen
		k.covar.IncrementalUpdate((float*)FeatMat.ptr(t.Index),FeatMat.cols);
	else
		k.covar.IncrementalUpdate((float*)FeatMat.ptr(t.Index),FeatMat.cols);
}

// Debug Float Output for log
void CInspectPrivate::putFloatMat(FILE *pFile, cv::Mat *pMat, char *prepend, char *append)
{
	float *pVal;
	for(int i=0; i<pMat->rows; i++)
	{
		if(i>0) fprintf(pFile,"\n");
		pVal = (float*)pMat->ptr(i);
		fprintf(pFile,"%s",prepend);
		fprintf(pFile,"%g",pVal[0]);
		for(int j=1; j<pMat->cols; j++)
		{
			fprintf(pFile,",%g",pVal[j]);
		}
	}
	fprintf(pFile,"%s",append);
}

#if(0)
std::vector<std::array<float,2>> CInspectPrivate::getHistogramRaw()
{
	// get minmax and mean-sigma of raw
	// assume scores are gaussian centered at 0 mean
	double max = 0.0F;
	double min = FLT_MAX;
	double sum=0.0, sumSq=0.0;
	int N = 0;
	double val;
	for(int i=0; i<(int)m_TileVec.size(); i++)
	{
		val = m_TileVec[i].scoreRaw;
		if(val < min)
			min = val;
		if(val > max)
			max = val;
		// positive score
		sum += val; sumSq += val*val;
		N++;
	}
	// calc mean and sigma
	double mean = sum / N;
	double sigma = sqrt(N * sumSq - sum*sum) / N;
	//     reject data closer than 2 sigma to mean
	// and reject data smaller than twice the mean
	N = 0; sum = 0.0; sumSq = 0.0;
	for(int i=0; i<(int)m_TileVec.size(); i++)
	{
		val = m_TileVec[i].scoreRaw;
		if( abs(val - mean) > sigma*2 )
			continue;
		if( val > mean*2 )
			continue;
		sum += val; sumSq += val*val;
		N++;
	}

	// re-calc mean and sigma
	mean = sum / N;
	sigma = sqrt(N * sumSq - sum*sum) / N;
	double zeroSigma = sqrt(N * sumSq)/ N;
	// reset Min and Max to 3 sigma
	min = mean - 3*sigma;
	max = mean + 3*sigma;
	min = min < 0.0 ? 0.0 : min;

	std::vector<int> hist;
	hist.reserve(100);
	std::vector<float> bintop;
	bintop.reserve(100);
	double binwidth = (max-min) / 99.0;	// stretch for top bin
	min -= binwidth;

	// set up the N (=100) bins
	for(int i=0; i<100; i++)
	{
		hist.push_back(0);
		bintop.push_back((float)(min +  binwidth * i));
	}
	// Histogram into the bins
	int bin;
	for(int i=0; i<(int)m_TileVec.size(); i++)
	{
		bin = (int)floor((m_TileVec[i].scoreRaw-min) / binwidth);
		bin = bin > 99 ? 99 : bin;
		++hist[bin];
	}
	// convert bins to vector of points
	std::vector<std::array<float,2>> histdata;
	std::array<float,2> PointF;
	for(int i=0; i<100; i++)
	{
		PointF[0] = bintop[i];
		PointF[1] = (float)hist[i];
		histdata.push_back(PointF);
	}
	return histdata;
}
std::vector<std::array<float,2>> CInspectPrivate::getHistogramScores()
{
	std::vector<std::array<float,2>> hist;
	return hist;
}
#endif	// histogram
