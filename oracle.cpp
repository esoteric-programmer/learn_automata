#include <iostream>
#include "oracle.h"
#include "general.h"

Automaton* Oracle::get_automaton() {
	return this->c->get_automaton();
}

Oracle::Oracle(Concept* c, std::vector<std::string> counter_examples) {
	if (!c) {
		throw std::invalid_argument("cannot create equivalence oracle from concept nullptr");
	}
	this->c = c;
	this->eq = 0;
	this->mq = 0;
	std::unordered_map<unsigned long int, size_t> alphabet;
	size_t i;
	Alphabet sigma = this->c->get_alphabet();
	for (i=0; i<sigma.size(); i++) {
		if (alphabet.find(sigma[i]) != alphabet.end()) {
			throw std::runtime_error("invalid alphabet: it contains at least one unicode character more than once");
		}
		alphabet.insert(std::make_pair(sigma[i], i));
	}
	std::vector<Word> ce(counter_examples.size());
	std::vector<std::string>::const_iterator ce_str;
	i = 0;
	for (ce_str = counter_examples.begin(); ce_str != counter_examples.end(); ++ce_str) {
		std::string::const_iterator w=ce_str->begin();
		while (w!=ce_str->end()) {
			std::unordered_map<unsigned long int, size_t>::const_iterator idx;
			idx = alphabet.find(next_unicode(w,ce_str->end()));
			if (idx == alphabet.end()) {
				throw std::runtime_error("invalid symbol in counter example"); // TODO: simply ignore counter example?
			}
			ce[i].push_back(idx->second);
		}
		i++;
	}
	this->counter_examples = ce;
}

Alphabet Oracle::get_alphabet() {
	return this->c->get_alphabet();
}

bool Oracle::decide(Word word) {
	this->mq++;
	return this->c->decide(word);
}

Word Oracle::difference(Concept* other, int algorithm) {
	return this->difference(other, algorithm, true);
}

Word Oracle::difference(Concept* other, int algorithm, bool count_query) {
	if (count_query) {
		this->eq++;
	}
	size_t i;
	for (i=0;i<this->counter_examples.size();i++) {
		if (this->c->decide(this->counter_examples[i]) != other->decide(this->counter_examples[i])) {
			return this->counter_examples[i];
		}
	}
	return this->c->difference(other, algorithm);
}

void Oracle::reset_queries() {
	this->eq = 0;
	this->mq = 0;
}
unsigned long int Oracle::get_eq_queries() {
	return this->eq;
}
unsigned long int Oracle::get_mq_queries() {
	return this->mq;
}

Oracle::~Oracle() {
}
