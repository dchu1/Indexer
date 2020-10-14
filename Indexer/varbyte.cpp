#include <iostream>
#include <bitset>
#include <cmath>
#include <vector>
#include "varbyte.h"
using namespace std;
using namespace varbyte;

void varbyte::print_byte_as_bits(unsigned char val) {
	for (int i = 7; 0 <= i; i--) {
		cout << ((val & (1 << i)) ? '1' : '0');
	}
	cout << "\n";
}

//vector<unsigned char> varbyte::encode(const vector<unsigned int>& nums)
//{
//	unsigned char multiplicand, temp;
//	unsigned char highest_bit_one = 128;
//	int base, size, i;
//	int remaining_value;
//	vector<unsigned char> result;
//	for (const auto& num : nums)
//	{
//		remaining_value = num;
//		i = (int)(log(num) / log(128)); // largest power of 128
//		while (remaining_value >= 128)
//		{
//			base = pow(128, i);
//			multiplicand = num / base;
//			temp = multiplicand | highest_bit_one;
//			result.push_back(temp);
//			remaining_value = remaining_value % base;
//			i--;
//			print_byte_as_bits(temp);
//		}
//		temp = (unsigned char)num;
//		result.push_back(temp);
//		print_byte_as_bits(temp);
//	}
//	return result;
//}

vector<unsigned char> varbyte::encode(const unsigned int& num)
{
	unsigned char multiplicand, temp;
	unsigned char highest_bit_one = 128;
	int base, size, i;
	int remaining_value;
	vector<unsigned char> result;
	remaining_value = num;
	i = (int)(log(num) / log(128)); // largest power of 128
	while (remaining_value >= 128)
	{
		base = pow(128, i);
		multiplicand = num / base;
		temp = multiplicand | highest_bit_one;
		result.push_back(temp);
		remaining_value = remaining_value % base;
		i--;
		//print_byte_as_bits(temp);
	}
	temp = (unsigned char)num;
	result.push_back(temp);
	//print_byte_as_bits(temp);
	return result;
}

vector<unsigned int> varbyte::decode(const vector<char>& vect)
{
	int start = 0, end = 0, accumulator = 0, size = 1, val;
	vector<unsigned int> result;
	for (const auto& byte : vect)
	{
		// check if the highest order bit is 0
		if ((byte >> 7) ^ 1)
		{
			size = end - start + 1;
			for (int i = size-1; i >= 0; i--)
			{
				// clear the highest bit
				val = vect[start] & ~(1UL << 7);
				accumulator = accumulator + pow(128, i) * val;
				start++;
			}
			result.push_back(accumulator);
			start = end + 1;
			accumulator = 0;
		}
		end++;
	}
	return result;
}