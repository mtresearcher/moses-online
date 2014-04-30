/*
 * MultiTaskLearning.cpp
 *
 *  Created on: Apr 8, 2014
 *      Author: prashant
 */

#include "MultiTaskLearning.h"


namespace Moses {

vector<float> MultiTaskLearning::GetLearningRate(int task){
	if(m_intMatrix.find(task) != m_intMatrix.end()){
		return m_intMatrix[task];
	}
	else{
		UserMessage::Add("false value passed to find the learning rate");
	}
}

void MultiTaskLearning::SetInteractionMatrix(int user, vector<float>& values){
	if(user<m_users && values.size() == m_users){
		for(int i=0; i<values.size(); i++){
			cerr<<"Learning Rates : "<<values[i]<<endl;
			m_intMatrix[user].push_back(values[i]);
		}
	}
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

MultiTaskLearning::MultiTaskLearning(int tasks) {
	m_users=tasks;
}

MultiTaskLearning::~MultiTaskLearning() {
}

} /* namespace Moses */
