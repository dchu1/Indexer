// Util.cpp : Defines the functions for the static library.

#include "Util.h"
#include <vector>

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
		while (counter < numints)
		{
			// check if the highest order bit is 0
			if ((arr[end] >> 7) ^ 1)
			{
				size = end - start + 1;
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

	//std::vector<unsigned int> decode(const std::vector<char>& vect)
	//{
	//	int start = 0, end = 0, accumulator = 0, size = 1, val;
	//	std::vector<unsigned int> result;
	//	for (const auto& byte : vect)
	//	{
	//		// check if the highest order bit is 0
	//		if ((byte >> 7) ^ 1)
	//		{
	//			size = end - start + 1;
	//			for (int i = size - 1; i >= 0; i--)
	//			{
	//				// clear the highest bit
	//				val = vect[start] & ~(1UL << 7);
	//				accumulator = accumulator + pow(128, i) * val;
	//				start++;
	//			}
	//			result.push_back(accumulator);
	//			start = end + 1;
	//			accumulator = 0;
	//		}
	//		end++;
	//	}
	//	return result;
	//}
}
