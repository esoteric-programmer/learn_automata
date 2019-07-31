#include <fstream>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/property_map/dynamic_property_map.hpp>
#include <boost/graph/graph_utility.hpp>
#include "automaton.h"
#include "unicode.h"
#include <set>

struct DotVertex {
  std::string name;
  std::string final;
  std::string start;
};

struct DotEdge {
  std::string label;
};

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS,
                              DotVertex, DotEdge> graph_t;

Automaton parse_automata(std::string fName)
{
  graph_t g;
  boost::dynamic_properties dp(boost::ignore_other_properties);

  
  dp.property("final",       boost::get(&DotVertex::final,         g));
  dp.property("start",       boost::get(&DotVertex::start,         g));
  dp.property("label",       boost::get(&DotEdge::label,         g));
  std::ifstream dot(fName);


  boost::read_graphviz(dot,g,dp);

  std::set<std::string> alphabet;
  for (auto ed : boost::make_iterator_range(edges(g))) {
    alphabet.insert(g[ed].label);
  }

  std::map<std::string, int> m;
  std::map<int,std::string> minv;
    int j = 0;
  for(auto l: alphabet){
    m[l] = j;
    minv[j] = l;
    j++;
  }


  std::cout << "Vertices: " << num_vertices(g) << "\n";
  std::string tmp_alph ="";
  for (std::string s: alphabet){
    tmp_alph.append(utf8(m[s]));
  }


  Alphabet alph;
	std::string::const_iterator alphabet_it = tmp_alph.begin();
	while (alphabet_it != tmp_alph.end()) {
		alph.push_back(next_unicode(alphabet_it, tmp_alph.end()));
	}


  std::cout << "Alphabet: ";
  for (auto a: alph){
    std::cout << a << " ";
  }
  std::cout << "\n";

  std::vector<bool> states(num_vertices(g));
  int i = 0;
  int start = -1;
  int finalstate = -1;
  for (auto vd : boost::make_iterator_range(vertices(g))) {
    if(g[vd].final.find("true") == std::string::npos){
      states[i] = 1;
      finalstate = i;

    }
    else{
      states[i] = 0;
    }
    if(g[vd].start.find("true") == std::string::npos){
    start = i;
    }
    i++;
  }

  if(start == -1){
    throw std::runtime_error("No start state was specified.");
  }

  if(finalstate == -1){
    throw std::runtime_error("No final state was specified.");
  }



	Formula q0;
  Term t0;
  t0.insert(start);
  q0.terms.push_back(t0);


  Transitions tr;
  for (auto ed : boost::make_iterator_range(edges(g))) {
    unsigned long int sv = source(ed,g);
    unsigned long int tv = target(ed,g);
      Formula f;
      Term t;
      t.insert(tv);
      f.terms.push_back(t);
      
      unsigned long int symb = m[g[ed].label];

      std::cout << "symb: " << symb << "\n";
      
      tr.insert(std::make_pair(std::make_pair(sv,symb),f));
    }




  Automaton a = Automaton(alph, states, q0, tr);
  return a;
  
}
