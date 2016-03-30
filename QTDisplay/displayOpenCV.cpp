#include <QtWidgets\QStylePainter>
#include <QtWidgets\QStyleOptionFocusRect>
#include <QtGui\QImage>
#include <QtGui\QMouseEvent>
#include "QT_CV.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "qtdisplay.h"

#include "qtoverlay.h"

#ifndef min
#define min(a,b) ((a)<(b) ? (a) : (b))
#endif
#ifndef max
#define max(a,b) ((a)>(b) ? (a) : (b))
#endif

// Forward Declaration
static QRect ParentRect(QRect select, QRect parent, double factor);


QTDisplay::QTDisplay(QWidget* parent) : QWidget(parent)
{
	int i;
	QSize displaySize = parent->size();
	if(displaySize.width()%4 != 0) parent->resize(displaySize.width()%4, displaySize.height());
	setAttribute(Qt::WA_StaticContents);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	setFocusPolicy(Qt::StrongFocus);
	m_pMat = NULL;
	m_pPhysMat = new cv::Mat();
	m_pOverlay = NULL;
	m_pImage = NULL;
	m_pPixmap = NULL;
	ColorTable.resize(256);
	QtColorTable.resize(256);
	for(i=0; i<256; i++)
	{
		ColorTable[i] = qRgb(i,i,i);
		QtColorTable[i] = ColorTable[i];
	}

	m_pRubberObject = NULL;
	m_RubberObjectMode = BOX;	// default = Box

	clearZoom();
	setZoomSettings(ZoomSettings());
	m_DisplayMode = CROP_SCALE;	// default
	m_PolyActive = false;
	m_Snap = 10;	// default
	
}

QTDisplay::~QTDisplay(void)
{
	delete m_pPhysMat;
}
void QTDisplay::SetPalette(std::vector<QRgb> pPalette)
{

	for(int i=0; i<256; i++)
	{
		ColorTable[i] = pPalette[i];
		QtColorTable[i] = ColorTable[i];
	}
}

void QTDisplay::EnableRubberBand(bool Enable)
{
	m_RubberObjectEnabled = Enable;
}

QTOverlay* QTDisplay::GetOverlay()
{
	return m_pOverlay;
}

QSize QTDisplay::minimumSizeHint(void) const
{
	return QSize(m_pImage->size().width()/2,m_pImage->size().height()/2);
}


QSize QTDisplay::sizeHint(void) const
{
	return m_pImage->size();
}

void QTDisplay::AttachQImage(QImage &img)
{
	QImage::Format format = img.format();
	switch(format)
	{
	case QImage::Format_RGB32:
	case QImage::Format_ARGB32:
		m_QImgMat = qimage_to_mat_cpy(img,CV_8UC4);
		break;
	case QImage::Format_Indexed8:
	case QImage::Format_Grayscale8:
		m_QImgMat = qimage_to_mat_cpy(img,CV_8U);
		break;
	default:
		return;
	}
	if(m_QImgMat.type() == CV_8U)
		cv::cvtColor(m_QImgMat,m_QImgMat,CV_GRAY2BGRA);
	AttachMat(&m_QImgMat);
}

void QTDisplay::AttachMat(cv::Mat *mat)
{
	if(m_pOverlay != NULL)
	{
		delete m_pOverlay;
		m_pOverlay = NULL;
	}
	if(m_pImage != NULL)
	{
		delete m_pImage;
		m_pImage = NULL;
	}

	QSize displaySize = size();
	m_pMat = mat;
	clearZoom();

	getPhysical();	// m_pImageRoi now always physical

	if(m_pPixmap != NULL) delete m_pPixmap;
	m_pPixmap = new QPixmap(displaySize.width(),displaySize.height());
	zoomStack[curZoom].DisplayedRect.setTop(0);
	zoomStack[curZoom].DisplayedRect.setLeft(0);
	zoomStack[curZoom].DisplayedRect.setBottom(m_pPhysMat->rows-1);
	zoomStack[curZoom].DisplayedRect.setRight(m_pPhysMat->cols-1);
	// calculate magnification factor to fit into display
	double factor = min((double)displaySize.width() / m_pPhysMat->cols
		,(double)displaySize.height() / m_pPhysMat->rows);
	zoomStack[curZoom].factor = factor;
	zoomStack[curZoom].DrawnRect = RoiToDisplay(zoomStack[curZoom].DisplayedRect);

	m_pOverlay = new QTOverlay;
	m_pOverlay->SetDisplay(this);
	QSize size(m_pPhysMat->cols,m_pPhysMat->rows);
	m_pOverlay->SetPixmapSize(size);

	UpdateRoi();
}
void QTDisplay::SetOverlay(QImage &overlay)
{
	QSize size(m_pPhysMat->cols,m_pPhysMat->rows);
	if(m_pOverlay == NULL)
	{
		m_pOverlay = new QTOverlay;
		m_pOverlay->SetDisplay(this);
		m_pOverlay->SetPixmapSize(size);
	}
	QPainter p;
	p.setCompositionMode(QPainter::CompositionMode_SourceOver);
	p.begin(m_pOverlay->GetPixmapPointer());
	p.drawImage(QPoint(0,0),overlay);
	p.end();
}
QRect QTDisplay::GetRubberBand()
{
	return m_RubberRect;
}

