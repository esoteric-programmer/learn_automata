#ifndef TABLE_H
#define TABLE_H
#include "automaton.h"

#define BASIS_ALGORITHM_MAX_BASIS          0
#define BASIS_ALGORITHM_ANY_MIMIMAL_BASIS  1
#define BASIS_ALGORITHM_SET_COVER          2

/**
 * represents the table used by L*, NL*, UL*, AL*, and AL** learning algorithm
 */
class Table {
private:
	Alphabet alphabet;
	AutomatonType automaton_type;
	Concept* oracle;
	std::vector<Word> u;
	std::vector<Word> ua;
	std::vector<Word> v;
	std::vector<std::vector<bool>> table;

	Formula decompose(std::vector<bool> row, std::unordered_set<size_t> primes);
	Formula decompose_dfa(std::vector<bool> row, std::unordered_set<size_t> primes);
	Formula decompose_nfa(std::vector<bool> row, std::unordered_set<size_t> primes);
	Formula decompose_ufa(std::vector<bool> row, std::unordered_set<size_t> primes);
	Formula decompose_afa(std::vector<bool> row, std::unordered_set<size_t> primes);
	// algorithm specifies method: everything from Rows_high, Angluin/Bollig/..., or Set Cover method
	// 0: every row from u is part of the basis
	// 1: Angluin
	// 2: Set Cover
	std::unordered_set<size_t> construct_basis(int algorithm);
	size_t get_uncomposable_row_id(std::unordered_set<size_t> basis);
	std::unordered_set<size_t> minimize_basis(std::unordered_set<size_t> basis);

public:
	Table(
			Concept* oracle,
			AutomatonType destination_type);
	// throws an std::range_error if the table is closed
	Word uncomposable_word();
	// throws an std::range_error if the table is consistent
	// Word inconsistent_word();
	// constructs a new Automaton object and returns a pointer to this object.
	// the caller must delete the returned Automaton object later in order to avoid memory leaks
	Automaton* to_automaton(int basis_algorithm);
	// add some word and all prefixes/suffixes as row/col to table
	// returns whether table has been changed (false indicates that xL* runs into an infinite loop)
	bool add_row(Word row);
	bool add_col(Word col);
	std::string to_string();
	~Table();
};
#endif
