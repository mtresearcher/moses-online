/* 
 * File:   SingleTriggerModel.cpp
 * Author: pmathur
 * 
 * Created on August 13, 2013, 5:21 PM
 */

#include "SingleTriggerModel.h"

using namespace Moses;

SingleTriggerModel::SingleTriggerModel() : StatefulFeatureFunction("SingleTriggerModel",1) {
    
}

SingleTriggerModel::~SingleTriggerModel() {
}

FFState* SingleTriggerModel::Evaluate(const Hypothesis& hypo,
                                     const FFState* prev_state,
                                     ScoreComponentCollection* out) const
{
  Scores score(GetNumScoreComponents(), 0);
//  const LexicalReorderingState *prev = dynamic_cast<const LexicalReorderingState *>(prev_state);
//  LexicalReorderingState *next_state = prev->Expand(hypo.GetTranslationOption(), score);

  out->PlusEquals(this, score);

  return next_state;
}

const FFState* SingleTriggerModel::EmptyHypothesisState(const InputType &input) const
{
  return ;
}

