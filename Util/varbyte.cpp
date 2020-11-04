#include "compressor.h"

std::vector<unsigned char> Util::compression::varbyte::encode(const unsigned int* num, const int size) const
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

size_t Util::compression::varbyte::encode(const unsigned int* num, std::vector<unsigned char>& out_vec, const int size) const
{
	unsigned char highest_bit_one = 128;
	size_t written_size = 0;
	for (int it = 0; it < size; it++) {
		unsigned char multiplicand;
		int base, i;
		int remaining_value;
		remaining_value = *(num + it);
		i = (int)(log(remaining_value) / log(128)); // largest power of 128
		while (remaining_value >= 128)
		{
			++written_size;
			base = pow(128, i);
			multiplicand = remaining_value / base;
			out_vec.push_back(multiplicand | highest_bit_one);
			remaining_value = remaining_value % base;
			i--;
			//print_byte_as_bits(temp);
		}
		++written_size;
		out_vec.push_back((unsigned char)remaining_value);
		//print_byte_as_bits(temp);
	}
	return written_size;
}

size_t Util::compression::varbyte::encode(const unsigned int* num, unsigned char*& out_p, const int size) const
{
	unsigned char highest_bit_one = 128;
	size_t written_size = 0;
	for (int it = 0; it < size; it++) {
		unsigned char multiplicand, temp;
		int base, i;
		int remaining_value;
		remaining_value = *(num + it);
		i = (int)(log(remaining_value) / log(128)); // largest power of 128
		while (remaining_value >= 128)
		{
			++written_size;
			base = pow(128, i);
			multiplicand = remaining_value / base;
			temp = multiplicand | highest_bit_one;
			*out_p = temp;
			++out_p;
			remaining_value = remaining_value % base;
			i--;
			//print_byte_as_bits(temp);
		}
		++written_size;
		temp = (unsigned char)remaining_value;
		*out_p = temp;
		++out_p;
		//print_byte_as_bits(temp);
	}
	return written_size;
}

std::vector<unsigned int> Util::compression::varbyte::decode(const unsigned char* arr, int numints) const
{
	int start = 0, end = 0, accumulator = 0, size = 1, counter = 0, val;
	std::vector<unsigned int> result;
	result.reserve(numints);
	while (counter < numints)
	{
		// if we have read more than enough bytes for an unsigned int,
		// something is wrong with our input
		size = end - start + 1;
		if (size > sizeof(unsigned int))
		{
			throw std::invalid_argument("decode: overran memory boundary");
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
std::ifstream& Util::compression::varbyte::decode(std::ifstream& is, std::vector<unsigned int>& vec, int numints) const
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