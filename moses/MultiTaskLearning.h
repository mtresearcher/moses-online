/*
 * MultiTaskLearning.h
 *
 *  Created on: Apr 8, 2014
 *      Author: prashant
 */

#ifndef MULTITASKLEARNING_H_
#define MULTITASKLEARNING_H_

namespace Moses {

class MultiTaskLearning {

// interaction matrix

private:
	std::map<int, std::map<int, float> > intMatrix;
	int users;

public:
	int GetNumberOfTasks(){return users;};
	float GetLearningRate(int, int);
	MultiTaskLearning(std::map<int, std::map<int, float> >, int);
	virtual ~MultiTaskLearning();
};

} /* namespace Moses */
#endif /* MULTITASKLEARNING_H_ */
