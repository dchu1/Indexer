#pragma once
#include <vector>
#include <string>
class SnippetGenerator
{
	std::string generate_snippet(const unsigned int docid, const std::vector<std::string>& q);
};

