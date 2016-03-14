// QT_CV.h
// contains needed conversions for cv::Mat and QImage

#include <QtGui\qimage.h>
#include "opencv2\core\core.hpp"
using namespace cv;

inline QImage mat_to_qimage_ref(Mat &mat, QImage::Format format)
{
  return QImage(mat.data, mat.cols, mat.rows, mat.step, format);
}

inline QImage mat_to_qimage_cpy(Mat const &mat, QImage::Format format)
{
    return QImage(mat.data, mat.cols, mat.rows, 
                  mat.step, format).copy();
}

inline Mat qimage_to_mat_ref(QImage &img, int format)
{
    return Mat(img.height(), img.width(), 
            format, img.bits(), img.bytesPerLine());
}

inline Mat qimage_to_mat_cpy(QImage const &img, int format)
{    
    return Mat(img.height(), img.width(), format, 
                   (uchar*)(img.bits()), 
                   img.bytesPerLine()).clone();
}