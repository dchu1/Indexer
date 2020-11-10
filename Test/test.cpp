#include "pch.h"
#include <vector>
#include "Util.h"
#include "compressor.h"
#include <fstream>
#include <array>

TEST(UtilTestSuite, VarbyteEncodeDecode) {
	unsigned int temp[3] = { 0,50000,3000000 };
	Util::compression::compressor* cc = new Util::compression::varbyte();
	std::vector<unsigned char> enc;
	std::vector<unsigned int> dec;
	cc->encode(&temp[0], enc, 3);
	cc->decode(enc.data(), dec, 3);
	ASSERT_EQ(dec[0], temp[0]);
	ASSERT_EQ(dec[1], temp[1]);
	ASSERT_EQ(dec[2], temp[2]);
}

TEST(UtilTestSuite, VarbyteEncodeDecodeStream) {
	// write out the file
	std::string filepath = "D:\\test.bin";
	std::ofstream os(filepath, std::ios::out | std::ios::binary);
	unsigned int temp[3] = { 0,128,3000000 };
	std::vector<unsigned char> enc;
	std::vector<unsigned int> dec;
	Util::compression::compressor* cc = new Util::compression::varbyte();
	cc->encode(&temp[0], enc, 3);
	std::ostream_iterator<unsigned char> out_it(os);
	std::copy(enc.begin(), enc.end(), out_it);
	os.flush();

	// read the file
	std::ifstream ifs(filepath, std::ios::in | std::ios::binary);
	//std::istream& is= ifs;
	cc->decode(ifs, dec, 3);

	// assert equality
	ASSERT_EQ(dec[0], temp[0]);
	ASSERT_EQ(dec[1], temp[1]);
	ASSERT_EQ(dec[2], temp[2]);
}