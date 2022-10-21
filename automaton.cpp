#include <iostream>
#include <algorithm>
#include "general.h"
#include "automaton.h"

namespace std {
template<>
class hash<Term> {
public:
	size_t operator()(const Term &s) const {
		size_t h = 0;
		Term::const_iterator iterator;
		for (iterator = s.begin(); iterator != s.end(); ++iterator) {
			h |= (1 << *iterator);
		}
		return h;
	}
};

template<>
class hash<std::pair<std::set<Term>,std::set<Term>>> {
public:
	size_t operator()(const std::pair<std::set<Term>,std::set<Term>> &s) const {
		size_t h1 = 0;
		size_t h2 = 42;
		if (s.first.begin() != s.first.end()) {
			 h1 = std::hash<Term>()(*s.first.begin()) + s.first.size();
		}
		if (s.second.begin() != s.second.end()) {
			 h2 = std::hash<Term>()(*s.second.begin()) + s.second.size();
		}
		return h1 ^ ( h2 << 1 );
	}
};

template<>
class hash<std::set<Term>> {
public:
	size_t operator()(const std::set<Term> &s) const {
		size_t h = 0;
		if (s.begin() != s.end()) {
			 h = std::hash<Term>()(*s.begin()) + s.size();
		}
		return h;
	}
};
}

bool Automaton::evaluate_monomial(Term& m, std::vector<bool>& states) {
	Term::const_iterator iterator;
	bool mon = true;
	for (iterator = m.begin(); iterator != m.end(); ++iterator) {
		mon = (mon && states[*iterator]);
	}
	return mon;
}


bool Automaton::evaluate_clause(Term& m, std::vector<bool>& states) {
	Term::const_iterator iterator;
	bool cla = false;
	for (iterator = m.begin(); iterator != m.end(); ++iterator) {
		cla = (cla || states[*iterator]);
	}
	return cla;
}


bool Automaton::evaluate_formula(Formula& phi, std::vector<bool>& states) {
	TermList::const_iterator iterator;
	bool formula;
	if (phi.is_cnf) {
		formula = true;
		for (iterator = phi.terms.begin(); iterator != phi.terms.end(); ++iterator) {
			Term t = *iterator;
			formula = (formula && evaluate_clause(t, states));
		}
	}else{
		formula = false;
		for (iterator = phi.terms.begin(); iterator != phi.terms.end(); ++iterator) {
			Term t = *iterator;
			formula = (formula || evaluate_monomial(t, states));
		}
	}
	return formula;
}

Automaton::Automaton(
			Alphabet sigma,
			std::vector<bool> states,
			Formula q0,
			Transitions transitions) {
	this->sigma = sigma;
	this->states = states;
	this->q0 = q0;
	this->transitions = transitions;
	bool* complete = new bool[this->states.size() * this->sigma.size()];
	if (!complete) {
		throw std::bad_alloc();
	}
	// complete transitions...
	Transitions::const_iterator it;
	for (it = this->transitions.begin(); it != this->transitions.end(); ++it) {
		State q = it->first.first;
		Symbol a = it->first.second;
		if (q < this->states.size() && a < this->sigma.size()) {
			complete[q*this->sigma.size() + a] = true;
		}
	}
	unsigned long int i;
	for (i=0; i<this->states.size() * this->sigma.size(); i++) {
		if (!complete[i]) {
			State q = i / this->sigma.size();
			Symbol a = i%this->sigma.size();
			Formula _false;
			_false.is_cnf = false;
			this->transitions.insert(std::make_pair(std::make_pair(q, a), _false));
		}
	}
	delete complete;
}

Alphabet Automaton::get_alphabet() {
	return this->sigma;
}

Automaton* Automaton::get_automaton() {
	return this;
}


bool Automaton::decide(Word word) {
	std::vector<bool> tmp_state(this->states);
	while (!word.empty()) {
		Symbol last_symbol = word.back();
		std::vector<bool> tmp_state2(tmp_state.size());
		for(size_t i=0;i<tmp_state2.size(); i++) {
			Transitions::const_iterator formula =
					this->transitions.find(std::make_pair(i, last_symbol));
			if (formula != this->transitions.end()) {
				// evaluate formula on tmp_state
				Formula phi = formula->second;
				tmp_state2[i] = evaluate_formula(phi, tmp_state);
			}else{
				tmp_state2[i] = false;
			}
		}
		tmp_state = tmp_state2;
		word.pop_back();
	}
	return evaluate_formula(this->q0, tmp_state);
}


Word Automaton::difference(Concept* other, int algorithm) {
	if (other->get_alphabet() != this->sigma) {
		throw std::invalid_argument("cannot compare automata on different alphabets");
	}
	switch (algorithm) {
		case DIFFERENCE_ALGORITHM_AFA_DFS:
			return this->diff_dfs(other->get_automaton());
		case DIFFERENCE_ALGORITHM_AFA_DFS_INCREMENTAL:
			return this->diff_dfs_incremental(other->get_automaton());
		case DIFFERENCE_ALGORITHM_AFA_BACKWARD_EVALUATION:
			return this->diff(other->get_automaton());
		case DIFFERENCE_ALGORITHM_AFA_APPROX:
			return this->randomized_diff(other->get_automaton());
		default:
			throw std::runtime_error("not implemented");
	}
}


Word Automaton::randomized_diff(Automaton* other) {
	for (size_t k=0; k<10000; k++) {
			Word random(rand()%10); // TODO: t = size of target automaton; range: 0..t+2
			for (size_t j=0; j<random.size(); j++) {
				random[j] = rand()%this->sigma.size();
			}
			if (this->decide(random) != other->decide(random)) {
				return random;
			}
	}
	throw std::range_error("no difference found");
}


Word Automaton::diff_dfs(Automaton* other) {
	// bool term_lists_have_same_content(TermList l1, TermList l2)
	std::unordered_set<std::pair<std::set<Term>,std::set<Term>>> eq_set;
	return this->diff_dfs_recursion(other, this->q0, other->q0, Word(0), eq_set, -1);
}


// repeat diff_dfs with growing depth to find shortest string
Word Automaton::diff_dfs_incremental(Automaton* other) {
	// bool term_lists_have_same_content(TermList l1, TermList l2)
	std::unordered_set<std::pair<std::set<Term>,std::set<Term>>> eq_set;
	Word any_diff = this->diff_dfs_recursion(other, this->q0, other->q0, Word(0), eq_set, -1);
	size_t l;
	for (l=0; l<any_diff.size(); l++) {
		eq_set.clear();
		try {
			return this->diff_dfs_recursion(other, this->q0, other->q0, Word(0), eq_set, l);
		}catch(const std::range_error&) {
		}
	}
	return any_diff;
}

