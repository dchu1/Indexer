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
#include <iterator>
#include <boost/algorithm/string.hpp>
#include <stdexcept>
#include <set>
#include <functional>

/// <summary>
/// Generates a snippet
/// </summary>
/// <param name="p">page to generate snippet from</param>
/// <param name="query">query terms to use for scoring</param>
/// <returns>a string snippet</returns>
std::string Query::DefaultQEngine::generateSnippet(Page& p, std::vector<std::string>& query) const
{
	double score;
	// split into sentences (very simplistic version that just splits on new line characters)
	std::vector<std::string> sentences;
	std::vector<std::string> words;
	boost::split(sentences, p.body, boost::is_any_of("\r\n"));
	sentences.erase(std::remove_if(std::begin(sentences), std::end(sentences), [](std::string x) { return x.size() == 0; }), std::end(sentences));

	std::vector<std::pair<unsigned int, unsigned int>> scores;
	auto scorer = new ScorerBM25();
	scorer->_N = p.size;
	unsigned int accumulator = 0;

	// calculate avg sentence length
	for (auto& sent : sentences)
	{
		accumulator += sent.size();
	}
	scorer->_davg = accumulator / sentences.size();

	// calculate the metrics we are going to use to score using BM25. This means iterating over
	// sentences and calculating term frequencies
	// args: #sentences with term1, #term1 in sentence, #sentences with term2, # term2 in sentence...
	std::map<std::string, double> sentence_query_counts;
	std::map<std::string, double> query_sentence_counts;
	std::queue<double> args_queue;
	for (auto& q : query)
		query_sentence_counts[q] = 0;
	std::vector<double> args;
	args.reserve(1 + query.size()*2);

	for (auto& sent : sentences)
	{
		// initialize query counts
		for (auto& q : query)
			sentence_query_counts[q] = 0;

		// tokenize sentence. taken from boost tokenizer
		boost::split(words, sent, boost::is_any_of(" @#$%^&*<>`:.,?!()/\\+-_[]{}\"\'\r\n\t"));
		// get rid of empty strings and long strings (> 30 characters)
		words.erase(std::remove_if(std::begin(words), std::end(words), [](std::string x) { return x.size() == 0 || x.size() > 30; }), std::end(words));

		// iterate over each word to get counts of query terms
		for (auto& word : words)
		{
			auto it = sentence_query_counts.find(word);
			if (it != sentence_query_counts.end())
				sentence_query_counts[(*it).first]++;
		}

		// iterate over our query count to update our sentence count
		for (auto& counts : sentence_query_counts)
		{
			args_queue.push(counts.second);
			if (counts.second > 0)
				query_sentence_counts[counts.first]++;
		}
	}

	// score each sentence and put it in our score vector
	int i = 0;
	for (auto& sent : sentences)
	{
		args.push_back(sent.size());
		for (auto& counts : query_sentence_counts)
		{
			args.push_back(counts.second);
			args.push_back(args_queue.front());
			args_queue.pop();
				
		}
		score = scorer->score(args);
		scores.push_back(std::make_pair(score, i));
		args.clear();
		i++;
	}
	
	// return the top sentence and some surrounding sentences
	std::sort(scores.rbegin(), scores.rend());
	std::string ret_string = "";
	int ret_size = std::min(4, (int)sentences.size());
	int top_score_idx = scores[0].second;
	int front = std::min(ret_size / 2, top_score_idx);
	int back = std::min(ret_size / 2, (int)sentences.size() - top_score_idx - 1);

	// figure out how many sentences to take from in front and behind best sentence
	if (front + back != ret_size)
	{
		if (front > back)
			front += ret_size / 2 - back;
		else
			back += ret_size / 2 - front;
	}
	
	// construct our return string
	for (i = front; i > 0 ; i--)
	{
		ret_string += sentences[top_score_idx - i] + "\r\n";
	}
	for (i = 0; i <= back; i++)
	{
		ret_string += sentences[top_score_idx + i] + "\r\n";
	}
	return ret_string;
}

