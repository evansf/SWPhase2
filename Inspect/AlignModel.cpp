#include "stdafx.h"
#include "Inspect.h"

CAlignModel::~CAlignModel(void)
{
}

bool CAlignModel::contains(int x, int y)
{
	if(    (m_X < x) && (x < (m_X+m_Width))
		&& (m_Y < y) && (y < (m_Y+m_Height))  )
		return true;
	else
		return false;
}

void CAlignModel::save(FILE *pFile)
{
	fprintf(pFile,"%d,%d,%d,%d\n",(int)(m_X),(int)(m_Y),(int)m_Width,(int)m_Height);
}