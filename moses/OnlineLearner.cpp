/*
 * OnlineLearner.cpp
 *
 */

#include "OnlineLearner.h"
#include "StaticData.h"
#include "math.h"
#include "Util.h"

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/vector_proxy.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/triangular.hpp>
#include <boost/numeric/ublas/lu.hpp>


using namespace Optimizer;

namespace MatrixOps{
	//    ------------------- Kronecker Product code ---------------------------- //

	bool KroneckerProduct (const boost::numeric::ublas::matrix<double>& A, const boost::numeric::ublas::matrix<double>& B, boost::numeric::ublas::matrix<double>& C) {
		int rowA=-1,colA=-1,rowB=0,colB=0,prowB=1,pcolB=1;
		for(int i=0; i<C.size1(); i++){
			for(int j=0; j<C.size2(); j++){
				rowB=i%B.size1();
				colB=j%B.size2();
				if(pcolB!=0 && colB == 0) colA++;
				if(prowB!=0 && rowB==0) rowA++;
				prowB=rowB;
				pcolB=colB;
				if(colA >= A.size2()){colA=0; colB=0;pcolB=1;}
				C(i, j) = A(rowA, colA) * B(rowB, colB) ;
			}
		}
		return true;
	}
	//    ------------------------------- ends here ---------------------------- //
    //    ------------------- matrix inversion code ---------------------------- //
    template<class T>
    bool InvertMatrix (const boost::numeric::ublas::matrix<T>& input, boost::numeric::ublas::matrix<T>& inverse) {
    	typedef boost::numeric::ublas::permutation_matrix<std::size_t> pmatrix;

    	// create a working copy of the input
    	boost::numeric::ublas::matrix<T> A(input);

    	// create a permutation matrix for the LU-factorization
    	pmatrix pm(A.size1());

    	// perform LU-factorization
    	int res = boost::numeric::ublas::lu_factorize(A, pm);
    	if (res != 0)
    		return false;

    	// create identity matrix of "inverse"
    	inverse.assign(boost::numeric::ublas::identity_matrix<T> (A.size1()));

    	// backsubstitute to get the inverse
    	boost::numeric::ublas::lu_substitute(A, pm, inverse);

    	return true;
    }
}

