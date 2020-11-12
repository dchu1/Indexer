#include "SQLitePageStorage.h"

Page SQLitePageStorage::getDocument(unsigned int docid) const
{
    Page p;
	sqlite3_bind_int(_getDocPrepStmt, 1, docid);
    if (sqlite3_step(_getDocPrepStmt) == SQLITE_ROW)
    {
        // read in the columns (id, url, body, size)
        p.docid = sqlite3_column_int(_getDocPrepStmt, 0);
        p.url = std::string(reinterpret_cast<const char*>(sqlite3_column_text(_getDocPrepStmt, 1)));
        p.body = std::string(reinterpret_cast<const char*>(sqlite3_column_text(_getDocPrepStmt, 2)));
        p.size = sqlite3_column_int(_getDocPrepStmt, 3);
        sqlite3_reset(_getDocPrepStmt);
    }
    else
    {
        sqlite3_reset(_getDocPrepStmt);
        throw "Unable to get docid " + std::to_string(docid);
    }
    return p;
}
std::string SQLitePageStorage::getUrl(unsigned int docid) const
{
    return "";
}
std::string SQLitePageStorage::getBody(unsigned int docid) const
{
    return "";
}
size_t SQLitePageStorage::getSize(unsigned int docid) const
{
    return _size_cache[docid];
}
unsigned int SQLitePageStorage::average_document_length() const
{
    unsigned int avg;
    if (sqlite3_step(_getDocAvgLengthStmt) == SQLITE_ROW)
    {
        avg = sqlite3_column_int(_getDocAvgLengthStmt, 0);
        sqlite3_reset(_getDocAvgLengthStmt);
    }
    else
    {
        sqlite3_reset(_getDocAvgLengthStmt);
        throw "Unable to get average document length";
    }
    return avg;
}

unsigned int SQLitePageStorage::size() const
{
    return _num_rows;
}