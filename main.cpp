#include <iostream>
#include <ctime>
#include <string.h>
#include "oracle.h"
#include "table.h"
#include "random.h"

typedef enum {
	ALL = 0,
	L_STAR = 1,
	NL_STAR = 2,
	UL_STAR = 3,
	AL_STAR = 4,
	AL_STAR_STAR = 5,
	NO_UL = 6
} LearningAlgorithm;

const char* const algo_name[] = {"", "L*", "NL*", "UL*", "AL*", "AL**"};
const char* const automaton_name[] = {"", "DFA", "NFA", "UFA", "AFA", "RAFA"};

/**
 * implementation of learning algorithms L*, NL*, UL*, AL*, AL**
 * it doesn't use the "optimizations" proposed for AL* (and UL*)
 */
Automaton* xl_star(Oracle* c, LearningAlgorithm algo, FILE* output, bool verbose = false, bool determine_residuality = false) {
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
	bool al_star_done = false;
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
			if (!al_star_done && algo == AL_STAR_STAR && output != 0 && h != 0) {
				fprintf(output,"AL* output after %lu EQ and %lu MQ:\n", c->get_eq_queries(), c->get_mq_queries());
				fprintf(output,"%s", h->to_graphviz().c_str());
				fprintf(output,"AFA size: %lu state%s\n\n", h->num_states(), h->num_states()==1?"":"s");
				fflush(output);
			}
			al_star_done = true;
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
	std::cout
			<< "Usage: " << executable_name << " [OPTIONS]..." << std::endl
			<< "  --algorithm <alg>  The parameter <alg> should be one of the following:" << std::endl
			<< "                       L     run Angluin's L* algorithm" << std::endl
			<< "                       NL    run NL* algorithm" << std::endl
			<< "                       UL    run UL* algorithm" << std::endl
			<< "                       AL    run AL* algorithm" << std::endl
			<< "                       ALP   run AL** algorithm (includes running AL*)" << std::endl
			<< "                       ALL   run all algorithms mentioned above [default]" << std::endl
			<< "                       noUL  run L*, NL*, and AL**" << std::endl
			<< "  --rat1             Learn a random automaton generated by RAT1." << std::endl
			<< "  --rat2             Learn a random automaton generated by RAT2 [default]." << std::endl
			<< "  --seed <seed>      Seed used for generation of random automaton (32bit int)." << std::endl
			<< "  --regex <s> <exp>  Learn automaton equivalent to regular expression." << std::endl
			<< "                     The parameter <s> contains all symbols of the alphabet."  << std::endl
			<< "                     The parameter <exp> defines the regular expression." << std::endl
			<< "                     Only the following meta characters are supported:"  << std::endl
			<< "                       ( ) ? + * . | [ ] \\" << std::endl
			<< "                     Inside of [], the following meta characters are supported:" << std::endl
			<< "                       ^ - \\" << std::endl
			<< "                     Example: --regex abc 'b*|[^b]+'" << std::endl
			<< "  --ce <examples>    Counter examples that shall be provided to the learner." << std::endl
			<< "                     Multiple counter examples are separated by comma." << std::endl
			<< "  --help             Show this help message." << std::endl;
	// counter example for AL* residuality:
	// ./automata --regex abcdefghijklmnopqr '(c(n|k|f|fp)|d(o|l|f|fp)|b(n|j|fp|fq|fg(g|r))|e(o|m|fp|fq|fg(g|r))|hq|i(g|r)|afp|afgg|an|ao|hg(g|r))g*' --ce afp,dl,an,ck,ao,em,bj,hq,ir
}

