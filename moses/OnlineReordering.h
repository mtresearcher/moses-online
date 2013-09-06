/* 
 * File:   OnlineReordering.h
 * Author: prashant
 *
 * Created on July 4, 2013, 5:45 PM
 */

#ifndef ONLINEREORDERING_H
#define	ONLINEREORDERING_H

#include "Util.h"
#include "TypeDef.h"
#include "FeatureFunction.h"
#include "Hypothesis.h"
#include "Factor.h"
#include "TrellisPath.h"
#include "TrellisPathList.h"
#include "Manager.h"

namespace Moses {


class OnlineReordering : public StatefulFeatureFunction {

public:
	FFState* Evaluate(
	    const Hypothesis& cur_hypo,
	    const FFState* prev_state,
	    ScoreComponentCollection* accumulator) const;

	FFState* EvaluateChart(
	    const ChartHypothesis& /* cur_hypo */,
	    int /* featureID - used to index the state in the previous hypotheses */,
	    ScoreComponentCollection* accumulator) const;

	//! return the state associated with the empty hypothesis for a given sentence
	FFState* EmptyHypothesisState(const InputType &input) const;

    OnlineReordering();
    virtual ~OnlineReordering();
    void CaptureTerAndRecord(std::string src, std::string trg);
    inline std::string GetScoreProducerWeightShortName(unsigned) const { return "orm"; };
private:
    std::map<std::string, int> m_reo;
};

}
#endif	/* ONLINEREORDERING_H */

