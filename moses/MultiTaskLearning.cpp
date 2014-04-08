/*
 * MultiTaskLearning.cpp
 *
 *  Created on: Apr 8, 2014
 *      Author: prashant
 */

#include "MultiTaskLearning.h"

namespace Moses {

float MultiTaskLearning::GetLearningRate(int x, int y){
	if(m_intMatrix.find(x) != m_intMatrix.end()){
		if(m_intMatrix[x].find(y) != m_intMatrix[x].end()){
			return m_intMatrix[x][y];
		}
		else{
			UserMessage::Add("false value passed to find the learning rate");
		}
	}
	else{
		UserMessage::Add("false value passed to find the learning rate");
	}
	return 0;
}

ScoreComponentCollection MultiTaskLearning::GetWeightsVector(int user){
	if(m_user2weightvec.find(user) != m_user2weightvec.end()){
		return m_user2weightvec[user];
	}
	else{
		UserMessage::Add("Requested weights from a wrong user");
	}
}

void MultiTaskLearning::SetWeightsVector(int user, ScoreComponentCollection weightVec){
	if(user <= m_users){
		m_user2weightvec[user] = weightVec;
	}
	return;
}

MultiTaskLearning::MultiTaskLearning(std::map<int, std::map<int, double> > matrix, int tasks) {
	m_users=tasks;
	m_intMatrix=matrix;
}

MultiTaskLearning::~MultiTaskLearning() {
}

} /* namespace Moses */
