#pragma once
// data class to hold input image and processed images
// This class is deleted (if non-NULL) before passing the
// (NULL) pointer to this class into CInspectPrivate classes
// for the first of the inspections.  CInspectPrivate then 
// creates a new instance into which the data is placed.
// Subsequent calls, the pointer will be non-NULL and the 
// inspections will use the preprocessed images, avoiding
// re-processing.
#include "opencv2/core/core.hpp"

class CInspectData
{
public:
	CInspectData(void);
	~CInspectData(void);
	int create(cv::Mat & image);
	cv::Mat m_ImgIn;
	cv::Mat m_ImgInBW;
	cv::Mat m_ImgAligned;
	cv::Mat m_Warped;
	cv::Mat m_ImgRun;
	cv::Mat m_Overlay;
};