QRect QTDisplay::GetDrawnRect()
{
	return m_RubberRect;
}

QLine QTDisplay::GetDrawnLine()
{
	QLine temp(m_RubberRect.topLeft(),m_RubberRect.bottomRight());
	return temp;
}

void QTDisplay::UpdateRoi(void)
{
	UpdatePhysical();
	this->repaint();	// to stimulate paint
}

static void DilatePixmap(QPixmap &map, int repeats)
{
	QImage img = map.toImage();
	cv::Mat cvImg = qimage_to_mat_ref(img,CV_8UC4);
	cv::dilate(cvImg,cvImg,cv::Mat(),cv::Point(-1,-1),repeats);
	map = map.fromImage(img);
}

void QTDisplay::UpdatePhysical(void)
{
	int width, height;
	width = m_pPhysMat->cols;
	height = m_pPhysMat->rows;

	if(m_pImage == NULL)
	{
		switch(m_pMat->channels())
		{
		case 1:
			m_pImage = new QImage((unsigned char*)m_pPhysMat->ptr(0),width,height,QImage::Format_Indexed8);
			m_pImage->setColorTable(QtColorTable);
			break;
		case 3:
		case 4:
			m_pImage = new QImage((unsigned char*)m_pPhysMat->ptr(0),width,height,QImage::Format_RGB32);
			break;
		default:
			break;
		}
	}

	QSize roiSize(width,height);
	QSize displaySize = m_pPixmap->size();

	QPainter painter(m_pPixmap);
	painter.initFrom(this);

	QSize ISize;
	ISize = m_pImage->size();
	QRectF target( 0.0, 0.0, displaySize.width()-1, displaySize.height()-1 );
	QRectF source( 0.0, 0.0, ISize.width()-1, ISize.height()-1 );
	painter.setBackgroundMode( Qt::OpaqueMode );
	QImage ZoomedImage;

		QImage ImageZoomed(m_pImage->copy(zoomStack[curZoom].DisplayedRect));
		QSize imageSize = ImageZoomed.size();
		ZoomedImage = ImageZoomed.scaled(displaySize);
		painter.drawImage( target, ZoomedImage, target );

	QPixmap PixmapRect;
	m_pOverlay->GetPixmap(PixmapRect,zoomStack[curZoom].DisplayedRect);
	double scaleing = (double)imageSize.width() / displaySize.width();
	if(scaleing > 1.5)	// pixels might disapear
		DilatePixmap(PixmapRect, (int)scaleing);
	painter.setBackgroundMode( Qt::TransparentMode );
	painter.drawPixmap( target,PixmapRect.scaled(displaySize),target );
	painter.end();
	update();
}

