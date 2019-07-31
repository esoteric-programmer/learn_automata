#include "unicode.h"

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
