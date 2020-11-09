#pragma once
#include <fstream>
#include <vector>
#include <string>
#include <glibmm/ustring.h>
#include "tokenizer.h"
#include <libxml++/libxml++.h>
#include <libxml/HTMLparser.h>
#include <libxml/HTMLtree.h>

struct Document {
	std::string url;
	Glib::ustring body;
	//std::wstreampos start_pos;
	//std::wstreampos end_pos;
};
class Parser
{
public:
	std::wifstream& next_document(std::wifstream& is, Document& p);
	std::vector<std::string> parse(Glib::ustring& str);
	Parser(Tokenizer *tok)
	{
		_tok = tok;
	}
	~Parser()
	{
	}
private:
	void dom_extract(const xmlpp::Node* node, std::vector<std::string>& results, std::vector<std::string>& blacklist_tags);
	Tokenizer* _tok;
};