Word Automaton::diff_dfs_recursion(
		Automaton* other,
		Formula this_state,
		Formula other_state,
		Word word, std::unordered_set<std::pair<std::set<Term>,
		std::set<Term>>>& eq_set, int depth_left) {
	// look for equivalency of states
	this_state = to_dnf(this_state);
	other_state = to_dnf(other_state);
	std::set<Term> s1(this_state.terms.begin(), this_state.terms.end());
	std::set<Term> s2(other_state.terms.begin(), other_state.terms.end());
	std::pair<std::set<Term>,std::set<Term>> eq = std::make_pair(s1,s2);
	if (eq_set.find(eq) != eq_set.end()) {
		throw std::range_error("no difference");
	}else{
		eq_set.insert(eq);
	}

	// compare state
	if (evaluate_formula(this_state, this->states) != evaluate_formula(other_state, other->states)) {
		return word;
	}

	// check computation depth
	if (depth_left == 0) {
		throw std::range_error("no difference");
	}

	Symbol a;
	for (a=0; a<this->sigma.size(); a++) {
		Formula new_this_state = this->get_next_config(this_state, a);
		Formula new_other_state = other->get_next_config(other_state, a);
		Word new_word = word;
		new_word.push_back(a);
		try {
			return diff_dfs_recursion(other, new_this_state, new_other_state, new_word, eq_set, depth_left>0?depth_left-1:-1);
		}catch(const std::range_error&){
		}
	}
	throw std::range_error("no difference");
}


Word Automaton::get_element_not_in(Automaton* other) {
	if (other->get_alphabet() != this->sigma) {
		throw std::invalid_argument("cannot compare automata on different alphabets");
	}
	// construct (A /\ !B)-automaton
	size_t states_A = this->states.size();
	size_t states_B = other->states.size();
	std::vector<bool> states(states_A + states_B);
	size_t i;
	for (i=0;i<states_A;i++) {
		states[i] = this->states[i];
	}
	for (i=0;i<states_B;i++) {
		states[i+states_A] = !other->states[i];
	}
	Transitions t(this->transitions);
	Transitions::const_iterator it;
	for (it = other->transitions.begin(); it != other->transitions.end(); ++it) {
		std::pair<std::pair<State, Symbol>, Formula> nt;
		nt.first = it->first;
		nt.first.first += states_A;
		nt.second.is_cnf = !it->second.is_cnf;
		TermList::const_iterator it2;
		for (it2 = it->second.terms.begin(); it2 != it->second.terms.end(); ++it2) {
			Term::const_iterator it3;
			Term t2;
			for (it3 = it2->begin(); it3 != it2->end(); ++it3) {
				t2.insert(*it3 + states_A);
			}
			nt.second.terms.push_back(t2);
		}
		t.insert(nt);
	}
	TermList* additional_dnf = 0;
	TermList modified_q0;
	Formula q0;
	if (this->q0.is_cnf != other->q0.is_cnf) {
		// can be merged
		q0.is_cnf = this->q0.is_cnf;
		q0.terms.insert(q0.terms.end(), this->q0.terms.begin(), this->q0.terms.end());
		TermList::const_iterator it2;
		for (it2 = other->q0.terms.begin(); it2 != other->q0.terms.end(); ++it2) {
			Term::const_iterator it3;
			Term t2;
			for (it3 = it2->begin(); it3 != it2->end(); ++it3) {
				t2.insert(*it3 + states_A);
			}
			q0.terms.push_back(t2);
		}
	}else{
		// q0 state must be the CNF
		q0.is_cnf = true;
		// dnf will be given to get_any_accepted_word as additional_dnf parameter
		if (this->q0.is_cnf) {
			q0.terms.insert(q0.terms.end(), this->q0.terms.begin(), this->q0.terms.end());
			TermList::const_iterator it2;
			for (it2 = other->q0.terms.begin(); it2 != other->q0.terms.end(); ++it2) {
				Term::const_iterator it3;
				Term t2;
				for (it3 = it2->begin(); it3 != it2->end(); ++it3) {
					t2.insert(*it3 + states_A);
				}
				modified_q0.push_back(t2);
			}
			additional_dnf = &modified_q0;
		}else{
			additional_dnf = &this->q0.terms;
			TermList::const_iterator it2;
			for (it2 = other->q0.terms.begin(); it2 != other->q0.terms.end(); ++it2) {
				Term::const_iterator it3;
				Term t2;
				for (it3 = it2->begin(); it3 != it2->end(); ++it3) {
					t2.insert(*it3 + states_A);
				}
				q0.terms.push_back(t2);
			}
		}
	}
	/*
	if (!q0.terms.size() && q0.is_cnf) {
		q0.terms = *additional_dnf;
		q0.is_cnf = false;
		additional_dnf = 0;
	}
	*/
	Automaton a(this->sigma, states, q0, t);
	return a.get_any_accepted_word(additional_dnf);
}

unsigned long int Automaton::num_states() {
	return this->states.size();
}

Word Automaton::diff(Automaton* other) {
	bool use_a = false;
	bool use_b = false;
	Word a;
	Word b;
	try {
		a = this->get_element_not_in(other);
		use_a = true;
	}catch(const std::range_error&){
	}
	try {
		b = other->get_element_not_in(this);
		use_b = true;
	}catch(const std::range_error&){
	}
	// Word::const_iterator w = a.begin();
	if (!use_a && !use_b) {
		throw std::range_error("no difference");
	}
	if (!use_a) {
		return b;
	}
	if (!use_b) {
		return a;
	}
	return (a.size()<=b.size()?a:b);
}