void QTDisplay::getPhysical()
{
	QSize displaySize = size();
	int displayWidth = displaySize.width();
	int displayHeight = displaySize.height();
	double DispAspect = ((double)displayWidth)/displayHeight;

	int roiWidth,roiHeight;
	double RoiAspect;
	roiWidth = m_pMat->cols;
	roiHeight = m_pMat->rows;

	RoiAspect = ((double)roiWidth)/roiHeight ;

	int physWidth, physHeight;

	switch(m_DisplayMode)
	{
	case CROP_SCALE:
		// make aspect ratios the same
		if( RoiAspect > DispAspect)
		{
			physHeight = roiHeight;
			physWidth = (int)((double)roiWidth * (DispAspect/RoiAspect));
		}
		else
		{
			physWidth = roiWidth;
			physHeight = (int)((double)roiHeight * (RoiAspect/DispAspect));
		}
		break;
	case FIT_SCALE:
		if( RoiAspect > DispAspect)
		{
			physWidth = roiWidth;
			physHeight = (int)((double)physWidth/DispAspect);
		}
		else
		{
			physHeight = roiHeight;
			physWidth = (int)((double)physHeight * DispAspect);
		}
		break;
	case FIT_VERTICAL:
		physHeight = roiHeight;
		physWidth = (int)((double)physHeight * DispAspect);
		break;
	case FIT_HORIZONTAL:
		physWidth = roiWidth;
		physHeight = (int)((double)physWidth/DispAspect);
		break;
	case CROP_ONLY:
		physWidth = displayWidth;
		physHeight = displayHeight;
		break;
	default:
		break;
	}
	physWidth -= physWidth%4;
	int i,j;
	unsigned char *pSrc;

	unsigned long val;
	switch(m_pMat->channels())
	{
	case 1:
		m_pPhysMat->create(physHeight,physWidth,CV_8UC1);
		physWidth = MIN(m_pMat->cols,physWidth);
		physHeight = MIN(m_pMat->rows,physHeight);
		for(i=0; i<physHeight; i++)
		{
			pSrc = m_pMat->ptr(i);
			unsigned char* pDst = (unsigned char*)m_pPhysMat->ptr(i);
			for(j=0; j<physWidth; j++)
			{
				pDst[j] = pSrc[j];
			}
		}
		break;
	case 3:
		m_pPhysMat->create(physHeight,physWidth,CV_8UC4);
		physWidth = MIN(m_pMat->cols,physWidth);
		physHeight = MIN(m_pMat->rows,physHeight);
		for(i=0; i<physHeight; i++)
		{
			pSrc = m_pMat->ptr(i);
			unsigned long* pDst = (unsigned long*)m_pPhysMat->ptr(i);
			for(j=0; j<physWidth; j++)
			{
				val = (((pSrc[3*j+2]<<8)+pSrc[3*j+1])<<8)+pSrc[3*j];
				pDst[j] = val + 0xFF000000;	// alpha one
			}
		}
		break;
	case 4:
		m_pPhysMat->create(physHeight,physWidth,CV_8UC4);
		physWidth = MIN(m_pMat->cols,physWidth);
		physHeight = MIN(m_pMat->rows,physHeight);
		for(i=0; i<physHeight; i++)
		{
			pSrc = m_pMat->ptr(i);
			unsigned long* pDst = (unsigned long*)m_pPhysMat->ptr(i);
			for(j=0; j<physWidth; j++)
			{
				val = (((pSrc[4*j+2]<<8)+pSrc[4*j+1])<<8)+pSrc[4*j];
				pDst[j] = val + 0xFF000000;	// alpha one
			}
		}
		break;
	default:
		break;
	}
}

void QTDisplay::setZoomSettings(const ZoomSettings &settings)
{
    zoomStack.clear();
	zoomStack.push_back(settings);
    curZoom = 0;
}
void QTDisplay::clearZoom()
{
	setZoomSettings(ZoomSettings());
}

void QTDisplay::paintEvent(QPaintEvent *event)
{
	if(m_pPixmap == NULL) return;

    QStylePainter painter(this);

    painter.drawPixmap(0, 0, *m_pPixmap);

    if (hasFocus()) {
        QStyleOptionFocusRect option;
        option.initFrom(this);
        option.backgroundColor = palette().dark().color();
        painter.drawPrimitive(QStyle::PE_FrameFocusRect, option);
    }
}

void QTDisplay::resizeEvent(QResizeEvent *event)
{
	if(0)
	{
		QResizeEvent *p;
		p = event;
	}

}

void QTDisplay::UnDrawShape(void)
{

}

void QTDisplay::ReDrawShape(QColor color)
{

	m_pOverlay->SetColor(color);

}

void QTDisplay::zoomOut()
{
    if (curZoom > 0) {
        --curZoom;
		UnDrawShape();
		ReDrawShape();
        UpdateRoi();
    }
}

