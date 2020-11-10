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
			virtual size_t decode(const unsigned char* input, std::vector<unsigned int>& output, int numints) const = 0;
			virtual std::ifstream& decode(std::ifstream& is, std::vector<unsigned int>& vec, int numints) const = 0;
			virtual size_t decode_bytes(const unsigned char* input, std::vector<unsigned int>& output, int numbytes) const = 0;
			virtual size_t encode(const unsigned int* num, std::vector<unsigned char>& out_vec, int size) const = 0;
			virtual std::ofstream& encode(std::ofstream& os, const unsigned int* input, int size) const = 0;
		};
		class varbyte : public compressor
		{
		public:
			size_t decode(const unsigned char* input, std::vector<unsigned int>& output, int numints) const;
			std::ifstream& decode(std::ifstream& is, std::vector<unsigned int>& vec, int numints) const;
			size_t decode_bytes(const unsigned char* input, std::vector<unsigned int>& output, int numbytes) const;
			size_t encode(const unsigned int* num, std::vector<unsigned char>& out_vec, int size) const;
			std::ofstream& encode(std::ofstream& os, const unsigned int* input, int size) const;
			size_t _decode(unsigned int* output, const unsigned char* input) const;
			size_t _encode(std::vector<unsigned char>& output, const unsigned int input) const;
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


