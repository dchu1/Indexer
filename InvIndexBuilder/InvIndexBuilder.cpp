// InvIndexBuilder.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>
#include <string>
#include <array>
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
unsigned int write_to_index(std::ofstream& os, std::array<unsigned int, 2>& metadata, const std::vector<unsigned int>& block_docids, std::vector<unsigned int> block_frequencies)
{
    // encode our data
    auto docids_encoded = Util::encode(block_docids.data(), block_docids.size());
    auto frequencies_encoded = Util::encode(block_frequencies.data(), block_frequencies.size());
    
    // update the metadata with the correct number of bytes
    metadata[1] = docids_encoded.size() + frequencies_encoded.size();
    auto metadata_encoded = Util::encode(&metadata[0], 2);

    os.write((char*)metadata_encoded.data(), metadata_encoded.size());
    os.write((char*)docids_encoded.data(), docids_encoded.size());
    os.write((char*)frequencies_encoded.data(), frequencies_encoded.size());
    return metadata_encoded.size() + docids_encoded.size() + frequencies_encoded.size();
}

unsigned int write_to_lexicon(std::ofstream& os, std::string& word, unsigned int position)
{
    size_t size = word.size();
    std::vector<unsigned char> v = Util::encode(&position, 1);
    os.write((char*)v.data(), v.size());
    v = Util::encode(&size, 1);
    os.write((char*)v.data(), v.size());
    os.write(word.c_str(), size);
    return 0;
}
// .\InvIndexBuilder.exe inputfile outputpath blocksize
int main(int argc, char* argv[])
{
    // this will record the position in the file that the word's inverted index
    // starts at. Not sure if unsigned int will be large enough...
    unsigned int curr_position = 0;
    std::string outpath = std::string(argv[2]);

    // this will hold data for the current block we are building
    unsigned int blocksize = std::stoi(argv[3]);
    std::vector<unsigned int> block_docids;
    block_docids.reserve(blocksize);
    std::vector<unsigned int> block_frequencies;
    block_frequencies.reserve(blocksize);
    std::array<unsigned int, 2> metadata;
    Util::Posting_str ps;
    unsigned int prev_docid = 0;

    std::string line, current_word;
    std::ifstream is(argv[1], std::ios::in);
    std::ofstream index_os(outpath + "\\index.bin", std::ios::out | std::ios::binary);
    std::ofstream lexicon_os(outpath + "\\lexicon.bin", std::ios::out | std::ios::binary);
    if (is) {
        // start getting postings
        nextPostingStr(is, ps);
        current_word = ps.term;
        prev_docid = ps.docid;
        block_docids.push_back(ps.docid);
        block_frequencies.push_back(ps.frequency);
        while (nextPostingStr(is, ps))
        {
            // if we are still in the same word and have space in the block, just add the data
            // to our block
            if (ps.term.compare(current_word) == 0 && block_docids.size() < block_docids.capacity())
            {
                // store the difference in docids
                block_docids.push_back(ps.docid - prev_docid - 1);
                block_frequencies.push_back(ps.frequency);
                prev_docid = ps.docid;
            }
            // if not equal, we are finished with current word so flush to file
            // and start a new block and new word
            //if we have reached max block size, flush to file and start a new
            // block
            else
            {
                // flush to file and start a new block
                metadata[0] = prev_docid;
                auto num_bytes = write_to_index(index_os, metadata, block_docids, block_frequencies);
                prev_docid = ps.docid;
                block_docids.push_back(ps.docid);
                block_frequencies.push_back(ps.frequency);

                // if a different word, we also need to update our lexicon
                if (ps.term.compare(current_word) != 0)
                {
                    // write our lexicon entry out to file
                    write_to_lexicon(lexicon_os, current_word, curr_position);
                    // update our position by # of bytes written
                    curr_position += num_bytes;
                    current_word = ps.term;
                }
            }
        }
    }
    else {
        return 1;
    }

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
