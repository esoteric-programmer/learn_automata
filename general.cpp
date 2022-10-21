#include <algorithm>
#include <stdexcept>
#include "general.h"

std::string utf8(unsigned long int unicode) {
	std::string ret("");
	if (unicode <= 0x7f) {
		ret.push_back((char)unicode);
	}else if (unicode <= 0x7ff) {
		ret.push_back( (char) (unicode / 0x40) + 0xC0 );
		ret.push_back( (char) (unicode % 0x40) + 0x80 );
	}else if (unicode <= 0xffff) {
		ret.push_back( (char) ((unicode / 0x40) / 0x40) + 0xE0);
		ret.push_back( (char) ((unicode / 0x40) % 0x40) + 0x80 );
		ret.push_back( (char) (unicode % 0x40) + 0x80 );
	}else if (unicode <= 0x10ffff) {
		ret.push_back( (char) (((unicode / 0x40) / 0x40) / 0x40) + 0xF0);
		ret.push_back( (char) (((unicode / 0x40) / 0x40) % 0x40) + 0x80 );
		ret.push_back( (char) ((unicode / 0x40) % 0x40) + 0x80 );
		ret.push_back( (char) (unicode % 0x40) + 0x80 );
	}else{
		throw std::runtime_error("invalid unicode code point");
	}
	return ret;
}

unsigned long int next_unicode(std::string::const_iterator& it, std::string::const_iterator end) {
	unsigned long int ret = 0;
	if (it == end) {
		throw std::out_of_range("end of string reached");
	}
	if (*it <= 0x7F) {
		ret = *it;
		++it;
		return ret;
	}
	if (*it < 0xC0) {
		throw std::runtime_error("invalid utf-8 character");
	}
	unsigned char left = 0;
	if (*it < 0xE0) {
		ret = ((unsigned long int)*it & 0x1F);
		left = 1;
	}else if (*it < 0xF0) {
		ret = ((unsigned long int)*it & 0x0F);
		left = 2;
	}else if (*it < 0xF8) {
		ret = ((unsigned long int)*it & 0x07);
		left = 3;
	}else{
		throw std::runtime_error("invalid utf-8 character");
	}
	std::string::const_iterator tmp = it;
	++it;
	while (left) {
		if (it == end) {
			it = tmp;
			throw std::out_of_range("end of string reached");
		}
		if (*it < 0x80 || *it >= 0xC0) {
			it = tmp;
			throw std::runtime_error("invalid utf-8 character");
		}
		ret *= 0x40;
		ret += ((unsigned long int)*it & 0x3F);
		++it;
		left--;
	}
	return ret;
}

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
