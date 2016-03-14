#pragma once
#include <stdio.h>
#include "Inspect.h"
#include "opencv2/core/core.hpp"

class INSPECTAPI CAlignModel
{
public:
	CAlignModel(int x, int y, int width, int height) {m_X=x;m_Y=y;m_Width=width; m_Height=height;};
	~CAlignModel(void);
	bool contains(int x, int y);
	void setPos(int x, int y){m_X=x;m_Y=y;};
	void setCenter(int x, int y){m_X=x-m_Width/2;m_Y=y-m_Height/2;};
	void setSize(int width, int height)
		{m_Width=width; m_Height=height;};
	void save(FILE *pFile, double scale = 1.0);	// rescales the points if needed
	cv::Rect SearchRect(){return cv::Rect(m_X - m_Width,m_Y-m_Height,3*m_Width,3*m_Height);};
	cv::Rect ModelRect(){return cv::Rect(m_X,m_Y,m_Width,m_Height);};

	int m_X, m_Y; // location of top left
	int m_Width, m_Height;

};

