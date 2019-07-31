#include <iostream>
#include <algorithm>
#include "unicode.h"
#include "table.h"

Table::Table(
		Concept* oracle,
		AutomatonType destination_type) {
	this->oracle = oracle;
	this->alphabet = oracle->get_alphabet();
	this->automaton_type = destination_type;
	this->v.push_back(Word(0));
	this->add_row(Word(0));
}

Formula Table::decompose(std::vector<bool> row, std::unordered_set<size_t> primes) {
	switch(this->automaton_type) {
		case DFA: return this->decompose_dfa(row, primes);
		case NFA: return this->decompose_nfa(row, primes);
		case UFA: return this->decompose_ufa(row, primes);
		case AFA: return this->decompose_afa(row, primes);
		default: throw std::runtime_error("invalid automaton type");
	}
}

Formula Table::decompose_dfa(std::vector<bool> row, std::unordered_set<size_t> primes) {
	std::unordered_set<size_t>::const_iterator it;
	for (it=primes.begin();it!=primes.end();++it) {
		if (this->table[*it] == row) {
			Formula res;
			Term t;
			t.insert(*it);
			res.is_cnf = false;
			res.terms.push_back(t);
			return res;
		}
	}
	throw std::range_error("cannot decompose with given primes");
}

Formula Table::decompose_nfa(std::vector<bool> row, std::unordered_set<size_t> primes) {
	std::unordered_set<size_t>::const_iterator it;
	Formula res;
	res.is_cnf = false;
	std::vector<bool> covered(row.size());
	size_t i;
	for (i=0;i<covered.size();i++) {
		covered[i] = false;
	}
	for (it=primes.begin();it!=primes.end();++it) {
		bool subset = true;
		if (this->table[*it].size() != row.size()) {
			throw std::out_of_range("length(v) does not match table");
		}
		for (i=0;i<row.size();i++) {
			if (!row[i] && (this->table[*it])[i]) {
				subset = false;
				break;
			}
		}
		if (subset) {
			Term t;
			t.insert(*it);
			res.terms.push_back(t);
			for (i=0;i<row.size();i++) {
				covered[i] = (covered[i] || (this->table[*it])[i]);
			}
		}
	}
	if (covered != row) {
		throw std::range_error("cannot decompose with given primes");
	}
	return res;
}

Formula Table::decompose_ufa(std::vector<bool> row, std::unordered_set<size_t> primes) {
	std::unordered_set<size_t>::const_iterator it;
	Formula res;
	res.is_cnf = false;
	Term t;
	std::vector<bool> covered(row.size());
	size_t i;
	for (i=0;i<covered.size();i++) {
		covered[i] = true;
	}
	for (it=primes.begin();it!=primes.end();++it) {
		bool supset = true;
		if (this->table[*it].size() != row.size()) {
			throw std::out_of_range("length(v) does not match table");
		}
		for (i=0;i<row.size();i++) {
			if (row[i] && !(this->table[*it])[i]) {
				supset = false;
				break;
			}
		}
		if (supset) {
			t.insert(*it);
			for (i=0;i<row.size();i++) {
				covered[i] = (covered[i] && (this->table[*it])[i]);
			}
		}
	}
	if (covered != row) {
		throw std::range_error("cannot decompose with given primes");
	}
	res.terms.push_back(t);
	return res;
}

Formula Table::decompose_afa(std::vector<bool> row, std::unordered_set<size_t> primes) {
	Formula res;
	res.is_cnf = false;
	if (row.size() != this->v.size()) {
		throw std::out_of_range("length of row vector does not match table");
	}
	size_t i;
	for (i=0;i<row.size();i++) {
		if (!row[i]) {
			continue;
		}
		// create monomial
		Term t;
		// track satisfying assignments of monomial
		size_t j;
		std::vector<bool> sat(row.size());
		for (j=0;j<row.size();j++) {
			sat[j] = true;
		}
		std::unordered_set<size_t>::const_iterator primes_it;
		for (primes_it = primes.begin(); primes_it != primes.end(); ++primes_it) {
			if ((this->table[*primes_it])[i]) {
				// add primes[j] to monomial
				t.insert(*primes_it);
				// update satisfying assignment
				for (j=0;j<row.size();j++) {
					sat[j] = (sat[j] && (this->table[*primes_it])[j]);
				}
			}
		}
		// test satisfying assignments
		for (j=0;j<row.size();j++) {
			if (sat[j] && !row[j]) {
				throw std::range_error("cannot decompose with given primes");
			}
		}
		res.terms.push_back(t);
	}
	// simplify DNF
	return simplify_dnf(res);
}

