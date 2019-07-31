#ifndef UNICODE_H
#define UNICODE_H

#include<string>
#include <stdexcept>

std::string utf8(unsigned long int unicode);
unsigned long int next_unicode(std::string::const_iterator& it, std::string::const_iterator end);
#endif