Formula Automaton::get_next_config(Formula old_config, Symbol symbol) {
	old_config = to_dnf(old_config);
	Formula _false;
	_false.is_cnf = false;
	Formula result;
	result.is_cnf = false;
	TermList::iterator it;
	for (it = old_config.terms.begin(); it != old_config.terms.end(); ++it) {
		Term::iterator it2;
		std::list<std::pair<TermList*, TermList::const_iterator>> next_terms;
		for (it2 = it->begin(); it2 != it->end(); ++it2) {
			Transitions::const_iterator formula = this->transitions.find(std::make_pair(*it2, symbol));
			if (formula == this->transitions.end()) {
				break; // no transition found
			}
			if (formula->second.is_cnf) {
				TermList::const_iterator clause;
				for (clause = formula->second.terms.begin();clause != formula->second.terms.end(); ++clause) {
					if (clause->begin() == clause->end()) {
						// empty clause
						break;
					}
					Term::const_iterator literal;
					TermList* phi = new TermList();
					for (literal = clause->begin(); literal != clause->end(); ++literal) {
						Term t;
						t.insert(*literal);
						phi->push_back(t);
					}
					next_terms.push_back(std::pair<TermList*, TermList::const_iterator>(phi, phi->begin()));
				}
				if (clause != formula->second.terms.end()) {
					// empty clause
					break;
				}
			}else{
				TermList* phi = 0;
				if (formula != this->transitions.end() && formula->second.terms.size() > 0) {
					// evaluate dnf on tmp_state
					phi = new TermList(formula->second.terms);
					next_terms.push_back(std::pair<TermList*, TermList::const_iterator>(phi, phi->begin()));
				}else{
					break; // empty dnf
				}
			}
		}
		if (it2 == it->end()) {
			// generate DNF from next_terms
			do {
				// extract current term (if any), then search for term in accepted set.
				Term new_term;
				std::list<std::pair<TermList*, TermList::const_iterator>>::iterator it3;
				for (it3 = next_terms.begin(); it3 != next_terms.end(); ++it3) {
					if (it3->second->begin() == it3->second->end()) {
						// empty term!
					}else{
						new_term.insert(it3->second->begin(),it3->second->end());
					}
				}
				// add new_term to result
				result.terms.push_back(new_term);
				// get next term (with carry): increment next_terms.first().second, on overflow: reset to begin and iterate it in next element of next_terms list
				for (it3 = next_terms.begin(); it3 != next_terms.end(); ++it3) {
					++(it3->second);
					if (it3->second != it3->first->end()) {
						break;
					}else{
						it3->second = it3->first->begin();
					}
				}
				if (it3 == next_terms.end()) {
					break;
				}
			} while(true);
		}
		result = simplify_dnf(result);
	}
	return result;
}

State Automaton::get_nonresidual_state(int algorithm) {
	State s;
	for (s=0; s<this->states.size(); s++) {
		try {
			this->get_residual_string_for_state(s, algorithm);
		}catch(const std::range_error&){
			return s;
		}
	}
	throw std::range_error("automaton is residual");
}

Word Automaton::get_residual_string_for_state(State s, int algorithm) {
	 std::unordered_set<std::set<Term>> nms;
	 return this->get_residual_string_for_state_recursion(s, q0, Word(0), nms, algorithm);
}

Word Automaton::get_residual_string_for_state_recursion(
		State s,
		Formula config,
		Word word,
		std::unordered_set<std::set<Term>>& nonmatching_states,
		int algorithm) {
	config = to_dnf(config);
	std::set<Term> c(config.terms.begin(), config.terms.end());
	if (nonmatching_states.find(c) != nonmatching_states.end()) {
		throw std::range_error("no residual word found");
	}
	nonmatching_states.insert(c);

	Automaton a1 = *this;
	a1.q0.is_cnf = false;
	a1.q0.terms.clear();
	Term m;
	m.insert(s);
	a1.q0.terms.push_back(m);
	Automaton a2 = *this;
	a2.q0 = config;
	try {
		a1.difference(&a2, algorithm);
	}catch(const std::range_error&){
		return word;
	}

	Symbol a;
	for (a=0;a<this->sigma.size();a++) {
		Formula new_config = this->get_next_config(config, a);
		Word new_word = word;
		new_word.push_back(a);
		try {
			return this->get_residual_string_for_state_recursion(s, new_config, new_word, nonmatching_states, algorithm);
		}catch(const std::range_error&){
		}
	}
	throw std::range_error("no residual word found");
}


