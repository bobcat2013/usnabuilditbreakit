#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <iostream>
#include "SQLBackend.h"
#include "except.h"

typedef std::uint8_t byte;
typedef std::vector<byte> buffer;

const char * invalid_type_errors[] = {
	"expected type pending_struct",
	"expected type struct",
	"expected type bit string",
	"expected type int",
	"expected type character string",
	"expected type float",
	"expected type utf8 string",
	"expected type RESERVED"
};
	

struct chunk_header {
	std::uint16_t id;
	std::uint8_t flags;
	std::uint8_t len_high;
	std::uint8_t len_mid;
	std::uint8_t len_low;
	
	//types
	enum type_t {
		PENDING,
		STRUCT,
		BITS,
		INT,
		CHAR,
		FLOAT,
		UTF8,
		RESERVED
	};
	//functions - small, so suggest that the compiler inline them
	std::uint32_t len() const {
		return (((uint32_t)len_high) << 16)
			| (((uint32_t)len_mid) << 8)
			| ((uint32_t)len_low);
	}
	std::uint32_t short_data() const {
		return len(); //the len field is data for a short field
	}
	type_t type() const {
		return (type_t)(flags >> 5);
	}
	bool is_compressed() const {
		return flags & (1 << 4);
	}
	bool is_encrypted() const {
		return flags & (1 << 3);
	}
	bool is_short() const {
		return flags & (1 << 2);
	}
	bool is_array() const {
		return flags & (1 << 1);
	}
	bool reserved_bit() const {
		return flags & 1;
	}
	void do_fix() {
		flags <<= 1; //to work with their incorrect test data
	}
	void construct(std::uint8_t* p) {
		std::memcpy((void*)this, (void*)p, sizeof(*this));
		construct();
	}
	
	void construct() {
		do_fix();
	}
	
} __attribute__((__packed__));

struct compression_header {
	std::uint8_t method;
	std::uint8_t orglen_high;
	std::uint8_t orglen_mid;
	std::uint8_t orglen_low;
	
	//functions
	std::uint32_t orglen() {
		return (((uint32_t)orglen_high) << 16)
			| (((uint32_t)orglen_mid) << 8)
			| ((uint32_t)orglen_low);
	}
} __attribute__((__packed__));

struct file_header {
	long version;
	std::string date;
};

byte * buffer_ptr(buffer& buf, std::size_t index = 0) {
	return &(buf[index]);
}

bool buffer_has(const buffer& buf, std::size_t left, std::size_t index) {
	return (buf.size() - index) >= left;
}

bool buffer_empty(const buffer& buf, std::size_t index) {
	return index >= buf.size();
}

void check_element_len(const buffer& buf, std::size_t len, std::size_t index) {
	if (!buffer_has(buf, len, index)) {
		throw parse_error("element length exceeds struct length");
	}
}

void check_element_type(chunk_header::type_t actual, chunk_header::type_t desired) {
	if (desired != actual) {
		throw schema_error(invalid_type_errors[desired]);
	}
}

void check_buffer_empty(const buffer& buf, std::size_t index) {
	if (!buffer_empty(buf, index)) {
		throw schema_error("extra data in struct");
	}
}

void read_data(std::ifstream& stream, char* data, std::size_t len) {
	stream.read(data, len);
	if (stream.eof()) {
		throw parse_error("expected more data");
	}
}

void read_data_buf(std::ifstream& stream, buffer& buf, std::size_t len) {
	buf.reserve(++len); //have an extra byte at the end to play with
	read_data(stream, (char*)buffer_ptr(buf), len);
}

void read_struct_buf(std::ifstream& stream, buffer& buf) {
	chunk_header head;
	read_data(stream, (char*)&head, sizeof(head));
	head.construct();
	check_element_type(head.type(), chunk_header::STRUCT);
	read_data_buf(stream, buf, head.len());
}

void get_data_buf(buffer& buf, void* dest, std::size_t& index, std::size_t len) {
	std::memcpy(dest, buffer_ptr(buf, index), len);
	index += len;
}

void get_header_buf(buffer& buf, chunk_header& h, std::size_t& index) {
	check_element_len(buf, sizeof(h), index);
	h.construct(buffer_ptr(buf, index));
	index += sizeof(h);
}

long get_int_buf(buffer& buf, std::size_t& index) {
	chunk_header head;
	get_header_buf(buf, head, index);
	std::uint32_t len = head.len();
	check_element_len(buf, len, index);
	check_element_type(head.type(), chunk_header::INT);
	byte* p = buffer_ptr(buf, index);
	for (; *p == 0 && len != 0; ++p) {
		--len;
	}
	long ret = 0;
	if (len > sizeof(ret)) {
		throw parse_error("integer overflow");
	}
	for (; len != 0; --len) {
		ret <<= 8;
		ret |= *(p++);
	}
	index += len;
	return ret;
}

char * get_char_buf(buffer& buf, std::size_t& index) {
	chunk_header head;
	get_header_buf(buf, head, index);
	std::uint32_t len = head.len();
	check_element_len(buf, len, index);
	check_element_type(head.type(), chunk_header::CHAR);
	char * ret = (char*)buffer_ptr(buf, index);
	ret[len] = '\0'; //WILL INVALIDATE NEXT CHUNK'S ID!!!!
	//alternate: will use up the extra allocated byte in buffer
	//	that we always have: see read_data_buf()
	index += len;
	return ret;
}

file_header read_file_head(std::ifstream& file, buffer& buf) {
	chunk_header head;
	file_header fhead;
	std::size_t iterator = 0;
	read_struct_buf(file, buf);
	fhead.version = get_int_buf(buf, iterator);
	fhead.date = get_char_buf(buf, iterator);
	check_buffer_empty(buf, iterator);
	return fhead;
}

void parse_file(const char* ifname, const char* ofname) {
	std::ifstream file(ifname, std::ifstream::in | std::ifstream::binary);
	buffer chunkbuf;
	chunk_header head;
	file_header fhead = read_file_head(file, chunkbuf);
	TransactionStream backend(ofname, fhead.version, fhead.date.c_str());
	std::size_t iterator;
	while (!file.eof()) {
		read_struct_buf(file, chunkbuf);
		iterator = 0;
		long account_num = get_int_buf(chunkbuf, iterator);
		char * date = get_char_buf(chunkbuf, iterator);
		char * money = get_char_buf(chunkbuf, iterator);
		check_buffer_empty(chunkbuf, iterator);
		backend.AddTransaction(account_num, date, money);
	}
	file.close();
}

int main(int argc, char ** argv) {
	std::ostream& error = std::cerr;
	if (argc != 3) {
		error << "Invalid number of arguments\n"
			"Usage: " << argv[0] << " SDXF SQL\n";
		return 1;
	}
	try {
		parse_file(argv[1], argv[2]);
	}
	catch (parse_error& p) {
		error << "MALFORMED DATA SAFE EXIT\n";
	}
	catch (...) {
		//some other sort of error
		error << "Unknown error occurred\n";
	}
	return 0;
}
	
	
