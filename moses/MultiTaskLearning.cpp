/*
 * MultiTaskLearning.cpp
 *
 *  Created on: Apr 8, 2014
 *      Author: prashant
 */

#include "MultiTaskLearning.h"

namespace Moses {

float  MultiTaskLearning::GetLearningRate(int x, int y){
	if(intMatrix.find(x) != intMatrix.end()){
		if(intMatrix[x].find(y) != intMatrix[x].end()){
			return intMatrix[x][y];
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

MultiTaskLearning::MultiTaskLearning(std::map<int, std::map<int, float> > matrix, int tasks) {
	// TODO Auto-generated constructor stub
	users=tasks;
	intMatrix=matrix;
}

MultiTaskLearning::~MultiTaskLearning() {
	// TODO Auto-generated destructor stub
}

} /* namespace Moses */