void QTDisplay::zoomIn()
{

	double factor;
	UnDrawShape();
    ZoomSettings prevSettings = zoomStack[curZoom];

	// get the display aspect (X/Y)
	double displayAspect;
	int width, height;
	width = m_pPixmap->width();
	height = m_pPixmap->height();
	displayAspect = (double)width/height;

	// adjust rect to aspect of display
	QRect temp;
	temp = m_RubberRect;

	QPoint center, corner;
	center = m_RubberRect.center();
	double boxAspect = m_RubberRect.width()/m_RubberRect.height();
	if(boxAspect > displayAspect)
	{
		// width dominates
		corner.setX(m_RubberRect.width()/2);
		corner.setY((int)(((m_RubberRect.width() / displayAspect) /2)));		
	}
	else
	{
		corner.setY(m_RubberRect.height()/2);
		corner.setX((int)(((m_RubberRect.height() * displayAspect) /2)));
	}
	if( (center.x()-corner.x()) < 0 )
		center.setX(corner.x());
	if( (center.y()-corner.y()) < 0 )
		center.setY(corner.y());
	temp.setTopLeft(center-corner);
	temp.setBottomRight(center+corner);
	if(temp.bottom() > height)
		temp.setBottom(height-1);
	if(temp.right() > width)
		temp.setRight(width-1);

	// put drawn rect into current zoom data
	zoomStack[curZoom].DrawnRect = temp;


	// build the new zoom data
    ZoomSettings settings;
	temp = DisplayToRoi(temp);
	settings.DisplayedRect = temp;
	// calculate magnification factor to fit into display
	factor = min((double)m_pPixmap->width() / temp.width() 
		,(double)m_pPixmap->height() / temp.height() );
	settings.factor = factor;

	if (++curZoom >= zoomStack.size()) 
        zoomStack.push_back(settings);
	else
		zoomStack[curZoom] = settings;	// reuse

    UpdateRoi();
}

//*************************************************************************
ZoomSettings::ZoomSettings()
{
	factor = 1.0;
	DrawnRect.setBottom(0);
	DrawnRect.setTop(0);
	DrawnRect.setLeft(0);
	DrawnRect.setRight(0);

	DisplayedRect.setBottom(0);
	DisplayedRect.setTop(0);
	DisplayedRect.setLeft(0);
	DisplayedRect.setRight(0);

}

void QTDisplay::ClearOverlay(void)
{

	m_pOverlay->begin();
	m_pOverlay->Clear();
	m_pOverlay->end();

	UpdateRoi();
}

void QTDisplay::ShowOverlay(void)
{
    m_pOverlay->drawPicture();    // draw the picture at (0,0)
}

QRect QTDisplay::RoiToDisplay(QRect InRect)
{
	QPoint P0,P1;
	P0 = InRect.topLeft(); P1 = InRect.bottomRight();
	QRect temp(RoiToDisplay(P0),RoiToDisplay(P1));

	return temp;
}

QPoint QTDisplay::RoiToDisplay(QPoint InPoint)
{
	double factor;
	int i=curZoom;

		factor = zoomStack[i].factor;
		InPoint -= zoomStack[i].DisplayedRect.topLeft();
		QPoint temp;
		temp.setX((int)(InPoint.x()*factor));
		temp.setY((int)(InPoint.y()*factor));

	return temp;
}

QRect QTDisplay::DisplayToRoi(QRect InRect)
{
	QPoint P0,P1;
	P0 = InRect.topLeft(); P1 = InRect.bottomRight();
	QRect temp(DisplayToRoi(P0),DisplayToRoi(P1));
	return temp;
}

QPoint QTDisplay::DisplayToRoi(QPoint InPoint)
{
	double factor;

		factor = zoomStack[curZoom].factor;
		QPoint temp;
		temp.setX((int)(InPoint.x()/factor));
		temp.setY((int)(InPoint.y()/factor));
		temp += zoomStack[curZoom].DisplayedRect.topLeft();

	return temp;
}
void QTDisplay::SetLinesize(int s)
{
	m_pOverlay->SetLinesize(s);
}
void QTDisplay::DrawPixmap(QPixmap &img, QPoint p, QColor color)
{
	m_pOverlay->SetColor(color);
	m_pOverlay->DrawPixmap(img,p);
}
void QTDisplay::DrawPoint(QPoint p, QColor color)
{
	m_pOverlay->SetColor(color);
	m_pOverlay->DrawPoint(p);
}
void QTDisplay::ErasePoint(QPoint p)
{
	m_pOverlay->UndrawPoint(p);
}
void QTDisplay::DrawRect(QRect rect, QColor color)
{
	m_pOverlay->SetColor(color);
	m_pOverlay->DrawRect(rect);
}
void QTDisplay::EraseRect(QRect rect)
{
	m_pOverlay->UndrawRect(rect);
}
void QTDisplay::DrawLine(QLine line, QColor color)
{
	m_pOverlay->SetColor(color);
	m_pOverlay->DrawLine(line);
}
void QTDisplay::EraseLine(QLine line)
{
	m_pOverlay->UndrawLine(line);
}
void QTDisplay::DrawCross(QPoint Point, QColor color)
{
	m_pOverlay->SetColor(color);
	m_pOverlay->DrawCross(Point);
}
void QTDisplay::EraseCross(QPoint Point)
{
	m_pOverlay->UndrawCross(Point);
}
void QTDisplay::DrawEllipse(QRect Rect, QColor color)
{
	m_pOverlay->SetColor(color);
	m_pOverlay->DrawEllipse(Rect);
}
void QTDisplay::EraseEllipse(QRect Rect)
{
	m_pOverlay->UndrawEllipse(Rect);
}

