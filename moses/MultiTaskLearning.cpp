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

boost::numeric::ublas::matrix<double> MultiTaskLearning::GetWeightsMatrix() {
	boost::numeric::ublas::matrix<double> A(m_user2weightvec.begin()->second.Size(), m_users);
	for(int i=0;i<m_users;i++){
		FVector weightVector = m_user2weightvec[i].GetScoresVector();
		weightVector.printCoreFeatures();
		const std::valarray<float>& scoreVector = m_user2weightvec[i].GetScoresVector().getCoreFeatures();
		for(size_t j=0; j<scoreVector.size(); j++){
			A(j,i) = scoreVector[j];
		}
	}
	return A;
}

void MultiTaskLearning::SetWeightsVector(int user, ScoreComponentCollection weightVec){
	if(user < m_users){	// < because the indexing starts from 0
		m_user2weightvec[user] = weightVec;
	}
	return;
}

MultiTaskLearning::MultiTaskLearning(int tasks, float learningrate):StatelessFeatureFunction("MultiTaskLearning",1) {
	m_users=tasks;
	m_learningrate=learningrate;
	m_learnmatrix=true;
}

MultiTaskLearning::MultiTaskLearning(int tasks):StatelessFeatureFunction("MultiTaskLearning",1) {
	m_users=tasks;
	m_learnmatrix=false;
}

MultiTaskLearning::~MultiTaskLearning() {
}

} /* namespace Moses */
