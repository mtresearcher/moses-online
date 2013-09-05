/* 
 * File:   SingleTriggerModel.cpp
 * Author: pmathur
 * 
 * Created on August 13, 2013, 5:21 PM
 */

#include "SingleTriggerModel.h"

using namespace Moses;

SingleTriggerModel::SingleTriggerModel() : StatelessFeatureFunction("SingleTriggerModel", 1) {

}

SingleTriggerModel::~SingleTriggerModel() {
}

void SingleTriggerModel::Read(const std::string filename)
{
    
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