int main(int argc, char *argv[]) {
	// seeding
	unsigned int seed = 0;
	std::string sigma = "";
	std::string regex = "";
	LearningAlgorithm algo = ALL;
	bool rat2 = true;
	std::vector<std::string> counter_examples = std::vector<std::string>(0);


	for(int i = 1; i < argc; i++){
		if (std::string(argv[i]) == "--help"){
			print_usage(argv[0]);
			return 0;
		}else if(std::string(argv[i]) == "--regex" && i+2 < argc){
			sigma = argv[i+1];
			regex = argv[i+2];
			i+=2;
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
			}else if (algos.compare("ALL") == 0){
				algo = ALL;
			}else if (algos.compare("noUL") == 0){
				algo = NO_UL;
			}else{
				std::cerr << "error: invalid algorithm: " << argv[i] << std::endl;
				print_usage(argv[0]);
				return 1;
			}
		}else if(std::string(argv[i]) == "--seed" && i+1 < argc){
			seed = strtoul(argv[i+1],0,0);
			i++;
		}else if(std::string(argv[i]) == "--rat1"){
			rat2 = false;
		}else if(std::string(argv[i]) == "--rat2"){
			rat2 = true;
		}else if(std::string(argv[i]) == "--ce" && i+1 < argc){
			char* ce = strtok(argv[i+1],",");
			i++;
			counter_examples.clear();
			while (ce) {
				counter_examples.push_back(std::string(ce));
				ce = strtok(NULL, ",");
			}
		}else{
			std::cerr << "error: invalid or malformed argument: " << argv[i] << std::endl;
			print_usage(argv[0]);
			return 1;
		}
	}

	if (seed == 0 && sigma == "" && regex == "") {
		FILE* random = fopen("/dev/urandom","rb");
		bool flag = false;
		if (random) {
			if (fread(&seed, sizeof(unsigned int), 1, random) == 1) {
				flag = true;
			}
			fclose(random);
		}
		if (!flag) {
			seed = (unsigned int)time(NULL);
		}
	}

	FILE* output = 0;
	if (regex == "" && sigma == "") {
		std::cout << "Seed: " << seed << std::endl;
		srand(seed);
		char fn[100];
		sprintf(fn,"seed_%u.log",seed);
		output = fopen(fn,"w");
	}else{
		output = fopen("regex.log","w");
	}

	Automaton _c = (regex != "" && sigma!= "")?Automaton(sigma,regex):(rat2?generate_afa("abc", 6):generate_afa_fisman());

	Oracle c(&_c, counter_examples); // build equivalence (and membership) oracle from automaton _c

	if (regex == "" && sigma == "") {
		fprintf(output,"Random target AFA:\n");
		fprintf(output,"%s\n", _c.to_graphviz().c_str());
		fflush(output);
	}else{
		fprintf(output,"Target language:\n");
		fprintf(output,"Sigma: %s\n",sigma.c_str());
		fprintf(output,"Regular expression: %s\n\n",regex.c_str());
	}

	//// LEARN TARGET LANGUAGE
	for (LearningAlgorithm current_algo = ((algo==ALL || algo==NO_UL)?L_STAR:algo); current_algo<=AL_STAR_STAR && (algo==ALL || algo == NO_UL || current_algo==algo); current_algo=(LearningAlgorithm)(current_algo+1)) {
		if (algo==NO_UL && current_algo == UL_STAR) {
			//skip UL*
			continue;
		}
		if ((algo==ALL || algo==NO_UL) && current_algo == AL_STAR) {
			//Running AL** until the first test for residuality is equivalent to a run of AL*
			continue;
		}
		std::cout << "Running " << algo_name[current_algo] << "..." << std::flush;
		clock_t begin = clock();
		Automaton* a = xl_star(&c, current_algo, output);
		clock_t end = clock();
		std::cout << " Finished after " << (double(end - begin) / CLOCKS_PER_SEC) << "seconds." << std::endl;
		if (a) {
			std::cout << automaton_name[current_algo] << " size: " << a->num_states() << " state" << (a->num_states()==1?"":"s") << std::endl;
			fprintf(output,"%s output after %lu EQ and %lu MQ:\n", algo_name[current_algo], c.get_eq_queries(), c.get_mq_queries());
			fprintf(output,"%s", a->to_graphviz().c_str());
			fprintf(output,"%s size: %lu state%s\n\n", automaton_name[current_algo], a->num_states(), a->num_states()==1?"":"s");
			delete a;
		}
		c.reset_queries();
		fflush(output);
	}
	fclose(output);
	return 0;

}
