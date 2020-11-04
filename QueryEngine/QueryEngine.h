#pragma once

#include <fstream>
#include <string>
#include <map>
#include <vector>
#include "Util.h"

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
		unsigned int freq;
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
		virtual void loadUrls(const std::string& urlFilePath) = 0;
	};
	class DefaultQEngine : public QEngine
	{
	public:
		ListPointer openList(const std::string& term) override;
		void closeList(ListPointer& lp) override;
		unsigned int nextGEQ(ListPointer& lp, unsigned int k) override;
		unsigned int getFreq(ListPointer& lp) override;
		void loadLexicon(const std::string& lexiconFilePath) override;
		void loadUrls(const std::string& urlFilePath) override;
		DefaultQEngine(const std::string& indexFilePath, const Util::compression::CompressorCreator* cc) : _indexFilePath(indexFilePath)
		{
			_compressor = cc->create();
		}
		~DefaultQEngine()
		{
			delete _compressor;
		}
	private:
		std::string _indexFilePath;
		std::map<std::string,Util::lexicon::LexiconMetadata> _lexicon_map;
		std::vector<Util::urltable::UrlTableEntry> _url_table;
		Util::compression::compressor* _compressor;
	};
}