#ifndef RANDOM_H
#define RANDOM_H
#include "automaton.h"

/**
 * RAT2
 */
Automaton generate_afa(std::string alphabet, unsigned long int num_states);

/**
 * RAT1
 */
Automaton generate_afa_fisman();
#endif