namespace Moses {

void OnlineLearner::chop(string &str) {
	int i = 0;
	while (isspace(str[i]) != 0) {
		str.replace(i, 1, "");
	}
	while (isspace(str[str.length() - 1]) != 0) {
		str.replace(str.length() - 1, 1, "");
	}
	return;
}

int OnlineLearner::split_marker_perl(string str, string marker, vector<string> &array) {
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

int OnlineLearner::getNGrams(std::string str, map<string, int>& ngrams) {
	vector<string> words;
	int numWords = split_marker_perl(str, " ", words);
	for (int i = 1; i <= 4; i++) {
		for (int start = 0; start < numWords - i + 1; start++) {
			string str = "";
			for (int pos = 0; pos < i; pos++) {
				str += words[start + pos] + " ";
			}
			ngrams[str]++;
		}
	}
	return str.size();
}

void OnlineLearner::compareNGrams(map<string, int>& hyp, map<string, int>& ref, map<int, float>& countNgrams, map<int, float>& TotalNgrams) {
	for (map<string, int>::const_iterator itr = hyp.begin(); itr != hyp.end(); itr++) {
		vector<string> temp;
		int ngrams = split_marker_perl((*itr).first, " ", temp);
		TotalNgrams[ngrams] += hyp[(*itr).first];
		if (ref.find((*itr).first) != ref.end()) {
			if (hyp[(*itr).first] >= ref[(*itr).first]) {
				countNgrams[ngrams] += hyp[(*itr).first];
			} else {
				countNgrams[ngrams] += ref[(*itr).first];
			}
		}
	}
	for (int i = 1; i <= 4; i++) {
		TotalNgrams[i]+=0.1;
		countNgrams[i]+=0.1;
	}
	return;
}

float OnlineLearner::GetBleu(std::string hypothesis, std::string reference)
{
	double bp=1;
	map<string, int> hypNgrams, refNgrams;
	map<int, float> countNgrams, TotalNgrams;
	map<int, double> BLEU;
	int length_translation = getNGrams(hypothesis, hypNgrams);
	int length_reference = getNGrams(reference, refNgrams);
	compareNGrams(hypNgrams, refNgrams, countNgrams, TotalNgrams);

	for(int i=1; i<=4; i++)
	{
		BLEU[i] = (countNgrams[i]*1.0)/(TotalNgrams[i]*1.0);
	}
	double ratio = ((length_reference*1.0+1.0) / (length_translation*1.0+1.0) );
	if(length_translation < length_reference)
		bp = exp(1 - ratio);
	return ((bp * exp( ( log(BLEU[1]) + log(BLEU[2]) + log(BLEU[3]) + log(BLEU[4]) ) / 4))*100);
}


bool OnlineLearner::has_only_spaces(const std::string& str) {
	return (str.find_first_not_of (' ') == str.npos);
}

bool OnlineLearner::SetPostEditedSentence(std::string s)
{
	if(m_postedited.empty())
	{
		m_postedited=s;
		return true;
	}
	else
	{
		cerr<<"post edited already exists.. "<<m_postedited<<endl;
		return false;
	}
}

OnlineLearner::OnlineLearner(OnlineAlgorithm algorithm, float w_learningrate, float f_learningrate,
		bool normaliseScore):StatelessFeatureFunction("OnlineLearner",0){
	flr = f_learningrate;
	wlr = w_learningrate;
	m_learn=false;
	m_PPindex=0;
	m_normaliseScore=normaliseScore;
	implementation=algorithm;
	cerr<<"Initialization Online Learning Model\n";
}

OnlineLearner::OnlineLearner(OnlineAlgorithm algorithm, float w_learningrate, float f_learningrate,
		float slack, float scale_margin, float scale_margin_precision,	float scale_update,
		float scale_update_precision, bool boost, bool normaliseMargin, bool normaliseScore, int sigmoidParam,
		bool onlyOnlineScoreProducerUpdate):StatelessFeatureFunction("OnlineLearner",0){
	flr = f_learningrate;
	wlr = w_learningrate;
	m_PPindex=0;
	m_learn=false;
	m_normaliseScore=normaliseScore;
	implementation=algorithm;
	optimiser = new Optimizer::MiraOptimiser(slack, scale_margin, scale_margin_precision, scale_update,
			scale_update_precision, boost, normaliseMargin, sigmoidParam, onlyOnlineScoreProducerUpdate);
	cerr<<"Initialization Online Learning Model\n";
}


void OnlineLearner::ShootUp(std::string sp, std::string tp, float margin){
	if(m_feature.find(sp)!=m_feature.end())
	{
		if(m_feature[sp].find(tp)!=m_feature[sp].end())
		{
			float val=m_feature[sp][tp];
			val += flr * margin;
			m_feature[sp][tp]=val;
		}
		else
		{
			m_feature[sp][tp]=flr*margin;
		}
	}
	else
	{
		m_feature[sp][tp]=flr*margin;
	}
	//if(m_feature[sp][tp]>1){m_feature[sp][tp]==1;}
}
void OnlineLearner::ShootDown(std::string sp, std::string tp, float margin){
	if(m_feature.find(sp)!=m_feature.end())
	{
		if(m_feature[sp].find(tp)!=m_feature[sp].end())
		{
			float val=m_feature[sp][tp];
			val -= flr * margin;
			m_feature[sp][tp]=val;
		}
		else
		{
			m_feature[sp][tp]= 0;
		}
	}
	else
	{
		m_feature[sp][tp]= 0;
	}
	if(m_feature[sp][tp]<0){m_feature[sp][tp]==0;}
}

void OnlineLearner::DumpFeatures(std::string filename)
{
	ofstream file;
	file.open(filename.c_str(), ios::out);
	if(file.is_open())
	{
		pp_feature::iterator itr1=m_feature.begin();
		while(itr1!=m_feature.end())
		{
			std::map<std::string, float>::iterator itr2=(*itr1).second.begin();
			while(itr2!=(*itr1).second.end())
			{
				file << itr1->first <<"|||"<<itr2->first<<"|||"<<itr2->second<<endl;
				itr2++;
			}
			itr1++;
		}
	}
	file.close();
}
void OnlineLearner::ReadFeatures(std::string filename)
{
	ifstream file;
	file.open(filename.c_str(), ios::in);
	std::string line;
	if(file.is_open())
	{
		while(getline(file, line)){
			chop(line);
			std::vector<string> splits;
			split_marker_perl(line, "|||", splits);		// line:string1|||string2|||score
			if(splits.size()==3){
				float score;
				stringstream(splits[2])>>score;
				m_feature[splits[0]][splits[1]] = score;
			}
			else{
				TRACE_ERR("The format of feature file does not comply!");
			}
		}
	}
	file.close();
}

//float OnlineLearner::calcMargin(Hypothesis* oracle, Hypothesis* bestHyp)
//{
//	return (oracle->GetScore()-bestHyp->GetScore());
//}
// clears history
void OnlineLearner::RemoveJunk()
{
	m_postedited.clear();
	PP_ORACLE.clear();
	PP_BEST.clear();
}
// insertion of sparse features .. for now its the phrase pair
// have to generalize this later !
void OnlineLearner::Insert(std::string sp, std::string tp)
{
	if(m_featureIdx.find(sp)==m_featureIdx.end())
	{
		if(m_featureIdx[sp].find(tp)==m_featureIdx[sp].end())
		{
			m_featureIdx[sp][tp]=sparseweightvector.GetSize();
			sparseweightvector.AddFeat(0.001);
			m_PPindex++;
			return;
		}
	}
	else if(m_featureIdx.find(sp)!=m_featureIdx.end())
	{
		if(m_featureIdx[sp].find(tp)==m_featureIdx[sp].end())
		{
			m_featureIdx[sp][tp]=sparseweightvector.GetSize();
			sparseweightvector.AddFeat(0.001);
			m_PPindex++;
		}
		return;
	}
	return;
}

int OnlineLearner::RetrieveIdx(std::string sp,std::string tp)
{
	if(m_featureIdx.find(sp)!=m_featureIdx.end())
	{
		if(m_featureIdx[sp].find(tp)!=m_featureIdx[sp].end())
		{
			return m_featureIdx[sp][tp];
		}
	}
	return m_PPindex;
}

OnlineLearner::~OnlineLearner() {
	pp_feature::iterator iter;
	for(iter=m_feature.begin(); iter!=m_feature.end(); iter++)
	{
		(*iter).second.clear();
	}
}

void OnlineLearner::Evaluate(const TargetPhrase& tp, ScoreComponentCollection* out) const
{
	float score=0.0;
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
	pp_feature::const_iterator it;
	it=m_feature.find(s);
	if(it!=m_feature.end())
	{
		std::map<std::string, float>::const_iterator it2;
		it2=it->second.find(t);
		if(it2!=it->second.end())
		{
			score=it2->second;
		}
	}
	if(m_normaliseScore)
		score = (2/(1+exp(-score))) - 1;	// normalising score!
	//cerr<<"Source : "<<s<<"\tTarget : "<<t<<"\tScore : "<<score<<endl;
	const std::string featureName = s+"|||"+t;
	out->SparsePlusEquals(featureName, score);
}


void OnlineLearner::Evaluate(const PhraseBasedFeatureContext& context, ScoreComponentCollection* accumulator) const
{
	const TargetPhrase& tp = context.GetTargetPhrase();
	Evaluate(tp, accumulator);
}

void OnlineLearner::EvaluateChart(const ChartBasedFeatureContext& context, ScoreComponentCollection* accumulator) const
{
	const TargetPhrase& tp = context.GetTargetPhrase();
	Evaluate(tp, accumulator);
}

void OnlineLearner::PrintHypo(const Hypothesis* hypo, ostream& HypothesisStringStream) {
	if (hypo->GetPrevHypo() != NULL) {
		PrintHypo(hypo->GetPrevHypo(), HypothesisStringStream);
		Phrase p = hypo->GetCurrTargetPhrase();
		for (size_t pos = 0; pos < p.GetSize(); pos++) {
			const Factor *factor = p.GetFactor(pos, 0);
			HypothesisStringStream << *factor << " ";
		}
		std::string sourceP = hypo->GetSourcePhraseStringRep();
		std::string targetP = hypo->GetTargetPhraseStringRep();
		PP_BEST[sourceP][targetP]=1;
	}
}
void OnlineLearner::Decay(int lineNum)
{
	float decay_value = 1.0/(exp(lineNum)*1.0);
	pp_feature::iterator itr1=m_feature.begin();
	while(itr1!=m_feature.end())
	{
		std::map<std::string, float>::iterator itr2=(*itr1).second.begin();
		while(itr2!=(*itr1).second.end())
		{
			float score=m_feature[itr1->first][itr2->first];
			score *= decay_value;
			m_feature[itr1->first][itr2->first]=score;
			itr2++;
		}
		itr1++;
	}
}
void OnlineLearner::RunOnlineLearning(Manager& manager)
{
	const TranslationSystem &trans_sys = StaticData::Instance().GetTranslationSystem(TranslationSystem::DEFAULT);
	const StaticData& staticData = StaticData::Instance();
	const std::vector<Moses::FactorType>& outputFactorOrder=staticData.GetOutputFactorOrder();
	ScoreComponentCollection weightUpdate = staticData.GetAllWeights();
	cerr<<"Total number of scores are :"<<weightUpdate.Size()<<"\n";
	const Hypothesis* hypo = manager.GetBestHypothesis();
	//	Decay(manager.m_lineNumber);
	stringstream bestHypothesis;
	PP_BEST.clear();
	PrintHypo(hypo, bestHypothesis);
	float bestbleu = GetBleu(bestHypothesis.str(), m_postedited);
	float bestScore=hypo->GetScore();
	cerr<<"Best Hypothesis : "<<bestHypothesis.str()<<endl;
	cerr<<"Post Edit       : "<<m_postedited<<endl;
	TrellisPathList nBestList;
	int nBestSize = 100;
	manager.CalcNBest(nBestSize, nBestList, true);

	std::string bestOracle;
	std::vector<string> HypothesisList, HypothesisHope, HypothesisFear;
	std::vector<float> loss, BleuScore, BleuScoreHope, BleuScoreFear, oracleBleuScores, lossHope, lossFear, modelScore, oracleModelScores;
	std::vector<std::vector<float> > losses, BleuScores, BleuScoresHope, BleuScoresFear, lossesHope, lossesFear, modelScores;
	std::vector<ScoreComponentCollection> featureValue,featureValueHope, featureValueFear, oraclefeatureScore;
	std::vector<std::vector<ScoreComponentCollection> > featureValues, featureValuesHope, featureValuesFear;
	std::map<int, map<string, map<string, int> > > OracleList;
	TrellisPathList::const_iterator iter;
	pp_list BestOracle,ShootemUp, ShootemDown,Visited;
	float maxBleu=0.0, maxScore=0.0,oracleScore=0.0;
	int whichoracle=-1;
	for (iter = nBestList.begin(); iter != nBestList.end(); ++iter) {
		whichoracle++;
		const TrellisPath &path = **iter;
		PP_ORACLE.clear();
		const std::vector<const Hypothesis *> &edges = path.GetEdges();
		stringstream oracle;
		for (int currEdge = (int) edges.size() - 1; currEdge >= 0; currEdge--) {
			const Hypothesis &edge = *edges[currEdge];
			CHECK(outputFactorOrder.size() > 0);
			size_t size = edge.GetCurrTargetPhrase().GetSize();
			for (size_t pos = 0; pos < size; pos++) {
				const Factor *factor = edge.GetCurrTargetPhrase().GetFactor(pos, outputFactorOrder[0]);
				oracle << *factor;
				oracle << " ";
			}
			std::string sourceP = edge.GetSourcePhraseStringRep();  // Source Phrase
			std::string targetP = edge.GetTargetPhraseStringRep();  // Target Phrase
			if(!has_only_spaces(sourceP) && !has_only_spaces(targetP) )
			{
				PP_ORACLE[sourceP][targetP]=1;	// phrase pairs in the current nbest_i
				OracleList[whichoracle][sourceP][targetP]=1;	// list of all phrase pairs given the nbest_i
//				Insert(sourceP, targetP);	// I insert all the phrase pairs that I see in NBEST list
			}
		}
		oracleScore=path.GetTotalScore();
		float oraclebleu = GetBleu(oracle.str(), m_postedited);
		if(implementation != FOnlyPerceptron){
			HypothesisList.push_back(oracle.str());
			BleuScore.push_back(oraclebleu);
			featureValue.push_back(path.GetScoreBreakdown());
			modelScore.push_back(oracleScore);
		}
		if(oraclebleu > maxBleu)
		{
			cerr<<"NBEST : "<<oracle.str()<<"\t|||\tBLEU : "<<oraclebleu<<endl;
			maxBleu=oraclebleu;
			maxScore=oracleScore;
			bestOracle = oracle.str();
			pp_list::const_iterator it1;
			oracleBleuScores.clear();
			oraclefeatureScore.clear();
			BestOracle=PP_ORACLE;
			oracleBleuScores.push_back(oraclebleu);
			oraclefeatureScore.push_back(path.GetScoreBreakdown());
		}
// ------------------------trial--------------------------------//
		if(implementation==FPercepWMira)
		{
			if(oraclebleu > bestbleu)
			{
				pp_list::const_iterator it1;
				for(it1=PP_ORACLE.begin(); it1!=PP_ORACLE.end(); it1++)
				{
					std::map<std::string, int>::const_iterator itr1;
					for(itr1=(it1->second).begin(); itr1!=(it1->second).end(); itr1++)
					{
						if(PP_BEST[it1->first][itr1->first]!=1 && Visited[it1->first][itr1->first]!=1)
						{
							ShootUp(it1->first, itr1->first, abs(oracleScore-bestScore));
							Visited[it1->first][itr1->first]=1;
						}
					}
				}
				for(it1=PP_BEST.begin(); it1!=PP_BEST.end(); it1++)
				{
					std::map<std::string, int>::const_iterator itr1;
					for(itr1=(it1->second).begin(); itr1!=(it1->second).end(); itr1++)
					{
						if(PP_ORACLE[it1->first][itr1->first]!=1 && Visited[it1->first][itr1->first]!=1)
						{
							ShootDown(it1->first, itr1->first, abs(oracleScore-bestScore));
							Visited[it1->first][itr1->first]=1;
						}
					}
				}
			}
			if(oraclebleu < bestbleu)
			{
				pp_list::const_iterator it1;
				for(it1=PP_ORACLE.begin(); it1!=PP_ORACLE.end(); it1++)
				{
					std::map<std::string, int>::const_iterator itr1;
					for(itr1=(it1->second).begin(); itr1!=(it1->second).end(); itr1++)
					{
						if(PP_BEST[it1->first][itr1->first]!=1 && Visited[it1->first][itr1->first]!=1)
						{
							ShootDown(it1->first, itr1->first, abs(oracleScore-bestScore));
							Visited[it1->first][itr1->first]=1;
						}
					}
				}
				for(it1=PP_BEST.begin(); it1!=PP_BEST.end(); it1++)
				{
					std::map<std::string, int>::const_iterator itr1;
					for(itr1=(it1->second).begin(); itr1!=(it1->second).end(); itr1++)
					{
						if(PP_ORACLE[it1->first][itr1->first]!=1 && Visited[it1->first][itr1->first]!=1)
						{
							ShootUp(it1->first, itr1->first, abs(oracleScore-bestScore));
							Visited[it1->first][itr1->first]=1;
						}
					}
				}
			}
		}
// ------------------------trial--------------------------------//
	}
	VERBOSE(1, "Read all the oracles in the list!\n");

	//	Update the weights
	if(implementation == FPercepWMira || implementation == Mira)
	{
		for (int i=0;i<HypothesisList.size();i++) // same loop used for feature values, modelscores
		{
			float bleuscore = BleuScore[i];
			loss.push_back(maxBleu-bleuscore);
		}
		modelScores.push_back(modelScore);
		featureValues.push_back(featureValue);
		BleuScores.push_back(BleuScore);
		losses.push_back(loss);
		oracleModelScores.push_back(maxScore);
		cerr<<"Updating the Weights\n";
		size_t update_status = optimiser->updateWeights(weightUpdate,featureValues, losses,
				BleuScores, modelScores, oraclefeatureScore,oracleBleuScores, oracleModelScores,wlr);
		StaticData::InstanceNonConst().SetAllWeights(weightUpdate);
		weightUpdate.PrintCoreFeatures();
		cerr<<endl;
	}
	return;
}

void OnlineLearner::RunOnlineMultiTaskLearning(Manager& manager, int task)
{
	const TranslationSystem &trans_sys = StaticData::Instance().GetTranslationSystem(TranslationSystem::DEFAULT);
	const StaticData& staticData = StaticData::Instance();
	const std::vector<Moses::FactorType>& outputFactorOrder=staticData.GetOutputFactorOrder();
	ScoreComponentCollection weightUpdate = staticData.GetAllWeights();
	std::vector<const ScoreProducer*> sps = trans_sys.GetFeatureFunctions();
	ScoreProducer* sp = const_cast<ScoreProducer*>(sps[0]);
	for(int i=0;i<sps.size();i++)
	{
		if(sps[i]->GetScoreProducerDescription().compare("OnlineLearner")==0)
		{
			sp=const_cast<ScoreProducer*>(sps[i]);
			break;
		}
	}
//	m_weight=weightUpdate.GetScoreForProducer(sp);	// permanent weight stored in decoder

	const Hypothesis* hypo = manager.GetBestHypothesis();
	//	Decay(manager.m_lineNumber);
	stringstream bestHypothesis;
	PP_BEST.clear();
	PrintHypo(hypo, bestHypothesis);
	float bestbleu = GetBleu(bestHypothesis.str(), m_postedited);
	float bestScore=hypo->GetScore();
	cerr<<"Best Hypothesis : "<<bestHypothesis.str()<<endl;
	cerr<<"Post Edit       : "<<m_postedited<<endl;
	TrellisPathList nBestList;
	int nBestSize = 100;
	manager.CalcNBest(nBestSize, nBestList, true);

	//------setting the hyperparameters online-----
	if(staticData.GetHyperParameterAsWeight()){
		wlr=staticData.m_wlr;
		flr=staticData.m_flr;
		optimiser->setSlack(staticData.m_C);
	}
	//----------------------------------------------


	std::string bestOracle;
	std::vector<string> HypothesisList, HypothesisHope, HypothesisFear;
	std::vector<float> loss, BleuScore, BleuScoreHope, BleuScoreFear, oracleBleuScores, lossHope, lossFear, modelScore, oracleModelScores;
	std::vector<std::vector<float> > losses, BleuScores, BleuScoresHope, BleuScoresFear, lossesHope, lossesFear, modelScores;
	std::vector<ScoreComponentCollection> featureValue,featureValueHope, featureValueFear, oraclefeatureScore;
	std::vector<std::vector<ScoreComponentCollection> > featureValues, featureValuesHope, featureValuesFear;
	std::map<int, map<string, map<string, int> > > OracleList;
	TrellisPathList::const_iterator iter;
	pp_list BestOracle,ShootemUp, ShootemDown,Visited;
	float maxBleu=0.0, maxScore=0.0,oracleScore=0.0;
	int whichoracle=-1;
	for (iter = nBestList.begin(); iter != nBestList.end(); ++iter) {
		whichoracle++;
		const TrellisPath &path = **iter;
		PP_ORACLE.clear();
		const std::vector<const Hypothesis *> &edges = path.GetEdges();
		stringstream oracle;
		for (int currEdge = (int) edges.size() - 1; currEdge >= 0; currEdge--) {
			const Hypothesis &edge = *edges[currEdge];
			CHECK(outputFactorOrder.size() > 0);
			size_t size = edge.GetCurrTargetPhrase().GetSize();
			for (size_t pos = 0; pos < size; pos++) {
				const Factor *factor = edge.GetCurrTargetPhrase().GetFactor(pos, outputFactorOrder[0]);
				oracle << *factor;
				oracle << " ";
			}
			std::string sourceP = edge.GetSourcePhraseStringRep();  // Source Phrase
			std::string targetP = edge.GetTargetPhraseStringRep();  // Target Phrase
			if(!has_only_spaces(sourceP) && !has_only_spaces(targetP) )
			{
				PP_ORACLE[sourceP][targetP]=1;	// phrase pairs in the current nbest_i
				OracleList[whichoracle][sourceP][targetP]=1;	// list of all phrase pairs given the nbest_i
			}
		}
		oracleScore=path.GetTotalScore();
		float oraclebleu = GetBleu(oracle.str(), m_postedited);
		if(implementation != FOnlyPerceptron){
			HypothesisList.push_back(oracle.str());
			BleuScore.push_back(oraclebleu);
			featureValue.push_back(path.GetScoreBreakdown());
			modelScore.push_back(oracleScore);
		}
		if(oraclebleu > maxBleu)
		{
			cerr<<"NBEST : "<<oracle.str()<<"\t|||\tBLEU : "<<oraclebleu<<endl;
			maxBleu=oraclebleu;
			maxScore=oracleScore;
			bestOracle = oracle.str();
			pp_list::const_iterator it1;
			oracleBleuScores.clear();
			oraclefeatureScore.clear();
			BestOracle=PP_ORACLE;
			oracleBleuScores.push_back(oraclebleu);
			oraclefeatureScore.push_back(path.GetScoreBreakdown());
		}
		// ------------------------trial--------------------------------//
		if(implementation==FSparsePercepWSparseMira || implementation==FPercepWMira)
		{
			if(oraclebleu > bestbleu)
			{
				pp_list::const_iterator it1;
				for(it1=PP_ORACLE.begin(); it1!=PP_ORACLE.end(); it1++)
				{
					std::map<std::string, int>::const_iterator itr1;
					for(itr1=(it1->second).begin(); itr1!=(it1->second).end(); itr1++)
					{
						if(PP_BEST[it1->first][itr1->first]!=1 && Visited[it1->first][itr1->first]!=1)
						{
							ShootUp(it1->first, itr1->first, abs(oracleScore-bestScore));
							Visited[it1->first][itr1->first]=1;
						}
					}
				}
				for(it1=PP_BEST.begin(); it1!=PP_BEST.end(); it1++)
				{
					std::map<std::string, int>::const_iterator itr1;
					for(itr1=(it1->second).begin(); itr1!=(it1->second).end(); itr1++)
					{
						if(PP_ORACLE[it1->first][itr1->first]!=1 && Visited[it1->first][itr1->first]!=1)
						{
							ShootDown(it1->first, itr1->first, abs(oracleScore-bestScore));
							Visited[it1->first][itr1->first]=1;
						}
					}
				}
			}
			if(oraclebleu < bestbleu)
			{
				pp_list::const_iterator it1;
				for(it1=PP_ORACLE.begin(); it1!=PP_ORACLE.end(); it1++)
				{
					std::map<std::string, int>::const_iterator itr1;
					for(itr1=(it1->second).begin(); itr1!=(it1->second).end(); itr1++)
					{
						if(PP_BEST[it1->first][itr1->first]!=1 && Visited[it1->first][itr1->first]!=1)
						{
							ShootDown(it1->first, itr1->first, abs(oracleScore-bestScore));
							Visited[it1->first][itr1->first]=1;
						}
					}
				}
				for(it1=PP_BEST.begin(); it1!=PP_BEST.end(); it1++)
				{
					std::map<std::string, int>::const_iterator itr1;
					for(itr1=(it1->second).begin(); itr1!=(it1->second).end(); itr1++)
					{
						if(PP_ORACLE[it1->first][itr1->first]!=1 && Visited[it1->first][itr1->first]!=1)
						{
							ShootUp(it1->first, itr1->first, abs(oracleScore-bestScore));
							Visited[it1->first][itr1->first]=1;
						}
					}
				}
			}
		}
		// ------------------------trial--------------------------------//
	}

	//	Update the weights
	if(implementation == FPercepWMira || implementation == Mira)
	{
		for (int i=0;i<HypothesisList.size();i++) // same loop used for feature values, modelscores
		{
			float bleuscore = BleuScore[i];
			loss.push_back(maxBleu-bleuscore);
		}
		modelScores.push_back(modelScore);
		featureValues.push_back(featureValue);
		BleuScores.push_back(BleuScore);
		losses.push_back(loss);
		oracleModelScores.push_back(maxScore);
		cerr<<"Updating the Weights\n";
		// update the weights for ith task with ith learningrate

		for(int z=0; z<StaticData::InstanceNonConst().GetMultiTaskLearner()->GetNumberOfTasks(); z++){
			weightUpdate = StaticData::InstanceNonConst().GetMultiTaskLearner()->GetWeightsVector(z);
			size_t update_status = optimiser->updateMultiTaskLearningWeights(weightUpdate,sp,featureValues, losses,
					BleuScores, modelScores, oraclefeatureScore,oracleBleuScores, oracleModelScores,
					staticData.GetMultiTaskLearner()->GetKdKdMatrix(),
					staticData.GetMultiTaskLearner()->GetNumberOfTasks(),task);
			// set the weights in the memory for ith task
			weightUpdate.PrintCoreFeatures();
			cerr<<endl;
			StaticData::InstanceNonConst().GetMultiTaskLearner()->SetWeightsVector(task, weightUpdate);
		}

		// update the interaction matrix
		bool miraUpdateIntMatrix = StaticData::InstanceNonConst().GetMultiTaskLearner()->IfUpdateIntMatrix();
	    if(miraUpdateIntMatrix){
	    	updateIntMatrix();
	    }
	}
	return;
}

void OnlineLearner::updateIntMatrix(){

	boost::numeric::ublas::matrix<double> W = StaticData::InstanceNonConst().GetMultiTaskLearner()->GetWeightsMatrix();
	std::cerr << "\n\nWeight Matrix = ";
	std::cerr << W << endl;
	boost::numeric::ublas::matrix<double> updated = StaticData::InstanceNonConst().GetMultiTaskLearner()->GetInteractionMatrix();

	if(updateType == vonNeumann){
		// log (A^{-1}_t) = log (A^{-1}_{t-1}) - \frac{\eta} * (W^T_{t-1} \times W_{t-1} + W_{t-1} \times W^T_{t-1})
		float eta = StaticData::Instance().GetMultiTaskLearner()->GetLearningRateIntMatrix();
		boost::numeric::ublas::matrix<double> sub = prod(trans(W), W) + trans(prod(trans(W), W)) ;
		std::transform(updated.data().begin(), updated.data().end(), updated.data().begin(), ::log);
		updated -= eta * sub;
		std::transform(updated.data().begin(), updated.data().end(), updated.data().begin(), ::exp);
		StaticData::InstanceNonConst().GetMultiTaskLearner()->SetInteractionMatrix(updated);
		std::cerr << "Updated = ";
		std::cerr << updated << endl;
	}

	if(updateType == logDet){
		// A^{-1}_t = A^{-1}_{t-1} + \frac{\eta}{2} * (W^T_{t-1} \times W_{t-1} + W_{t-1} \times W^T_{t-1})
		float eta = StaticData::Instance().GetMultiTaskLearner()->GetLearningRateIntMatrix();
		boost::numeric::ublas::matrix<double> adding = prod(trans(W), W) + trans(prod(trans(W), W)) ;
		updated += eta * adding;
		StaticData::InstanceNonConst().GetMultiTaskLearner()->SetInteractionMatrix(updated);
		std::cerr << "Updated = ";
		std::cerr << updated << endl;

	}
	// update kd x kd matrix because it is used in weight update not the interaction matrix
	int size = StaticData::Instance().GetAllWeights().Size();
	int tasks= StaticData::InstanceNonConst().GetMultiTaskLearner()->GetNumberOfTasks();
	boost::numeric::ublas::matrix<double> kdkdmatrix (tasks*size, tasks*size);
	boost::numeric::ublas::identity_matrix<double> m (size);
	MatrixOps::KroneckerProduct(updated, m, kdkdmatrix);
	StaticData::InstanceNonConst().GetMultiTaskLearner()->SetKdKdMatrix(kdkdmatrix);

}

}