// algorithm specifies method: everything from Rows_high, Angluin/Bollig/..., or Set Cover method
std::unordered_set<size_t> Table::construct_basis(int algorithm) {
	std::unordered_set<size_t> p;
	switch (algorithm) {
		case BASIS_ALGORITHM_MAX_BASIS: {
			size_t i;
			for (i=0;i<this->u.size();i++) {
				p.insert(i);
			}
			return p;
		}
		case BASIS_ALGORITHM_ANY_MIMIMAL_BASIS: {
			// add row if is cannot be composed
			try {
				while (1) {
					size_t r = this->get_uncomposable_row_id(p);
					p.insert(r);
				}
			}catch(const std::range_error&){
			}
			return this->minimize_basis(p);
		}
		case BASIS_ALGORITHM_SET_COVER: {
			if (this->automaton_type != AFA) {
				return this->construct_basis(BASIS_ALGORITHM_ANY_MIMIMAL_BASIS);
			}
			// build set M of sets mu (X = \bigcup_{mu\in M} mu
			std::unordered_map<size_t, std::unordered_set<std::pair<size_t, size_t>>> m;
			size_t u;
			for (u=0;u<this->u.size();u++){
				std::unordered_set<std::pair<size_t, size_t>> mu;
				size_t v;
				for (v=0;v<this->v.size();v++){
					size_t i;
					for (i=0;i<this->v.size();i++){
						if ((this->table[u])[v] != 0 && (this->table[u])[i] == 0) {
							mu.insert(std::make_pair(v,i));
						}
					}
				}
				m.insert(std::make_pair(u,mu));
			}
			// while any set mu in M is not empty (i.e. X is not covered completely)
			// find largest set mu in M, add it to basis, remove elements of mu from other sets in M
			while (m.size()) {
				std::pair<std::unordered_map<size_t, std::unordered_set<std::pair<size_t, size_t>>>::iterator, size_t> max(m.end(), 0);
				std::unordered_map<size_t, std::unordered_set<std::pair<size_t, size_t>>>::iterator it;
				for (it = m.begin(); it != m.end(); ++it) {
					if (it->second.size() > max.second) {
						max.first = it;
						max.second = it->second.size();
					}
				}
				if (max.first == m.end()) {
					break;
				}
				p.insert(max.first->first);
				for (it = m.begin(); it != m.end(); ++it) {
					if (it != max.first) {
						std::unordered_set<std::pair<size_t, size_t>>::const_iterator tmp;
						for (tmp = max.first->second.begin(); tmp != max.first->second.end(); ++tmp) {
							it->second.erase(*tmp);
						}
					}
				}
				m.erase(max.first->first);
			}
			return this->minimize_basis(p);
		}
		default: {
			throw std::runtime_error("unknown basis construction sheme");
		}
	}
}

std::unordered_set<size_t> Table::minimize_basis(std::unordered_set<size_t> p) {
	// try to remove some unneccessary rows
	bool changed;
	do {
		changed = false;
		std::unordered_set<size_t> q;
		std::unordered_set<size_t>::const_iterator r = p.begin();
		while (!changed && r != p.end()) {
			q = p;
			q.erase(*r);
			try{
				this->get_uncomposable_row_id(q);
			}catch(const std::range_error&){
				p = q;
				changed = true;
			}
			++r;
		}
	}while(changed);
	return p;
}

size_t Table::get_uncomposable_row_id(std::unordered_set<size_t> basis) {
	size_t i;
	for (i=0;i<this->u.size();i++) {
		try {
			this->decompose(this->table[i], basis);
		}catch(const std::range_error&){
			return i;
		}
	}
	throw std::range_error("parameter is a basis");
}

Word Table::uncomposable_word() {
	size_t i;
	std::unordered_set<size_t> p = this->construct_basis(BASIS_ALGORITHM_MAX_BASIS);
	for (i=this->u.size();i<this->table.size();i++) {
		try {
			this->decompose(this->table[i], p);
		}catch(const std::range_error&){
			return this->ua[i-this->u.size()];
		}
	}
	throw std::range_error("table is closed");
}

