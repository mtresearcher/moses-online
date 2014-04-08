/* 
 * File:   SingleTriggerModel.cpp
 * Author: prashant
 * 
 * Created on August 13, 2013, 5:21 PM
 */

#include "SingleTriggerModel.h"

using namespace std;
namespace Moses {

    namespace {
        void ParserDeath(const std::string &file, size_t line_num) {
            stringstream strme;
            strme << "Syntax error at " << file << ":" << line_num;
            UserMessage::Add(strme.str());
            abort();
        }
        template <class It> StringPiece GrabOrDie(It &it, const std::string &file, size_t line_num) {
            if (!it) ParserDeath(file, line_num);
            return *it++;
        }
    }
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

    void SingleTriggerModel::RemoveJunk(){return;}
    void SingleTriggerModel::RunInstance(Manager& manager){return;}
    bool SingleTriggerModel::SetPostEditedSentence(std::string& sent){
    	m_postedited=sent;
    	return true;
    }

    SingleTriggerModel::SingleTriggerModel(std::string file, bool sigmoidParam) : StatelessFeatureFunction("SingleTriggerModel", 1) {
        Read(file);
        m_source.resize(0);
        m_postedited.resize(0);
        m_stm.empty();
        m_sigmoidParam=true;
        m_active=true;
    }

    SingleTriggerModel::SingleTriggerModel(bool sigmoidParam) : StatelessFeatureFunction("SingleTriggerModel", 1) {
    	m_source.resize(0);
    	m_postedited.resize(0);
    	m_stm.empty();
    	m_sigmoidParam=true;
    	m_active=true;
    }

    SingleTriggerModel::~SingleTriggerModel() {
        m_postedited.resize(0);
        m_source.resize(0);
        m_stm.empty();
    }

    float SingleTriggerModel::getScore(std::string& s, std::string& t)
    {
        return m_stm[s][t];
    }
    void SingleTriggerModel::Read(const std::string& filePath) {
        // read the trigger model
    	PrintUserTime("Start loading Interlingual Single Trigger Model...");

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
        PrintUserTime("Loaded Interlingual Single Trigger Model...");
    }

    // this function should be called before decoding of a sentence starts

    void SingleTriggerModel::SetSentence(std::string& sent){
        m_source = sent;
    }
    void SingleTriggerModel::Evaluate(const TargetPhrase& tp, ScoreComponentCollection* out) const {
        float score = 0.0;
        std::vector<string> str;
        split_marker_perl(m_source, " ", str);
        std::string t = "";
        size_t endpos = tp.GetSize();
        for (size_t pos = 0; pos < endpos; ++pos) {
            t = tp.GetWord(pos).GetFactor(0)->GetString();
            for (int i = 0; i < str.size(); i++) {
                std::map<std::string, map<std::string, float> >::const_iterator it = m_stm.find(str[i]);
                if (it != m_stm.end()) {
                    map<std::string, float>::const_iterator itr = m_stm.at(str[i]).find(t);
                    if (itr != it->second.end()) {
                        score += m_stm.at(str[i]).at(t);
                    }
                }
            }
        }
        if(m_sigmoidParam || true)
            score=(score/(1+abs(score)));
        out->PlusEquals(this, score);
    }

    void SingleTriggerModel::Evaluate(const PhraseBasedFeatureContext& context, ScoreComponentCollection* accumulator) const {
        const TargetPhrase& tp = context.GetTargetPhrase();
        Evaluate(tp, accumulator);
    }

    void SingleTriggerModel::EvaluateChart(const ChartBasedFeatureContext& context, ScoreComponentCollection* accumulator) const {
        const TargetPhrase& tp = context.GetTargetPhrase();
        Evaluate(tp, accumulator);
    }
    map<int, string> SingleTriggerModel::SubPhrases(const string& s1, const string& s2)
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
    		string string1 = str1[i];
    		std::transform(string1.begin(), string1.end(), string1.begin(), ::tolower);
    		for(int j=unmatch; j<str2.size();j++)
    		{
    			string string2 = str2[i];
    			std::transform(string2.begin(), string2.end(), string2.begin(), ::tolower);
    			if(string1.compare(string2) != 0){
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
}
