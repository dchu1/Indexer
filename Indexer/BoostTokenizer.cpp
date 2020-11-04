#include "Tokenizer.h"
#include <boost/algorithm/string.hpp>
#include <algorithm>

std::vector<std::string> tokenize_boost(std::string const& str)
{
    auto results = std::vector<std::string>{};
    boost::split(results, str, boost::is_any_of(" `:.,?!()/\\-_[]{}\"\'\r\n\t"));
    // get rid of empty strings and long strings (> 30 characters)
    results.erase(remove_if(begin(results), end(results), [](std::string x) { return x.size() == 0 || x.size() > 30; }), end(results));
    return results;
}