/// <summary>
/// Loads a lexicon from file into memory
/// </summary>
/// <param name="lexiconFilePath">File path to lexicon file</param>
/// <param name="compressor">compression object used to decompress file</param>
void Query::DefaultQEngine::loadLexicon(const std::string& lexiconFilePath, Util::compression::compressor* compressor)
{
	std::ifstream lexicon(lexiconFilePath, std::ios::in | std::ios::binary);
	// check if open
	if (lexicon)
	{
		try
		{
			// this bool controls whether we should write to previous entry's next_lexicon_entry_pos field
			// this is false for the first term, and also false if the previous term was an empty space.
			// this is a very inelegant solution 
			bool write_next_lexicon_entry_pos = false;
			Util::lexicon::LexiconEntry le;
			// Read an entry from our lexicon and store the metadata (including updating the previous
			// term's end position in the index.
			while (Util::lexicon::read_lexicon(lexicon, le, compressor))
			{
				// special case where the string is " ", which I just use to calculate
				// the end position. The rest of the data is not important, as it will be read from index file
				if (write_next_lexicon_entry_pos)
					_lexicon_values.back().next_lexicon_entry_pos = le.metadata.position;
				if (le.term.compare(" ") != 0)
				{
					_lexicon_terms.push_back(le.term);
					_lexicon_values.push_back(le.metadata);
					write_next_lexicon_entry_pos = true;
				}
				else
				{
					// if the entry is an empty space, we want to update the previous entries field,
					// but we don't want the next term to update the previous field (since we do not
					// create an entry for this term)
					write_next_lexicon_entry_pos = false;
				}
			}
			// set the final entries next_lexicon_entry_pos to end of file
			lexicon.clear();
			_lexicon_values.back().next_lexicon_entry_pos = lexicon.tellg();
		}
		catch (std::exception& e)
		{
			std::cout << e.what() << " Last read term: " + _lexicon_terms.back() << std::endl;
			throw e;
		}
	}
	else
		throw "Unable to open lexicon";
}

