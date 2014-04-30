/*
 * MultiTaskLearning.h
 *
 *  Created on: Apr 8, 2014
 *      Author: prashant
 */

#ifndef MULTITASKLEARNING_H_
#define MULTITASKLEARNING_H_

#include "UserMessage.h"
#include "ScoreComponentCollection.h"
#include "Util.h"
#include <map>
#include <vector>

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/vector_proxy.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/triangular.hpp>
#include <boost/numeric/ublas/lu.hpp>

using namespace std;

namespace Moses {

class MultiTaskLearning {

// interaction matrix

	map<int, vector<float> > m_intMatrix;
	int m_users;
	map<int, ScoreComponentCollection> m_user2weightvec;
	int m_currtask;
	boost::numeric::ublas::matrix<double> m_kdkdmatrix;

public:
	const int GetNumberOfTasks() const {return m_users;};
	void SetCurrentTask(int taskid){m_currtask=taskid;};
	int GetCurrentTask() const {return m_currtask;};
	void SetInteractionMatrix(int x, vector<float>& values);
	void SetKdKdMatrix(boost::numeric::ublas::matrix<double>&);
	boost::numeric::ublas::matrix<double>& GetKdKdMatrix(){return m_kdkdmatrix;};
	vector<float> GetLearningRate(int);
	ScoreComponentCollection GetWeightsVector(int);
	void SetWeightsVector(int, ScoreComponentCollection);
	MultiTaskLearning(int);
	virtual ~MultiTaskLearning();
};

} /* namespace Moses */
#endif /* MULTITASKLEARNING_H_ */
