#include "compressor.h"
#include <iostream>
size_t Util::compression::varbyte::encode(const unsigned int* input, std::vector<unsigned char>& out_vec, int size) const
{
	size_t written_size = 0;
	try {
		for (int i = 0; i < size; i++) {
			written_size += _encode(out_vec, input[i]);
		}
	}
	catch (std::exception& e)
	{
		throw e;
	}
	return written_size;
}

std::ofstream& Util::compression::varbyte::encode(std::ofstream& os, const unsigned int* input, int size) const
{
	std::vector<unsigned char> temp_char_vec;
	temp_char_vec.reserve(size*sizeof(unsigned int));
	try {
		encode(input, temp_char_vec, size);
		os.write((char*)(temp_char_vec.data()), temp_char_vec.size());
	}
	catch (std::exception& e)
	{
		throw e;
	}
	return os;
}

size_t Util::compression::varbyte::decode(const unsigned char* input, std::vector<unsigned int>& output, int numints) const
{
	size_t read_size = 0;
	unsigned int temp_ui;
	try {
		for (int i = 0; i < numints; i++)
		{
			read_size += _decode(&temp_ui, &input[read_size]);
			output.push_back(temp_ui);
		}
	}
	catch (std::exception& e)
	{
		throw e;
	}
	return read_size;
}

std::ifstream& Util::compression::varbyte::decode(std::ifstream& is, std::vector<unsigned int>& vec, int numints) const
{
	unsigned int counter = 0;
	std::vector<unsigned char> temp_char_vec;
	unsigned char c;
	while (counter < numints && is.get(reinterpret_cast<char&>(c)))
	{
		if ((c >> 7) ^ 1)
			++counter;
		temp_char_vec.push_back(c);
	}
	if (is)
	{
		if (counter != numints)
			throw "decode: did not read expected # of ints";
		temp_char_vec.size();
		decode(temp_char_vec.data(), vec, numints);
	}
	return is;
}

size_t Util::compression::varbyte::decode_bytes(const unsigned char* input, std::vector<unsigned int>& output, int numbytes) const
{
	size_t read_size = 0, r=0;
	unsigned int temp_ui;
	try {
		for (int i = 0; i < numbytes; i += r)
		{
			r = _decode(&temp_ui, &input[read_size]);
			read_size += r;
			output.push_back(temp_ui);
		}
	}
	catch (std::exception& e)
	{
		throw e;
	}
	return read_size;
}
// returns number of bytes written
size_t Util::compression::varbyte::_encode(std::vector<unsigned char>& output, const unsigned int input) const
{
	unsigned char multiplicand, temp, highest_bit_one = 128;
	unsigned int base, size, i, remaining_value, counter = 0;
	remaining_value = input;
	i = (int)(log(remaining_value) / log(128)); // largest power of 128
	while (remaining_value >= 128)
	{
		counter++;
		base = pow(128, i);
		multiplicand = remaining_value / base;
		temp = multiplicand | highest_bit_one;
		output.push_back(temp);
		remaining_value = remaining_value % base;
		i--;
		//print_byte_as_bits(temp);
	}
	if (counter++ > sizeof(unsigned int))
		throw "encode: wrote too many bytes";
	temp = (unsigned char)remaining_value;
	output.push_back(temp);
	return counter;
}

// returns the number of bytes read
size_t Util::compression::varbyte::_decode(unsigned int* output, const unsigned char* input) const
{
	int accumulator = 0, size = 0, val;
	bool finished = false;
	for (size = 0; size < sizeof(unsigned int) + 1; size++)
	{
		// check if the highest order bit is 0
		// if it is, we've reached the end of an int
		if ((input[size] >> 7) ^ 1)
		{
			for (int i = size; i >= 0; i--)
			{
				// clear the highest bit
				val = input[size-i] & ~(1UL << 7);
				accumulator = accumulator + pow(128, i) * val;
			}
			*output = accumulator;
			finished = true;
			break;
		}
	}
	
	// if we have read more than enough bytes for an unsigned int,
	// something is wrong with our input
	if (!finished)
		throw std::invalid_argument("decode: overran memory boundary");
	return size + 1;
}