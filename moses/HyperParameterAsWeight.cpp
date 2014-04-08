

#include "HyperParameterAsWeight.h"
#include "moses/StaticData.h"

using namespace std;

namespace Moses
{

  HyperParameterAsWeight::HyperParameterAsWeight(const string str)
    :StatelessFeatureFunction("HyperParameters", 3)
  {

    // hack into StaticData and change anything you want
    // as an example, we have 2 weights and change
    //   1. slack
    //   2. f_learningrate
    //   3. w_learningrate

    StaticData &staticData = StaticData::InstanceNonConst();

    vector<float> weights = staticData.GetWeights(this);

    staticData.m_C = weights[0];
    staticData.m_flr = weights[1];
    staticData.m_wlr = weights[2];

  }

  void HyperParameterAsWeight::Evaluate(const TargetPhrase& tp, ScoreComponentCollection* out) const {
	  float score = 0.0;
	  out->PlusEquals(this, score);
  }
  void HyperParameterAsWeight::Evaluate(const PhraseBasedFeatureContext& context, ScoreComponentCollection* accumulator) const {
	  const TargetPhrase& tp = context.GetTargetPhrase();
	  Evaluate(tp, accumulator);
  }

  void HyperParameterAsWeight::EvaluateChart(const ChartBasedFeatureContext& context, ScoreComponentCollection* accumulator) const {
	  const TargetPhrase& tp = context.GetTargetPhrase();
	  Evaluate(tp, accumulator);
  }
}