Word Automaton::get_any_accepted_word(TermList* additional_dnf) {
	// test whether number of states is small enough; otherwise we have hash collisions and will run even slower...
	size_t tmp_size_t = ((size_t)-1);
	size_t tmp_states = this->states.size();
	while (tmp_states > 0 && tmp_size_t > 0) {
		tmp_states--;
		tmp_size_t >>= 1;
	}
	if (!tmp_size_t) {
		// error: too many states to use terms as key in unordered map
		throw std::overflow_error("too many states in automaton");
	}

	// enumerate all terms, identify accepted terms
	std::unordered_map<Term, std::pair<Symbol, Term>> accepted;
	std::unordered_map<Term, std::pair<Symbol, Term>> add_to_accepted;
	TermList not_accepted;
	Term m;
	Term empty_m;
	accepted.insert(std::pair<Term, std::pair<Symbol, Term>>(m,std::pair<Symbol,Term>((Symbol)-1,empty_m)));
	do {
		size_t s = 0;
		while (m.find(s) != m.end()) {
			m.erase(s);
			s++;
		}
		if (s >= this->states.size()) {
			break;
		}
		m.insert(s);
		if (evaluate_monomial(m, this->states)) {
			accepted.insert(std::pair<Term, std::pair<Symbol, Term>>(m,std::pair<Symbol,Term>((Symbol)-1,empty_m)));
		}else{
			not_accepted.push_back(m);
		}
	} while(true);
	bool changed;
	// int y=0;
	do {
		changed = false;
		// if any term of q0 is in accepted, reconstruct word and return
		TermList::const_iterator q0;
		if (this->q0.is_cnf) {
			// find satisfied monomial...
			std::list<std::pair<TermList*, TermList::const_iterator>> next_monomials;
			for (q0 = this->q0.terms.begin(); q0 != this->q0.terms.end(); ++q0) {
				if (q0->begin() != q0->end()) {
					TermList* f = new TermList;
					Term::const_iterator l;
					for (l=q0->begin();l!=q0->end();++l) {
						Term m;
						m.insert(*l);
						f->push_back(m);
					}
					next_monomials.push_back(std::make_pair(f, f->begin()));
				}else{
					// empty clause in this->q0, cannot be satisfied, -> automaton does not accept anything
					throw std::range_error("element does not exist");
				}
			}
			if (additional_dnf) {
				if (additional_dnf->begin() ==  additional_dnf->end()) {
					// empty dnf, thus no satisfying assignment exists
					throw std::range_error("element does not exist");
				}
				next_monomials.push_back(std::make_pair(additional_dnf, additional_dnf->begin()));
			}
			std::list<std::pair<TermList*, TermList::const_iterator>>::iterator itm;
			// iterate over all possible monomials...
			do {
				// extract current term (if any), then search for term in accepted set.
				Term new_monomial;
				for (itm = next_monomials.begin(); itm != next_monomials.end(); ++itm) {
					new_monomial.insert(itm->second->begin(),itm->second->end());
				}
				// test whether new_term is satisfied. if so, move "it" from not_accepted to accepted and update "changed"
				std::unordered_map<Term, std::pair<Symbol, Term>>::const_iterator start_term = accepted.find(new_monomial);
				if (start_term != accepted.end()) {
					// found accepted word
					Word ret(0);
					while (start_term->second.first != (Symbol)-1) {
					ret.push_back(start_term->second.first);
						start_term = accepted.find(start_term->second.second);
						if (start_term == accepted.end()) {
							throw std::runtime_error("internal error");
						}
					}
					return ret;
				}
				// get next term (with carry): increment next_terms.first().second, on overflow: reset to begin and iterate it in next element of next_terms list
				for (itm = next_monomials.begin(); itm != next_monomials.end(); ++itm) {
					++(itm->second);
					if (itm->second != itm->first->end()) {
						break;
					}else{
						itm->second = itm->first->begin();
					}
				}
				if (itm == next_monomials.end()) {
					break;
				}
			} while(true);
			// clean up next_monomials
			for (itm = next_monomials.begin(); itm != next_monomials.end(); ++itm) {
				if (itm->first != additional_dnf) {
					delete itm->first;
				}
			}

		}else{
			// q0 is in DNF
			for (q0 = this->q0.terms.begin(); q0 != this->q0.terms.end(); ++q0) {
				std::unordered_map<Term, std::pair<Symbol, Term>>::const_iterator start_term = accepted.find(*q0);
				if (start_term != accepted.end()) {
					// found accepted word
					Word ret(0);
					while (start_term->second.first != (Symbol)-1) {
					ret.push_back(start_term->second.first);
						start_term = accepted.find(start_term->second.second);
						if (start_term == accepted.end()) {
							throw std::runtime_error("internal error");
						}
					}
					return ret;
				}
			}
		}
		// test all terms from not_accepted list to be accepted? -> update changed variable
		TermList::iterator it = not_accepted.begin();
		while (it != not_accepted.end()) {
			bool hasBeenAdded = false;
			for (Symbol cur_symbol = 0; cur_symbol < this->sigma.size(); cur_symbol++) {
				// add some symbol and test whether new terms can be satisfied...
				std::list<std::pair<TermList*, TermList::const_iterator>> next_terms;
				std::list<std::pair<TermList*, TermList::const_iterator>>::iterator it3;
				Term::iterator it2;
				for (it2 = it->begin(); it2 != it->end(); ++it2) {
					Transitions::const_iterator formula = this->transitions.find(std::make_pair(*it2, cur_symbol));
					if (formula == this->transitions.end()) {
						break; // no transition found
					}
					if (formula->second.is_cnf) {
						TermList::const_iterator clause;
						for (clause = formula->second.terms.begin();clause != formula->second.terms.end(); ++clause) {
							if (clause->begin() == clause->end()) {
								// empty clause
								break;
							}
							Term::const_iterator literal;
							TermList* phi = new TermList();
							for (literal = clause->begin(); literal != clause->end(); ++literal) {
								Term t;
								t.insert(*literal);
								phi->push_back(t);
							}
							next_terms.push_back(std::pair<TermList*, TermList::const_iterator>(phi, phi->begin()));
						}
						if (clause != formula->second.terms.end()) {
							// empty clause
							break;
						}
					}else{
						TermList* phi = 0;
						if (formula != this->transitions.end() && formula->second.terms.size() > 0) {
							// evaluate dnf on tmp_state
							phi = new TermList(formula->second.terms);
						}else{
							break; // empty dnf
						}
						next_terms.push_back(std::pair<TermList*, TermList::const_iterator>(phi, phi->begin()));
					}

				}
				if (it2 != it->end()) {
					for (it3 = next_terms.begin(); it3 != next_terms.end(); ++it3) {
						delete it3->first;
					}
					continue;
				}
				// as long as last it of next_terms is not at its end: move first it of next_terms...; search term in accepted-set
				do {
					// extract current term (if any), then search for term in accepted set.
					Term new_term;
					for (it3 = next_terms.begin(); it3 != next_terms.end(); ++it3) {

						if (it3->second->begin() == it3->second->end()) {
							// empty term!
						}else{
							new_term.insert(it3->second->begin(),it3->second->end());
						}
					}
					// test whether new_term is satisfied. if so, move "it" from not_accepted to accepted and update "changed"
					if (accepted.find(new_term) != accepted.end()) {
						add_to_accepted.insert(std::pair<Term, std::pair<Symbol, Term>>(*it,std::pair<Symbol,Term>(cur_symbol,new_term)));
						hasBeenAdded = true;
						changed = true;
						break;
					}
					// get next term (with carry): increment next_terms.first().second, on overflow: reset to begin and iterate it in next element of next_terms list
					for (it3 = next_terms.begin(); it3 != next_terms.end(); ++it3) {
						++(it3->second);
						if (it3->second != it3->first->end()) {
							break;
						}else{
							it3->second = it3->first->begin();
						}
					}
					if (it3 == next_terms.end()) {
						break;
					}
				} while(true);
				// clean up next_terms!
				for (it3 = next_terms.begin(); it3 != next_terms.end(); ++it3) {
					delete it3->first;
				}
				if (hasBeenAdded) {
					break;
				}
			}

			if (hasBeenAdded) {
				not_accepted.erase(it++);
			}else{
				++it;
			}
		}
		if (add_to_accepted.begin() != add_to_accepted.end()) {
			accepted.insert(add_to_accepted.begin(),add_to_accepted.end());
			add_to_accepted.clear();
		}
	}while (changed);
	throw std::range_error("element does not exist");
}


