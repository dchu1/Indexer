#include <iostream>
#include "QueryEngine.h"
#include "Util.h"
#include <array>
#include <queue>
#include <iterator>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <stack>
#include "PageStorage.h"
#include "SQLitePageStorage.h"

// ./QueryEngine indexfile lexiconfile pagesdb
int main(int argc, char* argv[])
{
	std::string line;
	Util::compression::CompressorCreator* cc = new Util::compression::VarbyteCreator();
	Scorer* scorer = new ScorerBM25();
	PageStorage* ps = new SQLitePageStorage(argv[3]);
	auto timerStart = std::chrono::steady_clock::now();
	Query::DefaultQEngine qe(argv[1], argv[2], ps, cc, scorer);
	std::cout << "Initializing Query Engine took: " << std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - timerStart).count() << " seconds." << '\n';
	while ((std::cout << "Please enter a query: "), std::getline(std::cin, line))
	{
		if (line.size() == 0)
			continue;
		std::stack<Page> results;
		// construct a stream from the string
		std::stringstream strstr(line);

		// use stream iterators to copy the stream to the vector as whitespace separated strings
		std::istream_iterator<std::string> it(strstr);
		std::istream_iterator<std::string> end;
		std::vector<std::string> query(it, end);
		std::cout << "Enter [c,d] for [conjunctive, disjunctive]" << std::endl;
		std::cin >> line;
		try
		{
			if (line.compare("d") == 0)
			{
				results = qe.getTopKDisjunctive(query, 10);
			}
			else if (line.compare("c") == 0)
			{
				results = qe.getTopKConjunctive(query, 10);
			}
			else
			{
				std::cout << "wrong input" << std::endl;
				continue;
			}

			while (!results.empty())
			{
				std::cout << results.top().url << '\n';
				std::cout << qe.generateSnippet(results.top(), query) << '\n' << std::endl;
				results.pop();
			}
		}
		catch (std::exception& e)
		{
			std::cout << e.what() << std::endl;
		}
	}
	delete cc;
}