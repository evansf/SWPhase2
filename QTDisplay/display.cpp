#ifndef OPENCV

#include <QtGui>
#include <QImage>

#include "qtdisplay.h"

#include "qtoverlay.h"
#include "roi.h"

// Forward Declaration
static QRect ParentRect(QRect select, QRect parent, double factor);


CDisplay::CDisplay(QWidget* parent) : QWidget(parent)
{
	int i;
	QSize displaySize = parent->size();
	if(displaySize.width()%4 != 0) parent->resize(displaySize.width()%4, displaySize.height());
	setAttribute(Qt::WA_StaticContents);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	setFocusPolicy(Qt::StrongFocus);
	m_pRoi = NULL;
	m_pOverlay = NULL;
	m_pImage = NULL;
	m_pPixmap = NULL;
	m_pPhysRoi = new CRoi;
	ColorTable.resize(256);
	for(i=0; i<256; i++)
	{
		ColorTable[i] = qRgb(i,i,i);
	}
    rubberBandIsShown = false;
	rubberBandEnabled = false;
	rubberBandMode = 0;	// default = Box
	SetRubberBandColor(Qt::white);
	m_RectShows = false;

	clearZoom();
	setZoomSettings(ZoomSettings());
	m_DisplayMode = CROP_SCALE;	// default
	
}

CDisplay::~CDisplay(void)
{
}
void CDisplay::SetPalette(std::vector<QRgb> pPalette)
{

	for(int i=0; i<256; i++) ColorTable[i] = pPalette[i];
}

void CDisplay::EnableRubberBand(bool Enable)
{
	rubberBandEnabled = Enable;
}

void CDisplay::SetRubberBandColor(QColor color)
{
	rubberBandColor = color;
}

COverlay* CDisplay::GetOverlay()
{
	return m_pOverlay;
}

QSize CDisplay::minimumSizeHint(void) const
{
	return QSize(m_pImage->size().width()/2,m_pImage->size().height()/2);
}


QSize CDisplay::sizeHint(void) const
{
	return m_pImage->size();
}

void CDisplay::AttachRoi(CRoi* pRoi)
{

	if(m_pOverlay != NULL)
		delete m_pOverlay;

	QSize displaySize = size();
	m_pRoi = pRoi;
	m_pMat = NULL;
	clearZoom();

	getPhysical();	// m_pImageRoi now always physical

	if(m_pPixmap != NULL) delete m_pPixmap;
	m_pPixmap = new QPixmap(displaySize.width(),displaySize.height());
	zoomStack[curZoom].factor = (double)m_pPhysRoi->Width()/displaySize.width();
	zoomStack[curZoom].DisplayedRect.setTop(0);
	zoomStack[curZoom].DisplayedRect.setLeft(0);
	zoomStack[curZoom].DisplayedRect.setBottom(m_pPhysRoi->Height()-1);
	zoomStack[curZoom].DisplayedRect.setRight(m_pPhysRoi->Width()-1);
	zoomStack[curZoom].DrawnRect = zoomStack[curZoom].DisplayedRect;

	m_pOverlay = new COverlay;
	m_pOverlay->SetDisplay(this);

	UpdateRoi();
}

QRect CDisplay::GetRubberBand()
{
	return rubberBandRect;
}

QRect CDisplay::GetDrawnRect()
{
	return DrawnRect;
}

QLine CDisplay::GetDrawnLine()
{
	return DrawnLine;
}

void CDisplay::UpdateRoi(void)
{
	if(m_pImage != NULL)
		delete m_pImage;

	getPhysical();	// m_pPhysRoi now the physical
	UpdatePhysical();
}

