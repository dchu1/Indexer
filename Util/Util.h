#pragma once
#include <vector>
#include <string>
#include <istream>

namespace Util
{
    struct Posting {
        unsigned int termid;
        unsigned int docid;
        unsigned int frequency;
        Posting(const Posting& obj) : termid{obj.termid}, docid{ obj.docid }, frequency{ obj.frequency }
        {
        }
        Posting(unsigned int a, unsigned int b, unsigned int c) : termid{ a }, docid{ b }, frequency{ c }
        {
        }
        Posting() = default;
        bool operator<(const Posting& rhs) const {
            return this->termid < rhs.termid || (this->termid == rhs.termid && this->docid < rhs.docid);
        }
        bool operator>(const Posting& rhs) const {
            return rhs < *this;
        }
        bool operator==(const Posting& rhs) const {
            return this->termid == rhs.termid && this->docid == rhs.docid;
        }
    };

    struct Posting_str {
        std::string term;
        unsigned int docid;
        unsigned int frequency;
        Posting_str(const Posting_str &obj) : term(obj.term), docid{obj.docid}, frequency{obj.frequency}
        {
        }
        Posting_str() = default;
        bool operator<(const Posting_str& rhs) const {
            return this->term.compare(rhs.term) < 0 || (this->term.compare(rhs.term)==0 && this->docid < rhs.docid);
        }
        bool operator>(const Posting_str& rhs) const {
            return rhs < *this;
        }
        bool operator==(const Posting_str& rhs) const {
            return this->term.compare(rhs.term) == 0 && this->docid == rhs.docid;
        }
    };
    
	std::vector<unsigned char> encode(const unsigned int* num, int size);
	std::vector<unsigned int> decode(const unsigned char* arr, int numints);
    std::istream& decode(std::istream& is, std::vector<unsigned int>& vec, int numints);
	void print_byte_as_bits(unsigned char val);
}