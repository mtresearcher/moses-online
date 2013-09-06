/* 
 * File:   SingleTriggerModel.cpp
 * Author: pmathur
 * 
 * Created on August 13, 2013, 5:21 PM
 */

#include "SingleTriggerModel.h"
#include "StaticData.h"
#include<sstream>
using namespace Moses;

SingleTriggerModel::SingleTriggerModel() : StatelessFeatureFunction("SingleTriggerModel", 1) {

}

SingleTriggerModel::~SingleTriggerModel() {
}

void SingleTriggerModel::Read(const std::string filePath)
{
    // read the trigger model
    
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
        float score = FloorScore(TransformScore(static_cast<float> (strtod(scoreString->data(), &err_ind))));
        if (err_ind == scoreString->data()) {
            stringstream strme;
            strme << "Bad number " << scoreString << " on line " << line_num;
            UserMessage::Add(strme.str());
            abort();
        }
        // Insertion in memory
        m_stm[std::pair<sourcePhraseString, targetPhraseString>]=score;
    }
}

void SingleTriggerModel::SetSentence(std::string sent)
{
    m_sentence = sent;
}

void SingleTriggerModel::Evaluate(const TargetPhrase& tp, ScoreComponentCollection* out) const {

}

void SingleTriggerModel::Evaluate(const PhraseBasedFeatureContext& context, ScoreComponentCollection* accumulator) const {
    const TargetPhrase& tp = context.GetTargetPhrase();
    Evaluate(tp, accumulator);
}

void SingleTriggerModel::EvaluateChart(const ChartBasedFeatureContext& context, ScoreComponentCollection* accumulator) const {
    const TargetPhrase& tp = context.GetTargetPhrase();
    Evaluate(tp, accumulator);
}


