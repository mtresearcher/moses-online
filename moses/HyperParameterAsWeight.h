
#include "FeatureFunction.h"
#include "StaticData.h"

namespace Moses
{
  /**
   * Baseclass for phrase-table or generation table feature function
   **/
  class HyperParameterAsWeight : public StatelessFeatureFunction
  {
  public:
    HyperParameterAsWeight(const std::string);

    void Evaluate(const PhraseBasedFeatureContext& context,	ScoreComponentCollection* accumulator) const;
    void EvaluateChart(const ChartBasedFeatureContext& context, ScoreComponentCollection* accumulator) const;
    void Evaluate(const TargetPhrase& tp, ScoreComponentCollection* out) const;
    inline std::string GetScoreProducerWeightShortName(unsigned) const { return "hpw"; };

  };

} // namespace
