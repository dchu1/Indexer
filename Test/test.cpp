#include "pch.h"
#include <vector>
#include "Util.h"

TEST(UtilTestSuite, EncodeDecode) {
	auto temp = std::vector<unsigned int>{ 0,50000,500000 };
	std::vector<unsigned char> v1 = Util::encode(0);
	std::vector<unsigned char> v2 = Util::encode(50000);
	std::vector<unsigned char> v3 = Util::encode(500000);
	v1.insert(end(v1), begin(v2), end(v2));
	v1.insert(end(v1), begin(v3), end(v3));
	std::vector<unsigned int> dec = Util::decode(v1.data(), 3);
	ASSERT_EQ(dec[0], temp[0]);
	ASSERT_EQ(dec[1], temp[1]);
	ASSERT_EQ(dec[2], temp[2]);
}