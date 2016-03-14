#include "qtoverlay.h"
#include "qtdisplay.h"
#include <QtGui\QPainter>
#include <QtGui\QPicture>
#include <QtGui\QBitmap>

QTOverlay::QTOverlay(void) : QPainter()
{
	m_pPixmap = NULL;
	m_pPicture = new QPicture;
	m_Color = QColor(Qt::red);
	m_pDisplay = NULL;
	SetLinesize(3);
}

QTOverlay::~QTOverlay(void)
{
}
bool QTOverlay::begin(void)
{
	return QPainter::begin(m_pPixmap);
}
bool QTOverlay::end(void)
{
	return QPainter::end();
}

void QTOverlay::Clear(void)
{
	m_pPixmap->fill(Qt::transparent);
	delete m_pPicture;
	m_pPicture = new QPicture;
}

void QTOverlay::SetDisplay(QTDisplay *pDisplay)
{
	m_pDisplay = pDisplay;

}
void QTOverlay::SetPixmapSize(QSize size)
{
	if(m_pPixmap != NULL)
	{
		delete m_pPixmap;
	}
	m_pPixmap = new QPixmap(size);
	m_pPixmap->fill(Qt::transparent);	// fills with transparent
}

void QTOverlay::Update(void)
{
	// set transparent mask from background
	m_pPixmap->setMask(m_pPixmap->createMaskFromColor(Qt::white));
}

QPixmap* QTOverlay::GetPixmapPointer(void)
{
	return m_pPixmap;
}
void QTOverlay::GetPixmap(QPixmap &PMap, QRect rect)
{
	PMap = m_pPixmap->copy(rect);
}

void QTOverlay::drawPicture(void)
{
	begin();
	QPainter::drawPicture(0,0,*m_pPicture);    // draw the picture at (0,0)
	end();

}

void QTOverlay::SetColor(QColor color)
{
//	QColor c = QColor(Qt::red);
	m_Color = color;
}
void QTOverlay::SetLinesize(int s)
{
	m_linesize = s;
//	QPen p = pen();
//	p.setWidth(m_linesize);
//	setPen(p);
}
void QTOverlay::DrawPixmap(QPixmap &img, QPoint p)
{
	begin();
	setCompositionMode(QPainter::CompositionMode_SourceOver);
	setCompositionMode(QPainter::CompositionMode_SourceOver);
	drawPixmap(p.x(),p.y(),img);
	end();
}
void QTOverlay::DrawPoint(QPoint p)
{
	begin();
	setBackgroundMode(Qt::TransparentMode);
	setCompositionMode(QPainter::CompositionMode_SourceOver);
	setMyPen();
	drawPoint(p);
	end();
}
void QTOverlay::UndrawPoint(QPoint p)
{
	begin();
	setBackgroundMode(Qt::TransparentMode);
	setCompositionMode(QPainter::CompositionMode_Source);
	setMyPen(true);
	drawPoint(p);
	end();
}
void QTOverlay::setMyPen(bool undraw)
{
	QPen p = pen();
	p.setWidth(m_linesize);
	if(undraw)
		p.setColor(Qt::transparent);
	else
		p.setColor(m_Color);
	setPen(p);
}
void QTOverlay::DrawRect(QRect rect)
{
	begin();
	setBackgroundMode(Qt::TransparentMode);
	setCompositionMode(QPainter::CompositionMode_SourceOver);
	setMyPen();
	drawRect(rect);
	end();
}
void QTOverlay::UndrawRect(QRect rect)
{
	begin();
	setBackgroundMode(Qt::TransparentMode);
	setCompositionMode(QPainter::CompositionMode_Source);
	setMyPen(true);	
	drawRect(rect);
	end();
}
void QTOverlay::DrawLine(QLine line)
{
	begin();
	setBackgroundMode(Qt::TransparentMode);
	setCompositionMode(QPainter::CompositionMode_SourceOver);
	setMyPen();
	line = ClipLine(line);
	drawLine(line);
	end();
}
void QTOverlay::UndrawLine(QLine line)
{
	begin();
	setBackgroundMode(Qt::TransparentMode);
	setCompositionMode(QPainter::CompositionMode_Source);
	setMyPen(true);
	drawLine(ClipLine(line));
	end();
}
void QTOverlay::DrawEllipse(QRect OuterRect)
{
	begin();
	setBackgroundMode(Qt::TransparentMode);
	setCompositionMode(QPainter::CompositionMode_SourceOver);
	setMyPen();
	drawEllipse(OuterRect);
	end();
}
void QTOverlay::UndrawEllipse(QRect OuterRect)
{
	begin();
	setBackgroundMode(Qt::TransparentMode);
	setCompositionMode(QPainter::CompositionMode_Source);
	setMyPen(true);
	drawEllipse(OuterRect);
	end();
}
void QTOverlay::DrawCross(QPoint Point)
{
	QLine line;
	begin();
	setBackgroundMode(Qt::TransparentMode);
	setCompositionMode(QPainter::CompositionMode_SourceOver);
	setMyPen();
	line = QLine(Point.x() - 5,Point.y(),
		         Point.x() + 5,Point.y());
	drawLine(ClipLine(line));
	line = QLine(Point.x()    ,Point.y() - 5,
		         Point.x()    ,Point.y() + 5);
	drawLine(ClipLine(line));
	end();
}
void QTOverlay::UndrawCross(QPoint Point)
{
	QLine line;
	begin();
	setBackgroundMode(Qt::TransparentMode);
	setCompositionMode(QPainter::CompositionMode_Source);
	setMyPen(true);
	line = QLine(Point.x() - 5,Point.y(),
		         Point.x() + 5,Point.y());
	drawLine(ClipLine(line));
	line = QLine(Point.x()    ,Point.y() - 5,
		         Point.x()    ,Point.y() + 5);
	drawLine(ClipLine(line));
	end();
}
// LineCode returns a four bit code as to where relative to the boundaries the point lies
#define LEFT 1		// 0001
#define RIGHT 2		// 0010
#define BOTTOM 4	// 0100
#define TOP 8		// 1000
static int LineCode(int x, int y, int BoxX, int BoxY, int BoxWidth, int BoxHeight)
{
	int c=0;
	if(x < BoxX)
	{
		c = LEFT;  // 0001
	}
	else
	{
		if(x>(BoxX + BoxWidth))
		{
			c = RIGHT;// 0010
		}
	}
	if(y < BoxY)
		c += TOP;  // 1000 graphic box top is y min
	else
		if(y > (BoxY + BoxHeight))
			c += BOTTOM;  //0100
	return c;
}