#if(1)
void QTDisplay::mousePressEvent(QMouseEvent *event)
{
	QPoint p = event->pos();
	switch(m_RubberObjectMode)
	{
	case DISABLED:	// same as box for display zoom
	case BOX:
		m_RubberOrigin = p;
		if(m_pRubberObject == NULL)
		{
			m_pRubberObject = new QRubberBand(QRubberBand::Rectangle,this);
		}
		m_pRubberObject->setGeometry(QRect(m_RubberOrigin, QSize()));
		m_pRubberObject->show();
		break;
	case LINE:
		break;
	case POLY:
		if(!m_PolyActive)
		{
			m_PolyOrigin = p;
		}
		break;
	default:
		break;
	}
#if(0)
	if(m_RubberObjectEnabled)
	{
		if(m_RubberObjectMode == BOX)
		{
			m_RubberOrigin = event->pos();
			m_RubberActive = true;
			if(m_pRubberObject == NULL)
			{
				m_pRubberObject = new QRubberBand(QRubberBand::Rectangle,this);
			}
			m_pRubberObject->setGeometry(QRect(m_RubberOrigin, QSize()));
			m_pRubberObject->show();
		}
		if(m_RubberObjectMode == LINE)
		{
			if(m_RubberActive)
			{
				// click ends this line and begins anotherline
				m_RubberLineEnd = event->pos();
				m_RubberOrigin = event->pos();
				emit lineKnot(m_RubberLineEnd);
				if( (m_RubberLineEnd.x() - m_RubberOrigin.x() + m_RubberLineEnd.y() - m_RubberOrigin.y()) < SNAP)
					m_RubberActive = false;
			}
			else
			{
				m_RubberActive = true;
				m_RubberOrigin = event->pos();
				m_RubberLineEnd = m_RubberOrigin;
				emit PolyKnot(m_RubberLineEnd);
			}
		}

	}
#endif
	emit MousePress(event);
}
void QTDisplay::mouseMoveEvent(QMouseEvent *event)
{
	switch(m_RubberObjectMode)
	{
	case DISABLED:	// same as box to support display zoom
	case BOX:
		m_pRubberObject->setGeometry(QRect(m_RubberOrigin,event->pos()).normalized());
		break;
	case LINE:
		break;
	default:
		break;
	}
#if(0)
	if(m_RubberActive)
	{
		if(m_RubberObjectMode == BOX)
			m_pRubberObject->setGeometry(QRect(m_RubberOrigin,event->pos()).normalized());
		if(m_RubberObjectMode == LINE)
		{
			undrawRubberLine();
			m_RubberLineEnd = event->pos();
			drawRubberLine();
		}
	}
#endif
	emit MouseMove(event);
}
void QTDisplay::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
		return;
	QRect Rect = QRect(m_RubberOrigin,event->pos()).normalized();
	if( (Rect.width() + Rect.height()) < 6)
	{
		emit clicked(event);
		emit clicked();
		emit MouseRelease(event);
		return;
	}
	QRect r;
	QPoint p = event->pos();
	switch(m_RubberObjectMode)
	{
	case DISABLED:
		m_pRubberObject->hide();
		m_RubberRect = Rect;
		break;
	case BOX:
		m_RubberObjectMode = DISABLED;
		m_pRubberObject->hide();
		m_RubberRect = Rect;
		emit RectDrawn(m_RubberRect);
		break;
	case POLY:
		if(m_PolyActive)
		{
			r = QRect(m_PolyOrigin,p).normalized();
			if( (r.width() + r.height()) < m_Snap)
			{
				m_RubberObjectMode = DISABLED;
				m_PolyActive = false;
				p = m_PolyOrigin;	// close the polygon
			}
		}
		else
		{
			m_PolyActive = true;
		}
		emit PolyKnot(p);
		if(!m_PolyActive)	// ended this click
			emit PolyDrawn();
		break;
	default:
		break;
	}
