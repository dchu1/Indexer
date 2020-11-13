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

	// Get our user's query
	while ((std::cout << "Please enter a query: "), std::getline(std::cin, line))
	{
		if (line.size() == 0)
			continue;
		std::stack<Query::PageResult> results;
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
				timerStart = std::chrono::steady_clock::now();
				results = qe.getTopKDisjunctive(query, 10);
				std::cout << "Lookup took: " << std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - timerStart).count() << " seconds." << '\n';
			}
			else if (line.compare("c") == 0)
			{
				timerStart = std::chrono::steady_clock::now();
				results = qe.getTopKConjunctive(query, 10);
				std::cout << "Lookup took: " << std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - timerStart).count() << " seconds." << '\n';
			}
			else
			{
				std::cout << "wrong input" << std::endl;
				continue;
			}

			// iterate through our results and generate snippets
			while (!results.empty())
			{
				std::cout << "Document ID: " << results.top().p.docid << '\n';
				std::cout << results.top().p.url << '\n';
				std::cout << "Score: " << results.top().score << '\n';
				for (auto& freq : results.top().query_freqs)
					std::cout << freq.first << ": " << freq.second << " occurrences" << '\n';
				std::cout << qe.generateSnippet(results.top().p, query) << '\n' << std::endl;
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