// Uses line codes to clip a line to be within a box

QLine QTOverlay::ClipLine(QLine LineIn)
{
	QPoint P1, P2;
	P1 = LineIn.p1();
	P2 = LineIn.p2();
	int X,Y;
	int CodeFrom, CodeTo, CodeTemp;
	int BoxinX, BoxinY, BoxinWidth, BoxinHeight;
	BoxinX = BoxinY = 1;
	BoxinWidth = m_pPixmap->width() - 2;
	BoxinHeight = m_pPixmap->height() - 2;

	CodeFrom = LineCode(P1.x(), P1.y(), BoxinX, BoxinY, BoxinWidth, BoxinHeight);
	CodeTo = LineCode(P2.x(), P2.y(),  BoxinX, BoxinY, BoxinWidth, BoxinHeight);
	while((CodeFrom != 0) || (CodeTo != 0))
	{
		if((CodeFrom * CodeTo) != 0) break;
		if((CodeTemp = CodeFrom) == 0) CodeTemp = CodeTo;
		if(CodeTemp & LEFT)	// crosses left edge
		{
			// clip edge
			Y = P1.y() + (P2.y() - P1.y())*(BoxinX - P1.x())/(P2.x() - P1.x());
			X = BoxinX;
		}
		else
			if(CodeTemp & RIGHT)	// Crosses right edge
			{
			// clip edge
			Y = P1.y() + (P2.y() - P1.y())*(BoxinX+BoxinWidth - P1.x())/(P2.x() - P1.x());
			X = BoxinX+BoxinWidth;
			}
			else
				if(CodeTemp & TOP)	// Crosses Top Edge
				{
				// clip edge
				X = P1.x() + (P2.x() - P1.x())*(BoxinY - P1.y())/(P2.y() - P1.y());
				Y = BoxinY;
				}
				else
					if(CodeTemp & BOTTOM)	// Crosses bottom edge
					{
					// clip edge
					X = P1.x() + (P2.x() - P1.x())*(BoxinY+BoxinHeight - P1.y())/(P2.y() - P1.y());
					Y = BoxinY+BoxinHeight;
					}
		// re-check for more need to clip
		if(CodeTemp == CodeFrom)
		{
			P1.setX(X); P1.setY(Y);
			CodeFrom = LineCode(X,Y, BoxinX, BoxinY, BoxinWidth, BoxinHeight);
		}
		else
		{
			P2.setX(X); P2.setY(Y);
			CodeTo = LineCode(X,Y, BoxinX, BoxinY, BoxinWidth, BoxinHeight);
		}
		
	}
	return (QLine(P1,P2));
}
