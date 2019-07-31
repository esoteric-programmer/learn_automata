#ifndef ORACLE_H
#define ORACLE_H
#include "automaton.h"

class Oracle : public Concept {
private:
public:
	virtual Word difference(Concept* other, int algorithm) = 0;
	virtual Word difference(Concept* other, int algorithm, bool count_query) = 0;
	virtual void reset_queries() = 0;
	virtual unsigned long int get_eq_queries() = 0;
	virtual unsigned long int get_mq_queries() = 0;
	virtual ~Oracle();
};
#endif
