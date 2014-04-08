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
#include <map>
#include <vector>

using namespace std;

namespace Moses {

class MultiTaskLearning {

// interaction matrix

	map<int, map<int, float> > m_intMatrix;
	int m_users;
	map<int, ScoreComponentCollection> m_user2weightvec;

public:
	int GetNumberOfTasks(){return m_users;};
	void SetInteractionMatrix(int x, int y, float value){if(x<m_users && y < m_users) m_intMatrix[x][y]=value;};
	float GetLearningRate(int, int);
	ScoreComponentCollection GetWeightsVector(int);
	void SetWeightsVector(int, ScoreComponentCollection);
	MultiTaskLearning(int);
	virtual ~MultiTaskLearning();
};

} /* namespace Moses */
#endif /* MULTITASKLEARNING_H_ */
