/* 
 * File:   SingleTriggerModel.h
 * Author: prashant
 *
 * Created on August 13, 2013, 5:21 PM
 */

#ifndef SINGLETRIGGERMODEL_H
#define	SINGLETRIGGERMODEL_H

#include "FeatureFunction.h"
#include "StaticData.h"
#include "Util.h"
#include "util/file_piece.hh"
#include "util/tokenize_piece.hh"
#include "UserMessage.h"
#include <string.h>
#include <sstream>

using namespace std;

typedef string trigger;
typedef string word;

namespace Moses {

class SingleTriggerModel : public StatelessFeatureFunction {
    public:
        SingleTriggerModel(std::string file, bool m_param);
        virtual ~SingleTriggerModel();
        void Evaluate(const PhraseBasedFeatureContext& context,	ScoreComponentCollection* accumulator) const;
        void EvaluateChart(const ChartBasedFeatureContext& context, ScoreComponentCollection* accumulator) const;
        void Read(const std::string filename);
        void SetSentence(std::string sent);
        inline std::string GetScoreProducerWeightShortName(unsigned) const { return "stm"; };
        float getScore(std::string s, std::string t);
    private:
        void Evaluate(const TargetPhrase& tp, ScoreComponentCollection* out) const;
        std::string m_sentence;
        std::map<std::string, std::map<std::string, float> > m_stm;
        bool m_sigmoidParam;
};
}
#endif	/* SINGLETRIGGERMODEL_H */

