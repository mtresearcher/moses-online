/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#include "PhraseDictionaryMultiModelCounts.h"

#define LINE_MAX_LENGTH 100000
#include "../../phrase-extract/SafeGetline.h" // for SAFE_GETLINE()

using namespace std;

// from phrase-extract/tables-core.cpp
vector<string> tokenize( const char* input )
{
  vector< string > token;
  bool betweenWords = true;
  int start=0;
  int i=0;
  for(; input[i] != '\0'; i++) {
    bool isSpace = (input[i] == ' ' || input[i] == '\t');

    if (!isSpace && betweenWords) {
      start = i;
      betweenWords = false;
    } else if (isSpace && !betweenWords) {
      token.push_back( string( input+start, i-start ) );
      betweenWords = true;
    }
  }
  if (!betweenWords)
    token.push_back( string( input+start, i-start ) );
  return token;
}

namespace Moses

{
PhraseDictionaryMultiModelCounts::PhraseDictionaryMultiModelCounts(size_t numScoreComponent,
    PhraseDictionaryFeature* feature): PhraseDictionaryMultiModel(numScoreComponent, feature)
{
    m_feature_load = feature;
    m_mode = "instance_weighting"; //TODO: set this in config; use m_mode to switch between interpolation and instance weighting
    m_combineFunction = InstanceWeighting;
    //m_mode = "interpolate";
    //m_combineFunction = LinearInterpolationFromCounts;
}

PhraseDictionaryMultiModelCounts::~PhraseDictionaryMultiModelCounts()
{
    RemoveAllInColl(m_lexTable_e2f);
    RemoveAllInColl(m_lexTable_f2e);
    RemoveAllInColl(m_pd);
    RemoveAllInColl(m_inverse_pd);
}

bool PhraseDictionaryMultiModelCounts::Load(const vector<FactorType> &input
                                  , const vector<FactorType> &output
                                  , const vector<string> &files
                                  , const vector<float> &weight
                                  , size_t tableLimit
                                  , const LMList &languageModels
                                  , float weightWP)
{
  m_languageModels = &languageModels;
  m_weight = weight;
  m_weightWP = weightWP;
  m_input = input;
  m_output = output;
  m_tableLimit = tableLimit;

  m_numModels = files.size();

  for(size_t i = 0; i < m_numModels; ++i){

      string impl, file, main_table, target_table, lex_e2f, lex_f2e;

      string delim = ":";
      size_t delim_pos = files[i].find(delim);
      if (delim_pos >= files[i].size()) {
        UserMessage::Add("Phrase table must be specified in this format: Implementation:Path");
        CHECK(false);
      }

      impl = files[i].substr(0,delim_pos);
      file = files[i].substr(delim_pos+1,files[i].size());
      main_table = file + "/count-table";
      target_table = file + "/count-table-target";
      lex_e2f = file + "/lex.counts.e2f";
      lex_f2e = file + "/lex.counts.f2e";
      size_t componentTableLimit = 0; // using 0, because we can't trust implemented pruning algorithms with count tables.

      PhraseTableImplementation implementation = (PhraseTableImplementation) Scan<int>(impl);

      if (implementation == Memory) {

            //how many actual scores there are in the phrase tables
            size_t numScoresCounts = 3;
            size_t numScoresTargetCounts = 1;

            if (!FileExists(main_table) && FileExists(main_table + ".gz")) main_table += ".gz";
            if (!FileExists(target_table) && FileExists(target_table + ".gz")) target_table += ".gz";

            PhraseDictionaryMemory* pdm = new PhraseDictionaryMemory(m_numScoreComponent, m_feature_load);
            pdm->SetNumScoreComponentMultiModel(numScoresCounts); //instead of complaining about inequal number of scores, silently fill up the score vector with zeroes
            pdm->Load( input, output, main_table, weight, componentTableLimit, languageModels, weightWP);
            m_pd.push_back(pdm);

            PhraseDictionaryMemory* pdm_inverse = new PhraseDictionaryMemory(m_numScoreComponent, m_feature_load);
            pdm_inverse->SetNumScoreComponentMultiModel(numScoresTargetCounts);
            pdm_inverse->Load( input, output, target_table, weight, componentTableLimit, languageModels, weightWP);
            m_inverse_pd.push_back(pdm_inverse);
      }
      else if (implementation == Compact) {
#ifndef WIN32
            PhraseDictionaryCompact* pdc = new PhraseDictionaryCompact(m_numScoreComponent, implementation, m_feature_load);
            pdc->SetNumScoreComponentMultiModel(m_numScoreComponent); //for compact models, we need to pass number of log-linear components to correctly resize the score vector
            pdc->Load( input, output, main_table, weight, componentTableLimit, languageModels, weightWP);
            m_pd.push_back(pdc);

            PhraseDictionaryCompact* pdc_inverse = new PhraseDictionaryCompact(m_numScoreComponent, implementation, m_feature_load);
            pdc_inverse->SetNumScoreComponentMultiModel(m_numScoreComponent);
            pdc_inverse->Load( input, output, target_table, weight, componentTableLimit, languageModels, weightWP);
            m_inverse_pd.push_back(pdc_inverse);
#else
            CHECK(false);
#endif
      }
      else {
        UserMessage::Add("phrase table type unknown to multi-model mode");
        CHECK(false);
      }

      lexicalTable* e2f = new lexicalTable;
      LoadLexicalTable(lex_e2f, e2f);
      lexicalTable* f2e = new lexicalTable;
      LoadLexicalTable(lex_f2e, f2e);

      m_lexTable_e2f.push_back(e2f);
      m_lexTable_f2e.push_back(f2e);

  }
  return true;
}


const TargetPhraseCollection *PhraseDictionaryMultiModelCounts::GetTargetPhraseCollection(const Phrase& src) const
{

  vector<vector<float> > multimodelweights;
  bool normalize;
  normalize = (m_mode == "interpolate") ? true : false;
  multimodelweights = getWeights(4,normalize);

  //source phrase frequency is shared among all phrase pairs
  vector<float> fs(m_numModels);

  map<string,multiModelCountsStatistics*>* allStats = new(map<string,multiModelCountsStatistics*>);

  CollectSufficientStatistics(src, fs, allStats);

  TargetPhraseCollection *ret = CreateTargetPhraseCollectionCounts(src, fs, allStats, multimodelweights);

  ret->NthElement(m_tableLimit); // sort the phrases for pruning later
  const_cast<PhraseDictionaryMultiModelCounts*>(this)->CacheForCleanup(ret);
  return ret;
}


void PhraseDictionaryMultiModelCounts::CollectSufficientStatistics(const Phrase& src, vector<float> &fs, map<string,multiModelCountsStatistics*>* allStats) const
//fill fs and allStats with statistics from models
{
  for(size_t i = 0; i < m_numModels; ++i){

    TargetPhraseCollection *ret_raw = (TargetPhraseCollection*)  m_pd[i]->GetTargetPhraseCollection( src);
    if (ret_raw != NULL) {

      TargetPhraseCollection::iterator iterTargetPhrase;
      for (iterTargetPhrase = ret_raw->begin(); iterTargetPhrase != ret_raw->end();  ++iterTargetPhrase) {

        TargetPhrase * targetPhrase = *iterTargetPhrase;
        vector<float> raw_scores = targetPhrase->GetScoreBreakdown().GetScoresForProducer(m_feature);

        string targetString = targetPhrase->GetStringRep(m_output);
        if (allStats->find(targetString) == allStats->end()) {

          multiModelCountsStatistics * statistics = new multiModelCountsStatistics;
          statistics->targetPhrase = new TargetPhrase(*targetPhrase); //make a copy so that we don't overwrite the original phrase table info

          statistics->fst.resize(m_numModels);
          statistics->ft.resize(m_numModels);
          Scores scoreVector(5);
          scoreVector[0] = -raw_scores[0];
          scoreVector[1] = -raw_scores[1];
          scoreVector[2] = -raw_scores[2];
          statistics->targetPhrase->SetScore(m_feature, scoreVector, ScoreComponentCollection(), m_weight, m_weightWP, *m_languageModels); // set scores to 0

          (*allStats)[targetString] = statistics;

        }
        multiModelCountsStatistics * statistics = (*allStats)[targetString];

        statistics->fst[i] = UntransformScore(raw_scores[0]);
        statistics->ft[i] = UntransformScore(raw_scores[1]);
        fs[i] = UntransformScore(raw_scores[2]);
        (*allStats)[targetString] = statistics;
      }
    }
  }

  // get target phrase frequency for models which have not seen the phrase pair
  for ( map< string, multiModelCountsStatistics*>::const_iterator iter = allStats->begin(); iter != allStats->end(); ++iter ) {
    multiModelCountsStatistics * statistics = iter->second;

    for (size_t i = 0; i < m_numModels; ++i) {
        if (!statistics->ft[i]) {
            statistics->ft[i] = GetTargetCount(static_cast<const Phrase&>(*statistics->targetPhrase), i);
        }
    }
  }
}


TargetPhraseCollection* PhraseDictionaryMultiModelCounts::CreateTargetPhraseCollectionCounts(const Phrase &src, vector<float> &fs, map<string,multiModelCountsStatistics*>* allStats, vector<vector<float> > &multimodelweights) const
{
  TargetPhraseCollection *ret = new TargetPhraseCollection();
  for ( map< string, multiModelCountsStatistics*>::const_iterator iter = allStats->begin(); iter != allStats->end(); ++iter ) {

    multiModelCountsStatistics * statistics = iter->second;

    if (statistics->targetPhrase->GetAlignTerm().GetSize() == 0) {
        UserMessage::Add("models need to include alignment information for computation of lexical weights.\nUse --phrase-word-alignment during training; for on-disk tables, also set -alignment-info when creating on-disk tables, and -use-alignment-info during decoding.");
        CHECK(false);
    }

    pair<vector< set<size_t> >, vector< set<size_t> > > alignment = GetAlignmentsForLexWeights(src, static_cast<const Phrase&>(*statistics->targetPhrase), statistics->targetPhrase->GetAlignTerm());
    vector< set<size_t> > alignedToT = alignment.first;
    vector< set<size_t> > alignedToS = alignment.second;
    double lexst = ComputeWeightedLexicalTranslation(static_cast<const Phrase&>(*statistics->targetPhrase), src, alignedToS, m_lexTable_e2f, multimodelweights[1], m_output, m_input );
    double lexts = ComputeWeightedLexicalTranslation(src, static_cast<const Phrase&>(*statistics->targetPhrase), alignedToT, m_lexTable_f2e, multimodelweights[3], m_input, m_output );

    Scores scoreVector(5);
    scoreVector[0] = FloorScore(TransformScore(m_combineFunction(statistics->fst, statistics->ft, multimodelweights[0])));
    scoreVector[1] = FloorScore(TransformScore(lexst));
    scoreVector[2] = FloorScore(TransformScore(m_combineFunction(statistics->fst, fs, multimodelweights[2])));
    scoreVector[3] = FloorScore(TransformScore(lexts));
    scoreVector[4] = FloorScore(TransformScore(2.718));

    statistics->targetPhrase->SetScore(m_feature, scoreVector, ScoreComponentCollection(), m_weight, m_weightWP, *m_languageModels);

    ret->Add(statistics->targetPhrase);

    delete statistics;
  }
  delete allStats;
  return ret;
}


float PhraseDictionaryMultiModelCounts::GetTargetCount(const Phrase &target, size_t modelIndex) const {

    TargetPhraseCollection *ret_raw = (TargetPhraseCollection*)  m_inverse_pd[modelIndex]->GetTargetPhraseCollection(target);

    // in inverse mode, we want the first score of the first phrase pair (note: if we were to work with truly symmetric models, it would be the third score)
    if (ret_raw != NULL) {
        TargetPhrase * targetPhrase = *(ret_raw->begin());
        return UntransformScore(targetPhrase->GetScoreBreakdown().GetScoresForProducer(m_feature)[0]);
    }

    // target phrase unknown
    else return 0;
}


pair<PhraseDictionaryMultiModelCounts::AlignVector,PhraseDictionaryMultiModelCounts::AlignVector> PhraseDictionaryMultiModelCounts::GetAlignmentsForLexWeights(const Phrase &phraseS, const Phrase &phraseT, const AlignmentInfo &alignment) const {

    AlignVector alignedToT (phraseT.GetSize());
    AlignVector alignedToS (phraseS.GetSize());
    AlignmentInfo::const_iterator iter;

    for (iter = alignment.begin(); iter != alignment.end(); ++iter) {
    const pair<size_t,size_t> &alignPair = *iter;
        size_t s = alignPair.first;
        size_t t = alignPair.second;
        alignedToT[t].insert( s );
        alignedToS[s].insert( t );
  }
  return make_pair(alignedToT,alignedToS);
}


double PhraseDictionaryMultiModelCounts::ComputeWeightedLexicalTranslation( const Phrase &phraseS, const Phrase &phraseT, AlignVector &alignment, const vector<lexicalTable*> &tables, vector<float> &multimodelweights, const vector<FactorType> &input_factors, const vector<FactorType> &output_factors ) const {
  // lexical translation probability

  double lexScore = 1.0;
  string null = "NULL";

  // all target words have to be explained
  for(size_t ti=0; ti<alignment.size(); ti++) {
    const set< size_t > & srcIndices = alignment[ ti ];
    Word t_word = phraseT.GetWord(ti);
    string ti_str = t_word.GetString(output_factors, false);
    if (srcIndices.empty()) {
      // explain unaligned word by NULL
      lexScore *= GetLexicalProbability( null, ti_str, tables, multimodelweights );
    } else {
      // go through all the aligned words to compute average
      double thisWordScore = 0;
      for (set< size_t >::const_iterator si(srcIndices.begin()); si != srcIndices.end(); ++si) {
        string s_str = phraseS.GetWord(*si).GetString(input_factors, false);
        thisWordScore += GetLexicalProbability( s_str, ti_str, tables, multimodelweights );
      }
      lexScore *= thisWordScore / (double)srcIndices.size();
    }
  }
  return lexScore;
}

// get lexical probability for single word alignment pair
double PhraseDictionaryMultiModelCounts::GetLexicalProbability( string &wordS, string &wordT, const vector<lexicalTable*> &tables, vector<float> &multimodelweights ) const {
    vector<float> joint_count (m_numModels);
    vector<float> marginals (m_numModels);

    for (size_t i=0;i < m_numModels;i++) {
        lexicalMapJoint::iterator joint_s = tables[i]->joint.find( wordS );
        if (joint_s == tables[i]->joint.end()) joint_count[i] = 0.0;
        else {
            lexicalMap::iterator joint_t = joint_s->second.find( wordT );
            if (joint_t == joint_s->second.end()) joint_count[i] = 0.0;
            else joint_count[i] = joint_t->second;
        }

        lexicalMap::iterator marginal_s = tables[i]->marginal.find( wordS );
        if (marginal_s == tables[i]->marginal.end()) marginals[i] = 0.0;
        else marginals[i] = marginal_s->second;
        }

    double lexProb = m_combineFunction(joint_count, marginals, multimodelweights);

  return lexProb;
}


void PhraseDictionaryMultiModelCounts::LoadLexicalTable( string &fileName, lexicalTable* ltable) {

  cerr << "Loading lexical translation table from " << fileName;
  ifstream inFile;
  inFile.open(fileName.c_str());
  if (inFile.fail()) {
    cerr << " - ERROR: could not open file\n";
    exit(1);
  }
  istream *inFileP = &inFile;

  char line[LINE_MAX_LENGTH];

  int i=0;
  while(true) {
    i++;
    if (i%100000 == 0) cerr << "." << flush;
    SAFE_GETLINE((*inFileP), line, LINE_MAX_LENGTH, '\n', __FILE__);
    if (inFileP->eof()) break;

    vector<string> token = tokenize( line );
    if (token.size() != 4) {
      cerr << "line " << i << " in " << fileName
           << " has wrong number of tokens, skipping:\n"
           << token.size() << " " << token[0] << " " << line << endl;
      continue;
    }

    double joint = atof( token[2].c_str() );
    double marginal = atof( token[3].c_str() );
    string wordT = token[0];
    string wordS = token[1];
    ltable->joint[ wordS ][ wordT ] = joint;
    ltable->marginal[ wordS ] = marginal;
  }
  cerr << endl;

}


vector<float> PhraseDictionaryMultiModelCounts::MinimizePerplexity(vector<pair<string, string> > &phrase_pair_vector) {

    const StaticData &staticData = StaticData::Instance();
    const string& factorDelimiter = staticData.GetFactorDelimiter();

    map<pair<string, string>, size_t> phrase_pair_map;

    for ( vector<pair<string, string> >::const_iterator iter = phrase_pair_vector.begin(); iter != phrase_pair_vector.end(); ++iter ) {
        phrase_pair_map[*iter] += 1;
    }

    map<pair<string, string>, multiModelCountsOptimizationCache*>* optimizerStats = new(map<pair<string, string>, multiModelCountsOptimizationCache*>);

    for ( map<pair<string, string>, size_t>::const_iterator iter = phrase_pair_map.begin(); iter != phrase_pair_map.end(); ++iter ) {

        pair<string, string> phrase_pair = iter->first;
        string source_string = phrase_pair.first;
        string target_string = phrase_pair.second;

        vector<float> fs(m_numModels);
        map<string,multiModelCountsStatistics*>* allStats = new(map<string,multiModelCountsStatistics*>);
        multiModelCountsOptimizationCache * cache = new multiModelCountsOptimizationCache;

        Phrase sourcePhrase(0);
        sourcePhrase.CreateFromString(m_input, source_string, factorDelimiter);

        CollectSufficientStatistics(sourcePhrase, fs, allStats); //optimization potential: only call this once per source phrase

        //phrase pair not found; leave cache empty
        if (allStats->find(target_string) == allStats->end()) {
            delete allStats;
            continue;
        }

        multiModelCountsStatistics * targetStatistics = (*allStats)[target_string];
        cache->sourcePhrase = sourcePhrase;
        cache->fs = fs;
        cache->ft = targetStatistics->ft;
        cache->fst = targetStatistics->fst;
        cache->targetPhrase = targetStatistics->targetPhrase;
        pair<vector< set<size_t> >, vector< set<size_t> > > alignment = GetAlignmentsForLexWeights(sourcePhrase, static_cast<const Phrase&>(*targetStatistics->targetPhrase), targetStatistics->targetPhrase->GetAlignTerm());
        cache->alignment = alignment;

        (*optimizerStats)[phrase_pair] = cache;
        delete allStats;
        }

    vector<float> ret (m_numModels*4);
    for (size_t iFeature=0; iFeature < 4; iFeature++) {

        dlib::matrix<double,0,1> starting_point;
        starting_point.set_size(m_numModels);
        starting_point = 1.0;

        try {
            dlib::find_min_bobyqa(PerplexityFunctionCounts(phrase_pair_map, optimizerStats, this, iFeature),
                            starting_point,
                            2*m_numModels+1,    // number of interpolation points
                            dlib::uniform_matrix<double>(m_numModels,1, 1e-09),  // lower bound constraint
                            dlib::uniform_matrix<double>(m_numModels,1, 1e100),   // upper bound constraint
                            1.0,    // initial trust region radius
                            1e-5,  // stopping trust region radius
                            10000    // max number of objective function evaluations
            );
        }
        catch (dlib::bobyqa_failure& e)
        {
            cerr << e.what() << endl;
        }

        vector<float> weight_vector (m_numModels);

        for (int i=0; i < starting_point.nr(); i++) {
            weight_vector[i] = starting_point(i);
        }
        if (m_mode == "interpolate") {
            weight_vector = normalizeWeights(weight_vector);
        }
        else if (m_mode == "instance_weighting") {
            float first_value = weight_vector[0];
            for (int i=0; i < starting_point.nr(); i++) {
                weight_vector[i] = weight_vector[i]/first_value;
            }
        }
        cerr << "Weight vector for feature " << iFeature << ": ";
        for (size_t i=0; i < m_numModels; i++) {
            ret[(iFeature*m_numModels)+i] = weight_vector[i];
            cerr << weight_vector[i] << " ";
        }
        cerr << endl;

        cerr << "Cross-entropy: " << PerplexityFunctionCounts(phrase_pair_map, optimizerStats, this, iFeature)(starting_point) << endl;
    }

    for ( map<pair<string, string>, multiModelCountsOptimizationCache*>::const_iterator iter = optimizerStats->begin(); iter != optimizerStats->end(); ++iter ) {
        multiModelCountsOptimizationCache * cache = iter->second;
        delete cache;
    }
    delete optimizerStats;
    return ret;

}


// calculate weighted probability based on instance weighting of joint counts and marginal counts
double InstanceWeighting(vector<float> &joint_counts, vector<float> &marginals, vector<float> &multimodelweights) {

    double joint_counts_weighted =  inner_product(joint_counts.begin(), joint_counts.end(), multimodelweights.begin(), 0.0);
    double marginals_weighted = inner_product(marginals.begin(), marginals.end(), multimodelweights.begin(), 0.0);

    return joint_counts_weighted/marginals_weighted;
}


// calculate linear interpolation of relative frequency estimates based on joint count and marginal counts
//unused for now; enable in config?
double LinearInterpolationFromCounts(vector<float> &joint_counts, vector<float> &marginals, vector<float> &multimodelweights) {

    vector<float> p(marginals.size());

    for (size_t i=0;i < marginals.size();i++) {
        if (marginals[i] != 0) {
            p[i] = joint_counts[i]/marginals[i];
        }
    }

    double p_weighted = inner_product(p.begin(), p.end(), multimodelweights.begin(), 0.0);

    return p_weighted;
}

} //namespace