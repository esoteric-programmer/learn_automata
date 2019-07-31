#include <iostream>
#include <ctime>
#include "equivalence_oracle.h"
#include "table.h"
#include "unicode.h"
#include "random.h"
#include "parse.h"

typedef enum {
	UNDEFINED_ALGO,
	L_STAR,
	NL_STAR,
	UL_STAR,
	AL_STAR,
	AL_STAR_STAR,
} LearningAlgorithm;

// TODO: implement "optimizations" of AL_STAR (and UL_STAR)
// determine_residuality: perform residuality test on final hypotheis and print result? costs a lot of performance
Automaton* xl_star(Oracle* c, LearningAlgorithm algo = L_STAR, bool verbose = true, bool determine_residuality = true, FILE* output = 0) {
	AutomatonType type;
	bool test_residuality = false;
	switch (algo) {
		case L_STAR:
			type = DFA;
			break;
		case NL_STAR:
			type = NFA;
			break;
		case UL_STAR:
			type = UFA;
			break;
		case AL_STAR:
			type = AFA;
			break;
		case AL_STAR_STAR:
			type = AFA;
			test_residuality = true;
			break;
		default:
			return NULL;
	}
	if (!c) {
		throw std::runtime_error("target concept is NULL");
	}
	Automaton* h = 0;
	if (verbose) {
		std::cout << "initializing table" << std::endl;
	}
	Table t(c, type);
	Automaton* h2 = 0;
	while(1) {
	try {
		while(1) {
			try {
				while(1) {
					try{
						while(1) {
							if (verbose) {
								std::cout << "testing closedness" << std::endl;
							}
							Word u = t.uncomposable_word();
							if (t.add_row(u)) {
								if (verbose) {
									std::cout << "inserted row: " << c->get_alphabet().string(u) << std::endl;
								}
							}else{
								throw std::runtime_error("error while looking for non-decomposable row");
							}
						}
					}catch(const std::range_error&){
						if (verbose) {
							std::cout << "is closed" << std::endl;
						}
					}
					if (verbose) {
						std::cout << "drawing table" << std::endl;
						std::cout << t.to_string();
						std::cout << "generating hypothesis" << std::endl;
					}
					h = t.to_automaton(BASIS_ALGORITHM_SET_COVER);//BASIS_ALGORITHM_ANY_MIMIMAL_BASIS);
					// std::cout << "at the moment, the hypothesis has " << h->num_states() << " state" << (h->num_states()==1?"":"s") << "." << std::endl;
					if (h) {
						if (verbose) {
							std::cout << "the hypothesis is as follows" << std::endl;
							std::cout << h->to_graphviz();
							std::cout << "testing hypothesis" << std::endl;
						}
						Word d = c->difference(h, DIFFERENCE_ALGORITHM_AFA_DFS_INCREMENTAL);
						if (c->decide(d) == h->decide(d)) {
							throw std::runtime_error("error: invalid difference string");
						}
						if (t.add_col(d)) {
							if (verbose) {
								std::cout << "inserted counter example: " << c->get_alphabet().string(d) << std::endl;
							}
						}else{
							if (verbose) {
								std::cout << "found counter example, but it is already a column of T" << std::endl;
							}
							throw std::runtime_error("infinite loop detected");
						}
						delete h;
						h = 0;
					}else{
						throw std::runtime_error("hypothesis is NULL");
					}
				}
			}catch(const std::range_error&){
				if (verbose) {
					std::cout << "is equivalent" << std::endl;
				}
			}
			if (determine_residuality && verbose) {
				std::cout << "testing residuality" << std::endl;
				if (h) {
					try {
						size_t nr = h->get_nonresidual_state(DIFFERENCE_ALGORITHM_AFA_DFS_INCREMENTAL);
						std::cout << "exact testing result: automaton is not residual due to state q" << nr << std::endl;
					}catch(const std::range_error&){
						if (verbose) {
							std::cout << "exact testing result: automaton is residual" << std::endl;
						}
					}
				}
			}
			if (!test_residuality) {
				throw std::range_error("skip residuality test");
			}
			if (verbose) {
				std::cout << "testing residuality with AL** method" << std::endl;
			}
			h2 = t.to_automaton(BASIS_ALGORITHM_MAX_BASIS);
			if (h2) {
				if (verbose) {
					std::cout << "the hypothesis is as follows" << std::endl;
					std::cout << h2->to_graphviz();
					std::cout << "testing hypothesis" << std::endl;
				}
				Word d = c->difference(h2, DIFFERENCE_ALGORITHM_AFA_DFS_INCREMENTAL);
				if (c->decide(d) == h2->decide(d)) {
					throw std::runtime_error("error: invalid difference string");
				}
				if (t.add_col(d)) {
					if (verbose) {
						std::cout << "inserted counter example: " << c->get_alphabet().string(d) << std::endl;
					}
				}else{
					if (verbose) {
						std::cout << "found counter example, but it is already a column of T" << std::endl;
					}
					throw std::runtime_error("infinite loop detected");
				}
				delete h2;
				h2 = 0;
			}else{
				throw std::runtime_error("hypothesis is NULL");
			}
		}
	}catch(const std::range_error&){
		if (test_residuality) {
			if (verbose) {
				std::cout << "is residual" << std::endl;
			}
		}else if (h2) {
			delete h2;
			h2 = 0;
		}
		break;
	}
	}
	if (determine_residuality && !verbose) {
		std::cout << "exact residuality testing:" << std::endl;
		if (h) {
			try {
				size_t nr = h->get_nonresidual_state(DIFFERENCE_ALGORITHM_AFA_DFS_INCREMENTAL);
				//size_t nr = h->get_nonresidual_state(DIFFERENCE_ALGORITHM_AFA_DFS);
				//size_t nr = h->get_nonresidual_state(DIFFERENCE_ALGORITHM_AFA_BACKWARD_EVALUATION);
				std::cout << "automaton is not residual due to state q" << nr << std::endl;
			}catch(const std::range_error&){
					std::cout << "automaton is residual" << std::endl;
			}
		}
	}
	return h;
}

