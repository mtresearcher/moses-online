/*
 * MultiTaskLearning.cpp
 *
 *  Created on: Apr 8, 2014
 *      Author: prashant
 */

#include "MultiTaskLearning.h"
#include "StaticData.h"

namespace Moses {

void MultiTaskLearning::Evaluate(const PhraseBasedFeatureContext& context, ScoreComponentCollection* accumulator) const
{
	accumulator->Assign(this, 1);
}

void MultiTaskLearning::EvaluateChart(const ChartBasedFeatureContext& context, ScoreComponentCollection* accumulator) const
{
	accumulator->Assign(this, 1);
}

void MultiTaskLearning::SetInteractionMatrix(boost::numeric::ublas::matrix<double>& interactionMatrix){
	m_intMatrix = interactionMatrix;
}

void MultiTaskLearning::SetKdKdMatrix(boost::numeric::ublas::matrix<double>& kdkdmatrix){
	m_kdkdmatrix=kdkdmatrix;
}

ScoreComponentCollection MultiTaskLearning::GetWeightsVector(int user) {
	if(m_user2weightvec.find(user) != m_user2weightvec.end()){
		return m_user2weightvec[user];
	}
	else{
		UserMessage::Add("Requested weights from a wrong user");
	}
}

void MultiTaskLearning::SetWeightsVector(int user, ScoreComponentCollection weightVec){
	if(user < m_users){	// < because the indexing starts from 0
		m_user2weightvec[user] = weightVec;
	}
	return;
}

MultiTaskLearning::MultiTaskLearning(int tasks):StatelessFeatureFunction("MultiTaskLearning",1) {
	m_users=tasks;
}

MultiTaskLearning::~MultiTaskLearning() {
}

} /* namespace Moses */