/// <summary>
/// Creates a ListPointer by reading either from the Lexicon in memory, or from the Index file if the term is not
/// in the in-memory lexicon.
/// </summary>
/// <param name="term">the word for the ListPointer</param>
/// <returns>A ListPointer object</returns>
Query::ListPointer Query::DefaultQEngine::openList(const std::string& term)
{
	// initialize our lp
	Query::ListPointer lp;
	lp.term = term;
	lp.curr_chunk_docid = 0;
	lp.curr_chunk_pos = 0;
	lp.curr_chunk_max_docid = 0;
	lp.curr_chunk.reserve(256);

	// search for term in our term vector and get associated index
	size_t index;
	bool found = false;
	{
		// find the index in our term vector that is before our desired term
		auto it = std::lower_bound(_lexicon_terms.begin(), _lexicon_terms.end(), term);
		// if we found a term...
		if (it != _lexicon_terms.end()) {
			index = std::distance(_lexicon_terms.begin(), it);
			// if the term we found is the one we are looking for, set found = true
			if (*it == term)
				found = true;
			else // if we didn't find the term, we decrement index by 1 since lower_bound finds the first index >= search term
				--index;
		}
		// I don't actually know under what situation this would occur as
		// it should at worst find the last element
		else
			throw term +" not found";
	}
	// if not found, we need to seek through the index file, starting from the ending position of
	// the previous term
	
	// open the index file
	std::ifstream ind_is;
	ind_is.open(_indexFilePath, std::ios::in | std::ios::binary);
	if (!ind_is)
		throw "Unable to open index";

	if (!found)
	{
		std::vector<unsigned int> temp_int_vec;
		std::vector<unsigned char> temp_str;
		unsigned char c;
		unsigned int counter = 0;

		// seek to correct position
		ind_is.seekg(_lexicon_values[index].next_lexicon_entry_pos, ind_is.beg);

		// start reading. If the first value is not 0, then we are in a term that is in our
		// lexicon, and so the term does not exist
		while (_compressor->decode(ind_is, temp_int_vec, 1) && temp_int_vec[0] == 0)
		{
			// if we are in this loop, we are reading a sparse lexicon. read in doc freq
			// and size of string and the string
			temp_int_vec.clear();
			if (_compressor->decode(ind_is, temp_int_vec, 2))
			{
				temp_str.reserve(temp_int_vec[1] + 1);
				while (counter++ < temp_int_vec[1] && ind_is.get((char&)c))
					temp_str.push_back(c);
				temp_str.push_back('\0');
				counter = 0;
				
				// check if this is the term we want. if so, set our bool and break
				if (term.compare((char*)temp_str.data()) == 0)
				{
					found = true;
					lp.beg_pos = ind_is.tellg();
					lp.num_docs = temp_int_vec[0];
					break;
				}
				// otherwise we need to skip to the next, which means reading in
				// the metadata and seeking forward
				else
				{
					unsigned int num_chunks;
					temp_int_vec.clear();
					// read in the first int which is the # of blocks
					_compressor->decode(ind_is, temp_int_vec, 1);
					num_chunks = temp_int_vec[0];
					temp_int_vec.clear();

					// read in the metadata (last docid,# bytes) to get how much
					// to skip:
					unsigned int accumulator = 0;
					for (int i = 0; i < num_chunks; i++)
					{
						_compressor->decode(ind_is, temp_int_vec, 2);
						accumulator += temp_int_vec[1];
						temp_int_vec.clear();
					}

					// skip forward:
					ind_is.seekg(accumulator, ind_is.cur);
				}
				temp_str.clear();
			}
		}	
	}
	// this is the case where we can read from the in memory lexicon, seek to the position in our lexicon
	else
	{
		lp.beg_pos = _lexicon_values[index].position;
		lp.end_pos = _lexicon_values[index].next_lexicon_entry_pos;
		lp.num_docs = _lexicon_values[index].docid_count;
		ind_is.seekg(_lexicon_values[index].position, ind_is.beg);
	}

	if (!found)
	{
		throw std::invalid_argument("term not found");
	}

	unsigned int num_chunks;
	// read in the first int which is the # of blocks
	_compressor->decode(ind_is, lp.curr_chunk, 1);
	num_chunks = lp.curr_chunk[0];
	lp.curr_chunk.pop_back();

	// read in the metadata (last docid,# bytes) and store:
	std::vector<unsigned int> pos;
	pos.reserve(2);
	unsigned int accumulator = 0;
	for (int i = 0; i < num_chunks; i++)
	{
		_compressor->decode(ind_is, pos, 2);
		lp.chunk_positions[pos[0]] = PositionEntry{ pos[1], accumulator };
		accumulator += pos[1];
		pos.clear();
	}

	// read in the chunks of data. probably a better way to do this using istream_iterator
	unsigned char c;
	unsigned int counter = 0;
	unsigned int inv_list_size = accumulator; // lp.end_pos - ind_is.tellg();
	lp.inv_list.reserve(inv_list_size);
	while (counter++ < inv_list_size && ind_is.get((char&)c))
		lp.inv_list.push_back(c);
	return lp;
}

void Query::DefaultQEngine::closeList(Query::ListPointer& lp)
{
	// TODO
}

