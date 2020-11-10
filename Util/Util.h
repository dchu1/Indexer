#pragma once
#include <vector>
#include <string>
#include <fstream>
#include "compressor.h"

namespace Util
{
    struct Posting {
        unsigned int termid;
        unsigned int docid;
        unsigned int frequency;
        Posting(const Posting& obj) : termid{obj.termid}, docid{ obj.docid }, frequency{ obj.frequency }
        {
        }
        Posting(unsigned int a, unsigned int b, unsigned int c) : termid{ a }, docid{ b }, frequency{ c }
        {
        }
        Posting() = default;
        bool operator<(const Posting& rhs) const {
            return this->termid < rhs.termid || (this->termid == rhs.termid && this->docid < rhs.docid);
        }
        bool operator>(const Posting& rhs) const {
            return rhs < *this;
        }
        bool operator==(const Posting& rhs) const {
            return this->termid == rhs.termid && this->docid == rhs.docid;
        }
    };

    struct Posting_str {
        std::string term;
        unsigned int docid;
        unsigned int frequency;
        Posting_str(const Posting_str &obj) : term(obj.term), docid{obj.docid}, frequency{obj.frequency}
        {
        }
        Posting_str() = default;
        bool operator<(const Posting_str& rhs) const {
            return this->term.compare(rhs.term) < 0 || (this->term.compare(rhs.term)==0 && this->docid < rhs.docid);
        }
        bool operator>(const Posting_str& rhs) const {
            return rhs < *this;
        }
        bool operator==(const Posting_str& rhs) const {
            return this->term.compare(rhs.term) == 0 && this->docid == rhs.docid;
        }
    };

    namespace lexicon
    {
        struct LexiconMetadata
        {
            unsigned int position;
            unsigned int docid_count;
            // for now next_lexicon_entry_pos is only used by query engine
            // might move to someother data structure
            unsigned int next_lexicon_entry_pos;
        };
        struct LexiconEntry
        {
            LexiconMetadata metadata;
            std::string term;
        };
        std::ifstream& read_lexicon(std::ifstream& is, LexiconEntry& le, Util::compression::compressor* compressor);
    }
    namespace urltable
    {
        struct UrlTableEntry
        {
            std::string url;
            unsigned int size;
        };
        std::ifstream& get_url(std::ifstream& is, UrlTableEntry& ute);
    }
    namespace index
    {
        struct ChunkMetadata
        {
            unsigned int lastDocId;
            unsigned int size;
        };
        std::ifstream& get_chunk(std::ifstream& is, std::vector<unsigned int>& vec, unsigned int size);
    }
}