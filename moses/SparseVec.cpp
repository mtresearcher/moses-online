/*
 * SparseVec.cpp
 *
 *  Created on: May 8, 2013
 *      Author: prashant
 */

#include "SparseVec.h"
#include "Util.h"
#include <functional>
#include <numeric>
namespace Moses {

// empty constructor
SparseVec::SparseVec() {}
// copy constructor
SparseVec::SparseVec(const SparseVec& sv)
{
	m_SparseVector = sv.GetSparseVector();
}
// constructor based on size, initialize elements to zero
SparseVec::SparseVec(int size)
{
	m_SparseVector.resize(size);
}

float SparseVec::GetL1Norm()
{
	float sum=0;
	for (size_t i=0;i<m_SparseVector.size(); i++)
		sum+=m_SparseVector[i];
	return sum;
}

const std::vector<float> SparseVec::GetSparseVector() const
{
	return m_SparseVector;
}
void SparseVec::Assign(int idx, float val)
{
	m_SparseVector[idx]=val;
}
const size_t SparseVec::GetSize() const
{
	return m_SparseVector.size();
}
const float SparseVec::getElement(const int idx) const
{
	return m_SparseVector[idx];
}
void SparseVec::MultiplyEquals(std::vector<int>& IdxVec, std::vector<float>& Val)
{
	CHECK(IdxVec.size()==Val.size());
	for(int i=0; i<IdxVec.size(); i++)
	{
		MultiplyEqualsFeat(IdxVec[i], Val[i]);
	}
}
void SparseVec::MultiplyEquals(const float& val)
{
	*this *= val;
}
void SparseVec::PlusEquals(SparseVec& sv)
{
	CHECK(m_SparseVector.size() == sv.GetSize());
	for(size_t i=0;i<m_SparseVector.size(); i++)
	{
		m_SparseVector[i] += sv.getElement(i);
	}
}
void SparseVec::PlusEquals(std::vector<int>& IdxVec, std::vector<float>& Val)
{
	CHECK(IdxVec.size()==Val.size());
	for(int i=0; i<IdxVec.size(); i++)
	{
		PlusEqualsFeat(IdxVec[i], Val[i]);
	}
}
void SparseVec::MinusEquals(std::vector<int>& IdxVec, std::vector<float>& Val)
{
	CHECK(IdxVec.size()==Val.size());
	for(int i=0; i<IdxVec.size(); i++)
	{
		MinusEqualsFeat(IdxVec[i], Val[i]);
	}
}
void SparseVec::PlusEqualsFeat(int idx, float val)
{
	m_SparseVector[idx] += val;
	return;
}
void SparseVec::MinusEqualsFeat(int idx, float val)
{
	m_SparseVector[idx] -= val;
	return;
}
void SparseVec::MultiplyEqualsFeat(int idx, float scale)
{
	m_SparseVector[idx] *= scale;
	return;
}

void SparseVec::AddFeat(int idx, float val)
{
	m_SparseVector[idx]=val;
	return;
}

int SparseVec::AddFeat(float val)
{
	m_SparseVector.push_back(val);
	return m_SparseVector.size();
}
SparseVec& SparseVec::operator*= (const float& rhs) {
	//NB Could do this with boost::bind ?
	for(size_t i=0;i<m_SparseVector.size();i++)
	{
		m_SparseVector[i] *= rhs;
	}
	return *this;
}

} /* namespace SparseVector */