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
// this is a stateless function because we need to add an additional bias feature
class MultiTaskLearning : public StatelessFeatureFunction {
	int m_users;
	map<int, ScoreComponentCollection> m_user2weightvec;
	int m_currtask;
	boost::numeric::ublas::matrix<double> m_kdkdmatrix;
	boost::numeric::ublas::matrix<double> m_intMatrix;

public:
	const int GetNumberOfTasks() const {return m_users;};
	void SetCurrentTask(int taskid){m_currtask=taskid;};
	int GetCurrentTask() const {return m_currtask;};
	void SetInteractionMatrix(boost::numeric::ublas::matrix<double>& interactionMatrix);
	boost::numeric::ublas::matrix<double>& GetInteractionMatrix(){return m_intMatrix;};
	void SetKdKdMatrix(boost::numeric::ublas::matrix<double>&);
	boost::numeric::ublas::matrix<double>& GetKdKdMatrix(){return m_kdkdmatrix;};
	ScoreComponentCollection GetWeightsVector(int);
	void SetWeightsVector(int, ScoreComponentCollection);
	MultiTaskLearning(int);
	inline std::string GetScoreProducerWeightShortName(unsigned) const { return "mtl"; };
	void Evaluate(const PhraseBasedFeatureContext& context,	ScoreComponentCollection* accumulator) const;
	void EvaluateChart(const ChartBasedFeatureContext& context, ScoreComponentCollection* accumulator) const;
	virtual ~MultiTaskLearning();
};

} /* namespace Moses */
#endif /* MULTITASKLEARNING_H_ */
