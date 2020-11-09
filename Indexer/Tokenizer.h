#pragma once
#include <vector>
#include <string>

class Tokenizer
{
public:
	virtual std::vector<std::string> tokenize(std::string const& str) = 0;
};

class BoostTokenizer : public Tokenizer
{
public:
	std::vector<std::string> tokenize(std::string const& str) override;
};

