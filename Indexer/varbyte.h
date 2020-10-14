#pragma once
#include <vector>
namespace varbyte {
	std::vector<unsigned char> encode(const unsigned int& num);
	std::vector<unsigned int> decode(const std::vector<char>& vect);
	void print_byte_as_bits(unsigned char val);
}