/// <summary>
/// Gets the next document id greater than the one passed from the passed ListPointer
/// </summary>
/// <param name="lp">ListPointer to search</param>
/// <param name="k">document id to search for</param>
/// <returns>a document id greater than or equal to k. Returns numeric max if no document id found</returns>
unsigned int Query::DefaultQEngine::nextGEQ(Query::ListPointer& lp, unsigned int k)
{
	// check which chunk's last docid is the closest to target docid
	// if we get an end iterator, it means no chunk has a docid >= to k,
	// so just return max int size (which hopefully is not a valid docid)
	// map.upperbound is O(logn). It finds the closest value > what you pass
	auto it = lp.chunk_positions.lower_bound(k);
	if (it == lp.chunk_positions.end())
		return std::numeric_limits<unsigned int>::max();

	// check whether we are in the correct chunk. If not, we need to seek
	// and read in and decompress chunk
	if (it->first != lp.curr_chunk_max_docid)
	{
		// unencode data into current chunk
		lp.curr_chunk.clear();
		_compressor->decode_bytes(&lp.inv_list[it->second.relative_position], lp.curr_chunk, it->second.size);
		lp.curr_chunk_docid = lp.curr_chunk[0]; // set curr posting to the first docid in chunk
		lp.curr_chunk_pos = 0;
		lp.curr_chunk_max_docid = it->first;

		// turn docid differences into absolute docids
		unsigned int accumulator = 0;
		for (int i = 0; i < lp.curr_chunk.size() / 2; i++)
		{
			accumulator += lp.curr_chunk[i];
			lp.curr_chunk[i] = accumulator;
		}
	}

	// finally scan through the chunk until we find a docid >= k
	// if lp.curr_docid > k, we will start from beginning of chunk (since can't calculate docid reverse)
	if (lp.curr_chunk_docid > k)
		lp.curr_chunk_pos = 0;
	unsigned int starting_pos_offset = lp.curr_chunk_pos;
	for (auto it = lp.curr_chunk.begin() + starting_pos_offset; it != lp.curr_chunk.end(); it++, lp.curr_chunk_pos++)
	{
		if (*it >= k)
			break;
	}
	lp.curr_chunk_docid = lp.curr_chunk[lp.curr_chunk_pos];
	return lp.curr_chunk_docid;
}

/// <summary>
/// Returns the frequency based on the ListPointers current position
/// </summary>
/// <param name="lp">ListPointer to get frequency from</param>
/// <returns></returns>
unsigned int Query::DefaultQEngine::getFreq(ListPointer& lp)
{
	// if there is a cached chunk, return the current posting's frequency,
	// whose index is the current chunk idx + 1/2 the chunk size
	if (lp.curr_chunk_docid > 0)
		return lp.curr_chunk[lp.curr_chunk_pos + lp.curr_chunk.size() / 2];
	else
		throw "No current posting";
}

/// <summary>
/// load ListPointer into memory. If already in memory, do nothing
/// </summary>
/// <param name="query">query terms to load</param>
void Query::DefaultQEngine::loadList(std::vector<std::string>& query)
{
	std::set<std::string> query_set(query.begin(), query.end());
	std::set<std::string> terms_in_cache;
	std::set<std::string> terms_not_in_cache;
	// <last time used, index in _lp_vec>
	std::priority_queue<std::pair<unsigned int, unsigned int>> terms_to_throw_away;

	// search through our cached ListPointers to figure out which terms are already in our cache
	// and which we need to load from file.
	for (int i = 0; i < _lp_vec.size(); i++)
	{
		// if our term is in the query, increment the counter
		auto it = find(query_set.begin(), query_set.end(), _lp_vec[i].second);
		if (it != query_set.end())
		{
			_lp_vec[i].first = _query_counter;
			terms_in_cache.insert(*it);
		}
		// otherwise put it in our priority queue
		else
		{
			terms_to_throw_away.push(std::make_pair(_lp_vec[i].first, i));
		}
	}
	// now figure out what terms we need to load
	// get the difference and insert into our terms_not_in_cache
	std::set_difference(query_set.begin(), query_set.end(), terms_in_cache.begin(), terms_in_cache.end(),
		std::inserter(terms_not_in_cache, terms_not_in_cache.begin()));

	// free space if necessary by popping off our queue
	int over_capacity = _lp_vec.size() + terms_not_in_cache.size() - _lp_capacity;
	if (over_capacity > 0)
	{
		for (int i = 0; i < over_capacity; i++)
		{
			auto p = terms_to_throw_away.top();
			terms_to_throw_away.pop();
			// erase from our map
			_lp_map.erase(_lp_vec[p.second].second);
			// erase from our vec
			_lp_vec.erase(_lp_vec.begin() + p.second);
		}
	}
	try
	{
		// Load ListPointer from file
		for (auto& str : terms_not_in_cache)
		{
			_lp_map[str] = openList(str);
			_lp_vec.push_back(std::make_pair(_query_counter, str));
		}
	}
	catch (std::exception& e)
	{
		throw e;
	}
}

