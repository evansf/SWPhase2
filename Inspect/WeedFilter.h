#pragma once
#include "opencv2/core/core.hpp"
namespace WEEDS
{
// CWeedFilter implenents a fast Lookup to convert BGR or BGRA images
// to a float image implementing the Foliage transform.
// Alternatively, The original image can also be output masked by 
// the thresholded foliage image value
class CWeedFilter
{
public:
	CWeedFilter(void);
	~CWeedFilter(void);
	void filter2float(cv::Mat &src, cv::Mat &dst, float threshold = 0);
	void filterMasked(cv::Mat &src, cv::Mat &dst, float threshold);
	void CreateLUT(float threshold = 0);

private:
	float m_Threshold;
	static float LUT[256*256];	// lookup table filled at create
};
};	// namespace WEEDS
