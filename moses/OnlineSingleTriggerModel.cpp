/*
 * OnlineSingleTriggerModel.cpp
 *
 *  Created on: Oct 25, 2013
 *      Author: prashant
 */

#include "OnlineSingleTriggerModel.h"

using namespace std;
namespace Moses {
namespace {
	int split_marker_perl(string str, string marker, vector<string> &array) {
        int found = str.find(marker), prev = 0;
        while (found != string::npos) // warning!
        {
            array.push_back(str.substr(prev, found - prev));
            prev = found + marker.length();
            found = str.find(marker, found + marker.length());
        }
        array.push_back(str.substr(prev));
        return array.size() - 1;
    }
}

OnlineSingleTriggerModel::OnlineSingleTriggerModel(std::string file, bool sigmoidParam) : StatelessFeatureFunction("OnlineSingleTriggerModel", 1) {
	Read(file);
	m_src_sentence.resize(0);
	m_tgt_sentence.resize(0);
	m_postedited.resize(0);
	PP_BEST.clear();
	m_learn=false;
	m_stm.clear();
	m_sigmoidParam=true;
}

OnlineSingleTriggerModel::OnlineSingleTriggerModel(bool sigmoidParam) : StatelessFeatureFunction("OnlineSingleTriggerModel", 1) {
	m_src_sentence.resize(0);
	m_tgt_sentence.resize(0);
	m_postedited.resize(0);
	PP_BEST.clear();
	m_stm.clear();
	m_learn=false;
	m_sigmoidParam=true;
}

OnlineSingleTriggerModel::~OnlineSingleTriggerModel() {
	m_src_sentence.resize(0);
	m_tgt_sentence.resize(0);
	m_postedited.resize(0);
	PP_BEST.clear();
	m_stm.clear();
}

void OnlineSingleTriggerModel::RemoveJunk() {
	m_src_sentence.resize(0);
	m_tgt_sentence.resize(0);
	m_postedited.resize(0);
	PP_BEST.clear();
	m_stm.clear();
}

void OnlineSingleTriggerModel::Read(const std::string filePath) {
	// read the trigger model
	PrintUserTime("Start loading Online interlingual Single Trigger Model...");

	const StaticData& staticData = StaticData::Instance();
	util::FilePiece inFile(filePath.c_str(), staticData.GetVerboseLevel() >= 1 ? &std::cerr : NULL);
	size_t line_num = 0;
	while (true) {
		++line_num;
		StringPiece line;
		try {
			line = inFile.ReadLine();
		} catch (util::EndOfFileException &e) {
			break;
		}
		std::vector<std::string> blocks;
		split_marker_perl(line.as_string(), "|||", blocks);
		float score;
		stringstream(blocks[2])>>score; // this is an interesting way to change data type!
		// Insertion in memory
		m_stm[blocks[0]][blocks[1]]=score;
	}
	PrintUserTime("Loaded interlingual Single Trigger Model...");
}

bool OnlineSingleTriggerModel::SetSourceSentence(std::string sent) {
	if(m_src_sentence.empty())
	{
		m_src_sentence = sent;
		return true;
	}
	else
	{
		cerr<<"source sentence already exists.. "<<m_src_sentence<<endl;
		return false;
	}
}
bool OnlineSingleTriggerModel::SetTargetSentence(std::string sent) {
	if(m_tgt_sentence.empty())
	{
		m_tgt_sentence = sent;
		return true;
	}
	else
	{
		cerr<<"target sentence already exists.. "<<m_tgt_sentence<<endl;
		return false;
	}
}
bool OnlineSingleTriggerModel::SetPostEditedSentence(std::string sent)
{
	if(m_postedited.empty())
	{
		m_postedited=sent;
		return true;
	}
	else
	{
		cerr<<"post edited already exists.. "<<m_postedited<<endl;
		return false;
	}
}
void OnlineSingleTriggerModel::Evaluate(const TargetPhrase& tp, ScoreComponentCollection* out) const {
	float score = 0.0;
	std::string s="",t="";
	size_t endpos = tp.GetSize();
	for (size_t pos = 0 ; pos < endpos ; ++pos) {
		if (pos > 0){ t += " "; }
		t += tp.GetWord(pos).GetFactor(0)->GetString();
	}
	endpos = tp.GetSourcePhrase().GetSize();
	for (size_t pos = 0 ; pos < endpos ; ++pos) {
		if (pos > 0){ s += " "; }
		s += tp.GetSourcePhrase().GetWord(pos).GetFactor(0)->GetString();
	}
	if(m_stm.find(t) !=  m_stm.end())
		if(m_stm.at(t).find(s) !=  m_stm.at(t).end())
			score = m_stm.at(t).at(s);

	if(m_sigmoidParam)
		score=(score/(1+abs(score)));
	out->PlusEquals(this, score);
}
void OnlineSingleTriggerModel::PrintHypo(const Hypothesis* hypo, ostream& HypothesisStringStream) {
	if (hypo->GetPrevHypo() != NULL) {
		PrintHypo(hypo->GetPrevHypo(), HypothesisStringStream);
		Phrase p = hypo->GetCurrTargetPhrase();
		for (size_t pos = 0; pos < p.GetSize(); pos++) {
			const Factor *factor = p.GetFactor(pos, 0);
			HypothesisStringStream << *factor << " ";
		}
		std::string sourceP = hypo->GetSourcePhraseStringRep();
		std::string targetP = hypo->GetTargetPhraseStringRep();
		PP_BEST[targetP][sourceP]=1;
	}
}
void OnlineSingleTriggerModel::RunInstance(Manager& manager)
{
	const Hypothesis* hypo = manager.GetBestHypothesis();
	stringstream bestHypothesis;
	PrintHypo(hypo, bestHypothesis);
	m_tgt_sentence = bestHypothesis.str();
	float bestScore=hypo->GetScore();
	cerr<<"Best Hypothesis : "<<m_tgt_sentence<<endl;
	cerr<<"Post Edit       : "<<m_postedited<<endl;
	map<int, string> Phrases=SubPhrases(m_tgt_sentence, m_postedited);
	for(int i=0;i<Phrases.size(); i++)
	{
		if(PP_BEST.find(Phrases[i]) != PP_BEST.end())
		{	// if the sub phrase is matched in the best hypothesis then insert the pair in the cache
			std::map<std::string, int>::const_iterator itr;
			for(itr=PP_BEST[Phrases[i]].begin(); itr!=PP_BEST[Phrases[i]].end();itr++)
			{
				m_stm[itr->first][Phrases[i]]++;
				cerr<<itr->first<<" ||| "<<Phrases[i]<<endl;
			}
		}
	}
}

void OnlineSingleTriggerModel::Evaluate(const PhraseBasedFeatureContext& context, ScoreComponentCollection* accumulator) const {
	const TargetPhrase& tp = context.GetTargetPhrase();
	Evaluate(tp, accumulator);
}

void OnlineSingleTriggerModel::EvaluateChart(const ChartBasedFeatureContext& context, ScoreComponentCollection* accumulator) const {
	const TargetPhrase& tp = context.GetTargetPhrase();
	Evaluate(tp, accumulator);
}

map<int, string> OnlineSingleTriggerModel::SubPhrases(const string& s1, const string& s2)
{
	map<int, string> Phrases;
	int idx=0;
	bool flag=false;
	if(s1.empty() || s2.empty())
	{
		return Phrases;
	}
	vector<string> str1,str2;
	split_marker_perl(s1, " ", str1);
	split_marker_perl(s2, " ", str2);

	int unmatch=0;
	for(int i=0; i<str1.size();i++)
	{
		for(int j=unmatch; j<str2.size();j++)
		{
			if(str1[i].compare(str2[j]) != 0){
				if(flag==true)idx++;
				flag=false;
				continue;
			}
			else{
				string str=Phrases[idx];
				str.append(str2[j]);
				Phrases[idx]=str;
				unmatch=j;
				i++;
				flag=true;
			}
		}
	}
	return Phrases;
}

} /* namespace Moses */
