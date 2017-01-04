== Overview ==

This is a Python code for Twitter sentiment analysis.

The code requires the following external Python packages:

  * Natural Language Toolkit              (www.nltk.org)
  * Modular toolkit for Data Processing   (mdp-toolkit.sourceforge.net)
  * NumPy                                 (numpy.scipy.org)

It is run by invoking:

  python sentiment.py

It looks for a corpus called "sentiment.csv" in the working directory.  A free
corpus with the correct format can be downloaded here:

  http://www.sananalytics.com/lab/twitter-sentiment/


== Contact ==

The author, Niek Sanders, can be reached at njs@sananalytics.com

Feedback is greatly appreciated.


== Implementation ==

This is a trivial implementation of a sentiment classifier.

  1) Read in pre-classified tweets from a sentiment corpus.
     ( eg: http://www.sananalytics.com/lab/twitter-sentiment/ )

  2) Divide the tweets in to training and test sets.  All tweets are fed
     in to a simple, token-based feature extractor to produce feature vectors.

  3) (optional) Use a principle components transform to do dimensionality 
                reduction on the training set.  In principle, this should help
                Naive Bayes classifiers deal better with dependent features.  In
                practice, it does not seem to help for this particular feature
                extractor.

  4) Use the training set to teach either a Naive Bayes classifier or a Maximum
     Entropy classifier.

  5) Use the test set to score the classifier.  We dump both the overall
     accuracy score and the confusion matrix.


== Observations and Notes ==

* This classifier throws "neutral" and "irrelevant" tweets in the same category.

* No real effort was made on feature selection.  It is quite possible that
  purging certain terms from the feature extractor will improve classification
  accuracy.

* There are commented lines in sentiment.py which can be used to enable
  Principle Components dimensionality reduction or to switch classifiers.

* Classification results could probably be improved by using Parts of Speech
  (PoS) tagging together with actual semantic analysis.  The challenge is that
  tweets have both atrocious grammar and god-awful spelling.  A simpler PoS
  tagger may actually work better than a fancy one for tweets.

* Feature extractors should be internet acronym and emoticon aware.  For example
  <3 as an emoticon for "heart" or "love".  

* It may be advantageous to consider multiple, sequential tweets from a given
  user when doing classification.  Sarcasm, in particular, can sometimes be hard
  to identify given only a single tweet.


== Revisions ==

2/12/2012 - Minor bugfix in feature extractor.  hasLol was missing a comma.
            Thanks to Rahul Devan for pointing this out.

11/4/2011 - First public release.


== License ==

This classification code is provided free of charge and without restrictions.
It may be used for commercial products.  It may be redistributed.  It may be
modified.  IT COMES WITH NO WARRANTY OF ANY SORT.

If you use this code, attribution and acknowledgements are appreciated but not
required.  Buying the author a beer is, likewise, appreciated but not required.


