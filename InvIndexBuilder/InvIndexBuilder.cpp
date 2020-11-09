// InvIndexBuilder.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>
#include <string>
#include <array>
#include <chrono>
#include <algorithm>
#include "Util.h"

// taken from Mergesort. Might want to make this a util function
std::ifstream& nextPostingStr(std::ifstream& is, Util::Posting_str& posting)
{
    try {
        int counter = 0;
        // read in docid, freq, # of bytes of string
        std::vector<unsigned int>vec;
        vec.reserve(3);
        if (Util::decode(is, vec, 3))
        {
            posting.docid = vec[0];
            posting.frequency = vec[1];

            // read in string
            char* str = new char[vec[2] + 1];
            is.read(str, vec[2]);
            str[vec[2]] = '\0';
            posting.term = str;
            delete[] str;
        }
    }
    catch (std::exception& e)
    {
        // for now i'm just going to print some exception happened
        //cout << "Exception occurred: " << e.what() << endl;
        throw e;
    }
    return is;
}

// returns # of bytes written
// write in form (unencoded docid, unencoded length of chunk, encoded docids, encoded frequencies)
//unsigned int write_to_index(std::ofstream& os, const std::vector<unsigned int>& metadata, const std::vector<std::vector<unsigned char>>& chunks)
//{
//    // encode our data
//    auto docids_encoded = Util::encode(block_docids.data(), block_docids.size());
//    auto frequencies_encoded = Util::encode(block_frequencies.data(), block_frequencies.size());
//    
//    // update the metadata with the correct number of bytes
//    metadata[1] = docids_encoded.size() + frequencies_encoded.size();
//
//    os.write((char*)&metadata, sizeof(unsigned int)*2);
//    os.write((char*)docids_encoded.data(), docids_encoded.size());
//    os.write((char*)frequencies_encoded.data(), frequencies_encoded.size());
//    return sizeof(unsigned int) * 2 + docids_encoded.size() + frequencies_encoded.size();
//}

// write out in encoded form (position, count, size of term in bytes, term)
unsigned int write_to_lexicon(std::ofstream& os, std::string& word, unsigned int position, unsigned int counter)
{
    size_t size = word.size();
    std::vector<unsigned char> v = Util::encode(&position, 1);
    os.write((char*)v.data(), v.size());
    v = Util::encode(&counter, 1);
    os.write((char*)v.data(), v.size());
    v = Util::encode(&size, 1);
    os.write((char*)v.data(), v.size());
    os.write(word.c_str(), size);
    return 0;
}
// .\InvIndexBuilder.exe inputfile outputpath chunksizes encoding
int main(int argc, char* argv[])
{
    // this will record the position in the file that the word's inverted index
    // starts at. Only works as long as index is below 4 Gb...
    unsigned int start_position = 0;
    unsigned int curr_position = 0;
    std::string outpath = std::string(argv[2]);

    Util::compression::compressor* compress;
    compress = new Util::compression::varbyte;

    // data structures to hold temporary data
    unsigned int blocksize = std::stoi(argv[3]);
    std::vector<unsigned int> block_docids;
    block_docids.reserve(blocksize);
    std::vector<unsigned int> block_frequencies;
    block_frequencies.reserve(blocksize);

    // metadata will hold a tuple<last docid, #bytes in block>
    std::vector<unsigned int> metadata;
    Util::Posting_str ps;
    unsigned int prev_docid = 0;
    unsigned int doc_counter;

    std::string line, current_word;
    std::ifstream is(argv[1], std::ios::in | std::ios::binary);
    std::ofstream index_os(outpath + "\\index.bin", std::ios::out | std::ios::binary);
    std::ofstream lexicon_os(outpath + "\\lexicon.bin", std::ios::out | std::ios::binary);

    auto t1 = std::chrono::steady_clock::now();
    if (is) {
        // start getting postings
        // the first posting is treated differently since we
        // need to initialize current_word
        nextPostingStr(is, ps);
        current_word = ps.term;
        block_docids.push_back(ps.docid);
        block_frequencies.push_back(ps.frequency);
        doc_counter = 1;
        try {
            while (nextPostingStr(is, ps))
            {
                // if we are still in the same word and have space in the block, just add the data
                // to our block
                if (ps.term.compare(current_word) == 0)
                {
                    ++doc_counter;
                    // if we are starting a new chunk, we will use the docid as is,
                    // otherwise take the difference
                    block_docids.push_back(ps.docid);
                    block_frequencies.push_back(ps.frequency);
                }
                // if we have read a new word, we need to flush current word to file
                else
                {
                    // reserve data in our metadata array (need to store 2 ints per chunk + 1 int for num_chunks)
                    size_t num_chunks = doc_counter / blocksize + (doc_counter % blocksize != 0);
                    metadata.reserve(num_chunks * 2 + 1);
                    metadata.push_back(num_chunks);
                    std::vector<unsigned char> encoded_data;
                    encoded_data.reserve(doc_counter*2*sizeof(unsigned int));

                    // our chunk size is the minimum of block size or # of docids
                    size_t size = std::min(blocksize, block_docids.size());
                    size_t written_size = 0;

                    try
                    {
                        // build and encode chunks
                        for (int p = 0; p < block_docids.size(); p += size)
                        {
                            // update the size
                            size = std::min(blocksize, block_docids.size() - p);

                            // turn our docids into docid differences
                            int prev_term = block_docids[p];
                            int temp;
                            for (int i = 1; i < size; i++)
                            {
                                temp = block_docids[p + i];
                                block_docids[p + i] -= prev_term;
                                prev_term = temp;
                            }

                            // insert data into our chunk
                            written_size += compress->encode(block_docids.data() + p, encoded_data, size);
                            written_size += compress->encode(block_frequencies.data() + p, encoded_data, size);

                            // insert metadata last docid,# bytes
                            metadata.push_back(prev_term);
                            metadata.push_back(written_size);

                            curr_position += written_size;
                            written_size = 0;
                        }
                    }
                    catch (std::exception& e) 
                    {
                        std::string s(e.what());
                        s += " for term " + current_word;
                        throw s;
                    }
                    // flush to file
                    // write metadata
                    index_os.write((char*)metadata.data(), sizeof(unsigned int) * metadata.size());
                    curr_position += sizeof(unsigned int) * metadata.size();
                    // write encoded data
                    index_os.write((char*)encoded_data.data(), encoded_data.size());

                    // write our lexicon entry out to file
                    write_to_lexicon(lexicon_os, current_word, start_position, doc_counter);
                    start_position = curr_position;
                    current_word = ps.term;
                    doc_counter = 1;

                    // clear our data and add new data
                    metadata.clear();
                    block_docids.clear();
                    block_frequencies.clear();
                    block_docids.push_back(ps.docid);
                    block_frequencies.push_back(ps.frequency);
                }
            }
        }
        catch (std::exception e)
        {
            std::cout << e.what() << std::endl;
            throw e;
        }
    }
    else {
        return 1;
    }
    std::cout << "Total time: " << std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - t1).count() << " seconds." << std::endl;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
