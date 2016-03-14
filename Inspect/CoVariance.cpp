#include "CoVariance.h"
#include "opencv2/core/core.hpp"


CCoVariance::CCoVariance(void)
{
	m_pM12 = NULL;
	m_Samples = 0;
}


CCoVariance::~CCoVariance(void)
{
	if(m_pM12 != NULL) delete m_pM12;
}

// call Create Once to start the analysis
void CCoVariance::Create(float *data, int count)
{
	if(m_pM12 != NULL)
		delete m_pM12;
	m_Mean = cv::Mat(1,count,CV_32F,data).clone();
	// use initial data as mean estimate (i.e. sample # 1)
	m_Samples = 1;
	m_pM12 = new cv::Mat;
	m_pM12->create(count,count,CV_32F);
	*m_pM12 = 0.0F;	// variance of one sample is 0.0
}
// call Create with a vector of samples to create all at once
void CCoVariance::Create(std::vector<std::vector<float>> samples)
{
	if(samples.size() < 1) return;
	int count = samples[0].size();
	if(m_pM12 == NULL)
	{
		m_pM12 = new cv::Mat;
		m_pM12->create(count,count,CV_32F);
	}
	cv::calcCovarMatrix(samples,*m_pM12,m_Mean,cv::COVAR_NORMAL,CV_32F);
}
// call Create with a vector of samples to create all at once
void CCoVariance::Create(cv::Mat &samples)
{
	if(samples.empty()) return;
	int count = samples.cols;
	if(m_pM12 == NULL)
	{
		m_pM12 = new cv::Mat;
		m_pM12->create(count,count,CV_32F);
	}
	cv::calcCovarMatrix(samples,*m_pM12,m_Mean,cv::COVAR_NORMAL|cv::COVAR_ROWS,CV_32F);
}

// each call updates the means and updates the M12 
void CCoVariance::IncrementalUpdate(float* data, int count)
{
	if(m_Samples == 0)
	{
		Create(data,count);
		m_Samples --;	// create without calculating covar
	}
	float factor = 1.0F / (m_Samples+1);	// this is 1 / N
	cv::Mat Data(1,count,CV_32F,data);	// Data now contains data

	cv::Mat delta = Data.clone();	// delta now contains data
	delta -= m_Mean;	// delta now contans (newX - oldmean)	
	m_Mean = factor * ( (m_Samples * m_Mean) + Data);

	float *pF, *pD;
	pD = (float*)delta.ptr();
	for(int i=0; i<count; i++)
	{
		pF = (float*)m_pM12->ptr(i);
		for(int j=0; j<count; j++)
		{
			// update the i,jth element of M12
			// C[n](xy) = (C[n-1](X,Y) * (N-1) + ((N-1)/N)  * (X[n]-Xbar[n-1]) * (Y[n]-Ybar[n-1]) )
			//            -------------------------------------------------------------------------
			//                                           N
			// m_Samples is (N-1)  and factor is (1/N)
			pF[j] = factor * ( m_Samples * (pF[j] + factor * ( pD[i] * pD[j] )) );
		}
	}

	m_Samples++;
}
void CCoVariance::IncrementalUpdate(unsigned char* data, int count)
{
	float *pF = new float[count];
	for(int i=0; i<count; i++)
		pF[i] = (float)data[i];
	IncrementalUpdate(pF, count);
}
int CCoVariance::Invert()
{
	if(m_pM12 == NULL)
	{
		return -1;
	}
	cv::Mat covar = (*m_pM12);
	m_detC = cv::determinant(covar);
	if(m_detC <= FLT_EPSILON)
	{
		float *pF;
		for(int i=0; i<m_pM12->rows; i++)	// only need to cheat by making diagonal non-zero
		{
			pF = (float*)m_pM12->ptr(i);
			pF[i] = abs(pF[i]) <= FLT_EPSILON ? 0.1F : pF[i];
		}
	}
	if(cv::invert(covar,m_CovarInverse,cv::DECOMP_SVD) == 0.0)
	{
		m_CovarInverse = cv::Mat::eye(m_CovarInverse.size(),CV_32F);
		return -1;
	}
	return 0;
}

cv::Mat & CCoVariance::InverseMatrix()
{
	return m_CovarInverse;
}

void CCoVariance::Merge(CCoVariance & other)
{
	if(m_Mean.cols != other.m_Mean.cols)
	{
		// start out with copy of merge
		m_Mean = other.m_Mean;
		m_pM12 = new cv::Mat(other.m_pM12->size(),other.m_pM12->type());
		*m_pM12 = *(other.m_pM12);
		m_Samples = other.m_Samples;
	}
	else
	{
		int meansize =  m_Mean.cols;
		int count = m_Samples + other.m_Samples;
		float meanXa,meanXb,meanYa,meanYb;
		float* pThisM12, *pOtherM12;
		for(int m=0;m<meansize; m++)
		{
			// get means for this row
			meanYa = ((float*)m_Mean.ptr())[m];
			meanYb = ((float*)other.m_Mean.ptr())[m];
			pThisM12 = (float*)m_pM12->ptr(m);
			pOtherM12 = (float*)other.m_pM12->ptr(m);
			for(int n=0;n<meansize; n++)
			{
				// get means for this column
				meanXa = ((float*)m_Mean.ptr())[n];
				meanXb = ((float*)other.m_Mean.ptr())[n];
				// combine variances and adjust using means
				pThisM12[n] = pThisM12[n] + pOtherM12[n]
						+ (meanXa - meanXb)	* (meanYa - meanYb)
						* m_Samples * other.m_Samples / count ;
			}
			// and combine means
			((float*)m_Mean.ptr())[m] = (float)( meanYa * m_Samples
									+ meanYb * other.m_Samples ) / count;
		}
		m_Samples = count;
	}
}

float CCoVariance::MahalanobisRaw(cv::Mat &data)
{
	assert( ! m_CovarInverse.empty());
	double dRaw = cv::Mahalanobis(m_Mean,data,m_CovarInverse);
	float raw = (float)dRaw;
	// raw now proportional to the square of the "distance"
	return raw;	
}
float CCoVariance::MahalanobisRaw(float *Data)
{
	cv::Mat data(m_Mean.size(),CV_32F);
	return MahalanobisRaw(data);
}
float CCoVariance::MahalanobisRaw(unsigned char *Data)
{
	cv::Mat data(m_Mean.size(),CV_32F);
	float* pF = (float*)data.ptr();
	for(int i=0; i<m_Mean.cols; i++)
		pF[i] = (float)(Data[i]);
	return MahalanobisRaw(data);
}

float CCoVariance::MahalanobisScore(float raw, double expDivisor)
{
	// need a score of 1.0 for perfect and 0.0 for worst

	float factor = (float)(-1.0 / expDivisor);
	float score = 	 exp( factor * raw );

	return score;
}