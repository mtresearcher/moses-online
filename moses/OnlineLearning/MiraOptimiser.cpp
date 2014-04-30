#include "Optimiser.h"
#include "Hildreth.h"
#include "../StaticData.h"
#include <iomanip>

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/vector_proxy.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/triangular.hpp>
#include <boost/numeric/ublas/lu.hpp>

using namespace Moses;
using namespace std;

namespace Optimizer {



size_t MiraOptimiser::updateMultiTaskLearningWeights(
	ScoreComponentCollection& weightUpdate,
	const ScoreProducer* sp,
    const vector<vector<ScoreComponentCollection> >& featureValues,
    const vector<vector<float> >& losses,
    const vector<vector<float> >& bleuScores,
    const vector<vector<float> >& modelScores,
    const vector<ScoreComponentCollection>& oracleFeatureValues,
    const vector<float> oracleBleuScores,
    const vector<float> oracleModelScores,
    const boost::numeric::ublas::matrix<double>& regularizer,
    const int task,
    const int task_id) {
	cerr<<"Dimensions of regularizer : "<<regularizer.size1()<<" x "<<regularizer.size2()<<endl;
	// vector of feature values differences for all created constraints
	vector<ScoreComponentCollection> featureValueDiffs;
	vector<float> lossMinusModelScoreDiffs;
	vector<float> all_losses;

	// Make constraints for new hypothesis translations
	float epsilon = 0.0001;
	int violatedConstraintsBefore = 0;
	float oldDistanceFromOptimum = 0;

	// iterate over input sentences (1 (online) or more (batch))
	for (size_t i = 0; i < featureValues.size(); ++i) {
		//size_t sentenceId = sentenceIds[i];
		// iterate over hypothesis translations for one input sentence
		for (size_t j = 0; j < featureValues[i].size(); ++j) {
			ScoreComponentCollection featureValueDiff = oracleFeatureValues[i];
			featureValueDiff.MinusEquals(featureValues[i][j]);

			if (featureValueDiff.GetL1Norm() == 0) { // over sparse & core features values
				continue;
			}

			float loss = losses[i][j];

		  	// check if constraint is violated
		    bool violated = false;
//		    float modelScoreDiff = featureValueDiff.InnerProduct(currWeights);
		    float modelScoreDiff = oracleModelScores[i] - modelScores[i][j];
		    float diff = 0;

		    if (loss > modelScoreDiff)
		    	diff = loss - modelScoreDiff;
//		    cerr<<"Loss ("<<loss<<") - modelScoreDiff ("<<modelScoreDiff<<") = "<<diff<<"\n";
		    if (diff > epsilon)
		    	violated = true;
		    if (m_normaliseMargin) {
		      modelScoreDiff = (2*m_sigmoidParam/(1 + exp(-modelScoreDiff))) - m_sigmoidParam;
		      loss = (2*m_sigmoidParam/(1 + exp(-loss))) - m_sigmoidParam;
		      diff = 0;
		      if (loss > modelScoreDiff) {
			diff = loss - modelScoreDiff;
		      }
		    }

		    if (m_scale_margin) {
		      diff *= oracleBleuScores[i];
		    }

		    featureValueDiffs.push_back(featureValueDiff);
		    lossMinusModelScoreDiffs.push_back(diff);
		    all_losses.push_back(loss);
		    if (violated) {
		      ++violatedConstraintsBefore;
		      oldDistanceFromOptimum += diff;
		    }
		}
	}

	// run optimisation: compute alphas for all given constraints
	vector<float> alphas;
	ScoreComponentCollection summedUpdate;
	if (violatedConstraintsBefore > 0) {
//		cerr<<"Features values diff size : "<<featureValueDiffs.size() << " (of which violated: " << violatedConstraintsBefore << ")" << endl;
	  if (m_slack != 0) {
	    alphas = Hildreth::optimise(featureValueDiffs, lossMinusModelScoreDiffs, m_slack);
	    cerr<<"Alphas : ";for (int i=0;i<alphas.size();i++) cerr<<alphas[i]<<" ";cerr<<"\n";
	  } else {
	    alphas = Hildreth::optimise(featureValueDiffs, lossMinusModelScoreDiffs);
	    cerr<<"Alphas : ";for (int i=0;i<alphas.size();i++) cerr<<alphas[i]<<" ";cerr<<"\n";
	  }

	  // Update the weight vector according to the alphas and the feature value differences
	  // * w' = w' + SUM alpha_i * (h_i(oracle) - h_i(hypothesis))
	  for (size_t k = 0; k < featureValueDiffs.size(); ++k) {
	  	float alpha = alphas[k];
	  	ScoreComponentCollection update(featureValueDiffs[k]);

	    update.MultiplyEquals(alpha);

	    // build the feature matrix
	    boost::numeric::ublas::matrix<double> featureMatrix(update.Size()*task,1);
	    for(size_t i=0; i<task; i++){
	    	if(i == task_id){
	    		const Moses::FVector x = update.GetScoresVector();
	    		for(int j=0;j<x.size(); j++){
	    			featureMatrix (i*update.Size()+j,0) = x[j];
	    			if(x[j]!=0)
	    				cerr<<std::setprecision(9)<<i*update.Size()+j<<", 0 : "<<x[j]<<endl;
	    		}
	    	}
	    	else{
	    		for(int j=0;j<update.Size(); j++){
	    			featureMatrix (i*update.Size()+j,0) = 0;
//	    			cerr<<i*update.Size()+j<<", 0 : "<<0<<endl;
	    		}
	    	}
	    }
	    cerr<<"Dimensions of feature matrix : "<<featureMatrix.size1()<<" x "<<featureMatrix.size2()<<endl;
	    // take dot prod. of kdkd matrix and feature matrix
	    boost::numeric::ublas::matrix<double> C = boost::numeric::ublas::prod(regularizer, featureMatrix);
	    cerr<<"Dimensions of product : "<<C.size1()<<" x "<<C.size2()<<endl;
	    // make a ScoreComponentCollection that can be multiplied with the update ScoreComponentCollection
	    ScoreComponentCollection temp(update);
	    cerr<<"Temp Size : "<<temp.Size()<<endl;
	    for(size_t i=0;i<update.Size();i++){
	    	if(C(task_id*update.Size()+i, 1)!=0)
	    		cerr<<"Assigning : "<<i<<"th SP : "<<task_id*update.Size()+i<<" : "<<C(task_id*update.Size()+i, 1)<<endl;
	    	temp.Assign(i, C(task_id*update.Size()+i, 1));
	    }
//	    const ScoreComponentCollection learningrates(temp);
	    // here we also multiply with the co-regularization vector
	    update.MultiplyEquals(temp);

	    // sum updates
	    summedUpdate.PlusEquals(update);
	  }
	}
	else {
		return 1;
	}

	// scale update by BLEU of oracle (for batch size 1 only)
	if (oracleBleuScores.size() == 1) {
		if (m_scale_update) {
			summedUpdate.MultiplyEquals(oracleBleuScores[0]);
		}
	}
	if(m_onlyOnlineScoreProducerUpdate)
		weightUpdate.PlusEquals(sp,summedUpdate);
	else
		weightUpdate.PlusEquals(summedUpdate);

	return 0;
}

size_t MiraOptimiser::updateWeights(
	ScoreComponentCollection& weightUpdate,
	const ScoreProducer* sp,
    const vector<vector<ScoreComponentCollection> >& featureValues,
    const vector<vector<float> >& losses,
    const vector<vector<float> >& bleuScores,
    const vector<vector<float> >& modelScores,
    const vector<ScoreComponentCollection>& oracleFeatureValues,
    const vector<float> oracleBleuScores,
    const vector<float> oracleModelScores,
    float learning_rate) {

	// vector of feature values differences for all created constraints
	vector<ScoreComponentCollection> featureValueDiffs;
	vector<float> lossMinusModelScoreDiffs;
	vector<float> all_losses;

	// Make constraints for new hypothesis translations
	float epsilon = 0.0001;
	int violatedConstraintsBefore = 0;
	float oldDistanceFromOptimum = 0;
	// iterate over input sentences (1 (online) or more (batch))
	for (size_t i = 0; i < featureValues.size(); ++i) {
		//size_t sentenceId = sentenceIds[i];
		// iterate over hypothesis translations for one input sentence
		for (size_t j = 0; j < featureValues[i].size(); ++j) {
			ScoreComponentCollection featureValueDiff = oracleFeatureValues[i];
			featureValueDiff.MinusEquals(featureValues[i][j]);

			if (featureValueDiff.GetL1Norm() == 0) { // over sparse & core features values
				continue;
			}

			float loss = losses[i][j];

		  	// check if constraint is violated
		    bool violated = false;
//		    float modelScoreDiff = featureValueDiff.InnerProduct(currWeights);
		    float modelScoreDiff = oracleModelScores[i] - modelScores[i][j];
		    float diff = 0;

		    if (loss > modelScoreDiff)
		    	diff = loss - modelScoreDiff;
//		    cerr<<"Loss ("<<loss<<") - modelScoreDiff ("<<modelScoreDiff<<") = "<<diff<<"\n";
		    if (diff > epsilon)
		    	violated = true;
		    if (m_normaliseMargin) {
		      modelScoreDiff = (2*m_sigmoidParam/(1 + exp(-modelScoreDiff))) - m_sigmoidParam;
		      loss = (2*m_sigmoidParam/(1 + exp(-loss))) - m_sigmoidParam;
		      diff = 0;
		      if (loss > modelScoreDiff) {
			diff = loss - modelScoreDiff;
		      }
		    }

		    if (m_scale_margin) {
		      diff *= oracleBleuScores[i];
		    }

		    featureValueDiffs.push_back(featureValueDiff);
		    lossMinusModelScoreDiffs.push_back(diff);
		    all_losses.push_back(loss);
		    if (violated) {
		      ++violatedConstraintsBefore;
		      oldDistanceFromOptimum += diff;
		    }
		}
	}

	// run optimisation: compute alphas for all given constraints
	vector<float> alphas;
	ScoreComponentCollection summedUpdate;
	if (violatedConstraintsBefore > 0) {
//		cerr<<"Features values diff size : "<<featureValueDiffs.size() << " (of which violated: " << violatedConstraintsBefore << ")" << endl;
	  if (m_slack != 0) {
	    alphas = Hildreth::optimise(featureValueDiffs, lossMinusModelScoreDiffs, m_slack);
	    cerr<<"Alphas : ";for (int i=0;i<alphas.size();i++) cerr<<alphas[i]<<" ";cerr<<"\n";
	  } else {
	    alphas = Hildreth::optimise(featureValueDiffs, lossMinusModelScoreDiffs);
	    cerr<<"Alphas : ";for (int i=0;i<alphas.size();i++) cerr<<alphas[i]<<" ";cerr<<"\n";
	  }

	  // Update the weight vector according to the alphas and the feature value differences
	  // * w' = w' + SUM alpha_i * (h_i(oracle) - h_i(hypothesis))
	  for (size_t k = 0; k < featureValueDiffs.size(); ++k) {
	  	float alpha = alphas[k];
	  	ScoreComponentCollection update(featureValueDiffs[k]);
	    update.MultiplyEquals(alpha);

	    // sum updates
	    summedUpdate.PlusEquals(update);
	  }
	}
	else {
		return 1;
	}

	// apply learning rate
	if (learning_rate != 1) {
		summedUpdate.MultiplyEquals(learning_rate);
	}

	// scale update by BLEU of oracle (for batch size 1 only)
	if (oracleBleuScores.size() == 1) {
		if (m_scale_update) {
			summedUpdate.MultiplyEquals(oracleBleuScores[0]);
		}
	}
	if(m_onlyOnlineScoreProducerUpdate)
		weightUpdate.PlusEquals(sp,summedUpdate);
	else
		weightUpdate.PlusEquals(summedUpdate);

	return 0;
}

size_t MiraOptimiser::updateSparseWeights(
	SparseVec& UpdateVector,			// sparse vector
	const vector<vector<int> >& FeatureValues,	// index to hypothesis feature values in UpdateVector
    const vector<float>& losses,
    const vector<float>& bleuScores,
    const vector<float>& modelScores,
    const vector<vector<int> >& oracleFeatureValues,	// index to oracle feature values
    const float oracleBleuScores,
    const float oracleModelScores,
    float learning_rate) {

	vector<SparseVec> featureValueDiffs;
	vector<float> lossMinusModelScoreDiffs;
	vector<float> all_losses;
	// Make constraints for new hypothesis translations
	float epsilon = 0.0001;
	int violatedConstraintsBefore = 0;
	SparseVec featureValueDiff(UpdateVector.GetSize());

	// loop iterating over all oracles.
	for(int j=0;j<FeatureValues.size();j++){

		for(int i=0;i<oracleFeatureValues[0].size();i++){
			featureValueDiff.Assign(oracleFeatureValues[0][i],UpdateVector.getElement(oracleFeatureValues[0][i]));
			VERBOSE(3,"Inserting from Oracle at : "<<oracleFeatureValues[0][i]<<" value : "<<UpdateVector.getElement(oracleFeatureValues[0][i])<<endl);
		}

		for(int i=0;i<FeatureValues[j].size();i++){
			featureValueDiff.MinusEqualsFeat(FeatureValues[j][i],UpdateVector.getElement(FeatureValues[j][i]));
			VERBOSE(3, "Subtracting from features at : "<<oracleFeatureValues[0][i]<<" value : "<<UpdateVector.getElement(oracleFeatureValues[0][i])<<endl);
		}

		if (featureValueDiff.GetL1Norm() == 0) { // over sparse features values only
			continue;
		}

		float loss=losses[j];
		VERBOSE(3,"Loss : "<<loss<<endl);
		bool violated = false;
		float modelScoreDiff = oracleModelScores - modelScores[j];
		float diff = 0;
		if (loss > modelScoreDiff)
			diff = loss - modelScoreDiff;
		VERBOSE(3,"Diff : "<<diff<<endl);
		if (diff > epsilon){
			violated = true;
			VERBOSE(3,"Constraint violated!!!\n");
		}

		if (m_normaliseMargin) {
			modelScoreDiff = (2*m_sigmoidParam/(1 + exp(-modelScoreDiff))) - m_sigmoidParam;
			loss = (2*m_sigmoidParam/(1 + exp(-loss))) - m_sigmoidParam;
			diff = 0;
			if (loss > modelScoreDiff) {
				diff = loss - modelScoreDiff;
			}
		}
		if (m_scale_margin) {
			diff *= oracleBleuScores;
		}
		featureValueDiffs.push_back(featureValueDiff);
		lossMinusModelScoreDiffs.push_back(diff);
		all_losses.push_back(loss);
		if (violated) {
			++violatedConstraintsBefore;
		}
	}
	vector<float> alphas;
	SparseVec summedUpdate(UpdateVector.GetSize());
	if (violatedConstraintsBefore > 0) {
		if (m_slack != 0) {
			alphas = Hildreth::optimise(featureValueDiffs, lossMinusModelScoreDiffs, m_slack);
			cerr<<"Alphas : ";for(int i=0;i<alphas.size();i++) cerr<<alphas[i]<<" ";cerr<<endl;
		} else {
			alphas = Hildreth::optimise(featureValueDiffs, lossMinusModelScoreDiffs);
			cerr<<"Alphas : ";for(int i=0;i<alphas.size();i++) cerr<<alphas[i]<<" ";cerr<<endl;
		}
		for (size_t k = 0; k < featureValueDiffs.size(); ++k) {
			float alpha = alphas[k];
			SparseVec update(featureValueDiffs[k]);
			update.MultiplyEquals(alpha);
			// sum updates
			summedUpdate.PlusEquals(update);
		}
	}
	else {
		return 1;
	}
	if (learning_rate != 1) {
		summedUpdate.MultiplyEquals(learning_rate);
	}

	if (m_scale_update) {
		summedUpdate.MultiplyEquals(oracleBleuScores);
	}
	UpdateVector.PlusEquals(summedUpdate);

	return 0;
}



}