void CDisplay::UpdatePhysical(void)
{
	int width, height;
	width = m_pPhysRoi->Width();
	height = m_pPhysRoi->Height();

	switch(m_pPhysRoi->Type())
	{
	case CRoi::BW8:
			m_pImage = new QImage((unsigned char*)m_pPhysRoi->PixelPointer(),width,height,QImage::Format_Indexed8);
		m_pImage->setColorTable(ColorTable);
		break;
	case CRoi::RGB32:
			m_pImage = new QImage((unsigned char*)m_pPhysRoi->PixelPointer(),width,height,QImage::Format_RGB32);
		break;
	default:
		break;
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
	if(curZoom == 0)
	{
		QImage ZoomedImage = m_pImage->scaled(displaySize);
		QSize s = ZoomedImage.size();
		painter.drawImage( target, ZoomedImage, target );
	}
	else
	{
		QImage ImageZoomed(m_pImage->copy(zoomStack[curZoom].DisplayedRect));
		QImage ZoomedImage = ImageZoomed.scaled(displaySize);
		painter.drawImage( target, ZoomedImage, target );
	}

	painter.setBackgroundMode( Qt::TransparentMode );
	painter.drawPixmap( target,*m_pOverlay->GetPixmapPointer(),target );
	painter.end();
	update();
}

void CDisplay::getPhysical()
{
	QSize displaySize = size();
	int displayWidth = displaySize.width();
	int displayHeight = displaySize.height();
	double DispAspect = ((double)displayWidth)/displayHeight;

	int roiWidth,roiHeight;
	double RoiAspect;
	roiWidth = m_pRoi->Width();
	roiHeight = m_pRoi->Height();

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
	m_pPhysRoi->Create(physWidth,physHeight,m_pRoi->Type());
	m_pPhysRoi->CopyData(m_pRoi);

}

void CDisplay::setZoomSettings(const ZoomSettings &settings)
{
    zoomStack.clear();
    zoomStack.append(settings);
    curZoom = 0;
}
void CDisplay::clearZoom()
{
	setZoomSettings(ZoomSettings());
}

void CDisplay::paintEvent(QPaintEvent *event)
{
	if(0)
	{
		QPaintEvent *p;
		p = event;
	}
	if(m_pPixmap == NULL) return;

    QStylePainter painter(this);

    painter.drawPixmap(0, 0, *m_pPixmap);

    if (rubberBandIsShown)
	{
		painter.setPen(palette().light().color());
		switch(rubberBandMode)
		{
		case BOX:
			painter.drawRect(rubberBandRect.normalized()
										   .adjusted(0, 0, -1, -1));
			break;
		case LINE:
			painter.drawLine(rubberBandLine);
			break;
		default:
			break;
		}
    }

    if (hasFocus()) {
        QStyleOptionFocusRect option;
        option.initFrom(this);
        option.backgroundColor = palette().dark().color();
        painter.drawPrimitive(QStyle::PE_FrameFocusRect, option);
    }
}

void CDisplay::resizeEvent(QResizeEvent *event)
{
	if(0)
	{
		QResizeEvent *p;
		p = event;
	}

}

void CDisplay::mousePressEvent(QMouseEvent *event)
{
	if(rubberBandEnabled)
	{
		UnDrawShape();
		QRect rect(0, 0,
				   width(), height());

		if (event->button() == Qt::LeftButton) {
			if (rect.contains(event->pos()))
			{
				rect = DrawnRect;
				if(rect.contains(event->pos()))
				{
					m_MoveMode = true;
					m_BasePos = event->pos();
				}
				else
				{
					m_MoveMode = false;
					rubberBandRect.setTopLeft(event->pos());
					rubberBandRect.setBottomRight(event->pos());
					rubberBandLine = QLine(event->pos(),event->pos());
				}
				rubberBandIsShown = true;
				updateRubberBandRegion();
				setCursor(Qt::CrossCursor);
			}
		}
	}
	emit MousePress(event);
}
void CDisplay::mouseMoveEvent(QMouseEvent *event)
{
    if (rubberBandIsShown)
	{
        updateRubberBandRegion();
		if(m_MoveMode)
		{
			QPoint offset;
			offset = event->pos() - m_BasePos;
			rubberBandRect.setTopLeft(rubberBandRect.topLeft() + offset);
			rubberBandRect.setBottomRight(rubberBandRect.bottomRight() + offset);
			m_BasePos = event->pos();
		}
		else
		{
        rubberBandRect.setBottomRight(event->pos());
		rubberBandLine = QLine(rubberBandLine.p1(),event->pos());
		}
        updateRubberBandRegion();
    }
	emit MouseMove(event);
}
void CDisplay::mouseDoubleClickEvent(QMouseEvent *event)
{
	if(DrawnRect.contains(event->pos()))
	{
		zoomIn();
	}
	else
	{
		zoomOut();
	}
}
void CDisplay::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
	{
		if(rubberBandIsShown)
		{
			rubberBandIsShown = false;
			updateRubberBandRegion();
			unsetCursor();

			QRect rect = rubberBandRect.normalized();
			if (rect.width() < 4 || rect.height() < 4)
			{
				emit clicked(event);
				return;	// errant click?
			}
			DrawnRect = rect;
			DrawnLine = rubberBandLine;
			m_pOverlay->Clear();
			ReDrawShape();
			UpdatePhysical();
		}
		else
		{
		}
		emit clicked(event);
		emit clicked();
	}
	else
	{
		emit rightclicked();
	}
	emit MouseRelease(event);
}

void CDisplay::updateRubberBandRegion()
{
    QRect rect = rubberBandRect.normalized();
    update(rect.left(), rect.top(), rect.width(), 1);
    update(rect.left(), rect.top(), 1, rect.height());
    update(rect.left(), rect.bottom(), rect.width(), 1);
    update(rect.right(), rect.top(), 1, rect.height());
}

void CDisplay::UnDrawShape(void)
{

	if(!m_RectShows) return;
	if(rubberBandMode == 0)	// Box
		m_pOverlay->UndrawRect(DrawnRect);
	else	// Line
		m_pOverlay->UndrawLine(DrawnLine);

		m_RectShows = false;
}

void CDisplay::ReDrawShape(QColor color)
{
	// default for color is transparent -- means use existing color
	if (color == Qt::transparent) color = rubberBandColor;

	m_pOverlay->SetColor(color);
	if(rubberBandMode == 0)	// Box
		m_pOverlay->DrawRect(DrawnRect);
	else	// Line
		m_pOverlay->DrawLine(DrawnLine);

	m_RectShows = true;
}