/// <summary>
/// Search for the Top K results using conjunctive DAAT search
/// </summary>
/// <param name="query">query terms</param>
/// <param name="k">number of results to return</param>
/// <returns>returns results in the form of PageResults</returns>
std::stack<Query::PageResult> Query::DefaultQEngine::getTopKConjunctive(std::vector<std::string>& query, int k)
{
	// keeps track of queries served for the LP cache
	++_query_counter;

	// holds the top k scores
	std::priority_queue<
		std::pair<double, PageResult>,
		std::vector<std::pair<double, PageResult>>,
		std::greater<std::pair<double, PageResult>>> topk_pq;	// priority queue uses pair <score, docid>
	unsigned int i, d = 0, max_docid, min_idx = 0, curr_docid, min_docs = std::numeric_limits<unsigned int>::max();
	double min_score, score;
	std::stack<PageResult> page_stack;
	std::vector<std::reference_wrapper<ListPointer>> lp_vec;
	std::string min_str;

	// load list pointers
	try
	{
		loadList(query);
	}
	catch (std::exception& e)
	{
		throw e;
	}

	// for each term, get the ListPointer from the cache and push into a vector
	for (auto& str : query)
	{
		lp_vec.push_back(_lp_map[str]);
		std::cout << str << ": " << _lp_map[str].num_docs << " docs" << std::endl;
	}

	// sort the vectors smallest to largest
	std::sort(lp_vec.begin(), lp_vec.end());

	// get the max docid from the shortest list
	max_docid = lp_vec[0].get().chunk_positions.rbegin()->first;

	// get the first docid from the shortest list
	curr_docid = nextGEQ(lp_vec[0].get(), 1);

	// loop through until we have checked all relevent documents in the shortest list
	while (curr_docid <= max_docid)
	{
		// iterate through each list pointer until we get a docid that is not equal to our current docid
		for (int i = 1; i < lp_vec.size(); i++)
		{
			d = nextGEQ(lp_vec[i].get(), curr_docid);
			if (d != curr_docid)
				break;
		}

		// if d > curr_docid, one of our lists does not contain curr_docid, so get next docid in short list
		if (d > curr_docid)
			curr_docid = nextGEQ(lp_vec[0].get(), d);
		else // otherwise we did find a common docid, so calculate score
		{
			try
			{
				PageResult pr;
				std::vector<double> args;
				args.reserve(query.size() * 2 + 1);
				// push document length
				args.push_back(_ps->getSize(curr_docid));
				pr.p.size = _ps->getSize(curr_docid);
				pr.p.docid = curr_docid;
				// push the rest of the args
				for (auto lp : lp_vec)
				{
					unsigned int freq = getFreq(lp.get());
					pr.query_freqs[lp.get().term] = freq;
					args.push_back(static_cast<double>(lp.get().num_docs));
					args.push_back(static_cast<double>(freq));
				}
				// get the score
				score = _scorer->score(args);
				pr.score = score;
				// if queue size < k, push onto queue
				if (topk_pq.size() < k)
					topk_pq.emplace(std::pair<double, PageResult> {score, pr});

				// otherwise we need to check whether this score is higher than the smallest score in queue
				else if (score > topk_pq.top().first)
				{
					topk_pq.pop();
					topk_pq.push(std::pair<double, PageResult> {score, pr});
				}
				curr_docid = nextGEQ(lp_vec[0], curr_docid + 1);
			}
			catch (std::exception& e)
			{
				throw std::string("Error while scoring: ") + e.what();
			}
		}
	}

	// reverse pq and return docids
	try
	{
		while (!topk_pq.empty())
		{
			PageResult pr = topk_pq.top().second;
			pr.p = _ps->getDocument(pr.p.docid);
			pr.score = topk_pq.top().first;
			page_stack.push(pr);
			topk_pq.pop();
		}
		return page_stack;
	}
	catch (...)
	{
		throw "Error getting document";
	}
}

