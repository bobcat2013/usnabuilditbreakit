
all: receive

sqlite3.o: sqlite3.c sqlite3.h
	gcc -c -O3 -o sqlite3.o sqlite3.c

parse.o: parse.cpp SQLBackend.h
	g++ -c -o parse.o -std=c++11 -O3 parse.cpp
	
backend.o: SQLBackend.h SQLBackend.cpp sqlite3.h
	g++ -c -o backend.o -std=c++11 -O3 SQLBackend.cpp
	
receive: sqlite3.o parse.o backend.o
	g++ -o receive -std=c++11 -O3 parse.o sqlite3.o backend.o -pthread -ldl

