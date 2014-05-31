Extra Implementations in "this->master" branch. 

For computer assisted translation, we have implemented a online learning feature that learns from the corrections by the translator.
This feature stores the phrase pairs which occurs in Oracle translation (the closest translation to a post edited sentence).
These phrase pairs get rewarded if they are seen to be translated again in the future.
The feature also stores the phrase pairs in non-oracles and penalize them if they are used in the future translations.
This system has additional parameters, a weight for the online feature, and a learning rate for online algorithm. 
These parameters can be passed as

	1. "weight-ol <weight>" : is the initial weight of the online feature function
	2. "f_learningrate <rate>" : is the learning rate for online algorithm to update the feature 
	3. "w_learningrate <rate>" : is the learning rate for online algorithm to update the weight of online feature

There are different online learning algorithms implemented to update the features and feature weights. 

	1. update only additional feature : Perceptron 
	2. update features and weights : MIRA("-w_algorithm mira")
	3. update only feature weights with MIRA : "-w_algorithm onlyMira"

Input can be of two types.

	1. A source sentence one wants to translate. 
		source_segment
	2. If you want moses to learn from the post edited sentence, you have the option of passing the source and post edited sentence like this
		source_segment_#_postedited_segment

The decoder detects the delimiter "_#_" and automatically splits it based on the delimiter, and updates the models and weights.


Multi-Task Learning

In computer assisted translation, a single document is post-edited by a group of translators while a single SMT system is being run in backend. This version of moses allows to capture the bias between the translators and learn from the corrections of all the translators collectively. Note that Multi-task learning adds a bias feature in the model. 

Parameters:
	1. "weight-mtl <weight>" : weight for the bias feature
	2. "mtl-on" : a boolean which switches multi-tasking on.
	3. "mtl-matrix k <k*k matrix in a row>" : k is the total number of translators working on the document, k*k is the interaction matrix.

Usage: 
	1. A source sentence one wants to translate.
		<source_segment>_#_<userid>
	2. Send the feedback to the SMT system
		<source_segment>_#_<post-edite_segment>_#_<userid>


Interlingual Single Trigger Model 

Cross-lingual trigger model [1] : 
	
	Some source words triggers particular target words. We model this by calculating the PMI 
	of target and source pairs.

Examples for English -> Italian :

	1. market ... negozio (shop)
	2. food ... drogheria (grocery)
	3. travel ... citta (city)

Usage : 

	1. "weight-stm <weight>" : weight for the single trigger model
	2. "stm-file <filename>" : path to the stm

[1] Using inter-lingual triggers for machine translation
