#ifndef SISUTILITYHEADER
#define SISUTILITYHEADER

#if defined(INSPECTLIBRARY_EXPORT)
#   define INSPECTAPI   __declspec(dllexport)
#else
#   define INSPECTAPI   __declspec(dllimport)
#endif  // INSPECTLIBRARY_EXPORT

#include "opencv2/core/core.hpp"

#define NUMALIGNMODELS 4

typedef enum imgT
{
	UCHAR_ONEPLANE,		// CV_8UC1
	UCHAR_THREEPLANE,	// CV_8UC3
	UCHAR_FOURPLANE,	// CV_8UC4
//	SHORT_ONEPLANE,		// CV_16SC1
//	SHORT_THREEPLANE,	// CV_16SC3
//	SHORT_FOURPLANE,	// CV_16SC4
//	FLOAT32_ONEPLANE,	// CV_32F
	LASTTYPE
} IMGTYPE;

// to avoid dependancies on any specific input capture libraries
// we define out own input image structure
typedef class INSPECTAPI CInspectImage
{
public:
	IMGTYPE Imagetype;
	int width;
	int height;
	unsigned char *pPix;
	int imgStride;	// bytes from a pixel in one row to the adjacent pixel in the next row
} INSPECTIMAGE;

class INSPECTAPI CAlignModel
{
public:
	CAlignModel(){};
	~CAlignModel(void);
	bool contains(int x, int y);
	void setPos(int x, int y){m_X=x;m_Y=y;};
	void setRect(int x, int y, int w, int h){m_X=x;m_Y=y;m_Width=w; m_Height=h;};
	void setCenter(int x, int y){m_X=x-m_Width/2;m_Y=y-m_Height/2;};
	void setSize(int width, int height)
		{m_Width=width; m_Height=height;};
	void save(FILE *pFile);	

	int m_X, m_Y; // location of model rect
	int m_Width, m_Height;
};

// Declaration of parameters class
class INSPECTAPI SISParams
{
public:
	SISParams();
	int read(const char* filename);
	int write(const char* filename);
	cv::Rect CropRect(){return cv::Rect(m_x,m_y,m_w,m_h);};
	void SetCrop(cv::Rect rect){m_x=rect.x;m_y=rect.y;m_w=rect.width;m_h=rect.height;};
	float m_DisplayScale;
	int m_x,m_y,m_w,m_h;	// crop rectangle
	int m_ModelCount;
	CAlignModel m_Models[NUMALIGNMODELS];
};

void InspectImageToMat(INSPECTIMAGE &img, cv::Mat &I);
bool INSPECTAPI FindPointsToTrack(cv::Mat &img, cv::Mat &mask, std::vector<std::pair<float,float>> &points,
						int numpoints, cv::Size tilesize = cv::Size(0,0));
float INSPECTAPI Contrast(cv::Mat src, int x, int  y, int tilesize);
void INSPECTAPI Variance(cv::Mat &src, cv::Mat &dst, int tilesize);
int INSPECTAPI GetRandInt(int from, int to);

#endif	// SISUTILITYHEADER