Automaton* Table::to_automaton(int basis_algorithm) {
	// get basis
	std::unordered_set<size_t> _basis = this->construct_basis(basis_algorithm);
	std::vector<size_t> basis = std::vector<size_t>(_basis.begin(), _basis.end());
	// set states
	std::vector<bool> states(basis.size());
	size_t i;
	for (i=0; i<states.size(); i++) {
		states[i] = (this->table[basis[i]])[0];
	}
	// set q0
	Formula _q0 = this->decompose(this->table[0], _basis);
	Formula q0;
	q0.is_cnf = _q0.is_cnf;
	TermList::const_iterator it;
	for (it = _q0.terms.begin(); it != _q0.terms.end(); ++it) {
		Term t;
		Term::const_iterator it2;
		for (it2 = it->begin(); it2 != it->end(); ++it2) {
			// convert *it2 from this->table-index to basis-index
			std::vector<size_t>::const_iterator basis_it = std::find(basis.begin(), basis.end(), *it2);
			if (basis_it == basis.end()) {
				throw std::runtime_error("cannot find row from decomposition in basis");
			}
			size_t state = basis_it - basis.begin();
			t.insert(state);
		}
		q0.terms.push_back(t);
	}
	// transitions
	Transitions t;
	for (i=0; i<states.size(); i++) {
		size_t a;
		for (a=0; a<this->alphabet.size(); a++) {
			// state i, symbol j
			std::pair<State, Symbol> trigger = std::make_pair(i, a);
			// get transition
			Word ua = this->u[basis[i]];
			ua.push_back(a);
			std::vector<Word>::const_iterator ua_it = std::find(this->u.begin(), this->u.end(), ua);
			size_t ua_idx;
			if (ua_it != this->u.end()) {
				ua_idx = ua_it - this->u.begin();
			}else{
				ua_it = std::find(this->ua.begin(), this->ua.end(), ua);
				if (ua_it != this->ua.end()) {
					ua_idx = (ua_it - this->ua.begin()) + this->u.size();
				}else{
					throw std::runtime_error("cannot find row in lower table");
				}
			}
			Formula _q = this->decompose(this->table[ua_idx], _basis);
			// adjust transition
			Formula q;
			q.is_cnf = _q.is_cnf;
			TermList::const_iterator it;
			for (it = _q.terms.begin(); it != _q.terms.end(); ++it) {
				Term t;
				Term::const_iterator it2;
				for (it2 = it->begin(); it2 != it->end(); ++it2) {
					// convert *it2 from this->table-index to basis-index
					std::vector<size_t>::const_iterator basis_it = std::find(basis.begin(), basis.end(), *it2);
					if (basis_it == basis.end()) {
						throw std::runtime_error("cannot find row from decomposition in basis");
					}
					size_t state = basis_it - basis.begin();
					t.insert(state);
				}
				q.terms.push_back(t);
			}
			t.insert(make_pair(trigger, q));
		}
	}
	return new Automaton(this->alphabet, states, q0, t);
}

bool Table::add_row(Word row) {
	if (this->table.size() != this->u.size() + this->ua.size()) {
		throw std::out_of_range("length(u) + length(ua) does not match table");
	}
	bool anything_changed = false;
	// compute all prefixes of row
	size_t i;
	for (i=0;i<=row.size();i++) {
		Word prefix = Word(&row[0], &row[i]);
		// try to add current prefix to this->u
		if (std::find(this->u.begin(), this->u.end(), prefix) == this->u.end()) {
			// we need to add prefix to this->u
			anything_changed = true;
			size_t pos = this->u.size();
			this->u.push_back(prefix);
			// update this->table
			std::vector<bool> row(this->v.size());
			size_t j;
			for (j=0; j<this->v.size(); j++) {
				Word uv = prefix;
				uv.insert(uv.end(), this->v[j].begin(), this->v[j].end());
				row[j] = this->oracle->decide(uv);
			}
			this->table.insert(this->table.begin()+pos, row);
			// remove new vector from ua (if present)
			std::vector<Word>::iterator old = std::find(this->ua.begin(), this->ua.end(), prefix);
			if (old != this->ua.end()) {
				size_t pos = old - this->ua.begin();
				this->ua.erase(old);
				this->table.erase(this->table.begin()+this->u.size()+pos);
			}
			// we need to add prefix.a to this->ua
			size_t a;
			for (a=0; a<this->alphabet.size(); a++) {
				Word ua = prefix;
				ua.push_back(a);
				if (std::find(this->ua.begin(), this->ua.end(), ua) == this->ua.end()) {
					// insert ua at the end of this->ua
					this->ua.push_back(ua);
					std::vector<bool> row(this->v.size());
					size_t j;
					for (j=0; j<this->v.size(); j++) {
						Word uav = ua;
						uav.insert(uav.end(), this->v[j].begin(), this->v[j].end());
						row[j] = this->oracle->decide(uav);
					}
					this->table.push_back(row);
				}
			}
		}
	}
	if (this->table.size() != this->u.size() + this->ua.size()) {
		throw std::out_of_range("length(u) + length(ua) does not match table after insertion");
	}
	return anything_changed;
}

