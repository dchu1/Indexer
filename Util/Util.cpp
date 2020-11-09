// Util.cpp : Defines the functions for the static library.

#include <iostream>
#include "Util.h"
#include <vector>
#include <stdexcept>
namespace Util {
    //void print_byte_as_bits(unsigned char val) {
    //    for (int i = 7; 0 <= i; i--) {
    //        cout << ((val & (1 << i)) ? '1' : '0');
    //    }
    //    cout << "\n";
    //}

	std::vector<unsigned char> encode(const unsigned int *num, const int size)
	{
		std::vector<unsigned char> result;
		unsigned char highest_bit_one = 128;
		for (int it = 0; it < size; it++) {
			unsigned char multiplicand, temp;
			int base, size, i;
			int remaining_value;
			remaining_value = *(num + it);
			i = (int)(log(remaining_value) / log(128)); // largest power of 128
			while (remaining_value >= 128)
			{
				base = pow(128, i);
				multiplicand = remaining_value / base;
				temp = multiplicand | highest_bit_one;
				result.push_back(temp);
				remaining_value = remaining_value % base;
				i--;
				//print_byte_as_bits(temp);
			}
			temp = (unsigned char)remaining_value;
			result.push_back(temp);
			//print_byte_as_bits(temp);
		}
		return result;
	}

	std::vector<unsigned int> decode(const unsigned char* arr, int numints)
	{
		int start = 0, end = 0, accumulator = 0, size = 1, counter = 0, val;
		std::vector<unsigned int> result;
		result.reserve(numints);
		while (counter < numints)
		{
			// if we have read more than enough bytes for an unsigned int,
			// something is wrong with our input
			size = end - start + 1;
			if (size > sizeof(unsigned int) + 1)
			{
				throw std::invalid_argument("Decode: Read too many bytes");
			}
			// check if the highest order bit is 0
			// if it is, we've reached the end of an int
			if ((arr[end] >> 7) ^ 1)
			{
				for (int i = size - 1; i >= 0; i--)
				{
					// clear the highest bit
					val = arr[start] & ~(1UL << 7);
					accumulator = accumulator + pow(128, i) * val;
					start++;
				}
				result.push_back(accumulator);
				start = end + 1;
				accumulator = 0;
				++counter;
			}
			++end;
		}
		return result;
	}

	//std::vector<unsigned int> decode(std::istream& is, int numints)
	std::ifstream& decode(std::ifstream& is, std::vector<unsigned int>& vec, int numints)
	{
		unsigned char c;
		std::vector<unsigned char> char_vec;
		char_vec.reserve(numints * sizeof(unsigned int));
		int size = 0, counter = 0;
		try {
			while (counter < numints && is.get(reinterpret_cast<char&>(c)))
			//while (counter < numints && is >> c)
			{
				// check if the highest order bit is 0
				// if it is, we've reached the end of an int
				char_vec.push_back(c);
				if ((c >> 7) ^ 1)
				{
					++counter;
					size = 0;
				}
				++size;
				// if we have read more than enough bytes for an unsigned int,
				// something is wrong with our input
				if (size > sizeof(unsigned int))
				{
					throw std::invalid_argument("decode");
				}
			}
		}
		catch (std::exception& e)
		{
			// do some handling maybe. I think this would most likely happen if
			// istream ends unexpectedly (i.e. reach eof before expected number
			// of bytes
			throw e;
		}
		// if char_vec.size() == 0, it means the stream encountered eof before putting
		// anything in vector
		if (char_vec.size() > 0 && counter == numints)
			vec = decode(char_vec.data(), numints);
		return is;
	}

	namespace lexicon
	{
		std::ifstream& read_lexicon(std::ifstream& is, LexiconEntry& le)
		{
			try {
				std::vector<unsigned int> vec;
				if (decode(is, vec, 3))
				{
					le.metadata.position = vec[0];
					le.metadata.docid_count = vec[1];
					std::unique_ptr<char[]> buf(new char[vec[2] + 1]);
					is.read(buf.get(), vec[2]);
					buf[vec[2]] = '\0';
					le.term = buf.get();
				}
				return is;
			}
			catch (std::exception& e)
			{
				std::string msg = std::string(e.what()) + " Lexicon File Position: " + std::to_string(is.tellg());
				throw std::exception(msg.c_str());
			}
		}
	}
	namespace urltable
	{
		std::ifstream& get_url(std::ifstream& is, UrlTableEntry& ute)
		{
			try {
				std::vector<unsigned int> vec;
				if (decode(is, vec, 2))
				{
					ute.size = vec[0];
					std::unique_ptr<char[]> buf(new char[vec[1] + 1]);
					is.read(buf.get(), vec[1]);
					buf[vec[1]] = '\0';
					ute.url = buf.get();
				}
				return is;
			}
			catch (std::exception& e)
			{
				throw e;
			}
		}
	}
	namespace index
	{
		std::ifstream& get_chunk(std::ifstream& is, std::vector<unsigned int>& vec, unsigned int size)
		{
			return Util::decode(is, vec, size);
		}
	}
}
