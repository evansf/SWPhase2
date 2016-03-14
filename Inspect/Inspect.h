#ifndef INSPECT_HEADER
#define INSPECT_HEADER
// input images should be written to a file as a PNG format file.
// This interface intentionally does not understand OPENCV data objects

#include <string>
#include <vector>
#include <array>
#include "SISUtility.h"

using namespace std;

extern int cvType[];

#define FILENAMESIZE 1000
class CInspectPrivate;
class CInspectData;
#define NUMINSPECTIONS 10

class INSPECTAPI CInspect
{
public:
	// include error typedef
	#include "InspectErrors.h"
	typedef enum datatype
	{
		DCT,
		INTENSITY,
		WAVE,
		MOM,
		LASTENUM
	} DATATYPE;
public:
	CInspect(SISParams &params);
	~CInspect(void);

	ERR_INSP create(DATATYPE type, CInspectImage image, CInspectImage mask,
		int tilewidth, int tileheight, int stepcols=0, int steprows=0);
	ERR_INSP train(CInspectImage image);
	ERR_INSP trainEx(CInspectImage image);	// extends training for overlay marked tiles
	ERR_INSP optimize();
	ERR_INSP inspect(CInspectImage image, int *pFailIndex=NULL);
	double getRawScore(int *pInspectIndex=NULL, int *pX=NULL, int *pY=NULL);	// most recent inspect[i], x,y of worst
	double getFinalScore();
	double getMean(int InspectIndex = 0);
	double getSigma(int InspectIndex = 0);
	bool isOptimized(){return m_isOptimized;};
	void setRawScoreDiv(float g);
	void setLearnThresh(float t);
	void setInspectThresh(float t);
	void setMustAlign(bool b);
	// draws a rectangle on the inspection overlay
	void DrawRect( int x, int y, int width, int height, unsigned long xRGB);

	std::string &getErrorString(ERR_INSP err);
	CInspectImage getOverlayImage();
	CInspectImage getAlignedImage();
//	std::vector<std::array<float,2>> getHistogramRaw();
//	std::vector<std::array<float,2>> getHistogramScores();
	double getOpTime(){return m_OpTime;};	// gets elapsed time (sec) for most recent method call

protected:
	SISParams &m_params;	// inspection parameters from file
		// array of pointers to up to NUMINSPECTIONS specific inspections
	CInspectPrivate *m_pPrivate[NUMINSPECTIONS];
	int m_InspectionCount;	// number of inspect objects
	CInspectData *m_pDataGlobal;
private:
	bool m_isOptimized;
	double m_OpTime;
	double m_TotalScore;
	int m_FailX, m_FailY;
	int m_WorstInspection;
};

#endif // INSPECT_HEADER
