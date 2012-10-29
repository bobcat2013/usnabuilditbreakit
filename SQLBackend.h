#ifndef SQLBACKEND_H_INC
#define SQLBACKEND_H_INC

//#define TEXT_BACKEND

#include <cstdint>

#ifdef TEXT_BACKEND
#include <fstream>
#else
#include "sqlite3.h"
#endif

//should be abstract, but why pay the overhead?

class TransactionStream {
public:
	//TransactionStream(outfile, version, date)
	TransactionStream(const char*, long, const char*);
	~TransactionStream();
	//AddTransaction(AccountNum, Date, Amount)
	void AddTransaction(std::uint64_t, const char*, const char*);
private:
#ifdef TEXT_BACKEND
	std::ofstream out; //for testing
#else
	sqlite3 * connection;
	sqlite3_stmt * sql_update;
	sqlite3_stmt * sql_insert;
#endif
};

#endif
