#pragma once
#include "PageStorage.h"
#include <sqlite3.h>
#include <vector>

class SQLitePageStorage : public PageStorage
{
public:
	Page getDocument(unsigned int docid) const override;
	std::string getUrl(unsigned int docid) const override;
	std::string getBody(unsigned int docid) const override;
	size_t getSize(unsigned int docid) const override;
	unsigned int average_document_length() const override;
	unsigned int size() const override;
	SQLitePageStorage(std::string dbpath)
	{
		// open our connection
		_rc = sqlite3_open(dbpath.c_str(), &_db);
		if (_rc) {
			fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(_db));
			throw "Unable to open db connection";
		}
		else {
			fprintf(stderr, "Opened database successfully\n");
		}
		
		// create our prepared statements
		_rc = sqlite3_prepare_v2(
			_db,            // the handle to your (opened and ready) database
			"SELECT * FROM PAGES WHERE ID = ?;",    // the sql statement, utf-8 encoded
			-1,				// max length of sql statement (-1 means read until null termintaed)
			&_getDocPrepStmt,          // this is an "out" parameter, the compiled statement goes here
			nullptr);       // pointer to the tail end of sql statement (when there are 
							// multiple statements inside the string; can be null)
		if (_rc != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", _zErrMsg);
			throw _zErrMsg;
		}

		_rc = sqlite3_prepare_v2(
			_db,            // the handle to your (opened and ready) database
			"SELECT avg(SIZE) FROM (SELECT SIZE FROM PAGES LIMIT 10000);",    // the sql statement, utf-8 encoded
			-1,				// max length of sql statement (-1 means read until null termintaed)
			&_getDocAvgLengthStmt,          // this is an "out" parameter, the compiled statement goes here
			nullptr);       // pointer to the tail end of sql statement (when there are 
							// multiple statements inside the string; can be null)
		if (_rc != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", _zErrMsg);
			throw _zErrMsg;
		}

		_rc = sqlite3_prepare_v2(
			_db,            // the handle to your (opened and ready) database
			"SELECT COUNT(*) FROM PAGES;",    // the sql statement, utf-8 encoded
			-1,				// max length of sql statement (-1 means read until null termintaed)
			&_getNumRowsStmt,          // this is an "out" parameter, the compiled statement goes here
			nullptr);       // pointer to the tail end of sql statement (when there are 
							// multiple statements inside the string; can be null)
		if (_rc != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", _zErrMsg);
			throw _zErrMsg;
		}

		_rc = sqlite3_prepare_v2(
			_db,            // the handle to your (opened and ready) database
			"SELECT URL FROM PAGES;",    // the sql statement, utf-8 encoded
			-1,				// max length of sql statement (-1 means read until null termintaed)
			&_getUrlStmt,          // this is an "out" parameter, the compiled statement goes here
			nullptr);       // pointer to the tail end of sql statement (when there are 
							// multiple statements inside the string; can be null)
		if (_rc != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", _zErrMsg);
			throw _zErrMsg;
		}

		_rc = sqlite3_prepare_v2(
			_db,            // the handle to your (opened and ready) database
			"SELECT SIZE FROM PAGES;",    // the sql statement, utf-8 encoded
			-1,				// max length of sql statement (-1 means read until null termintaed)
			&_getSizeStmt,          // this is an "out" parameter, the compiled statement goes here
			nullptr);       // pointer to the tail end of sql statement (when there are 
							// multiple statements inside the string; can be null)
		if (_rc != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", _zErrMsg);
			throw _zErrMsg;
		}

		_rc = sqlite3_prepare_v2(
			_db,            // the handle to your (opened and ready) database
			"SELECT BODY FROM PAGES WHERE ID = ?;",    // the sql statement, utf-8 encoded
			-1,				// max length of sql statement (-1 means read until null termintaed)
			&_getBodyStmt,          // this is an "out" parameter, the compiled statement goes here
			nullptr);       // pointer to the tail end of sql statement (when there are 
							// multiple statements inside the string; can be null)

		if (_rc != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", _zErrMsg);
			throw _zErrMsg;
		}

		// get the size
		if (sqlite3_step(_getNumRowsStmt) == SQLITE_ROW)
		{
			_num_rows = sqlite3_column_int(_getNumRowsStmt, 0);
			sqlite3_reset(_getNumRowsStmt);
		}
		else
		{
			sqlite3_reset(_getNumRowsStmt);
			throw "Unable to get average document length";
		}

		// get the sizes of docs
		// get data for our cache
		_size_cache.reserve(_num_rows);
		while (sqlite3_step(_getSizeStmt) == SQLITE_ROW)
		{
			_size_cache.push_back(sqlite3_column_int(_getSizeStmt, 0));
		}
		sqlite3_reset(_getSizeStmt);
	}
	~SQLitePageStorage()
	{
		sqlite3_finalize(_getDocAvgLengthStmt);
		sqlite3_finalize(_getDocPrepStmt);
		sqlite3_finalize(_getNumRowsStmt);
		sqlite3_finalize(_getUrlStmt);
		sqlite3_finalize(_getSizeStmt);
		sqlite3_finalize(_getBodyStmt);
		sqlite3_close(_db);
		delete _getDocPrepStmt;
		delete _getDocAvgLengthStmt;
		delete _getNumRowsStmt;
		delete _getUrlStmt;
		delete _getSizeStmt;
		delete _getBodyStmt;
		delete _zErrMsg;
		delete _db;
	}
private:
	sqlite3* _db;
	char* _zErrMsg = 0;
	int _rc;
	sqlite3_stmt* _getDocPrepStmt;
	sqlite3_stmt* _getDocAvgLengthStmt;
	sqlite3_stmt* _getNumRowsStmt;
	sqlite3_stmt* _getUrlStmt;
	sqlite3_stmt* _getSizeStmt;
	sqlite3_stmt* _getBodyStmt;
	std::vector<int> _size_cache;
	unsigned int _num_rows;
};

