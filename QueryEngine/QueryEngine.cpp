// QueryEngine.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "QueryEngine.h"
#include "Util.h"
#include <array>
#include <queue>
#include <iterator>
#include <sstream>
#include <algorithm>
#include "PageStorage.h"

void Query::DefaultQEngine::loadLexicon(const std::string& lexiconFilePath)
{
	std::ifstream lexicon(lexiconFilePath, std::ios::in | std::ios::binary);
	// check if open
	if (lexicon)
	{
		try
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
				if (le.term.compare("48") == 0)
					std::cout << "48" << std::endl;
			}
		}
		catch (std::exception& e)
		{
			std::cout << e.what() << " Last read term: " + (*_lexicon_map.rbegin()).first << std::endl;
			throw e;
		}
	}
	else
		throw "Unable to open lexicon";
}

unsigned int Query::DefaultQEngine::loadUrls(const std::string& urlTableFilePath)
{
	//std::ifstream urltable(urlTableFilePath, std::ios::in | std::ios::binary);
	//// check if open
	//if (urltable)
	//{
	//	unsigned int length_accumulator = 0;
	//	Util::urltable::UrlTableEntry ute;
	//	while (urltable)
	//	{
	//		Util::urltable::get_url(urltable, ute);
	//		length_accumulator += ute.size;
	//		_url_table.push_back(ute);
	//	}
	//	_scorer->_N = _url_table.size();
	//	_scorer->_davg = length_accumulator / _url_table.size();
	//}
	//else
	//	throw "Unable to open url table";

	//std::ifstream url_is(urlTableFilePath, std::ios::in);
	//// check if open
	//std::string line;
	//Util::urltable::UrlTableEntry ute;
	//unsigned int length_accumulator = 0;
	//if (url_is)
	//{
	//	while (getline(url_is, line, ','))
	//	{
	//		ute.url = line;
	//		getline(url_is, line);
	//		ute.size = stoi(line);
	//		length_accumulator += ute.size;
	//		_url_table.push_back(ute);
	//	}
	//	return length_accumulator / _url_table.size();
	//}
	//else
	//	throw "Unable to open url table";
	return 0;
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
			lp.num_docs = _lexicon_map[term].docid_count;
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
		return std::numeric_limits<unsigned int>::infinity();

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
	unsigned int accumulator = 0;
	for (auto it = lp.curr_chunk.begin(); it != lp.curr_chunk.end(); it++)
	{
		accumulator += *it;
		if (accumulator >= k)
			break;
		
	}
	lp.curr_docid = accumulator;
	return lp.curr_docid;
}

unsigned int Query::DefaultQEngine::getFreq(ListPointer& lp)
{
	// if there is a cached chunk, return the current posting's frequency,
	// whose index is the current posting + 1/2 the size
	if (lp.curr_docid > 0)
		return lp.curr_chunk[lp.curr_chunk_idx + lp.curr_chunk.size() / 2];
	else
		throw "No current posting";
}

