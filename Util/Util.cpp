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
	namespace lexicon
	{
		std::ifstream& read_lexicon(std::ifstream& is, LexiconEntry& le, Util::compression::compressor* compressor)
		{
			try {
				std::vector<unsigned int> vec;
				if (compressor->decode(is, vec, 3))
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
				//if (decode(is, vec, 2))
				//{
				//	ute.size = vec[0];
				//	std::unique_ptr<char[]> buf(new char[vec[1] + 1]);
				//	is.read(buf.get(), vec[1]);
				//	buf[vec[1]] = '\0';
				//	ute.url = buf.get();
				//}
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
			//return Util::decode(is, vec, size);
			return is;
		}
	}
}
