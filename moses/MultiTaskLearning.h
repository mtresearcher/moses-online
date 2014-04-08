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
	float GetLearningRate(int, int);
	ScoreComponentCollection GetWeightsVector(int);
	void SetWeightsVector(int, ScoreComponentCollection);
	MultiTaskLearning(map<int, map<int, double> >, int);
	virtual ~MultiTaskLearning();
};

} /* namespace Moses */
#endif /* MULTITASKLEARNING_H_ */
