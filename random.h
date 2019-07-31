#ifndef RANDOM_H
#define RANDOM_H
#include "automaton.h"

Automaton generate_afa(std::string alphabet, unsigned long int num_states);
Automaton generate_afa_fisman();
#endif
