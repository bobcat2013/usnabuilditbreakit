
all: reader

reader: parse.cpp SQLBackend.h SQLBackend.cpp
	g++ -o reader -std=c++11 -O3 -l sqlite3 parse.cpp SQLBackend.cpp