std::string Automaton::graphviz_monomial(
		Term monomial,
		std::unordered_map<Term, size_t>* monomials,
		std::ostringstream* out,
		std::ostringstream* out_trans) {
	std::string destination;
	if (monomial.size() != 1) {
		// find out whether monomial already exists
		std::unordered_map<Term, size_t>::const_iterator mon = monomials->find(monomial);
		if (mon != monomials->end()) {
			destination = "m" + std::to_string(mon->second);
		}else{
			// add monomial
			size_t m = monomials->size();
			*out << "\tm" << m
					<< " [label = \"\", shape = square, style = filled, "
					"color=grey, fixedsize=true, width=.2];" << std::endl;
			Term::const_iterator state;
			for (state = monomial.begin(); state != monomial.end(); ++state) {
				*out_trans << "\tm" << m << " -> q" << *state << ";" << std::endl;
			}
			monomials->insert(std::make_pair(monomial, m));
			destination = "m" + std::to_string(m);
		}
	}else if (monomial.size() == 1) {
		// just add an edge
		destination = "q" + std::to_string(*monomial.begin());
	}
	return destination;
}


// TODO: state labels, symbol labels; support CNF?
std::string Automaton::to_graphviz() {
	std::ostringstream output_states,
			output_start_states,
			output_monomials,
			output_transitions_start,
			output_transitions_inner,
			output_transitions_monomial,
			ret;
	// states
	size_t i;
	for (i=0;i<this->states.size();i++) {
			output_states << "\tq" << i << " [label = \"q" << i << "\"";
			if (this->states[i]) {
				output_states << ", shape = doublecircle, final=\"true\" ";
			}
			output_states << "];" << std::endl;
	}

  
	// start states
	std::unordered_map<Term, size_t> monomials;
	TermList::const_iterator monomial;
	i = 0;
	for (
			monomial = this->q0.terms.begin();
			monomial != this->q0.terms.end();
			++monomial) {
		output_start_states << "\ts" << i
				<< " [shape = point, style = invis, start=\"true\" ];" << std::endl;
		output_transitions_start << "\ts" << i << " -> "
				<< Automaton::graphviz_monomial(
						*monomial, &monomials,
						&output_monomials,
						&output_transitions_monomial)
				<< (this->q0.is_cnf?" [color = red]":"")
				<< std::endl;
		i++;
	}

	// transitions
	std::unordered_map<std::string, std::set<size_t>> edges;
	Transitions::const_iterator transition;
	for (
			transition = this->transitions.begin();
			transition != this->transitions.end();
			++transition) {
		i = 0;
		for (
				monomial = transition->second.terms.begin();
				monomial != transition->second.terms.end();
				++monomial) {
			std::string t(
					"q"
					+ std::to_string(transition->first.first)
					+ " -> "
					+ Automaton::graphviz_monomial(
							*monomial,
							&monomials,
							&output_monomials,
							&output_transitions_monomial)
					+ (transition->second.is_cnf?" [color = red; ":" ["));
			// add symbol
			std::unordered_map<std::string, std::set<size_t>>::iterator tr = edges.find(t);
			if (tr != edges.end()) {
				tr->second.insert(transition->first.second);
			}else{
				std::set<size_t> states;
				states.insert(transition->first.second);
				edges.insert(std::make_pair(t,states));
			}
		}
	}

  // Irgendwo hier drin geht es kaputt
	std::unordered_map<std::string, std::set<size_t>>::const_iterator edge;
	for (edge = edges.begin(); edge != edges.end(); ++edge) {
		output_transitions_inner << "\t" << edge->first << "label = \"";
		std::set<size_t>::const_iterator symbol;
		for (symbol = edge->second.begin(); symbol != edge->second.end(); ++symbol) {
			if (symbol != edge->second.begin()) {
				output_transitions_inner << ", ";
			}
			if (*symbol != (size_t)-1) {
				if (*symbol >= this->sigma.size()) {
          
					throw std::out_of_range("invalid symbol reference");
				}
				if ((this->sigma[*symbol] >= 'a' && this->sigma[*symbol] <= 'z')
						|| (this->sigma[*symbol] >= 'A' && this->sigma[*symbol] <= 'Z')
						|| (this->sigma[*symbol] >= '0' && this->sigma[*symbol] <= '9')) {
					char ch[2] = {(char)(this->sigma[*symbol]), 0};
					output_transitions_inner << ch;
				}else{
					output_transitions_inner << "&#" << this->sigma[*symbol] << ";";
				}
			}else{
				output_transitions_inner << "&epsilon;";
			}
			// output_transitions_inner << *symbol;
		}
		output_transitions_inner << "\"];" << std::endl;
	}
	// header
	ret << "digraph finite_state_machine {" << std::endl << "\trankdir=LR;"
			<< std::endl << "\tnode [shape = circle];" << std::endl;
	// automaton
	ret << output_start_states.str();
	ret << output_states.str();
	ret << output_monomials.str();
	ret << output_transitions_start.str();
	ret << output_transitions_inner.str();
	ret << output_transitions_monomial.str();
	// footer
	ret << "}" << std::endl;
	return ret.str();
}


Automaton::~Automaton() {
}


// below: regexp parsing, thompson construction, and automaton-constructor from regexp

#define TOKEN_SYMBOL         0 // e.g. \27
#define TOKEN_OPEN_BRACKET   1 // (
#define TOKEN_CLOSE_BRACKET  2 // )
#define TOKEN_OR             3 // |
#define TOKEN_QUANTIFIER     4 // e.g.+
#define TOKEN_CHAR_SELECTION 5 // e.g. [^\20\0-\14\23-\25\29]
#define TOKEN_END_OF_EXPRESSION 255

std::string Automaton::regexp_utf8_to_alphabet (std::string regexp) {
	// replace utf-8 characters of regexp by symbol id (for alphabet "acb", we replace "a" by "\\0", "c" by "\\1", and "b" by "\\2".
	std::ostringstream ret;
	std::unordered_set<unsigned long int> regexp_reserved {'(', ')', '[', ']', '-', '.', '|', '^', '+', '?', '*', '\\'};
	std::unordered_map<unsigned long int, size_t> alphabet;
	size_t i;
	for (i=0; i<this->sigma.size(); i++) {
		if (alphabet.find(this->sigma[i]) != alphabet.end()) {
			throw std::runtime_error("invalid alphabet: it contains at least one unicode character more than once");
		}
		alphabet.insert(std::make_pair(this->sigma[i], i));
	}
	std::string::const_iterator it = regexp.begin();
	while (it != regexp.end()) {
		unsigned long int sym;
		sym = next_unicode(it, regexp.end());
		if (regexp_reserved.find(sym) != regexp_reserved.end()) {
			if (sym == '\\') {
				// do some special handling!
				sym = next_unicode(it, regexp.end());
				if (regexp_reserved.find(sym) == regexp_reserved.end()) {
					// invalid escape sequence... this is not good... ignore and reinsert everything
					ret << "\\" << utf8(sym);
					continue;
				}
			}else{
				// reserved character, don't change
				ret << utf8(sym);
				continue;
			}
		}
		std::unordered_map<unsigned long int, size_t>::const_iterator symbol = alphabet.find(sym);
		if (symbol == alphabet.end()) {
			// could not find symbol... this is not good... ignore and reinsert unicode character
			ret << utf8(sym);
		}else{
			ret << "\\" << symbol->second;
		}
	}
	return ret.str();
}