std::stack<Page> Query::DefaultQEngine::getTopKConjunctive(std::vector<std::string> query, int k)
{
	std::vector<Query::ListPointer> lp_vec;
	std::priority_queue<
		std::pair<double, Page>,
		std::vector<std::pair<double, Page>>,
		std::greater<std::pair<double, Page>>> topk_pq;	// priority queue uses pair <score, docid>
	unsigned int d = 0, max_docid, min_idx = 0, curr_docid;
	double min_score, score;
	std::stack<Page> page_stack;

	// load list pointers
	for (int i = 0; i < query.size(); i++)
	{
		// create a list pointer for each term
		lp_vec.push_back(openList(query[i]));
		// update the min_idx if necessary (we want to know the shortest list)
		if (lp_vec.back().num_docs < lp_vec[min_idx].num_docs)
			min_idx = i;
	}
	// update max_docid with the largest docid in the smallest inverted list
	max_docid = lp_vec[min_idx].chunk_positions.rbegin()->first;

	curr_docid = nextGEQ(lp_vec[min_idx], 1);
	while (curr_docid <= max_docid)
	{
		// iterate through each list pointer until we get a docid that is not equal to our current docid
		for (int i = 0; i < lp_vec.size(); i++)
		{
			if (i != min_idx)
			{
				d = nextGEQ(lp_vec[i], curr_docid);
				if (d != curr_docid)
					break;
			}
		}
		
		// if d > curr_docid, one of our lists does not contain curr_docid, so get next docid in short list
		if (d > curr_docid)
			curr_docid = nextGEQ(lp_vec[min_idx], d);
		else // otherwise we did find a common docid, so calculate score
		{
			Page p = _ps->getDocument(curr_docid);
			std::vector<double> args;
			args.reserve(lp_vec.size() * 2 + 1);
			// push document length
			args.push_back(p.size);
			// push the rest of the args
			for (auto& lp : lp_vec)
			{
				args.push_back(static_cast<double>(lp.num_docs));
				args.push_back(static_cast<double>(getFreq(lp)));
			}
			// get the score
			score = _scorer->score(args);
			// if queue size < k, push onto queue
			if (topk_pq.size() < k)
				topk_pq.emplace(std::pair<double, Page> {score, p});
			// otherwise we need to check whether this score is higher than the smallest score in queue
			else if (score > topk_pq.top().first)
			{
				topk_pq.pop();
				topk_pq.emplace(std::pair<double, Page> {score, p});
			}
			++curr_docid;
		}
	}

	// close list pointers
	for (auto& lp : lp_vec)
		closeList(lp);

	// reverse pq and return docids
	while (!topk_pq.empty())
	{
		page_stack.push(topk_pq.top().second);
		topk_pq.pop();
	}
	return page_stack;
}

std::stack<Page> Query::DefaultQEngine::getTopKDisjunctive(std::vector<std::string> query, int k)
{
	std::vector<Query::ListPointer> lp_vec;
	std::priority_queue<
		std::pair<double, Page>,
		std::vector<std::pair<double, Page>>,
		std::greater<std::pair<double, Page>>> topk_pq;	// priority queue uses pair <score, docid>
	std::priority_queue<
		unsigned int,
		std::vector<unsigned int>,
		std::greater<unsigned int>> docid_pq;	// priority queue

	unsigned int d, min_docid, min_idx = 0, curr_docid = 0;
	double min_score, score;
	std::stack<Page> page_stack;

	// load list pointers
	// update docid_pq with the smallest docid in each inverted list
	for (int i = 0; i < query.size(); i++)
	{
		// create a list pointer for each term
		lp_vec.push_back(openList(query[i]));
		// update the min_idx if necessary (we want to know the shortest list)
		unsigned int docid = nextGEQ(lp_vec.back(), 0);
		docid_pq.push(docid);
	}

	// iterate through our docid_pq, getting the smallest docid and calculate the scores
	while (!docid_pq.empty())
	{
		// get the min docid in our priority queue
		curr_docid = docid_pq.top();
		docid_pq.pop();
		Page p = _ps->getDocument(curr_docid);
		std::vector<double> args;
		// push document length
		args.push_back(p.size);

		// iterate through each list pointer. construct our args from each lp that has our curr_docid
		for (auto& lp : lp_vec)
		{
			// if the lp's curr docid == min docid, get the frequency, then advance the pointer
			if (lp.curr_docid == curr_docid)
			{
				args.push_back(static_cast<double>(lp.num_docs));
				args.push_back(static_cast<double>(getFreq(lp)));
				unsigned int docid = nextGEQ(lp, curr_docid+1);
				if (docid < std::numeric_limits<unsigned int>::max())
					docid_pq.push(docid);
			}
		}

		// score curr_docid
		score = _scorer->score(args);
		// if queue size < k, push onto queue
		if (topk_pq.size() < k)
			topk_pq.emplace(std::pair<double, Page> {score, p});
		// otherwise we need to check whether this score is higher than the smallest score in queue
		else if (score > topk_pq.top().first)
		{
			topk_pq.pop();
			topk_pq.emplace(std::pair<double, Page> {score, p});
		}
	}

	// close list pointers
	for (auto& lp : lp_vec)
		closeList(lp);

	// reverse pq and return docids
	while (!topk_pq.empty())
	{
		page_stack.push(topk_pq.top().second);
		topk_pq.pop();
	}
	return page_stack;
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
