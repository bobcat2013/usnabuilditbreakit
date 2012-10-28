#ifndef SQLBACKEND_H_INC
#define SQLBACKEND_H_INC

#include <fstream>

class TransactionStream {
public:
	//TransactionStream(outfile, version, date)
	TransactionStream(const char*, long, const char*);
	~TransactionStream();
	//AddTransaction(AccountNum, Date, Amount)
	void AddTransaction(long, const char*, const char*);
private:
	std::ofstream out;
};

#endif
