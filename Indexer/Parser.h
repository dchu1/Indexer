#pragma once
#include <fstream>
#include <vector>
#include <string>
#include <glibmm/ustring.h>
#include "tokenizer.h"

class Parser
{
public:
	std::wifstream& next_document(std::wifstream& is, std::pair<std::string, Glib::ustring>& p);
	Parser(Tokenizer *tok)
	{
		_tok = tok;
	}
	~Parser()
	{
	}
private:
	Tokenizer* _tok;
};