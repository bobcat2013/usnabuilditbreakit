
all: reader

sqlite3.o: sqlite3.c sqlite3.h
	gcc -c -O3 -o sqlite3.o sqlite3.c

parse.o: parse.cpp SQLBackend.h SQLBackend.cpp sqlite3.c sqlite3.h
	g++ -o parse.o -std=c++11 -O3 parse.cpp SQLBackend.cpp
	
reader: sqlite3.o parse.o
	g++ -o reader -std=c++11 -O3 parse.o sqlite3.o


