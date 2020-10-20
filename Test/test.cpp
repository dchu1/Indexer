#include "pch.h"
#include <vector>
#include "Util.h"

TEST(UtilTestSuite, EncodeDecode) {
	unsigned int temp[3] = { 0,50000,500000 };
	auto vec = Util::encode(temp, 3);
	std::vector<unsigned int> dec = Util::decode(vec.data(), 3);
	ASSERT_EQ(dec[0], temp[0]);
	ASSERT_EQ(dec[1], temp[1]);
	ASSERT_EQ(dec[2], temp[2]);
}