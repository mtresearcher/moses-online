/* 
 * File:   SingleTriggerModel.cpp
 * Author: pmathur
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

    SingleTriggerModel::SingleTriggerModel(std::string file) : StatelessFeatureFunction("SingleTriggerModel", 1) {
        Read(file);
        m_sentence.resize(0);
        m_stm.empty();
    }

    SingleTriggerModel::~SingleTriggerModel() {
        m_sentence.resize(0);
        m_stm.empty();
    }

    void SingleTriggerModel::Read(const std::string filePath) {
        // read the trigger model
        //    
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

            util::TokenIter<util::MultiCharacter> pipes(line, util::MultiCharacter("|||"));
            StringPiece sourcePhraseString(GrabOrDie(pipes, filePath, line_num));
            StringPiece targetPhraseString(GrabOrDie(pipes, filePath, line_num));
            StringPiece scoreString(GrabOrDie(pipes, filePath, line_num));
            char* err_ind;
            float score = FloorScore(TransformScore(static_cast<float> (strtod(scoreString.data(), &err_ind))));
            if (err_ind == scoreString.data()) {
                stringstream strme;
                strme << "Bad number " << scoreString << " on line " << line_num;
                UserMessage::Add(strme.str());
                abort();
            }
            // Insertion in memory
            m_stm[sourcePhraseString.as_string()][targetPhraseString.as_string()] = score;
        }
        PrintUserTime("Loaded Interlingual Single Trigger Model...");
    }

    // this function should be called before decoding of a sentence starts

    void SingleTriggerModel::SetSentence(std::string sent) {
        m_sentence = sent;
    }

    void SingleTriggerModel::Evaluate(const TargetPhrase& tp, ScoreComponentCollection* out) const {
        // for now assuming that we have m_sentence set already
        float score = 0.0;
        std::vector<string> str;
        int x = split_marker_perl(m_sentence, " ", str);
        std::string t = "";
        size_t endpos = tp.GetSize();
        for (size_t pos = 0; pos < endpos; ++pos) {
            t = tp.GetWord(pos).GetFactor(0)->GetString();
            for (int i = 0; i < x; i++) {
                std::map<std::string, map<std::string, float> >::const_iterator it = m_stm.find(str[i]);
                if (it != m_stm.end()) {
                    map<std::string, float>::const_iterator itr = it->second.find(t);
                    if (itr != it->second.end()) {
                        score += itr->second;
                    }
                }
            }
        }
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
}
