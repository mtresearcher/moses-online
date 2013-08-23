/* 
 * File:   SingleTriggerModel.h
 * Author: pmathur
 *
 * Created on August 13, 2013, 5:21 PM
 */

#ifndef SINGLETRIGGERMODEL_H
#define	SINGLETRIGGERMODEL_H

#include "FeatureFunction.h"

typedef string trigger;
typedef string word;

namespace Moses {

    class SingleTriggerModel : public StatefulFeatureFunction {
    public:
        SingleTriggerModel();
        virtual ~SingleTriggerModel();
        
        FFState* Evaluate(const Hypothesis& hypo,
                                     const FFState* prev_state,
                                     ScoreComponentCollection* out) const;
        const FFState* SingleTriggerModel::EmptyHypothesisState(const InputType &input) const;
        
    private:
        std::map<word, trigger> m_w2t;
    };
}
#endif	/* SINGLETRIGGERMODEL_H */

