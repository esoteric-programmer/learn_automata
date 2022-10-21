#ifndef AUTOMATON_H
#define AUTOMATON_H
#include "concept.h"

typedef enum {
	DFA,
	NFA,
	UFA,
	AFA
} AutomatonType;

/**
 * finite automaton
 */
class Automaton : public Concept {
private:
	Alphabet sigma;
	std::vector<bool> states;
	Formula q0;
	Transitions transitions;
	Word diff(Automaton* other);
	Word diff_dfs(Automaton* other);
	Word diff_dfs_incremental(Automaton* other);
	Word diff_dfs_recursion(Automaton* other, Formula this_config, Formula other_config, Word word, std::unordered_set<std::pair<std::set<Term>,std::set<Term>>>& eq_set, int depth_left);
	Word randomized_diff(Automaton* other);
	Word get_residual_string_for_state(State s, int algorithm);
	Word get_residual_string_for_state_recursion(State s, Formula config, Word word, std::unordered_set<std::set<Term>>& nonmatching_states, int algorithm);
	Automaton* get_automaton();
	static std::string graphviz_monomial(Term monomial, std::unordered_map<Term, size_t>* monomials, std::ostringstream* out, std::ostringstream* out_trans);
	Formula get_next_config(Formula old_config, Symbol symbol);
	/**
	 * find a shortest word that is accepted by the automaton. may be the empty word.
	 * if no such word exists, an std::range_error is thrown.
	 * if q0 of the automaton is in CNF and additional_dnf is non-zero, the start state of the automaton is considered to be (q0 /\ additional_dnf).
	 */
	Word get_any_accepted_word(TermList* additional_dnf);
	/**
	 * returns a counter example for L(this) being a subset of L(other).
	 * if no such counter example exists (i.e. L(this) is a subset of L(other)), an std::range_error is thrown.
	 */
	Word get_element_not_in(Automaton* other);

	// helper functions
	static bool evaluate_monomial(Term& m, std::vector<bool>& states);
	static bool evaluate_clause(Term& m, std::vector<bool>& states);
	static bool evaluate_formula(Formula& phi, std::vector<bool>& states);

	// helper functions for regexp parsig and thompson's construction
	typedef size_t Token;
	typedef std::pair<Token, std::string> TokenInfo;
	typedef std::vector<TokenInfo> LexedExpression;
	typedef std::pair<State, State> Subautomaton; // states: q, f
	void init_from_regexp(Alphabet sigma, std::string reg_exp);
	std::string regexp_utf8_to_alphabet (std::string regexp);
	static LexedExpression lex(std::string expr);
	static std::string lex_symbol(std::string::const_iterator& it, std::string::const_iterator end);
	static size_t str2sym(std::string s);
	void add_trans(std::pair<std::pair<State, Symbol>,Formula> trans);
	Subautomaton symbol(std::string s); // symbol (number or empty)
	Subautomaton symbol(std::vector<size_t> s); // symbols (list of symbols)
	Subautomaton epsilon();
	Subautomaton _union(Subautomaton a1, Subautomaton a2);
	Subautomaton concat(Subautomaton a1, Subautomaton a2);
	Subautomaton quantify(Subautomaton a1, std::string quantifier); // quantifier: "*" or "+" or "?"
	void read_next(LexedExpression& lexed);
	Subautomaton expression(LexedExpression& lexed);
	Subautomaton or_statement(LexedExpression& lexed);
	Subautomaton concat_statement(LexedExpression& lexed);
	Subautomaton parse_symbol(LexedExpression& lexed);
	Subautomaton quantifier(Subautomaton expression, LexedExpression& lexed);
	bool add_lambda_successors_to_formula(Formula& f);
	void remap_formula(Formula& f, std::vector<size_t> mapping);
	void remove_lambda_transitions();
	
	/**
	 * create from (limited) regular expression
	 */
	Automaton(Alphabet sigma, std::string reg_exp);

public:
	Automaton(
			Alphabet sigma,
			std::vector<bool> states, // false = not accepting; true = accepting
			Formula q0,
			Transitions transitions);
	/**
	 * create from (limited) regular expression
	 */
	Automaton(std::string alphabet, std::string reg_exp);
	virtual Alphabet get_alphabet();
	unsigned long int num_states();
	/**
	 * test whether a word is accepted by the automaton (membership oracle)
	 */
	virtual bool decide(Word word);
	/**
	 * test whether the set of accepted words of this automaton equals the set of words in the specified concept (equivalence oracle)
	 * returns a counter example. if no counter example exists (i.e. the sets are equivalent), an std::range_error is thrown.
	 * algorithm 0 is an 2-EXPTIME algorithm, while algorithm 1 is a EXPTIME algorithm. however, in practise algorithm 0 seems to be faster than algorithm 1.
	 */
	virtual Word difference(Concept* other, int algorithm = 0);
	// uses difference method, pass algorithm to use for difference computation
	State get_nonresidual_state(int algorithm = 0);
	std::string to_graphviz();
	virtual ~Automaton();
};
#endif
