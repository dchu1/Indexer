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
	struct PositionEntry
	{
		unsigned int size;
		unsigned int relative_position;
	};
	struct ListPointer
	{
		std::ifstream is;
		unsigned int num_chunks;
		unsigned int beg_pos;
		unsigned int end_pos;
		unsigned int curr_docid; // current docid. If its 0, then that it indicates no chunk read
		unsigned int curr_chunk_idx;
		unsigned int curr_max_docid; // the largest docid in the current chunk
		unsigned int num_docs;
		bool compressed;
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
		virtual void loadLexicon(const std::string& lexiconFilePath) = 0;
		virtual unsigned int loadUrls(const std::string& urlFilePath) = 0;
		virtual std::stack<Page> getTopKConjunctive(std::vector<std::string> query, int k) = 0;
		virtual std::stack<Page> getTopKDisjunctive(std::vector<std::string> query, int k) = 0;
	};
	class DefaultQEngine : public QEngine
	{
	public:
		ListPointer openList(const std::string& term) override;
		void closeList(ListPointer& lp) override;
		unsigned int nextGEQ(ListPointer& lp, unsigned int k) override;
		unsigned int getFreq(ListPointer& lp) override;
		void loadLexicon(const std::string& lexiconFilePath) override;
		unsigned int loadUrls(const std::string& urlFilePath) override;
		std::stack<Page> getTopKConjunctive(std::vector<std::string> query, int k) override;
		std::stack<Page> getTopKDisjunctive(std::vector<std::string> query, int k) override;
		DefaultQEngine(const std::string& indexFilePath, const std::string& lexiconFilePath, PageStorage* ps, Util::compression::CompressorCreator* cc, Scorer* scorer) : 
			_indexFilePath(indexFilePath)
		{
			_compressor = cc->create();
			loadLexicon(lexiconFilePath);
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
		std::map<std::string, Util::lexicon::LexiconMetadata> _lexicon_map;
		Util::compression::compressor* _compressor;
		Scorer* _scorer;
		PageStorage* _ps;
	};
}