std::string Automaton::lex_symbol(std::string::const_iterator& it, std::string::const_iterator end) {
	std::string s;
	do {
		++it;
		if (it == end) {
			break;
		}
		if (*it < '0' || *it > '9') {
			break;
		}
		s.push_back(*it);
	} while(true);
	if (!s.size()) {
		throw std::invalid_argument("unexpected character in regular expression (following backslash)");
	}
	return s;
}

size_t Automaton::str2sym(std::string s) {
	size_t a = 0;
	size_t i;
	for (i=0; i<s.size(); i++) {
		if (s[i] < '0' || s[i] > '9') {
			throw std::runtime_error("error: expected number after \"\\\" to form a symbol");
		}
		a *= 10;
		a += (s[i] - '0');
	}
	return a;
}

Automaton::LexedExpression Automaton::lex(std::string expr) {
	std::string::const_iterator it = expr.begin();
	Automaton::LexedExpression ret(0);
	while (it != expr.end()) {
		switch (*it) {
			case '\\':
				ret.push_back(std::make_pair(TOKEN_SYMBOL, lex_symbol(it, expr.end())));
				break;
			case '.':
				ret.push_back(std::make_pair(TOKEN_SYMBOL, std::string("")));
				++it;
			case '(':
				ret.push_back(std::make_pair(TOKEN_OPEN_BRACKET, std::string("")));
				++it;
				break;
			case ')':
				ret.push_back(std::make_pair(TOKEN_CLOSE_BRACKET, std::string("")));
				++it;
				break;
			case '|':
				ret.push_back(std::make_pair(TOKEN_OR, std::string("")));
				++it;
				break;
			case '+':
			case '*':
			case '?': {
				std::string s;
				s.push_back(*it);
				ret.push_back(std::make_pair(TOKEN_QUANTIFIER, s));
				++it;
				break;
			}
			case '[': {
				// quantifier
				++it;
				std::string s;
				bool begin = true;
				bool was_slash = false;
				bool was_symbol = false;
				do {
					if (it == expr.end()) {
						throw std::invalid_argument("unexpected end of regular expression (missing ])");
					}
					if (*it == ']') {
						break;
					}
					switch (*it) {
						case '\\': {
							s.push_back(*it);
							std::string s2 = lex_symbol(it, expr.end());
							s.append(s2);
							if (was_slash) {
								was_slash = false;
							}else{
								was_symbol = true;
							}
							break;
						}
						case '^':
							if (!begin) {
								throw std::invalid_argument("unexpected character (^) in regular expression");
							}
							s.push_back(*it);
							++it;
							break;
						case '-':
							if (!was_symbol) {
								throw std::invalid_argument("unexpected character (-) in regular expression");
							}
							was_symbol = false;
							was_slash = true;
							s.push_back(*it);
							++it;
							break;
						default:
							throw std::invalid_argument("unexpected character in regular expression (inside char selection)");
					}
					begin = false;
				} while(true);
				if (!s.size()) {
					throw std::invalid_argument("unexpected character in regular expression (empty char selection)");
				}
				ret.push_back(std::make_pair(TOKEN_CHAR_SELECTION, s));
				++it;
				break;
			}
			default:
				throw std::invalid_argument("unexpected character in regular expression (normal position)");
		}
	}
	return ret;
}

void Automaton::init_from_regexp(Alphabet sigma, std::string reg_exp) {
	this->sigma = sigma;
	this->states = std::vector<bool>(0);
	this->q0.is_cnf = false;
	Automaton::LexedExpression lexed = lex(this->regexp_utf8_to_alphabet(reg_exp));
	Automaton::Subautomaton a = this->or_statement(lexed);
	this->states[a.second] = true;
	Term t;
	t.insert(a.first);
	this->q0.terms.push_back(t);
	Automaton _a(this->sigma, this->states, this->q0, this->transitions);
	this->remove_lambda_transitions();
}

Automaton::Automaton(Alphabet sigma, std::string reg_exp) {
	this->init_from_regexp(sigma, reg_exp);
}

Automaton::Automaton(std::string alphabet, std::string reg_exp) {
	Alphabet alph;
	std::string::const_iterator alphabet_it = alphabet.begin();
	while (alphabet_it != alphabet.end()) {
		alph.push_back(next_unicode(alphabet_it, alphabet.end()));
	}
	this->init_from_regexp(alph, reg_exp);
}

void Automaton::add_trans(std::pair<std::pair<State, Symbol>,Formula> trans) {
	if (trans.second.is_cnf) {
		throw std::runtime_error("error: cannot merge CNFs...");
	}
	Transitions::iterator it = this->transitions.find(trans.first);
	if (it != this->transitions.end()) {
		if (it->second.is_cnf) {
			throw std::runtime_error("error: cannot merge CNFs...");
		}
		it->second.terms.insert(it->second.terms.end(), trans.second.terms.begin(), trans.second.terms.end());
	}else{
		this->transitions.insert(trans);
	}
}


Automaton::Subautomaton Automaton::symbol(std::string s) {
	size_t q = this->states.size();
	this->states.push_back(false);
	this->states.push_back(false);
	Formula f;
	f.is_cnf = false;
	Term t;
	t.insert(q+1);
	f.terms.push_back(t);
	size_t a;
	if (!s.size()) {
		for (a=0; a<this->sigma.size(); a++) {
			this->add_trans(std::make_pair(std::make_pair(q,a),f));
		}
	}else{
		a = str2sym(s);
		this->add_trans(std::make_pair(std::make_pair(q,a),f));
	}
	return std::make_pair(q,q+1);
}

