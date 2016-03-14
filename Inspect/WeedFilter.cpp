#include "stdafx.h"
#include "WeedFilter.h"
using namespace WEEDS;

// memory allocation for static table
float CWeedFilter::LUT[256*256];

CWeedFilter::CWeedFilter(void)
{
	m_Threshold = 0.0F;
}

CWeedFilter::~CWeedFilter(void)
{
}

void CWeedFilter::CreateLUT(float threshold)
{
	m_Threshold = threshold;
	// fill the Lookup table
	float *pG;	// green rows for LUT[g*256 + r]
	float temp;
	for(int g=0; g<256; g++)
	{
		pG = &LUT[g*256];
		for(int r=0; r<256; r++)
		{
			temp = (128.0F * (g-r)/(g+r)) + 128.0F;
			pG[r] = temp < threshold ? 0.0F : temp;
		}
	}
}

void CWeedFilter::filter2float(cv::Mat &src, cv::Mat &dst, float threshold)
{
	if(m_Threshold != threshold)
		CreateLUT(threshold);
	unsigned char *pSrc;
	int step = src.channels();
	float *pDst;
	dst.create(src.size(),CV_32F);
	for(int i=0; i<src.rows; i++)
	{
		pSrc = src.ptr(i);
		pDst = (float*)dst.ptr(i);
		for(int j=0; j<src.cols; j++)
		{
			pDst[j] = LUT[(step*j+1)*256 + (step*j+2)];
		}
	}
}

void CWeedFilter::filterMasked(cv::Mat &src, cv::Mat &dst, float threshold)
{
	if(m_Threshold != threshold)
		CreateLUT(threshold);
	unsigned long * pSrcL;
	unsigned long * pDstL;
	dst.create(src.size(),src.type());
	unsigned long maskL;
	unsigned long srcVal;
	for(int i=0; i<src.rows; i++)
	{
		pSrcL = (unsigned long*)src.ptr(i);
		pDstL = (unsigned long*)dst.ptr(i);
		for(int j=0; j<src.cols; j++)
		{
			srcVal = pSrcL[j] & 0x00FFFF00 >> 8;	// get index [g*256+r]
			maskL = LUT[srcVal] < threshold ? 0x00000000 : 0xffffffff;
			pDstL[j] = pSrcL[j] & maskL;
		}
	}
}