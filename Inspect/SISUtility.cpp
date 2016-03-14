// utility classes implementations
#include "SISUtility.h"
#include <stdio.h>
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

SISParams::SISParams()
{
	m_x=0,m_y=0,m_w=0,m_h=0;	// causes whole image to be used
	m_ModelCount = NUMALIGNMODELS;
}

#define GETLINE(pF,ln) fscanf_s(pF,"%[^\n]\n",ln,100)

int SISParams::read(const char* filename)
{
	char line[100];
	FILE *pFile;
	fopen_s(&pFile,filename,"r");
	if(!pFile)
	{
		return -1;
	}
	while(!feof(pFile))
	{
		// Skip comment lines
		do
			GETLINE(pFile,line);
		while(line[0] == '#');
		// Parameter names begin with unique characters!
		CAlignModel model;
		int x,y,w,h;
		switch(toupper(line[0]))
		{
		case 'C':	// Crop Rectangle
			GETLINE(pFile,line);
			sscanf_s(line,"%d,%d,%d,%d",&x,&y,&w,&h);
			m_x=x;m_y=y;m_w=w;m_h=h;
			break;
		case 'A':	// Align Rectangles
//			m_AlignModels.clear();
			int count;
			GETLINE(pFile,line);
			sscanf_s(line,"%d",&count);

			for(int i=0; i<count; i++)
			{
				GETLINE(pFile,line);
				sscanf_s(line,"%d,%d,%d,%d",&x,&y,&w,&h);
				model.setRect(x,y,w,h);
				m_Models[i] = model;
//				m_AlignModels.push_back(model);
			}
			break;
		case 'S':	// Display Scale
			float s;
			GETLINE(pFile,line);
			sscanf_s(line,"%f",&s);
			m_DisplayScale = s;
			break;
		default:
			break;
		}
	}
	fclose(pFile);
	return 0;
}

int SISParams::write(const char* filename)
{
	FILE *pFile;
	fopen_s(&pFile,filename,"w");
	if(!pFile)
	{
		return -1;
	}

		// output the crop rectangle
	fprintf(pFile,"# Crop Rectangle\nCROP\n");
	fprintf(pFile,"%d,%d,%d,%d\n",m_x,m_y,m_w,m_h);

	// then output the align models
	fprintf(pFile,"# Align Count then Models\nALIGN\n");
	fprintf(pFile, "%d\n",m_ModelCount);
	for(int i=0; i<m_ModelCount; i++)
	{
		m_Models[i].save(pFile);
//		m_AlignModels[i].save(pFile);
	}
	fprintf(pFile,"# DisplayScaling\nSCALE\n");
	fprintf(pFile, "%6.2f\n",m_DisplayScale);

	// close the parameters file
	fclose(pFile);
	return 0;
}

void InspectImageToMat(INSPECTIMAGE &img, cv::Mat &I)
{
	cv::Mat temp;
	switch(img.Imagetype)
	{
	case imgT::UCHAR_ONEPLANE:
		temp = cv::Mat(img.height,img.width,CV_8U,img.pPix,img.imgStride);
		break;
	case imgT::UCHAR_THREEPLANE:
		temp = cv::Mat(img.height,img.width,CV_8UC3,img.pPix,img.imgStride);
		break;
	case imgT::UCHAR_FOURPLANE:
		temp = cv::Mat(img.height,img.width,CV_8UC4,img.pPix,img.imgStride);
		break;
	default:
		temp = cv::Mat(img.height,img.width,CV_8U,img.pPix,img.imgStride);
		break;
	}
	I = temp;
}