Automaton::Subautomaton Automaton::symbol(std::vector<size_t> s) {
	size_t q = this->states.size();
	this->states.push_back(false);
	this->states.push_back(false);
	Formula f;
	f.is_cnf = false;
	Term t;
	t.insert(q+1);
	f.terms.push_back(t);
	std::vector<size_t>::const_iterator it;
	for (it = s.begin(); it != s.end(); ++it) {
		this->add_trans(std::make_pair(std::make_pair(q,*it),f));
	}
	return std::make_pair(q,q+1);
}


Automaton::Subautomaton Automaton::epsilon() {
	size_t q = this->states.size();
	this->states.push_back(false);
	return std::make_pair(q,q);
}


Automaton::Subautomaton Automaton::_union(Automaton::Subautomaton a1, Automaton::Subautomaton a2) {
	size_t q = this->states.size();
	this->states.push_back(false);
	this->states.push_back(false);
	size_t a = (size_t)-1; // epsilon transition
	{
		Formula f;
		f.is_cnf = false;
		Term t1;
		t1.insert(a1.first);
		f.terms.push_back(t1);
		Term t2;
		t2.insert(a2.first);
		f.terms.push_back(t2);
		this->add_trans(std::make_pair(std::make_pair(q,a),f));
	}
	{
		Formula f;
		f.is_cnf = false;
		Term t;
		t.insert(q+1);
		f.terms.push_back(t);
		this->add_trans(std::make_pair(std::make_pair(a1.second,a),f));
		this->add_trans(std::make_pair(std::make_pair(a2.second,a),f));
	}
	return std::make_pair(q,q+1);
}


Automaton::Subautomaton Automaton::concat(Automaton::Subautomaton a1, Automaton::Subautomaton a2) {
	size_t a = (size_t)-1; // epsilon transition
	Formula f;
	f.is_cnf = false;
	Term t;
	t.insert(a2.first);
	f.terms.push_back(t);
	this->add_trans(std::make_pair(std::make_pair(a1.second,a),f));
	return std::make_pair(a1.first,a2.second);
}


Automaton::Subautomaton Automaton::quantify(Automaton::Subautomaton a1, std::string quantifier) {
	if (quantifier.size() != 1) {
		throw std::runtime_error("error: invalid call to generate quantifier subautomaton: invalid length of quantifier string");
	}
	if (quantifier[0] != '*'
			&& quantifier[0] != '+'
			&& quantifier[0] != '?'){
		throw std::runtime_error("error: invalid call to generate quantifier subautomaton: invalid quantifier");
	}
	size_t a = (size_t)-1; // epsilon transition
	if (quantifier[0] != '+'){
		Formula f;
		f.is_cnf = false;
		Term t1;
		t1.insert(a1.second);
		f.terms.push_back(t1);
		this->add_trans(std::make_pair(std::make_pair(a1.first,a),f));
	}
	if (quantifier[0] != '?') {
		Formula f;
		f.is_cnf = false;
		Term t1;
		t1.insert(a1.first);
		f.terms.push_back(t1);
		this->add_trans(std::make_pair(std::make_pair(a1.second,a),f));
	}
	return a1;
}

void Automaton::read_next(Automaton::LexedExpression& lexed) {
	if (lexed.size()) {
		lexed.erase(lexed.begin());
	}
}

Automaton::Subautomaton Automaton::expression(Automaton::LexedExpression& lexed) {
	if (!lexed.size()) {
		// create f-q-automaton...
		size_t q = this->states.size();
		this->states.push_back(false);
		this->states.push_back(false);
		return std::make_pair(q,q+1);
	}
	if (lexed[0].first == TOKEN_OPEN_BRACKET) {
		this->read_next(lexed);
		Automaton::Subautomaton result = this->or_statement(lexed);
		if (!lexed.size()) {
			throw std::runtime_error("error: missing closing bracket at end of expression");
		}
		if (lexed[0].first != TOKEN_CLOSE_BRACKET) {
			throw std::runtime_error("error: invalid symbol. expected: closing bracket.");
		}
		this->read_next(lexed);
		return result;
	}
	return this->or_statement(lexed);
}

Automaton::Subautomaton Automaton::or_statement(Automaton::LexedExpression& lexed) {
	Automaton::Subautomaton left = this->concat_statement(lexed);
	while (lexed.size() && lexed[0].first == TOKEN_OR) {
		this->read_next(lexed);
		Automaton::Subautomaton right = this->concat_statement(lexed);
		left = this->_union(left,right);
	}
	return left;
}

Automaton::Subautomaton Automaton::concat_statement(Automaton::LexedExpression& lexed) {
	Automaton::Subautomaton left = this->parse_symbol(lexed);
	while(lexed.size() && lexed[0].first != TOKEN_OR && lexed[0].first != TOKEN_CLOSE_BRACKET) {
		Automaton::Subautomaton right = this->parse_symbol(lexed);
		left = this->concat(left,right);
	}
	return left;
}




Automaton::Subautomaton Automaton::parse_symbol(Automaton::LexedExpression& lexed) {
	if (!lexed.size()) {
		return this->epsilon();
	}
	Automaton::Subautomaton result;
	if (lexed[0].first == TOKEN_SYMBOL) {
		result = this->symbol(lexed[0].second);
		this->read_next(lexed);
	}else if (lexed[0].first == TOKEN_CHAR_SELECTION) {
		// throw std::runtime_error("parsing CHAR_SELECTION [] not implemented yet");
		bool inverted = false;
		std::vector<size_t> symbols;
		std::string::const_iterator it = lexed[0].second.begin();
		std::string::const_iterator end = lexed[0].second.end();
		if (it != end) {
			if (*it == '^') {
				inverted = true;
				++it;
			}
		}
		while (it != end) {
			if (*it == '\\') {
				size_t sym = str2sym(lex_symbol(it, end));
				if (*it == '-') {
					++it;
					if (*it != '\\') {
						throw std::runtime_error("error: expected character after minus-symbol in character range");
					}
					size_t sym2 = str2sym(lex_symbol(it, end));
					if (sym2 <= sym) {
						throw std::runtime_error("error: wrong order of symbols in character range");
					}
					for (;sym<=sym2;sym++) {
						symbols.push_back(sym);
					}
				}else{
					symbols.push_back(sym);
				}
			}else{
				throw std::runtime_error("error: unexpected symbol in character range");
			}
		}
		if (inverted) {
			std::unordered_set<size_t> alph;
			size_t i;
			for (i=0;i<this->sigma.size();i++) {
				alph.insert(i);
			}
			std::vector<size_t>::const_iterator s_it;
			for (s_it = symbols.begin(); s_it != symbols.end(); ++s_it) {
				alph.erase(alph.find(*s_it));
			}
			symbols = std::vector<size_t>(alph.begin(), alph.end());
		}
		result = this->symbol(symbols);
		this->read_next(lexed);
	}else if (lexed[0].first == TOKEN_OPEN_BRACKET) {
		result = this->expression(lexed);
	}else if (lexed[0].first == TOKEN_OR || lexed[0].first == TOKEN_CLOSE_BRACKET) {
		return this->epsilon();
	}else{
		throw std::runtime_error("error: unexpected character in regular expression");
	}
	return this->quantifier(result, lexed);
}

