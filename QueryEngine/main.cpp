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
		// construct a stream from the string
		std::stringstream strstr(line);

		// use stream iterators to copy the stream to the vector as whitespace separated strings
		std::istream_iterator<std::string> it(strstr);
		std::istream_iterator<std::string> end;
		std::vector<std::string> results(it, end);
		std::stack<Page> docids = qe.getTopKConjunctive(results, 10);
		while (!docids.empty())
		{
			std::cout << docids.top().docid << std::endl;
			docids.pop();
		}
			
	}
	delete cc;
}