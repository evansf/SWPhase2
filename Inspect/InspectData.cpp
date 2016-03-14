#include "stdafx.h"
#include "InspectData.h"

CInspectData::CInspectData(void)
{
}

CInspectData::~CInspectData(void)
{
}

int CInspectData::create(cv::Mat &image)
{
	m_ImgIn = image;
	return 0;
}