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

using namespace std;

namespace Moses {

class MultiTaskLearning {

// interaction matrix

	map<int, vector<float> > m_intMatrix;
	int m_users;
	map<int, ScoreComponentCollection> m_user2weightvec;
	int m_currtask;

public:
	const int GetNumberOfTasks() const {return m_users;};
	void SetCurrentTask(int taskid){m_currtask=taskid;};
	int GetCurrentTask() const {return m_currtask;};
	void SetInteractionMatrix(int x, vector<float>& values);
	vector<float> GetLearningRate(int);
	ScoreComponentCollection GetWeightsVector(int);
	void SetWeightsVector(int, ScoreComponentCollection);
	MultiTaskLearning(int);
	virtual ~MultiTaskLearning();
};

} /* namespace Moses */
#endif /* MULTITASKLEARNING_H_ */
