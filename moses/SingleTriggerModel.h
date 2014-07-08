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
        SingleTriggerModel(bool m_param);
        virtual ~SingleTriggerModel();
        void Evaluate(const PhraseBasedFeatureContext& context,	ScoreComponentCollection* accumulator) const;
        void EvaluateChart(const ChartBasedFeatureContext& context, ScoreComponentCollection* accumulator) const;
        void Read(const std::string& filename);
        void SetSentence(std::string& sent);
        bool SetPostEditedSentence(std::string& sent);
        inline std::string GetScoreProducerWeightShortName(unsigned) const { return "stm"; };
        float getScore(std::string& s, std::string& t);
        map<int, string> SubPhrases(const string& s1, const string& s2);
        bool IfActive(){return m_active;}
        void RemoveJunk();
        void RunInstance(Manager& manager);
        int SizeofSTM() const {return m_stm.size();};
    private:
        void Evaluate(const TargetPhrase& tp, ScoreComponentCollection* out) const;
        std::string m_source, m_postedited;
        std::map<std::string, std::map<std::string, std::pair<bool, float> > > m_stm;
        bool m_sigmoidParam, m_active;
};
}
#endif	/* SINGLETRIGGERMODEL_H */

