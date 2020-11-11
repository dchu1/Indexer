#pragma once

#include <fstream>
#include <string>
#include <map>
#include <vector>
#include "Util.h"
#include <queue>
#include <unordered_map>
#include "Scorer.h"
#include <stack>
#include "PageStorage.h"

namespace Query
{
	struct PageResult
	{
		Page p;
		std::unordered_map<std::string, unsigned int> query_freqs;
		friend bool operator<(const PageResult& l, const PageResult& r) { return l.p < r.p; };
	};
	struct PositionEntry
	{
		unsigned int size;
		unsigned int relative_position;
	};
	struct ListPointer
	{
		unsigned int beg_pos;
		unsigned int end_pos;
		unsigned int curr_chunk_docid; // current docid. If its 0, then that it indicates no chunk read
		unsigned int curr_chunk_pos;
		unsigned int curr_chunk_max_docid; // the largest docid in the current chunk
		unsigned int num_docs;
		std::vector<unsigned char> inv_list;
		std::vector<unsigned int> curr_chunk;
		// map last doc id to relative position, size
		std::map<unsigned int, PositionEntry> chunk_positions;
	};

	class QEngine
	{
	public:
		virtual ListPointer openList(const std::string& term) = 0;
		virtual void closeList(ListPointer& lp) = 0;
		virtual unsigned int nextGEQ(ListPointer& lp, unsigned int k) = 0;
		virtual unsigned int getFreq(ListPointer& lp) = 0;
		virtual void loadLexicon(const std::string& lexiconFilePath, Util::compression::compressor* compressor) = 0;
		virtual std::stack<PageResult> getTopKConjunctive(std::vector<std::string>& query, int k) = 0;
		virtual std::stack<PageResult> getTopKDisjunctive(std::vector<std::string>& query, int k) = 0;
		virtual std::string generateSnippet(PageResult&) const = 0;
	};
	class DefaultQEngine : public QEngine
	{
	public:
		ListPointer openList(const std::string& term) override;
		void closeList(ListPointer& lp) override;
		unsigned int nextGEQ(ListPointer& lp, unsigned int k) override;
		unsigned int getFreq(ListPointer& lp) override;
		void loadLexicon(const std::string& lexiconFilePath, Util::compression::compressor* compressor) override;
		std::stack<PageResult> getTopKConjunctive(std::vector<std::string>& query, int k) override;
		std::stack<PageResult> getTopKDisjunctive(std::vector<std::string>& query, int k) override;
		std::string generateSnippet(PageResult&) const override;
		void loadList(std::vector<std::string>& query);
		DefaultQEngine(const std::string& indexFilePath, const std::string& lexiconFilePath, PageStorage* ps, Util::compression::CompressorCreator* cc, Scorer* scorer) :
			_indexFilePath(indexFilePath), _lp_capacity{100}, _query_counter{0}
		{
			_compressor = cc->create();
			loadLexicon(lexiconFilePath, _compressor);
			_ps = ps;
			_scorer = scorer;
			_scorer->_davg = _ps->average_document_length();
			_scorer->_N = ps->size();
		}
		~DefaultQEngine()
		{
			delete _compressor;
		}
	private:
		std::unordered_map<std::string, ListPointer> listpointer_map;
		std::string _indexFilePath;
		Util::compression::compressor* _compressor;
		Scorer* _scorer;
		PageStorage* _ps;

		// Lexicon
		std::vector<std::string> _lexicon_terms;
		std::vector<Util::lexicon::LexiconMetadata> _lexicon_values;

		// LRU Cacheing for ListPointers
		// lp_map holds the strint to listpointer mapping
		std::unordered_map<std::string, ListPointer> _lp_map;
		// _lp_vec <the last time used, term>
		std::vector<std::pair<unsigned int, std::string>> _lp_vec;

		size_t _lp_capacity;
		unsigned int _query_counter;

	};
}