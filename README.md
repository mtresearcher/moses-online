Extra Implementations in "moses_onlinelearning" branch. 

For computer assisted translation, we have implemented a online learning feature that learns from the corrections by the translator.
This feature stores the phrase pairs which occurs in Oracle translation (the closest translation to a post edited sentence).
These phrase pairs get rewarded if they are seen to be translated again in the future.
The feature also stores the phrase pairs in non-oracles and penalize them if they are used in the future translations.
This system has additional parameters, a weight for the online feature, and a learning rate for online algorithm. 
These parameters can be passed as

	1. "weight-ol <weight>" : is the initial weight of the online feature function (default 0)
	2. "f_learningrate <rate>" : is the learning rate for online algorithm to update the feature (default 0)
	3. "w_learningrate <rate>" : is the learning rate for online algorithm to update the weight of online feature (default 0)

There are different online learning algorithms implemented to update the features and feature weights. 

	1. update only additional feature with Perceptron : "-weight-ol <online learner weight>" will activate this algorithm
	2. update features using Perceptron and feature weights using MIRA : "-w_algorithm alsoMira" will active this algorithm
	3. just feature weights update with MIRA : "-w_algorithm onlyMira" will activate this algorithm

Input can be of two types.

	1. A source sentence one wants to translate. 
		source_segment
	2. If you want moses to learn from the post edited sentence, you have the option of passing the source and post edited sentence like this
		source_segment_#_postedited_segment

The decoder detects the delimiter "_#_" and automatically splits it based on the delimiter, and updates the models and weights.

