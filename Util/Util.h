#pragma once
#include <vector>

namespace Util
{
    struct Posting {
        unsigned int termid;
        unsigned int docid;
        unsigned int frequency;
        bool operator<(const Posting& rhs) const {
            return this->termid < rhs.termid || (this->termid == rhs.termid && this->docid < rhs.docid);
        }
    };

	std::vector<unsigned char> encode(const unsigned int& num);
	std::vector<unsigned int> decode(const unsigned char* arr, int numints);
	void print_byte_as_bits(unsigned char val);
}