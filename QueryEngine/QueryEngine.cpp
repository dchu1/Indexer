// QueryEngine.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "QueryEngine.h"
#include "Util.h"
#include <array>

void Query::DefaultQEngine::loadLexicon(const std::string& lexiconFilePath)
{
	std::ifstream lexicon(lexiconFilePath, std::ios::in | std::ios::binary);
	// check if open
	if (lexicon)
	{
		// read our lexicon data into the lexicon map
		Util::lexicon::LexiconEntry le;
		std::string prev_term = "";
		while (lexicon)
		{
			// Read an entry from our lexicon and store the metadata
			Util::lexicon::read_lexicon(lexicon, le);
			_lexicon_map[le.term] = le.metadata;
			
			// update the previous term's metadata with the current term's position.
			// note that we are creating a dummy entry for "" for the first term.
			_lexicon_map[prev_term].next_lexicon_entry_pos = le.metadata.position;
			prev_term = le.term;
		}
	}
	else
		throw "Unable to open lexicon";
}

void Query::DefaultQEngine::loadUrls(const std::string& urlTableFilePath)
{
	std::ifstream urltable(urlTableFilePath, std::ios::in | std::ios::binary);
	// check if open
	if (urltable)
	{
		Util::urltable::UrlTableEntry ute;
		while (urltable)
		{
			Util::urltable::get_url(urltable, ute);
			_url_table.push_back(ute);
		}
	}
	else
		throw "Unable to open url table";
}

Query::ListPointer Query::DefaultQEngine::openList(const std::string& term)
{
	// check to make sure the term exists in our lexicon
	if (_lexicon_map.find(term) != _lexicon_map.end())
	{
		Query::ListPointer lp;
		// open our filestream to index file
		lp.is.open(_indexFilePath, std::ios::in | std::ios::binary);
		if (lp.is)
		{
			// initialize our lp
			lp.beg_pos = _lexicon_map[term].position;
			lp.end_pos = _lexicon_map[term].next_lexicon_entry_pos;
			lp.compressed = false;
			lp.freq = _lexicon_map[term].docid_count;
			lp.curr_docid = 0;
			lp.curr_chunk_idx = 0;
			lp.curr_max_docid = 0;
			lp.curr_chunk.reserve(128);

			// seek to correct position in file
			lp.is.seekg(lp.beg_pos, lp.is.beg);

			// read in the first int which is the # of blocks
			lp.is.read((char*)&lp.num_chunks, sizeof(unsigned int));
			//lp.is >> lp.num_chunks;

			// read in the metadata and store:
			// map[lastdocid] = (size, relative position)
			std::array<unsigned int,2> pos;
			// accumulates the relative position from start of file by adding size
			unsigned int accumulator = (lp.num_chunks * 2 + 1) * sizeof(unsigned int);;
			for (int i = 0; i < lp.num_chunks; i++)
			{
				lp.is.read((char*)&pos[0], sizeof(unsigned int)*2);
				lp.chunk_positions[pos[0]] = PositionEntry{ pos[1], accumulator };
				accumulator += pos[1];
			}
			return lp;
		}
		else
		{
			throw "Unable to open index";
		}
	}
	else
	{
		throw "Term not found in lexicon";
	}
}

void Query::DefaultQEngine::closeList(Query::ListPointer& lp)
{
	// TODO
}

unsigned int Query::DefaultQEngine::nextGEQ(Query::ListPointer& lp, unsigned int k)
{
	// check which chunk's last docid is the closest to target docid
	// if we get an end iterator, it means no chunk has a docid >= to k,
	// so just return infinity
	// map.upperbound is O(logn). It finds the closest value > what you pass
	auto it = lp.chunk_positions.upper_bound(k-1);
	if (it == lp.chunk_positions.end())
		std::numeric_limits<unsigned int>::infinity();

	// check whether we are in the correct chunk. If not, we need to seek
	// and read in and decompress chunk
	else if (it->first != lp.curr_max_docid)
	{
		lp.is.seekg(it->second.relative_position + lp.beg_pos);
		std::vector<char> temp_char_vec;
		_compressor->decode(lp.is, lp.curr_chunk, it->second.size);
		lp.curr_docid = lp.curr_chunk[0]; // set curr posting to the first docid in chunk
	}

	// finally scan through the chunk until we find a docid >= k
	// if lp.curr_docid > k, we will start from beginning of chunk (since can't calculate docid reverse)
	for (auto it = lp.curr_chunk.begin()+1; it != lp.curr_chunk.end(); it++)
	{
		lp.curr_docid += *it;
		if (*it >= k)
			break;
	}
	return lp.curr_docid;
}

unsigned int Query::DefaultQEngine::getFreq(ListPointer& lp)
{
	// if there is a cached chunk, return the current posting's frequency,
	// whose index is the current posting + 1/2 the size
	if (lp.curr_docid == 0)
		return lp.curr_chunk[lp.curr_chunk_idx + lp.curr_chunk.size() / 2];
	else
		throw "No current posting";
}

int main()
{
	Util::compression::CompressorCreator* cc = new Util::compression::VarbyteCreator();
	Query::DefaultQEngine qe("D:\\Users\\admin\\Documents\\NYU\\Fall_2020\\Search_Engines\\HW\\HW2\\debug\\index.bin", cc);
	qe.loadLexicon("D:\\Users\\admin\\Documents\\NYU\\Fall_2020\\Search_Engines\\HW\\HW2\\debug\\lexicon.bin");
	auto lp = qe.openList("&");
	qe.nextGEQ(lp, 10);
	std::cout << qe.getFreq(lp) << std::endl;
	delete cc;
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
