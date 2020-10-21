#include "pch.h"
#include <vector>
#include "Util.h"
#include <fstream>

TEST(UtilTestSuite, EncodeDecode) {
	unsigned int temp[3] = { 0,50000,500000 };
	auto vec = Util::encode(temp, 3);
	std::vector<unsigned int> dec = Util::decode(vec.data(), 3);
	ASSERT_EQ(dec[0], temp[0]);
	ASSERT_EQ(dec[1], temp[1]);
	ASSERT_EQ(dec[2], temp[2]);
}

TEST(UtilTestSuite, EncodeDecodeStream) {
	// write out the file
	std::string filepath = "D:\\test.bin";
	std::ofstream os(filepath, std::ios::out | std::ios::binary);
	unsigned int temp[3] = { 0,128,500000 };
	auto v = Util::encode(temp, 3);
	std::ostream_iterator<unsigned char> out_it(os);
	std::copy(v.begin(), v.end(), out_it);
	os.flush();

	// read the file
	std::ifstream ifs(filepath, std::ios::in | std::ios::binary);
	std::istream& is= ifs;
	std::vector<unsigned int> dec;
	Util::decode(is, dec, 3);

	// assert equality
	ASSERT_EQ(dec[0], temp[0]);
	ASSERT_EQ(dec[1], temp[1]);
	ASSERT_EQ(dec[2], temp[2]);
}