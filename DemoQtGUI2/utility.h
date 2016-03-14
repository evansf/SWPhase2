#ifndef UTILITYHEADER
#define UTILITYHEADER

#include "Inspect.h"
#include "opencv2/core/core.hpp"
#include <QtGui\qimage.h>

void InspectImageToMatGui(INSPECTIMAGE &img, cv::Mat &I);
INSPECTIMAGE QImageToInspectImage(QImage &img);
void InspectImageToQImage(INSPECTIMAGE &img, QImage &I);

#endif  //  UTILITYHEADER