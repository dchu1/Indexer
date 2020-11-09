#include <string>
#include <fstream>
#include <codecvt>
#include <chrono>
#include "Tokenizer.h"
#include "Inverter.h"
#include "Parser.h"
#include "sqlite3.h"
#include <boost/format.hpp>

using namespace std;

// ./parser infile outpath buffersize inverter[BSBI, SPIMI], windows_file[0,1], encode[0,1], use_termids[0,1], printouts
int main(int argc, char* argv[])
{
    // initialize variables and read command line args
    unsigned int curr_docid = 1;
    string input_file(argv[1]);
    string output_path(argv[2]);
    bool windows_file = stoi(argv[5]) == 1;
    bool encode = stoi(argv[6]) == 1;
    bool use_termids = stoi(argv[7]) == 1;
    ofstream url_os(output_path + "\\urltable.bin", ios::out | ofstream::binary);
    Tokenizer* tok = new BoostTokenizer();
    Parser parser = Parser(tok);
    double parseTime = 0, invertTime = 0, totalTime = 0, dbWriteTime = 0;

    // init db
    sqlite3* db;
    char* zErrMsg = 0;
    int rc;
    string db_path = output_path + "\\pages.db";
    rc = sqlite3_open(db_path.c_str(), &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return(0);
    }
    else {
        fprintf(stderr, "Opened database successfully\n");
    }

    // Change some PRAGMA to speed up writes
    //rc = sqlite3_exec(db, "PRAGMA synchronous = 0;", NULL, 0, &zErrMsg);
    rc = sqlite3_exec(db, "PRAGMA journal_mode = MEMORY; PRAGMA cache_size = 40000;", NULL, 0, &zErrMsg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        throw zErrMsg;
    }
    /* Create Table SQL statement */
    string sql("CREATE TABLE PAGES("  \
        "ID INT PRIMARY KEY     NOT NULL," \
        "URL            TEXT    NOT NULL," \
        "BODY           TEXT    NOT NULL," \
        "SIZE           INTEGER NOT NULL);");

    rc = sqlite3_exec(db, sql.c_str(), NULL, 0, &zErrMsg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        throw zErrMsg;
    }

    /* Create Insert SQL statement */
    sql = "INSERT INTO PAGES (ID,URL,BODY, SIZE) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt; // will point to prepared stamement object
    sqlite3_prepare_v2(
        db,            // the handle to your (opened and ready) database
        sql.c_str(),    // the sql statement, utf-8 encoded
        sql.length(),   // max length of sql statement
        &stmt,          // this is an "out" parameter, the compiled statement goes here
        nullptr);       // pointer to the tail end of sql statement (when there are 
                        // multiple statements inside the string; can be null)

    // choose our inverter based on command line args
    Inverter* inverter;
    if (string(argv[4]).compare("BSBI") == 0)
    {
        inverter = new BSBI_Inverter(output_path, "output", stoi(argv[3]), encode, use_termids);
    }
    else
    {
        inverter = new SPIMI_Inverter(output_path, "output", stoi(argv[3]), encode);
    }

    // https://stackoverflow.com/questions/11040703/convert-unicode-to-char/11040983#11040983
    // xmllib2 only works with utf8 strings, so need to convert from utf16 to utf8 (on windows file is UTF-16LE)
    // htmlReadDoc has an encoding parameter, but I couldn't figure out how to get it to correctly decode utf-16 to utf-8,
    // so i had to do it myself before passing it to htmlReadDoc
    //wstring_convert<codecvt_utf8_utf16<wchar_t>, wchar_t> convert;

    //read_file("D:\\Users\\admin\\Documents\\NYU\\Fall_2020\\Search_Engines\\HW\\HW2\\fulldocs-new.trec");
    wifstream is(input_file, ios::in);

    // This line is necessary for utf-16 documents made on Windows (but doesn't work on the
    // trec file for some reason
    if (windows_file)
        is.imbue(locale(is.getloc(), new codecvt_utf16<wchar_t, 0x10ffff, consume_header>));

    // if we weren't able to open the input file, return immediately
    if (!is)
        return 1;

    auto timerStart = chrono::steady_clock::now();
    auto invertTimerStart = chrono::steady_clock::now();
    auto parseTimerStart = chrono::steady_clock::now();
    auto dbWriteTimerStart = chrono::steady_clock::now();
    try {
        
        Document doc;
        auto t1 = chrono::steady_clock::now();
        auto t2 = chrono::steady_clock::now();
        parseTimerStart = chrono::steady_clock::now();

        // start a sqlite3 transaction
        rc = sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);

        // get the next document and put it into text
        while (parser.next_document(is, doc))
        {
            // if the text is empty, we will continue. This would presumably be either because a 
            // document is empty, or because we have reached the eof 
            if (doc.url == "") continue;

            // turn body to lowercase and get the bag of words from the text
            doc.body = doc.body.lowercase();
            auto bow = parser.parse(doc.body);

            parseTime += chrono::duration_cast<chrono::duration<double>>(chrono::steady_clock::now() - parseTimerStart).count();
            
            // write to db (for snippet generation)
            // https://stackoverflow.com/questions/61794729/how-does-prepared-statements-in-sqlite-c-work
            dbWriteTimerStart = chrono::steady_clock::now();
            
            // construct our Prepared Statement
            sqlite3_bind_int(stmt, 1, curr_docid);
            sqlite3_bind_text(
                stmt,             // previously compiled prepared statement object
                2,                // parameter index, 1-based
                doc.url.c_str(),  // the data
                doc.url.length(), // length of data
                SQLITE_STATIC);   // this parameter is a little tricky - it's a pointer to the callback
                                  // function that frees the data after the call to this function.
                                  // It can be null if the data doesn't need to be freed, or like in this case,
                                  // special value SQLITE_STATIC (the data is managed by the std::string
                                  // object and will be freed automatically).
            sqlite3_bind_text(
                stmt,             // previously compiled prepared statement object
                3,                // parameter index, 1-based
                doc.body.c_str(),  // the data
                doc.body.length(), // length of data
                SQLITE_STATIC);   // this parameter is a little tricky - it's a pointer to the callback
                                  // function that frees the data after the call to this function.
                                  // It can be null if the data doesn't need to be freed, or like in this case,
                                  // special value SQLITE_STATIC (the data is managed by the std::string
                                  // object and will be freed automatically).
            sqlite3_bind_int(stmt, 4, bow.size());
            rc = sqlite3_step(stmt);
            sqlite3_reset(stmt);

            if (rc != SQLITE_DONE) {
                fprintf(stderr, "SQL error: %s\n", zErrMsg);
                throw zErrMsg;
            }

            dbWriteTime += chrono::duration_cast<chrono::duration<double>>(chrono::steady_clock::now() - dbWriteTimerStart).count();

            // print out the url table as a plain comma delimited ascii file
            //url_os << doc.url << "," << bow.size() << "," << doc.start_pos << "," << doc.end_pos << '\n';
            // url_os << doc.url << "," << bow.size() << '\n';

            // invert our bag of words
            invertTimerStart = chrono::steady_clock::now();
            inverter->invert(curr_docid, bow);
            invertTime += chrono::duration_cast<chrono::duration<double>>(chrono::steady_clock::now() - invertTimerStart).count();

            // print out some times to keep track of progress and speed
            if ((curr_docid + 1) % stoi(argv[8]) == 0)
            {
                t2 = chrono::steady_clock::now();
                rc = sqlite3_exec(db, "END TRANSACTION;", NULL, NULL, NULL);
                chrono::duration<double> time_span = chrono::duration_cast<chrono::duration<double>>(t2 - t1);
                cout << "Read docid: " << curr_docid << "(" << time_span.count() << " sec)" << endl;
                t1 = chrono::steady_clock::now();
                rc = sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
            }
            ++curr_docid;
            parseTimerStart = chrono::steady_clock::now();
        }
    }
    catch (exception& e)
    {
        // if we get an exception, for now i'm just going to write it out to a file and end
        // ideally we would have more fine tuned exception handling.
        ofstream os(output_path + "error.txt", ios::out);
        os << "docid: " << curr_docid << "exception: " << e.what() << '\n';
        sqlite3_free(zErrMsg);
    }

    // clean up and print out timers
    invertTimerStart = chrono::steady_clock::now();
    inverter->flush_all();
    invertTime += chrono::duration_cast<chrono::duration<double>>(chrono::steady_clock::now() - invertTimerStart).count();
    rc = sqlite3_exec(db, "END TRANSACTION;", NULL, NULL, NULL);
    delete inverter;
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    std::cout << "Took total: " << chrono::duration_cast<chrono::duration<double>>(chrono::steady_clock::now() - timerStart).count() << " seconds." << '\n';
    std::cout << "Parsing took: " << parseTime << " seconds." << '\n';
    std::cout << "Writing to DB took: " << dbWriteTime << " seconds." << '\n';
    std::cout << "Inverting took: " << invertTime << " seconds." << '\n';
}