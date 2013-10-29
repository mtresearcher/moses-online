/*
 * OnlineSingleTriggerModel.h
 *
 *  Created on: Oct 25, 2013
 *      Author: prashant
 */

#ifndef ONLINESINGLETRIGGERMODEL_H_
#define ONLINESINGLETRIGGERMODEL_H_

#include "FeatureFunction.h"
#include "StaticData.h"
#include "Util.h"
#include "util/file_piece.hh"
#include "util/tokenize_piece.hh"
#include "UserMessage.h"
#include "Manager.h"
#include <string.h>
#include <sstream>

using namespace std;

typedef string trigger;
typedef string word;

namespace Moses {

class OnlineSingleTriggerModel : public StatelessFeatureFunction {
public:
	OnlineSingleTriggerModel(std::string file, bool sigmoidParam);
	OnlineSingleTriggerModel(bool sigmoidParam);
	virtual ~OnlineSingleTriggerModel();
	void RemoveJunk();
	void Evaluate(const PhraseBasedFeatureContext& context,	ScoreComponentCollection* accumulator) const;
	void EvaluateChart(const ChartBasedFeatureContext& context, ScoreComponentCollection* accumulator) const;
	void Read(const std::string filename);
	bool SetSourceSentence(std::string sent);
	bool SetTargetSentence(std::string sent);
	bool SetPostEditedSentence(std::string sent);
	inline std::string GetScoreProducerWeightShortName(unsigned) const { return "ostm"; };
	map<int, string> SubPhrases(const string& s1, const string& s2);
	void RunInstance(Manager& manager);
	void PrintHypo(const Hypothesis* hypo, ostream& HypothesisStringStream);

	inline size_t GetNumScoreComponents() const { return 1; };
	void SetOnlineSTMTrue() { m_learn=true; };
	void SetOnlineSTMFalse() { m_learn=false; };
	bool GetOnlineSTM() const { return m_learn; };

private:
	void Evaluate(const TargetPhrase& tp, ScoreComponentCollection* out) const;
	std::string m_src_sentence, m_tgt_sentence, m_postedited;
	std::map<std::string, std::map<std::string, int> > m_stm, PP_BEST;
	bool m_learn, m_sigmoidParam;
};

} /* namespace Moses */
#endif /* ONLINESINGLETRIGGERMODEL_H_ */
