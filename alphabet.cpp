#include "alphabet.h"
#include "unicode.h"

std::string Alphabet::string(Word word) {
	std::ostringstream ret;
	Word::const_iterator w;
	for (w = word.begin();w != word.end();++w) {
		ret << utf8((*this)[*w]);
	}
	return ret.str();
}