void print_usage(const char* executable_name) {
	std::cerr << "Usage:" << executable_name << " --file DFA.dot --algorithm [L,UL,NL,AL,ALP] [--seed seed] " << std::endl;
	std::cerr << "Call " << executable_name << " --help for a more detailed help message." << std::endl;
}

int main(int argc, char *argv[]) {
	// seeding
	unsigned int seed = 0;
	std::string fName = "";
	LearningAlgorithm algo = UNDEFINED_ALGO;


	if (argc < 2) {
		print_usage(argv[0]);
		return 0;
	}
	for(int i = 1; i < argc; i++){
		if (std::string(argv[i]) == "--help"){
			std::cout
				<< "The file argument should be in the dot-Format of graphviz.\n"
				<< "The (unique) start state should have the attribute 'start=\"true\"'" \
						" and all final states should have the attribute 'final=\"true\"'.\n"
				<< "Labels on edges correspond to the symbol for the transition."
				<< std::endl;
			std::cout
				<< "The algorithm argument L calls Angluin's L* algorithm; UL calls UL*; " \
						"NL calls NL*; AL calls AL* and ALP calls AL**."
				<< std::endl;
			return 0;
		}else if(std::string(argv[i]) == "--file" && i+1 < argc){
			fName = argv[i+1];
			i++;
		}else if(std::string(argv[i]) == "--algorithm" && i+1 < argc){
			std::string algos = std::string(argv[i+1]);
			i++;
			if (algos.compare("L") == 0){
				algo = L_STAR;
			}else if (algos.compare("UL") == 0){
				algo = UL_STAR;
			}else if (algos.compare("NL") == 0){
				algo = NL_STAR;
			}else if (algos.compare("AL") == 0){
				algo = AL_STAR;
			}else if (algos.compare("ALP") == 0){
				algo = AL_STAR_STAR;
			}else{
				std::cerr << "error: invalid algorithm: " << argv[i] << std::endl;
				print_usage(argv[0]);
				return 1;
			}
		}else if(std::string(argv[i]) == "--seed" && i+1 < argc){
			seed = strtoul(argv[i+1],0,0);
			i++;
			while (!seed){
				FILE* random = fopen("/dev/urandom","rb");
				if (!random){
					std::cerr << "error: cannot get seed from /dev/urandom" << std::endl;
					print_usage(argv[0]);
					return 1;
				}
				if (!fread(&seed, sizeof(unsigned int), 1, random)) {
					std::cerr << "error: cannot get seed from /dev/urandom" << std::endl;
					print_usage(argv[0]);
					return 1;
				}
				fclose(random);
			}
		}else{
			std::cerr << "error: invalid or malformed argument: " << argv[i] << std::endl;
			print_usage(argv[0]);
			return 1;
		}
	}
	if (algo == UNDEFINED_ALGO || (seed == 0 && fName == "")) {
		std::cerr << "error: missing at least one argument" << std::endl;
		print_usage(argv[0]);
		return 1;
	}

	FILE* output = 0;
	if (fName == "") {
	std::cout << "Seed: " << seed << std::endl;
	srand(seed);
	char fn[100];
	sprintf(fn,"seed_%u.log",seed);
	output = fopen(fn,"w");
	}else{
	std::string outName = fName + ".log";
	output = fopen(outName.c_str(),"w");
	}

	Automaton _c = (fName != "")?parse_automata(fName):generate_afa("abc", 6);

	EquivalenceOracle c(&_c, std::vector<std::string>(0)); // build equivalence (and membership) oracle from automaton _c


	//// LEARN TARGET LANGUAGE

	std::cout << "Learning Automaton..." << std::flush;
	clock_t begin = clock();
	Automaton* a = xl_star(&c, algo, false, false, output);
	clock_t end = clock();
	std::cout << " Finished after " << (double(end - begin) / CLOCKS_PER_SEC) << "seconds." << std::endl;
	if (a) {
		std::cout << "Automaton size: " << a->num_states() << " state" << (a->num_states()==1?"":"s") << std::endl;
		fprintf(output,"Output after %lu EQ and %lu MQ:\n", c.get_eq_queries(), c.get_mq_queries());
		fprintf(output,"%s", a->to_graphviz().c_str());
		fprintf(output,"Automata size: %lu state%s\n", a->num_states(), a->num_states()==1?"":"s");
	}
	fflush(output);
	return 0;

}
