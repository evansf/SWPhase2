#ifndef QTDISPLAY_H
#define QTDISPLAY_H

#include "qtdisplay_global.h"

#include "opencv2\core\core.hpp"


#include <QtCore\QObject>
#include <QtWidgets\QWidget>
#include <QtGui\QColor>
#include <QtGui\QPixmap>
#include <QtCore\QLine>
#include <QtCore\QVector>

class QTOverlay;
class QRubberBand;
class cv::Mat;

class ZoomSettings
{
public:
	ZoomSettings();
	double factor;
	QRect DisplayedRect;
	QRect DrawnRect;
};

class QTDISPLAY_EXPORT QTDisplay :
	public QWidget	// promote Widget to a QTDisplay
{
	Q_OBJECT
public:
	// constructor and widget stuff
	QTDisplay(QWidget* parent = 0);
	~QTDisplay();
	QSize sizeHint(void) const;
	QSize minimumSizeHint() const;

	// User Setup stuff
	void EnableRubberBand(bool Enable);
	void clearZoom();
	void setZoomSettings(const ZoomSettings &settings);
	typedef enum modes
	{
		CROP_SCALE,
		FIT_SCALE,
		FIT_VERTICAL,
		FIT_HORIZONTAL,
		CROP_ONLY
	} DISPLAYMODE;
	DISPLAYMODE m_DisplayMode;
	void setDisplayMode(DISPLAYMODE mode) {m_DisplayMode = mode;};
	void SetPalette(std::vector<QRgb> pPalette);	// allows color image from CV_8UC1
	typedef enum MouseModes
	{
		DISABLED = 0,
		BOX,
		LINE,
		POLY
	} MOUSEMODE;
	void SetRubberBandMode(MOUSEMODE mode) { m_RubberObjectMode = mode;};

	// The USER Stuff
	void AttachMat(cv::Mat *mat);
	void AttachQImage(QImage &img);
	void SetOverlay(QImage &overlay);
	void UpdateRoi(void);
	QRect GetRubberBand();
	void SetSnap(int dist){m_Snap = dist;};
	QRect GetDrawnRect();
	QLine GetDrawnLine();
	QImage *GetImage(){return m_pImage;}; 

	// Overlay Stuff
	void ClearOverlay(void);
	void ShowOverlay(void);
	QTOverlay* GetOverlay();
	QRect RoiToDisplay(QRect InRect);	// Convert Coordinates image to screen
	QPoint RoiToDisplay(QPoint InPoint);
	QRect DisplayToRoi(QRect InRect);
	QPoint DisplayToRoi(QPoint InPoint);
	void SetLinesize(int s);
	void DrawPoint(QPoint, QColor = Qt::red);
	void ErasePoint(QPoint);
	void DrawPixmap(QPixmap &img, QPoint p, QColor = Qt::red);
	void DrawRect(QRect, QColor = Qt::red);
	void EraseRect(QRect);
	void DrawLine(QLine, QColor = Qt::red);
	void EraseLine(QLine);
	void DrawCross(QPoint, QColor = Qt::red);
	void EraseCross(QPoint);
	void DrawEllipse(QRect, QColor = Qt::red);
	void EraseEllipse(QRect);

signals:	// you can attach to these signals as needed
	void MousePress(QMouseEvent *event);
	void MouseMove(QMouseEvent *event);
	void MouseRelease(int);
	void MouseRelease(QMouseEvent *event);
	void clicked(QMouseEvent *event);
	void clicked();
	void rightclicked();
	void PolyKnot(QPoint p);
	void RectDrawn(QRect box);
	void PolyDrawn();

protected:
	// action methods
	void paintEvent(QPaintEvent *event);
	void resizeEvent(QResizeEvent *event);
	void mousePressEvent(QMouseEvent *event);
 	void mouseDoubleClickEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);

private:
	void updateRubberBandRegion();
	void zoomIn();
	void zoomOut();
	void getPhysical(); 
	void UpdatePhysical(void);
	void UnDrawShape(void);
	void ReDrawShape(QColor color = Qt::transparent);
	void drawRubberLine();
	void undrawRubberLine();

	QTOverlay *m_pOverlay;
	cv::Mat m_QImgMat;
	cv::Mat *m_pMat;
	cv::Mat * m_pImageMat;
	cv::Mat *m_pPhysMat;
	QImage *m_pImage;	// the image built on the cv::Mat
	QPixmap *m_pPixmap;	// scaled pixelmap of widget client window
	std::vector<QRgb> ColorTable;	// color palette
	QVector<QRgb> QtColorTable;	// becomes QImage color palette
	int m_MousePressX;
	int m_MousePressY;

	QRubberBand *m_pRubberObject;
	MOUSEMODE m_RubberObjectMode;	// 0 = Box, 1 = line
	bool m_PolyActive;
	QPoint m_RubberOrigin;
	QPoint m_PolyOrigin;
	QPoint m_RubberLineEnd;
	bool m_RubberObjectEnabled;
	QRect m_RubberRect;	// drawn rect or line
	int m_Snap;

	std::vector<ZoomSettings> zoomStack;
	int curZoom;
	
};

#endif // QTDISPLAY_H
