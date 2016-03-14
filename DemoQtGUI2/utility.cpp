#include "utility.h"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "Qt_CV.h"

INSPECTIMAGE QImageToInspectImage(QImage &img)
{
	INSPECTIMAGE InsImg;
	InsImg.height    = img.height();
	InsImg.width     = img.width();
	InsImg.pPix      = img.bits();
	InsImg.imgStride = img.bytesPerLine();
	switch(img.format())
	{
	case QImage::Format_Indexed8:
		InsImg.Imagetype = imgT::UCHAR_ONEPLANE;
		break;
	case QImage::Format_RGB32:
	case QImage::Format_ARGB32:
		InsImg.Imagetype = imgT::UCHAR_FOURPLANE;
		break;
	default:
		InsImg.Imagetype = imgT::UCHAR_ONEPLANE;
		break;
	}
	return InsImg;
}
void InspectImageToQImage(INSPECTIMAGE &img, QImage &I)
{
	QImage::Format f;
	cv::Mat temp;
	switch(img.Imagetype)
	{
	case imgT::UCHAR_ONEPLANE:
		f = QImage::Format_Indexed8;
		temp = cv::Mat(img.height,img.width,CV_8U,img.pPix,img.imgStride);
		break;
	case imgT::UCHAR_THREEPLANE:
		f = QImage::Format_RGB32;
		temp = cv::Mat(img.height,img.width,CV_8UC3,img.pPix,img.imgStride);
		break;
	case imgT::UCHAR_FOURPLANE:
		f = QImage::Format_ARGB32;
		temp = cv::Mat(img.height,img.width,CV_8UC4,img.pPix,img.imgStride);
		break;
	default:
		f = QImage::Format_Indexed8;
		temp = cv::Mat(img.height,img.width,CV_8U,img.pPix,img.imgStride);
		break;
	}
	I = mat_to_qimage_cpy(temp,f);
}

void InspectImageToMatGui(INSPECTIMAGE &img, cv::Mat &I)
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
