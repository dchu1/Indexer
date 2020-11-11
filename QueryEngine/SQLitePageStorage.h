#pragma once
#include "PageStorage.h"
#include <sqlite3.h>

class SQLitePageStorage : public PageStorage
{
public:
	Page getDocument(unsigned int docid) const override;
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
	}
	~SQLitePageStorage()
	{
		sqlite3_finalize(_getDocAvgLengthStmt);
		sqlite3_finalize(_getDocPrepStmt);
		sqlite3_close(_db);
		delete _getDocPrepStmt;
		delete _getDocAvgLengthStmt;
		delete _getNumRowsStmt;
	}
private:
	sqlite3* _db;
	char* _zErrMsg = 0;
	int _rc;
	sqlite3_stmt* _getDocPrepStmt;
	sqlite3_stmt* _getDocAvgLengthStmt;
	sqlite3_stmt* _getNumRowsStmt;
};