bool Table::add_col(Word col) {
	if (this->table.size() != this->u.size() + this->ua.size()) {
		throw std::out_of_range("length(u) + length(ua) does not match table");
	}
	bool anything_changed = false;
	// compute all suffixes of col
	size_t i;
	for (i=0;i<=col.size();i++) { // empty word already added by constructor
		Word suffix = Word(&col[i], &col[col.size()]);
		// add suffix to v (if not already there)
		if (std::find(this->v.begin(), this->v.end(), suffix) == this->v.end()) {
			// suffix must be added
			anything_changed = true;
			this->v.push_back(suffix);
			// update this->table
			std::vector<std::vector<bool>>::iterator it;
			for (it = this->table.begin(); it != this->table.end(); it++) {
				Word uv;
				size_t idx = it - this->table.begin();
				if (idx < this->u.size()) {
					uv = this->u[idx];
				}else{
					uv = this->ua[idx - this->u.size()];
				}
				uv.insert(uv.end(), suffix.begin(), suffix.end());
				it->push_back(this->oracle->decide(uv));
				if (it->size() != this->v.size()) {
					throw std::out_of_range("length(v) does not match table");
				}
			}
		}
	}
	return anything_changed;
}

std::string Table::to_string() {
	std::ostringstream ret, br;
	size_t max_ua_len = 1;
	std::vector<Word>::const_iterator it;
	for (it = this->ua.begin(); it != this->ua.end(); ++it) {
		if (max_ua_len < it->size()) {
			max_ua_len = it->size();
		}
	}
	size_t i;
	for (i=0; i<max_ua_len; i++) {
		ret << " ";
	}
	for (it = this->v.begin(); it != this->v.end(); ++it) {
		ret << " | ";
		Word::const_iterator w;
		for (w=it->begin();w != it->end();++w) {
			char ch[2] = {(char)('a'+*w), 0};
			ret << ch;
		}
		if (it->size() < 1) {
			ret << "ε";
		}
	}
	ret << std::endl;
	for (i=0; i<max_ua_len; i++) {
		br << "-";
	}
	for (it = this->v.begin(); it != this->v.end(); ++it) {
		br << "-+-";
		Word::const_iterator w;
		for (w = it->begin();w != it->end();++w) {
			br << "-";
		}
		if (it->size() < 1) {
			br << "-";
		}
	}
	br << std::endl;
	std::string brk = br.str();
	ret << brk;
	std::vector<Word>::const_iterator u;
	for (u=this->u.begin();u!=this->u.end();++u) {
		i=u->size();
		if (i<1) {
			i=1;
		}
		for (; i<max_ua_len; i++) {
			ret << " ";
		}
		ret << this->alphabet.string(*u);
		if (u->size() < 1) {
			ret << "ε";
			i = 1;
		}
		for (it = this->v.begin(); it != this->v.end(); ++it) {
			ret << " | ";
			for (i=1; i<it->size(); i++) {
				ret << " ";
			}
			bool val = (this->table[u - this->u.begin()])[it - this->v.begin()];
			ret << (val?"1":"0");
		}
		ret << std::endl;
	}
	ret << brk;
	for (u=this->ua.begin();u!=this->ua.end();++u) {
		for (i=u->size(); i<max_ua_len; i++) {
			ret << " ";
		}
		ret << this->alphabet.string(*u);
		for (it = this->v.begin(); it != this->v.end(); ++it) {
			ret << " | ";
			for (i=1; i<it->size(); i++) {
				ret << " ";
			}
			bool val = (this->table[u - this->ua.begin() + this->u.size()])[it - this->v.begin()];
			ret << (val?"1":"0");
		}
		ret << std::endl;
	}
	return ret.str();
}

Table::~Table() {
}
