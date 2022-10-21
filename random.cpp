#include <stdlib.h>
#include "random.h"
#include "general.h"

Automaton generate_afa(std::string alphabet, unsigned long int num_states) {
	unsigned long num_universal_states = num_states / 2;
	Alphabet alph;
	std::string::const_iterator alphabet_it = alphabet.begin();
	while (alphabet_it != alphabet.end()) {
		alph.push_back(next_unicode(alphabet_it, alphabet.end()));
	}
	std::vector<bool> states(num_states);
	for (unsigned long int i=0; i<num_states; i++) {
		states[i] = ((rand()%2)==1);
	}
	Formula q0;
	q0.is_cnf = false;
	for (int i=0; i<2; i++) {
		Term t;
		t.insert(rand()%num_universal_states);
		q0.terms.push_back(t);
	}
	Transitions tr;
	for (unsigned long st = 0; st < num_states; st++) {
		for (unsigned long int s=0; s<alphabet.size(); s++) {
			Formula f;
			f.is_cnf = false;
			int non_det = 2;
			for (int i=0; i<non_det; i++) {
				Term t;
				int univ = 1 + (rand()%3>=2?1:0);
				for (int i=0; i<univ; i++) {
					t.insert(rand()%num_states);
				}
				f.terms.push_back(t);
			}
			tr.insert(std::make_pair(std::make_pair(st, s), f));
		}
	}
	return Automaton(
			alph,
			states,
			q0,
			tr);
}


Formula generate_dnf_fisman() {
	Formula f;
	f.is_cnf = false;
	int clauses = 1 + (rand()%3);
	bool clause = (rand()%2==1);
	for (int i=0; i<(clause?clauses:1); i++) {
		Term t;
		for (int j=0; j<(clause?1:clauses); j++) {
			t.insert(rand()%7);
		}
		f.terms.push_back(t);
	}
	return f;
}

Automaton generate_afa_fisman() {
	Alphabet alph;
	std::string alphabet = "abc";
	unsigned long int num_states = 7;
	unsigned long int num_accepting = 1 + (rand()%(7/2));
	std::string::const_iterator alphabet_it = alphabet.begin();
	while (alphabet_it != alphabet.end()) {
		alph.push_back(next_unicode(alphabet_it, alphabet.end()));
	}
	std::vector<bool> states(num_states);
	for (unsigned long int i=0; i<num_states; i++) {
		states[i] = false;
	}
	for (unsigned long int i=0; i<num_accepting; i++) {
		while (true) {
			unsigned long int state = rand() % 7;
			if (!states[state]) {
				states[state] = true;
				break;
			}
		}
	}
	Formula q0 = generate_dnf_fisman();
	Transitions tr;
	for (unsigned long st = 0; st < num_states; st++) {
		for (unsigned long int s=0; s<alphabet.size(); s++) {
			Formula f = generate_dnf_fisman();
			tr.insert(std::make_pair(std::make_pair(st, s), f));
		}
	}

	return Automaton(
			alph,
			states,
			q0,
			tr);
}
