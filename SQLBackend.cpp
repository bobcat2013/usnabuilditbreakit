#include "SQLBackend.h"

TransactionStream::TransactionStream(const char* ofname, long version, const char* date) :
	out(ofname)
{
	out << version << '\n' << date << '\n';
}

TransactionStream::~TransactionStream() {
	out.close();
}

void TransactionStream::AddTransaction(long account, const char* date, const char* amount) {
	out << "ID: " << account << '\n'
		<< "Balance Change: " << amount << '\n'
		<< "Date: " << date << '\n';
	//done
}