#define ENOUGHCONTRAST (0.90F)
#define LOOPLIMIT 1000
bool FindPointsToTrack(cv::Mat &img, cv::Mat &mask, std::vector<std::pair<float,float>> &points, int numpoints, cv::Size tilesize)
{

	bool domask = !mask.empty();

	int x,y;
	int rangeX = img.cols - tilesize.width;
	int rangeY = img.rows - tilesize.height;
	points.clear();
	std::pair<float,float> P;
	unsigned char* pMask;
	float contrast;
	int numloops = LOOPLIMIT;
	while( --numloops && ((int)points.size() < numpoints) )
	{
		x = GetRandInt(0,rangeX); y = GetRandInt(0,rangeY);

		if( domask )
		{
			pMask = (unsigned char*)mask.ptr(y);
			if(pMask[x] == 0)
				continue;
		}

		contrast = Contrast(img,x,y,15);	// contrast in 15X15 tile
		if(contrast < ENOUGHCONTRAST)
			continue;

		P.first = (float)x; P.second = (float)y;
		points.push_back(P);
	}
	if(numloops == 0)
		return false;
	else
		return true;
}

int GetRandInt(int from, int to)
{
	int span = to-from;
	float r = (float)rand()/RAND_MAX;
	return (int)(r * span + from);
}

// Michelson contrast parameter for tile at x,y
float Contrast(cv::Mat src, int x, int  y, int tilesize)
{
	cv::Rect rect = cvRect(x,y,tilesize,tilesize);
	cv::Mat tile = src(rect);
	double minval,maxval;
	cv::minMaxLoc(tile,&minval, &maxval);
	return (float)((maxval - minval) / (maxval + minval));
}

// remap image to pseudo-contrast image.  Image is arbitrarily scaled to meet
// application needs.  Uses integral images for speed.
void Variance(cv::Mat &src, cv::Mat &dst, int tilesize)
{
	int sizeoftile = tilesize;	// calculate variance in n X n blocks
	int N = sizeoftile * sizeoftile;
	double factor = 0.01;	// rescale the result to fit output data type
	cv::Mat temp;
	if(src.type() != CV_8UC1)
		src.convertTo(temp,CV_8UC1,1.0/256.0);
	else
		temp = src;
#if(0)
	cv::Mat HistEq;
	cv::equalizeHist( temp, HistEq );
	temp = HistEq;
#endif

	// Make integral image
	cv::Mat sumX;
	cv::Mat sumX2;
	cv::integral(temp,sumX, sumX2,CV_64F);

	int varWidth = temp.cols - sizeoftile;
	int varHeight = temp.rows - sizeoftile;
	dst.create(varHeight,varWidth, CV_32F );
	int integralX,integralY;
	int i,j;
	double tl,tr,bl,br;
	double sum,sum2;
	float *pPixOut;
	double *pIntegPixT, *pIntegPixB;
	double *pInteg2PixT, *pInteg2PixB;
	double variance;
	double maxVar = 0.0;	// track this for debug
	for(i=0; i<varHeight; i++)
	{
		integralY = i+1;
		pPixOut = (float *)dst.ptr(i);
		pIntegPixT = (double *)sumX.ptr(integralY);
		pIntegPixB = (double *)sumX.ptr(integralY+sizeoftile);
		pInteg2PixT = (double *)sumX2.ptr(integralY);
		pInteg2PixB = (double *)sumX2.ptr(integralY+sizeoftile);
		for(j=0; j<varWidth; j++)
		{
			integralX = j+1;

			tl = pIntegPixT[integralX];
			tr = pIntegPixT[integralX+sizeoftile];
			bl = pIntegPixB[integralX];
			br = pIntegPixB[integralX+sizeoftile];
			sum = br-tr+tl-bl;
			tl = pInteg2PixT[integralX];
			tr = pInteg2PixT[integralX+sizeoftile];
			bl = pInteg2PixB[integralX];
			br = pInteg2PixB[integralX+sizeoftile];
			sum2 = br-tr+tl-bl;

			variance = factor * (N * sum2 - sum * sum)/(N*N);
			if(variance > maxVar) maxVar = variance;	// track for debug
			pPixOut[j] = (float)(variance);
		}
	}

}
