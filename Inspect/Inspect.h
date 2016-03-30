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
	ERR_INSP trainEx(int failIndex);	// train data at index
	ERR_INSP optimize();
	ERR_INSP inspect(CInspectImage image, int *FailedInspectIndex=NULL, int *pFailTileIndex=NULL);

	void MarkTileSkip(int index);	// Find new worst score will skip this index
	ERR_INSP FindNewWorstScore(int *pfailIndex=NULL);	// failIndex = -1 if no more failures

	bool   isOptimized(){return m_isOptimized;};
	double getRawScore(int *pInspectIndex=NULL);	// most recent inspect[i], x,y of worst
	double getDScore(int *pInspectIndex=NULL);
	double getZScore(int *pInspectIndex=NULL);
	void   getXY(int index, int *pX, int *pY);
	double getTrueScore();	// just score of worst
	double getMean(int InspectIndex = 0);
	double getSigma(int InspectIndex = 0);
	double getOpTime(){return m_OpTime;};	// gets elapsed time (sec) for most recent method call
	std::string &getErrorString(ERR_INSP err);
	CInspectImage getOverlayImage();
	CInspectImage getAlignedImage();

	void setRawScoreDiv(float g);
	void setLearnThresh(float t);
	void setInspectThresh(float t);
	void setMustAlign(bool b);
	void setUseZScoreMode(bool b);
	void setPass2(bool b);

	// draws a rectangle on the inspection overlay
	void DrawRect( int x, int y, int width, int height, unsigned long xRGB);

protected:
		// inspection parameters from file
	SISParams &m_params;
		// array of pointers to up to NUMINSPECTIONS specific inspections
	CInspectPrivate *m_pPrivate[NUMINSPECTIONS];

	int m_InspectionCount;	// number of inspect objects
	double m_Threshold;
	CInspectData *m_pDataGlobal;	// pointer to Runtime Data
	bool m_ZScoreMode;
	bool m_Pass2;

private:
	bool m_isOptimized;
	double m_OpTime;
	double m_ZScore;
	double m_TrueScore;
	int m_FailIndex;	// tile index
	int m_WorstInspection;	// inspection type index
};

#endif // INSPECT_HEADER
