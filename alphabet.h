#ifndef ALPHABET_H
#define ALPHABET_H
#include "general.h"

class Alphabet : public std::vector<unsigned long int> {
public:
	std::string string(Word word);
};
#endif
