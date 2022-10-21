#ifndef EQUIVALENCE_ORACLE_H
#define EQUIVALENCE_ORACLE_H
#include "automaton.h"

/**
 * exact equivalence oracle
 * can also answer membership queries
 * both types of oracle queries are answered by querying the Automaton object provided as Concept class to the constructor
 * different from raw queries to the concept, this class tracks the number of queries and prioritizes a possible list of counter examples
 */
class Oracle : public Concept {
private:
	Concept* c;
	std::vector<Word> counter_examples;
	virtual Automaton* get_automaton();
	unsigned long int eq;
	unsigned long int mq;
public:
	Oracle(Concept* c, std::vector<std::string> counter_examples);
	virtual Alphabet get_alphabet();
	/**
	 * membership query
	 */
	virtual bool decide(Word word);
	/**
	 * exact equivalence query
	 * algorithm parameter: DFS, BFS, ...
	 */
	virtual Word difference(Concept* other, int algorithm);
	/**
	 * exact equivalence query
	 * algorithm parameter: DFS, BFS, ...
	 */
	virtual Word difference(Concept* other, int algorithm, bool count_query);
	/**
	 * reset oracle query counter
	 */
	virtual void reset_queries();
	/**
	 * get number of equivalence queries
	 */
	virtual unsigned long int get_eq_queries();
	/**
	 * get number of membership queries
	 */
	virtual unsigned long int get_mq_queries();
	virtual ~Oracle();
};
#endif
