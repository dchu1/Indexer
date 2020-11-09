#pragma once
#include <string>
#include <tuple>

struct Page
{
	unsigned int docid;
	unsigned int size;
	std::string url;
	std::string body;
	friend bool operator<(const Page& l, const Page& r) { return std::tie(l.docid, l.size) < std::tie(r.docid, r.size); };
};
class PageStorage
{
public:
	virtual Page getDocument(unsigned int docid) const = 0;
	virtual unsigned int average_document_length() const = 0;
	virtual unsigned int size() const = 0;
};

