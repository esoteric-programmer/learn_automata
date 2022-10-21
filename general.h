#ifndef GENERAL_H
#define GENERAL_H
#include <set>
#include <list>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <sstream>

/**
 * global types and helper functions
 */

typedef size_t Symbol;
typedef size_t State;
typedef std::vector<Symbol> Word;
typedef std::set<State> Term; // std::includes; include<algorithm>
typedef std::list<Term> TermList;
typedef struct {
	bool is_cnf; // otherwise: DNF
	TermList terms;
} Formula;
typedef std::unordered_map<std::pair<State, Symbol>, Formula> Transitions;

namespace std {
template<>
class hash<std::pair<State, Symbol>> {
public:
	size_t operator()(const std::pair<State, Symbol> &s) const;
};
}

std::string utf8(unsigned long int unicode);
unsigned long int next_unicode(std::string::const_iterator& it, std::string::const_iterator end);

Formula simplify_dnf(Formula dnf);
Formula to_dnf(Formula formula);
#endif
