#include "SQLBackend.h"
#include "except.h"

#include <cstring>
#include <cstdlib>

void check_numeric(char c) {
	if (c < '0' || c > '9') {
		throw parse_error("invalid date format");
	}
}

void check_dash(char c) {
	if (c != '-') {
		throw parse_error("invalid date format");
	}
}

//disgusting function because they can't seem to use a standard
//date format...
void reformat_date(const char * src, char * dest) {
	if (src == NULL) return;
	std::size_t len = std::strlen(src);
	std::size_t offset = 0;
	if (len < 8 || len > 10) {
		throw parse_error("invalid date format");
	}
	check_numeric(src[0]);
	if (src[1] == '-') {
		offset = 2;
		dest[8] = '0';
		dest[9] = src[0];
	}
	else {
		offset = 3;
		check_numeric(src[1]);
		check_dash(src[2]);
		dest[8] = src[0];
		dest[9] = src[1];
	}
	dest[7] = '-';
	check_numeric(src[offset]);
	if (src[offset+1] == '-') {
		dest[5] = '0';
		dest[6] = src[offset];
		offset += 2;
	}
	else {
		check_numeric(src[offset+1]);
		check_dash(src[offset+2]);
		dest[5] = src[offset];
		dest[6] = src[offset+1];
		offset += 3;
	}
	dest[4] = '-';
	if (len - offset != 4) {
		throw parse_error("invalid date format");
	}
	for (int i = 0; i < 4; ++i) {
		check_numeric(src[offset+i]);
		dest[i] = src[offset+i];
	}
	dest[10] = '\0';
	//DONE!
}

void check_date(char * date) {
	if (
		(date[0] == '0' && date[1] == '0'
			&& date[2] == '0' && date[3] == '0')
		||
		(date[5] == '0' && date[6] == '0')
		||
		(date[8] == '0' && date[9] == '0'))
	{
		throw parse_error("invalid date");
	}
}

#ifdef TEXT_BACKEND

TransactionStream::TransactionStream(const char* ofname, long version, const char* date) :
	out(ofname)
{
	out << version << '\n' << date << '\n';
}

TransactionStream::~TransactionStream() {
	out << '\n';
	out.close();
}

void TransactionStream::AddTransaction(long account, const char* date_s, const char* amount) {
	char date[11];
	reformat_date(date_s, date);
	check_date(date);
	out << "ID: " << account << '\n'
		<< "Balance Change: " << amount << '\n'
		<< "Date: " << date << '\n';
	//done
}

#else

const char * sql_create_str = "CREATE TABLE accounts(id INTEGER PRIMARY KEY, balance INTEGER, date DATETIME)";
const char * sql_update_str = "UPDATE accounts SET balance = balance + ?, date = date(?) WHERE id = ?";
const char * sql_insert_str = "INSERT INTO accounts VALUES (?, ?, date(?))";

TransactionStream::TransactionStream(const char* ofname, long version, const char* date) {
	int error;
	error = sqlite3_open_v2(
		ofname,
		&connection,
		SQLITE_OPEN_READWRITE,
		NULL
	);
	if (error != SQLITE_OK) {
		sqlite3_close(connection);
		error = sqlite3_open_v2(
			ofname,
			&connection,
			SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
			NULL
		);
		if (error != SQLITE_OK) {
			throw sql_error(sqlite3_errmsg(connection));
		}
		error = sqlite3_exec(
			connection,
			sql_create_str,
			NULL,
			NULL,
			NULL
		);
		if (error != SQLITE_OK) {
			throw sql_error(sqlite3_errmsg(connection));
		}
	}
	error = sqlite3_prepare_v2(
		connection,
		sql_update_str,
		std::strlen(sql_update_str) + 1,
		&sql_update,
		NULL
	);
	if (error != SQLITE_OK) {
		throw sql_error(sqlite3_errmsg(connection));
	}
	error = sqlite3_prepare_v2(
		connection,
		sql_insert_str,
		std::strlen(sql_insert_str) + 1,
		&sql_insert,
		NULL
	);
	if (error != SQLITE_OK) {
		throw sql_error(sqlite3_errmsg(connection));
	}
	
}

TransactionStream::~TransactionStream() {
	sqlite3_finalize(sql_insert);
	sqlite3_finalize(sql_update);
	sqlite3_close(connection);
}

void TransactionStream::AddTransaction(std::uint64_t account, const char * date_s, const char * amount_s) {
	long long amount = std::strtoll(amount_s, NULL, 10);
	if (amount == 0) return; //ignore
	char date[11];
	reformat_date(date_s, date);
	check_date(date);
	sqlite3_bind_int64(sql_update, 1, amount);
	sqlite3_bind_text(sql_update, 2, date, 11, SQLITE_STATIC);
	sqlite3_bind_int64(sql_update, 3, account);
	if (sqlite3_step(sql_update) != SQLITE_DONE) {
		throw sql_error(sqlite3_errmsg(connection));
	}
	if (sqlite3_changes(connection) == 0) {
		sqlite3_bind_int64(sql_insert, 1, account);
		sqlite3_bind_int64(sql_insert, 2, amount);
		sqlite3_bind_text(sql_insert, 3, date, 11, SQLITE_STATIC);
		if (sqlite3_step(sql_insert) != SQLITE_DONE) {
			throw sql_error(sqlite3_errmsg(connection));
		}
		sqlite3_clear_bindings(sql_insert);
		sqlite3_reset(sql_insert);
	}
	sqlite3_clear_bindings(sql_update);
	sqlite3_reset(sql_update);
}

#endif
