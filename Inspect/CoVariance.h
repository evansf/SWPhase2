#pragma once
#include "opencv2/core/core.hpp"

class CCoVariance
{
public:
	CCoVariance(void);
	~CCoVariance(void);
	void Create(float *firstdata, int count);	// initialize with one for incremental
	void Create(std::vector<std::vector<float>> samples);	// create covar for these samples
	void Create(cv::Mat &samples);	// create covar for these samples
	void IncrementalUpdate(float *data, int count);
	void IncrementalUpdate(unsigned char *data, int count);
	void Merge(CCoVariance &covar);
	int Invert();	// 0 = OK, after this operation, no more updates
	cv::Mat &InverseMatrix();
	cv::Mat &Means() {return m_Mean;};
	int Samples(){return m_Samples;};

	float MahalanobisRaw(float *data);
	float MahalanobisRaw(unsigned char *data);
	float MahalanobisRaw(cv::Mat &data);
	float MahalanobisScore(float raw, double expDiv = 200);
	cv::Mat *m_pM12;
	double m_detC;

private:
	cv::Mat m_Mean;
	cv::Mat m_CovarInverse;
	int m_Samples;

};

