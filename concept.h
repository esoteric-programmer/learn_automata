#ifndef CONCEPT_H
#define CONCEPT_H
#include "general.h"
#include "alphabet.h"

#define DIFFERENCE_ALGORITHM_AFA_DFS                 0
#define DIFFERENCE_ALGORITHM_AFA_DFS_INCREMENTAL     1
#define DIFFERENCE_ALGORITHM_AFA_BACKWARD_EVALUATION 2
#define DIFFERENCE_ALGORITHM_AFA_APPROX              3

class Automaton;
class EquivalenceOracle;

/**
 * represents target concept (virtual class)
 * use constructor of EquivalenceOracle class or constructor of Automaton class to create a Concept object
 */
class Concept {
friend class Automaton;
friend class Oracle;
private:
	virtual Automaton* get_automaton() = 0;
public:
	virtual Alphabet get_alphabet() = 0;
	/**
	 * membership oracle
	 */
	virtual bool decide(Word word) = 0;
	/**
	 * exact equivalence oracle
	 * algorithm parameter: DFS, BFS, ...
	 */
	virtual Word difference(Concept* other, int algorithm) = 0;
	virtual ~Concept();
};
#endif
