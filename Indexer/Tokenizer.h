#pragma once
#include <vector>
#include <string>

class Tokenizer
{
	virtual std::vector<std::string> tokenize(std::string const& str) = 0;
};

class BoostTokenizer : public Tokenizer
{
	std::vector<std::string> tokenize(std::string const& str) override;
};