void CDisplay::zoomOut()
{
    if (curZoom > 0) {
        --curZoom;
		UnDrawShape();
		DrawnRect = zoomStack[curZoom].DrawnRect;
		ReDrawShape();
        UpdateRoi();
    }
}

static QRect ParentRect(QRect select, QRect parent, double factor)
{
	QRect temp;
	temp.setTop((int)(factor * select.top()) + parent.top());
	temp.setLeft((int)(factor * select.left()) + parent.left());
	temp.setBottom(temp.top() + (int)(factor * select.height()));
	temp.setRight(temp.left() + (int)(factor * select.width()));

	return temp;
}

void CDisplay::zoomIn()
{
	QRect OldRect;
	double factor;
	OldRect = zoomStack[curZoom].DisplayedRect;
	factor = zoomStack[curZoom].factor;
	zoomStack[curZoom].DrawnRect = DrawnRect;
	UnDrawShape();
    ZoomSettings prevSettings = zoomStack[curZoom];
    ++curZoom;
    if (curZoom >= zoomStack.count()) 
	{
        ZoomSettings settings;

        zoomStack.resize(curZoom);
        zoomStack.append(settings);
	}
	double displayAspect;
	int width, height;
	width = m_pPixmap->width();
	height = m_pPixmap->height();
	displayAspect = (double)width/height;

	QRect temp;
	temp = DrawnRect;
	QPoint center, corner;
	center = DrawnRect.center();
	corner.setY(DrawnRect.height()/2);
	corner.setX((int)(DrawnRect.height() * displayAspect /2));

	temp.setTopLeft(center-corner);
	temp.setBottomRight(center+corner);
	temp = ParentRect(temp,OldRect,factor);
	zoomStack[curZoom].DisplayedRect = temp;
	factor = (double)temp.width() / m_pPhysRoi->Width();
	zoomStack[curZoom].factor = factor;
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


void CDisplay::ClearOverlay(void)
{

	m_pOverlay->begin();
	m_pOverlay->Clear();
	m_pOverlay->end();

	m_RectShows = false;
	UpdateRoi();
}

void CDisplay::ShowOverlay(void)
{
    m_pOverlay->drawPicture();    // draw the picture at (0,0)
}

QRect CDisplay::RoiToDisplay(QRect InRect)
{
	QPoint P0,P1;
	P0 = InRect.topLeft(); P1 = InRect.bottomRight();
	QRect temp(RoiToDisplay(P0),RoiToDisplay(P1));

	return temp;
}

QPoint CDisplay::RoiToDisplay(QPoint InPoint)
{
	double factor;
	factor = zoomStack[curZoom].factor;
	QPoint temp;
	temp.setX((int)(InPoint.x()/factor));
	temp.setY((int)(InPoint.y()/factor));
	return temp;
}

QRect CDisplay::DisplayToRoi(QRect InRect)
{
	QPoint P0,P1;
	P0 = InRect.topLeft(); P1 = InRect.bottomRight();
	QRect temp(DisplayToRoi(P0),DisplayToRoi(P1));
	return temp;
}

QPoint CDisplay::DisplayToRoi(QPoint InPoint)
{
	double factor;
	factor = zoomStack[curZoom].factor;
	QPoint temp;
	temp.setX((int)(InPoint.x()*factor));
	temp.setY((int)(InPoint.y()*factor));
	return temp;
}

void CDisplay::DrawPoint(QPoint p, QColor color)
{
	m_pOverlay->SetColor(color);
	m_pOverlay->DrawPoint(p);
	m_RectShows = false;
}
void CDisplay::ErasePoint(QPoint p)
{
	m_pOverlay->UndrawPoint(p);
}

void CDisplay::DrawRect(QRect rect, QColor color)
{
	m_pOverlay->SetColor(color);
	m_pOverlay->DrawRect(rect);
	m_RectShows = false;
}
void CDisplay::EraseRect(QRect rect)
{
	m_pOverlay->UndrawRect(rect);
}
void CDisplay::DrawLine(QLine line, QColor color)
{
	m_pOverlay->SetColor(color);
	m_pOverlay->DrawLine(line);
	m_RectShows = false;
}
void CDisplay::EraseLine(QLine line)
{
	m_pOverlay->UndrawLine(line);
}
void CDisplay::DrawCross(QPoint Point, QColor color)
{
	m_pOverlay->SetColor(color);
	m_pOverlay->DrawCross(Point);
	m_RectShows = false;
}
void CDisplay::EraseCross(QPoint Point)
{
	m_pOverlay->UndrawCross(Point);
}
void CDisplay::DrawEllipse(QRect Rect, QColor color)
{
	m_pOverlay->SetColor(color);
	m_pOverlay->DrawEllipse(Rect);
	m_RectShows = false;
}
void CDisplay::EraseEllipse(QRect Rect)
{
	m_pOverlay->UndrawEllipse(Rect);
}
void CDisplay::HideRubberBand()
{
	UnDrawShape();
}

#endif	// ifndef opencv