#if(0)
	if (event->button() == Qt::LeftButton)
	{
		if(m_RubberActive)
		{
			if(m_RubberObjectMode == BOX)
			{
				m_RubberActive = false;
				m_pRubberObject->hide();
				QRect rect = m_pRubberObject->rect();
				if( (rect.width() < 4) || (rect.height() < 4) )
				{
					emit clicked(event);
					emit clicked();
				}
				else
				{
					m_RubberRect = QRect(m_RubberOrigin,event->pos()).normalized();	// drawn rect or line
				}
			}
		}
		else
		{
			emit clicked(event);
			emit clicked();
		}
	}
#endif
	emit MouseRelease(event);
}
#else
void QTDisplay::mousePressEvent(QMouseEvent *event)
{
	if(m_RubberObjectEnabled)
	{
		if(m_RubberObjectMode == BOX)
		{
			m_RubberOrigin = event->pos();
			m_RubberActive = true;
			if(m_pRubberObject == NULL)
			{
				m_pRubberObject = new QRubberBand(QRubberBand::Rectangle,this);
			}
			m_pRubberObject->setGeometry(QRect(m_RubberOrigin, QSize()));
			m_pRubberObject->show();
		}
		if(m_RubberObjectMode == LINE)
		{
			if(m_RubberActive)
			{
				// click ends this line and begins anotherline
				m_RubberLineEnd = event->pos();
				m_RubberOrigin = event->pos();
				emit lineKnot(m_RubberLineEnd);
				if( (m_RubberLineEnd.x() - m_RubberOrigin.x() + m_RubberLineEnd.y() - m_RubberOrigin.y()) < SNAP)
					m_RubberActive = false;
			}
			else
			{
				m_RubberActive = true;
				m_RubberOrigin = event->pos();
				m_RubberLineEnd = m_RubberOrigin;
				emit lineKnot(m_RubberLineEnd);
			}
		}

	}
	emit MousePress(event);
}
void QTDisplay::mouseMoveEvent(QMouseEvent *event)
{
	if(m_RubberActive)
	{
		if(m_RubberObjectMode == BOX)
			m_pRubberObject->setGeometry(QRect(m_RubberOrigin,event->pos()).normalized());
		if(m_RubberObjectMode == LINE)
		{
			undrawRubberLine();
			m_RubberLineEnd = event->pos();
			drawRubberLine();
		}
	}
	emit MouseMove(event);
}

void QTDisplay::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
	{
		if(m_RubberActive)
		{
			if(m_RubberObjectMode == BOX)
			{
				m_RubberActive = false;
				m_pRubberObject->hide();
				QRect rect = m_pRubberObject->rect();
				if( (rect.width() < 4) || (rect.height() < 4) )
				{
					emit clicked(event);
					emit clicked();
				}
				else
				{
					m_RubberRect = QRect(m_RubberOrigin,event->pos()).normalized();	// drawn rect or line
				}
			}
		}
		else
		{
			emit clicked(event);
			emit clicked();
		}
	}
	emit MouseRelease(event);
}
#endif

void QTDisplay::mouseDoubleClickEvent(QMouseEvent *event)
{
	if(m_RubberRect.width() > 1)
	{
		zoomIn();
		m_RubberRect = QRect(0,0,1,1);
	}
	else
	{
		zoomOut();
	}
}

void QTDisplay::drawRubberLine()
{
	if(m_pOverlay == NULL) return;
	QLine line(m_RubberOrigin,m_RubberLineEnd);
	m_pOverlay->DrawLine(line);
	m_pOverlay->Update();
}
void QTDisplay::undrawRubberLine()
{
	if(m_pOverlay == NULL) return;
	QLine line(m_RubberOrigin,m_RubberLineEnd);
	m_pOverlay->UndrawLine(line);
	m_pOverlay->Update();
}
