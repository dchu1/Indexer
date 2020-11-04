#include "pch.h"
#include <vector>
#include "Util.h"
#include <fstream>
#include <array>

TEST(UtilTestSuite, EncodeDecode) {
	unsigned int temp[3] = { 0,50000,500000 };
	auto vec = Util::encode(temp, 3);
	std::vector<unsigned int> dec = Util::decode(vec.data(), 3);
	ASSERT_EQ(dec[0], temp[0]);
	ASSERT_EQ(dec[1], temp[1]);
	ASSERT_EQ(dec[2], temp[2]);
}

TEST(UtilTestSuite, EncodeDecodePointer) {
	Util::compression::compressor* comp;
	comp = new Util::compression::varbyte;
	std::vector<unsigned int> temp = { 0,128,500000 };
	std::vector<unsigned char> output;
	output.reserve(3 * sizeof(unsigned int));
	unsigned char* out_p = output.data();

	comp->encode(temp.data(), out_p, 3);
	auto dec = comp->decode(output.data(), 3);
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
	//std::istream& is= ifs;
	std::vector<unsigned int> dec;
	Util::decode(ifs, dec, 3);

	// assert equality
	ASSERT_EQ(dec[0], temp[0]);
	ASSERT_EQ(dec[1], temp[1]);
	ASSERT_EQ(dec[2], temp[2]);
}

TEST(InvIndexBuilderTestSuite, ReadLexicon) {
	//std::string filepath_lexicon("D:\\Users\\admin\\Documents\\NYU\\Fall_2020\\Search_Engines\\HW\\HW2\\temp\\m1\\m2\\lexicon.bin");
	//std::string filepath_index("D:\\Users\\admin\\Documents\\NYU\\Fall_2020\\Search_Engines\\HW\\HW2\\temp\\m1\\m2\\index.bin");
	//std::ifstream is_lex(filepath_lexicon, std::ios::in | std::ios::binary);
	//std::ifstream is_ind(filepath_index, std::ios::in | std::ios::binary);
	//Util::lexicon::LexiconEntry le = {};

	///* Data from our parser (term, docid, freq)
	//	##,5455,1
	//	##,16558,1
	//	##,21473,25
	//	##,43413,1
	//	##,44863,1
	//	##,48893,34
	//	##,49388,4
	//	##,50306,1
	//	##,51190,2
	//	##,52346,1
	//*/
	//if (is_lex)
	//{
	//	std::array<unsigned int, 2> metadata;
	//	std::vector<unsigned int> vec;
	//	ASSERT_TRUE(Util::lexicon::read_lexicon(is_lex, le));
	//	Util::lexicon::LexiconEntry le_first(le);
	//	ASSERT_TRUE(Util::lexicon::read_lexicon(is_lex, le)); // get the second entry

	//	// seek forward
	//	is_ind.seekg(le.metadata.position, is_ind.beg);
	//	ASSERT_TRUE(is_ind.read((char*)&metadata, 2 * sizeof(unsigned int)));

	//	ASSERT_TRUE(Util::decode(is_ind, vec, 10));
	//	ASSERT_EQ(vec[0], 5455);
		//ASSERT_EQ(vec[1] + 5455 + 1, 16558);
		//ASSERT_EQ(vec[2] + 16558 + 1, 21473);
		//ASSERT_EQ(vec[3] + 21473 + 1, 43413);
		//ASSERT_EQ(vec[4] + 43413 + 1, 44863);
	//}
	
}