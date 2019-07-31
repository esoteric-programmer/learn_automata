#ifndef EQUIVALENCE_ORACLE_H
#define EQUIVALENCE_ORACLE_H
#include "oracle.h"

class EquivalenceOracle : public Oracle {
private:
	Concept* c;
	std::vector<Word> counter_examples;
	virtual Automaton* get_automaton();
	unsigned long int eq;
	unsigned long int mq;
public:
	EquivalenceOracle(Concept* c, std::vector<std::string> counter_examples);
	virtual Alphabet get_alphabet();
	virtual bool decide(Word word);
	// algorithm parameter: DFS, BFS, ...
	virtual Word difference(Concept* other, int algorithm);
	virtual Word difference(Concept* other, int algorithm, bool count_query);
	virtual void reset_queries();
	virtual unsigned long int get_eq_queries();
	virtual unsigned long int get_mq_queries();
	virtual ~EquivalenceOracle();
};
#endif
