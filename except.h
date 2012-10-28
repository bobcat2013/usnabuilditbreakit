#ifndef EXCEPT_H_INC
#define EXCEPT_H_INC

#include <exception>

class parse_error : public std::exception {
public:
	parse_error(const char* c) : msg(c) {}
	const char * what() { return msg; }
private:
	const char * msg;
};

class schema_error : public parse_error {
public:
	schema_error(const char* c) : parse_error(c) {}
};

#endif
