/* 
 * File:   OnlineReordering.cpp
 * Author: prashant
 * 
 * Created on July 4, 2013, 5:45 PM
 */

#include "OnlineReordering.h"
#include <stdio.h>
#include "Util.h"
#include <string>
#define MAX_BUFFER 2048

namespace Moses {

OnlineReordering::OnlineReordering():StatefulFeatureFunction("OnlineReordering",1) {

}

OnlineReordering::~OnlineReordering() {
}

void OnlineReordering::CaptureTerAndRecord(std::string src, std::string trg)
{
	char str [MAX_BUFFER];
	char buffer[MAX_BUFFER];
    sprintf(str,"tercpp.0.6.2 -rSent \"%s\" -hSent \"%s\" --noTxtIds --printAlignmentsToSTDO", src.c_str(), trg.c_str());
	FILE *fp=popen(str, "r");
	if(fp!=NULL)
	{
		while (fgets(buffer, MAX_BUFFER, fp) != NULL)
		{
			if(buffer[0]=='\n')
				break;
			std::string str(buffer);
			int loc1=str.find('|||', 0);
			std::string targetWord = str.substr(0,loc1);
			std::string relPos = str.substr(loc1+3, str.size());
			targetWord=Trim(targetWord);
			relPos = str.substr(loc1+3);
			relPos=Trim(relPos);
			int relPosition=atoi(relPos.c_str());
			// do some checking here first
			m_reo[targetWord]=relPosition;
		}
	}
	pclose(fp);
}

FFState* OnlineReordering::Evaluate(const Hypothesis& cur_hypo, const FFState* prev_state, ScoreComponentCollection* accumulator) const
{
	FFState* x=NULL;

	return x;
}

FFState* OnlineReordering::EvaluateChart(const ChartHypothesis& , int featureID, ScoreComponentCollection* accumulator) const
{
	FFState* x=NULL;

	return x;
}

//! return the state associated with the empty hypothesis for a given sentence
FFState* OnlineReordering::EmptyHypothesisState(const InputType &input) const
{
	FFState* x=NULL;

	return x;
}


}
