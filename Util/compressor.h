#pragma once

#include <vector>
#include <fstream>

namespace Util
{
	namespace compression
	{
		class compressor
		{
		public:
			virtual std::vector<unsigned char> encode(const unsigned int* num, int size) const = 0;
			virtual std::vector<unsigned int> decode(const unsigned char* arr, int numints) const = 0;
			virtual std::ifstream& decode(std::ifstream& is, std::vector<unsigned int>& vec, int numints) const = 0;
			virtual size_t encode(const unsigned int* num, unsigned char*& out_p, int size) const = 0;
			virtual size_t encode(const unsigned int* num, std::vector<unsigned char>& out_vec, int size) const = 0;
		};
		class varbyte : public compressor
		{
		public:
			std::vector<unsigned char> encode(const unsigned int* num, int size) const override;
			std::vector<unsigned int> decode(const unsigned char* arr, int numints) const override;
			std::ifstream& decode(std::ifstream& is, std::vector<unsigned int>& vec, int numints) const override;
			size_t encode(const unsigned int* num, unsigned char*& out_p, int size) const override;
			size_t encode(const unsigned int* num, std::vector<unsigned char>& out_vec, int size) const override;	
		};

		class CompressorCreator
		{
		public:
			virtual compressor* create() const = 0;
			virtual ~CompressorCreator() {};
		};

		class VarbyteCreator : public CompressorCreator
		{
		public:
			compressor* create() const override
			{
				return new varbyte();
			}
		};
	}
}