/// <summary>
/// Returns the top K serach results using Disjunctive DAAT
/// </summary>
/// <param name="query">query terms to search for</param>
/// <param name="k">the number of results to return</param>
/// <returns>the top k results as PageResult objects</returns>
std::stack<Query::PageResult> Query::DefaultQEngine::getTopKDisjunctive(std::vector<std::string>& query, int k)
{
	// keeps track of # of queries for LP cache
	++_query_counter;

	std::priority_queue<
		std::pair<double, PageResult>,
		std::vector<std::pair<double, PageResult>>,
		std::greater<std::pair<double, PageResult>>> topk_pq;	// priority queue uses pair <score, docid>
	std::priority_queue<
		unsigned int,
		std::vector<unsigned int>,
		std::greater<unsigned int>> docid_pq;	// priority queue

	unsigned int d, min_docid, min_idx = 0, curr_docid = 0;
	double min_score, score;
	std::stack<PageResult> page_stack;

	// load list pointers
	try
	{
		loadList(query);
	}
	catch (std::exception& e)
	{
		throw e;
	}
	// get the smallest docids and put them in our queue
	for (auto& q : query)
	{
		std::cout << q << ": " << _lp_map[q].num_docs << " docs" << std::endl;
		docid_pq.push(nextGEQ(_lp_map[q], 1));
	}

	// iterate through our docid_pq, getting the smallest docid and calculate the scores
	while (!docid_pq.empty())
	{
		PageResult pr;
		// get the min docid in our priority queue
		curr_docid = docid_pq.top();
		docid_pq.pop();
		std::vector<double> args;
		// push document length
		args.push_back(_ps->getSize(curr_docid));
		pr.p.size = _ps->getSize(curr_docid);
		pr.p.docid = curr_docid;
		try
		{
			// iterate through each list pointer. construct our args from each lp that has our curr_docid
			for (auto& str : query)
			{
				// if the lp's curr docid == min docid, get the frequency, then advance the pointer
				if (_lp_map[str].curr_chunk_docid == curr_docid)
				{
					unsigned int freq = getFreq(_lp_map[str]);
					pr.query_freqs[str] = freq;
					args.push_back(static_cast<double>(_lp_map[str].num_docs));
					args.push_back(static_cast<double>(freq));
					unsigned int docid = nextGEQ(_lp_map[str], curr_docid + 1);
					if (docid < std::numeric_limits<unsigned int>::max())
						docid_pq.push(docid);
				}
			}
			score = _scorer->score(args);
		}
		catch(std::exception& e)
		{
			throw std::string("Error while scoring: ") + e.what();
		}

		// if queue size < k, push onto queue
		if (topk_pq.size() < k)
			topk_pq.push(std::pair<double,PageResult> {score, pr});
		// otherwise we need to check whether this score is higher than the smallest score in queue
		else if (score > topk_pq.top().first)
		{
			topk_pq.pop();
			topk_pq.push(std::pair<double, PageResult> {score, pr});
		}
		
	}

	// reverse pq and return docids
	try
	{
		while (!topk_pq.empty())
		{
			PageResult pr = topk_pq.top().second;
			pr.p = _ps->getDocument(pr.p.docid);
			pr.score = topk_pq.top().first;
			page_stack.push(pr);
			topk_pq.pop();
		}
		return page_stack;
	}
	catch (...)
	{
		throw "Error getting document";
	}
}