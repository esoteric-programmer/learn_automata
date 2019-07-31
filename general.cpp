#include <algorithm>
#include "general.h"

Formula simplify_dnf(Formula dnf) {
	if (dnf.is_cnf) {
		return dnf;
	}
	TermList::iterator dnf_it = dnf.terms.begin();
	while (dnf_it != dnf.terms.end()) {
		bool erase = false;
		TermList::iterator dnf_it2;
		for (dnf_it2 = dnf.terms.begin(); dnf_it2 != dnf.terms.end(); ++dnf_it2) {
			if (dnf_it != dnf_it2) {
				if (std::includes(dnf_it->begin(), dnf_it->end(), dnf_it2->begin(), dnf_it2->end())) {
					erase = true;
					break;
				}
			}
		}
		if (erase) {
			dnf.terms.erase(dnf_it++);
		}else{
			++dnf_it;
		}
	}
	return dnf;
}

Formula to_dnf(Formula formula) {
	if (!formula.is_cnf) {
		return formula;
	}
	// TODO: implement
	throw std::runtime_error("CNF to DNF convertion not implemented yet");
}

namespace std {
	size_t hash<std::pair<State, Symbol>>::operator()(const std::pair<State, Symbol> &s) const {
		size_t h1 = std::hash<std::size_t>()(s.first);
		size_t h2 = std::hash<std::size_t>()(s.second);
		return h1 ^ ( h2 << 1 );
	}
}