Automaton::Subautomaton Automaton::quantifier(Automaton::Subautomaton expression, Automaton::LexedExpression& lexed) {
	if (!lexed.size()) {
		return expression;
	}
	if (lexed[0].first == TOKEN_QUANTIFIER) {
		expression = this->quantify(expression, lexed[0].second);
		this->read_next(lexed);
		return expression;
	}else{
		// no quantifier
		return expression;
	}
}

// helper function for Automaton::remove_lambda_transitions()
bool Automaton::add_lambda_successors_to_formula(Formula& f) {
	if (f.is_cnf) {
		throw std::runtime_error("error: cannot remove lambda transitions, because at least one transition is not in DNF");
	}
	bool changes;
	bool ret = false;
	do {
		changes = false;

		TermList::const_iterator monom;
		TermList ins;
		for (monom = f.terms.begin(); monom != f.terms.end(); ++monom) {
			if (monom->size() > 1) {
				throw std::runtime_error("error: cannot remove lambda transitions, because at least one transition is not a simple clause");
			}
			if (monom->size()) {
				Transitions::iterator lambda = this->transitions.find(std::make_pair(*(monom->begin()), (size_t)-1));
				if (lambda != this->transitions.end()) {
					if (lambda->second.is_cnf) {
						throw std::runtime_error("error: cannot remove lambda transitions, because at least one transition is not in DNF");
					}
					TermList::iterator mon2;
					for (mon2 = lambda->second.terms.begin(); mon2 != lambda->second.terms.end(); ++mon2) {
						if (std::find(f.terms.begin(), f.terms.end(), *mon2) == f.terms.end() && std::find(ins.begin(), ins.end(), *mon2) == ins.end()) {
							ins.push_back(*mon2);
						}
					}
				}
			}
		}
		if (ins.size()) {
			changes = true;
			ret = true;
			f.terms.insert(f.terms.end(), ins.begin(), ins.end());
		}
	}while(changes);
	return ret;
}

// helper function for Automaton::remove_lambda_transitions()
void Automaton::remap_formula(Formula& f, std::vector<size_t> mapping) {
	if (mapping.size() != this->states.size()) {
		throw std::runtime_error("invalid mapping");
	}
	TermList::iterator monom;
	for (monom = f.terms.begin(); monom != f.terms.end(); ++monom) {
		Term::iterator state;
		Term new_term;
		for (state = monom->begin(); state != monom->end(); ++state) {
			new_term.insert(mapping[*state]);
		}
		*monom = new_term;
	}
}


void Automaton::remove_lambda_transitions() {

	// for every lambda transition from B to C and every transition T(A,s) to B,
	// add some transition T(A,s) to C.
	bool changes;
	do {
		changes = false;
		if (this->add_lambda_successors_to_formula(this->q0)) {
			changes = true;
		}
		Transitions::iterator it;
		for (it = this->transitions.begin(); it != this->transitions.end(); ++it) {
			if (this->add_lambda_successors_to_formula(it->second)) {
				changes = true;
			}
		}
	}while(changes);

	// remove lambda transitions and, for each state s, count number of remaining transitions starting in s
	size_t i;
	std::vector<size_t> outgoing(this->states.size());
	for (i=0;i<this->states.size(); i++) {
		outgoing[i] = 0;
		Transitions::const_iterator t = this->transitions.find(std::make_pair(i,(size_t)-1));
		if (t != this->transitions.end()) {
			this->transitions.erase(t);
		}
		size_t j;
		for (j=0; j<this->sigma.size(); j++) {
			t = this->transitions.find(std::make_pair(i,j));
			if (t != this->transitions.end()) {
				outgoing[i]++;
			}
		}
	}

	// remove transitions to non-accepting states without successor
	do {
		changes = false;
		Transitions::iterator it = this->transitions.begin();
		while (it != this->transitions.end()) {
			TermList::iterator term = it->second.terms.begin();
			while (term != it->second.terms.end()) {
				if (term->size() != 1) {
					throw std::runtime_error("unexpected transition");
				}
				size_t dest = *term->begin();
				if (!this->states[dest] && !outgoing[dest]) {
					// remove term
					it->second.terms.erase(term++);
				}else{
					++term;
				}
			}
			if (!it->second.terms.size()) {
				if (!outgoing[it->first.first]) {
					throw std::runtime_error("invalid counter value");
				}
				outgoing[it->first.first]--;
				this->transitions.erase(it++);
				changes = true;
			}else{
				++it;
			}
		}
	}while(changes);

	// remove non-accepting states wothout successor from q0
	TermList::iterator term = this->q0.terms.begin();
	while (term != this->q0.terms.end()) {
		if (term->size() != 1) {
			throw std::runtime_error("unexpected transition");
		}
		size_t dest = *term->begin();
		if (!this->states[dest] && !outgoing[dest]) {
			// remove term
			this->q0.terms.erase(term++);
		}else{
			++term;
		}
	}

	// remove isolated states
	size_t new_num_states = 0;
	std::vector<size_t> state_mapping(this->states.size());
	for (i=0; i<this->states.size(); i++) {
		if (outgoing[i] || this->states[i]) {
			this->states[new_num_states] = this->states[i];
			state_mapping[i] = new_num_states++;
		}else{
			state_mapping[i] = (size_t)-1;
		}
	}
	this->remap_formula(this->q0, state_mapping);
	Transitions new_trans;
	Transitions::iterator it;
	for (it = this->transitions.begin(); it != this->transitions.end(); ++it) {
		this->remap_formula(it->second, state_mapping);
		new_trans.insert(std::make_pair(std::make_pair(state_mapping[it->first.first], it->first.second),it->second));
	}
	this->transitions = new_trans;
	this->states.resize(new_num_states);
}
