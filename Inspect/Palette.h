#ifndef PALETTEHEADER
#define PALETTEHEADER

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "opencv2/core/core.hpp"

using namespace cv;

//
// BGRType is a simple 8-bit color triple for OpenCV images
//
typedef struct {
	unsigned char b,g,r;
} BGRType;
//
// RGBType is a simple 8-bit color triple for non-OpenCV images
//
typedef struct {
	unsigned char r,g,b;
} RGBType;


class ColorData
{
public:
	BGRType color;
};

class CPalette
{
public:
	CPalette(void);
	~CPalette(void);
	bool exists(){return m_exists;};
	virtual int Create(cv::Mat& src, int size);	// returns 0 on success
	virtual int Palettize(cv::Mat& src, cv::Mat& dst);	// returns 0 on success
	virtual void MakePaletteTable(){};
	void DePalettize(cv::Mat& src8U, cv::Mat& dstBGR);
	int Score(cv::Mat& PalImg, cv::Mat& Img, cv::Mat& ScoreImg);	// get score for color relative to its palette brothers
	int SavePalettizedAsPCX( cv::Mat& image, char* filename);	// matrix is Palettized pixels
	BGRType *m_palette;	// classic palette as an indexed array of colors (BGRType)
	ColorData *m_table;	// internal palette as an indexed array of data (class ColorData)

protected:
	virtual unsigned char QuantizeColor(BGRType color){return 0;};
	int m_PaletteSize;

private:
	bool m_exists;
};

inline RGBType BGR2RGB(BGRType bgr);
inline BGRType RGB2BGR(RGBType rgb);

#endif // PALETTEHEADER