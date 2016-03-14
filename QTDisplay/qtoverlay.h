#pragma once
#include <QtGui\QPainter>
#include <QtGui\QColor>
#include "qtdisplay_global.h"
class QPixmap;
class QTDisplay;

class QTDISPLAY_EXPORT QTOverlay :
	public QPainter
{
public:
	QTOverlay(void);
	void SetDisplay(QTDisplay* pDisplay);
	void SetPixmapSize(QSize size);
	void SetLinesize(int s);
	bool begin(void);
	bool end(void);
	void Update(void);
	QPixmap* GetPixmapPointer(void);
	void GetPixmap(QPixmap &PMap, QRect rect);
	void Clear(void);
	void SetColor(QColor color);
	void DrawPixmap(QPixmap &img, QPoint p);
	void DrawPoint(QPoint point);
	void UndrawPoint(QPoint point);
	void DrawRect(QRect rect);
	void UndrawRect(QRect rect);
	void DrawLine(QLine line);
	void UndrawLine(QLine line);
	void DrawEllipse(QRect OuterRect);
	void UndrawEllipse(QRect OuterRect);
	void DrawCross(QPoint Point);
	void UndrawCross(QPoint Point);

	QPicture* m_pPicture;
	QColor m_Color;

public:
	~QTOverlay(void);
protected:
	QLine ClipLine(QLine LineIn);
	QPixmap *m_pPixmap;
	QTDisplay *m_pDisplay;
	int m_linesize;
	void setMyPen(bool undraw = false);
public:
	void drawPicture(void);
};
