// Indexer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <locale>
#include <codecvt>
#include "Util.h"
#include <libxml++/libxml++.h>
#include <libxml/HTMLparser.h>
#include <libxml/HTMLtree.h>
#include <chrono>
#include <boost/algorithm/string.hpp>
#include "glibmm/ustring.h"
#include "Inverter.h"
#include "Tokenizer.h"
#include "Parser.h"

using namespace std;

//constexpr auto BUFFER_SIZE = 134217728; // 1073741824; // 1 GB

// using these global variables to measure time
double getDocTime = 0, parseTime = 0, invertTime = 0;

wifstream& next_document(wifstream& is, pair<string, Glib::ustring>& p);
vector<string> parse(Glib::ustring& str);
void dom_extract(const xmlpp::Node* node, vector<Glib::ustring>& results, vector<Glib::ustring>& blacklist_tags);
void tokenize(const Glib::ustring& str, vector<Glib::ustring>& vec);
vector<string> tokenize_boost(string const& str);

// ./parser infile outpath buffersize inverter[BSBI, SPIMI], windows_file[0,1], encode[0,1], use_termids[0,1], printouts
int main(int argc, char* argv[])
{
    unsigned int curr_docid = 1;
    string input_file(argv[1]);
    string output_path(argv[2]);
    bool windows_file = stoi(argv[5]) == 1;
    bool encode = stoi(argv[6]) == 1;
    bool use_termids = stoi(argv[7]) == 1;
    ofstream url_os(output_path + "\\urltable.bin", ios::out | ofstream::binary);

    // choose our inverter based on command line args
    Inverter *inverter;
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
    wstring_convert<codecvt_utf8_utf16<wchar_t>, wchar_t> convert;

    //read_file("D:\\Users\\admin\\Documents\\NYU\\Fall_2020\\Search_Engines\\HW\\HW2\\fulldocs-new.trec");
    wifstream is(input_file, ios::in);

    // This line is necessary for utf-16 documents made on Windows (but doesn't work on the
    // trec file for some reason
    if (windows_file) 
        is.imbue(locale(is.getloc(), new codecvt_utf16<wchar_t, 0x10ffff, consume_header>));

    // if we weren't able to open the input file, return immediately
    if (!is)
        return 1;

    auto timer1 = chrono::steady_clock::now();
    auto invertTimerStart = chrono::steady_clock::now();
    try {
        pair<string, Glib::ustring> text;
        auto t1 = chrono::steady_clock::now();
        auto t2 = chrono::steady_clock::now();
        
        // get the next document and put it into text
        while (next_document(is, text))
        {
            // if the text is empty, we will continue. This would presumably be either because a 
            // document is empty, or because we have reached the eof 
            if (text.first == "") continue;

            // get the bag of words from the text
            auto bow = parse(text.second);

            // print out the url table as a plain comma delimited ascii file
            url_os << text.first << ", " << bow.size() << '\n';
            invertTimerStart = chrono::steady_clock::now();

            // invert our bag of words
            inverter->invert(curr_docid, bow);
            invertTime += chrono::duration_cast<chrono::duration<double>>(chrono::steady_clock::now() - invertTimerStart).count();

            // print out some times to keep track of progress and speed
            if ((curr_docid+1) % stoi(argv[8]) == 0)
            {
                t2 = chrono::steady_clock::now();
                chrono::duration<double> time_span = chrono::duration_cast<chrono::duration<double>>(t2 - t1);
                cout << "Read docid: " << curr_docid << "(" << time_span.count() << " sec)" << endl;
                t1 = chrono::steady_clock::now();
            }
            ++curr_docid;
        }
    }
    catch (exception& e)
    {
        // if we get an exception, for now i'm just going to write it out to a file and end
        // ideally we would have more fine tuned exception handling.
        ofstream os(output_path + "error.txt", ios::out);
        os << "docid: " << curr_docid << "exception: " << e.what() << '\n';
    }

    // clean up and print out timers
    invertTimerStart = chrono::steady_clock::now();
    inverter->flush_all();
    invertTime += chrono::duration_cast<chrono::duration<double>>(chrono::steady_clock::now() - invertTimerStart).count();
    delete inverter;

    auto timer2 = chrono::steady_clock::now();
    chrono::duration<double> time_span = chrono::duration_cast<chrono::duration<double>>(timer2 - timer1);
    std::cout << "Took total: " << time_span.count() << " seconds." << '\n';
    std::cout << "Getting next document took: " << getDocTime << " seconds." << '\n';
    std::cout << "Parsing took: " << parseTime << " seconds." << '\n';
    std::cout << "Inverting took: " << invertTime << " seconds." << '\n';
}

// gets the next document from the input stream
wifstream& next_document(wifstream& is, pair<string, Glib::ustring>& p)
{
    auto getDocTimerStart = chrono::steady_clock::now();
    wstring line;
    Glib::ustring buf;
    Glib::ustring url;
    bool in_text = false;

    // https://stackoverflow.com/questions/11040703/convert-unicode-to-char/11040983#11040983
    // xmllib2 only works with utf8 strings, so need to convert from utf16 to utf8 (on windows file is UTF-16LE)
    // htmlReadDoc has an encoding parameter, but I couldn't figure out how to get it to correctly decode utf-16 to utf-8,
    // so i had to do it myself before passing it to htmlReadDoc
    wstring_convert<codecvt_utf8_utf16<wchar_t>, wchar_t> convert;

    // Check that we opened the input stream
    if (is) {
        // for now we are just going to look for text between <TEXT> and </TEXT> tags
        // and sends that to our HTML parser. We are also assuming that the text will fit
        // in memory, so we can process a full document at a time.
        while (getline(is, line, L'<'))
        {
            // If we are in text, read until we reach the /TEXT tag
            if (in_text)
            {
                buf += convert.to_bytes(line);
                // Look ahead and check what tag we are at
                getline(is, line, L'>');

                // if we are in the /TEXT tag, return
                if (line.compare(L"/TEXT") == 0)
                {
                    p = pair<string, Glib::ustring>{ url, buf.lowercase() };
                    getDocTime += chrono::duration_cast<chrono::duration<double>>(chrono::steady_clock::now() - getDocTimerStart).count();
                    return is;
                }

                // otherwise write our tag as part of our text and continue
                else
                {
                    buf += '<' + convert.to_bytes(line) + '>';
                }
            }
            // otherwise get the node name
            else
            {
                getline(is, line, L'>');
                // If we are entering a text tag, we want to keep what is between
                if (line.compare(L"TEXT") == 0)
                {
                    in_text = true;
                    // I am assuming the the text is deliminated with the first line being the url
                    // and update our url table
                    // chomp the new line character
                    getline(is, line);
                    getline(is, line);

                    // store the url
                    url = convert.to_bytes(line);
                    buf += url;
                }
            }
        }
        getDocTime += chrono::duration_cast<chrono::duration<double>>(chrono::steady_clock::now() - getDocTimerStart).count();
    }
    // if we reach this point, we did not find a closing TEXT tag
    return is;
}

// extracts the text starting from an xmlpp root node
// based on https://developer.gnome.org/libxml++-tutorial/stable/chapter-parsers.html#idm46
void dom_extract(const xmlpp::Node* node, vector<string>& results, vector<string>& blacklist_tags)
{
    const xmlpp::ContentNode* nodeContent = dynamic_cast<const xmlpp::ContentNode*>(node);
    const xmlpp::Element* nodeElement = dynamic_cast<const xmlpp::Element*>(node);

    // if its an element node, we want to check if the tag is blacklisted. If so, just return
    if (nodeElement && find(blacklist_tags.begin(), blacklist_tags.end(), nodeElement->get_name()) != blacklist_tags.end())
    {
        return;
    }
    else if (const xmlpp::TextNode* nodeText = dynamic_cast<const xmlpp::TextNode*>(node))
    {
        // tokenize using my func
        //tokenize(nodeText->get_content(), results);

        // tokenize using boost
        vector<string> tokens = tokenize_boost(nodeText->get_content());
        results.insert(end(results), begin(tokens), end(tokens));
    }

    if (!nodeContent)
    {
        // Recurse through child nodes:
        xmlpp::Node::NodeList list = node->get_children();
        for (xmlpp::Node::NodeList::iterator iter = list.begin(); iter != list.end(); ++iter)
        {
            dom_extract(*iter, results, blacklist_tags); //recursive
            delete *iter;
        }
        
    }
}

// returns a word of bags from a given string, which represents an html page
vector<string> parse(Glib::ustring& str)
{
    auto parseTimerStart = chrono::steady_clock::now();
    vector<string> blacklist_tags({ "noscript",
        "header",
        "meta",
        "head",
        "input",
        "script",
        "style" });
    auto results = vector<string>{};
    xmlDoc* doc = htmlReadDoc((xmlChar*)str.c_str(), NULL, NULL, HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);

    // Encapsulate raw libxml document in a libxml++ wrapper
    try {
        xmlNode* r = xmlDocGetRootElement(doc);
        xmlpp::Element* root = new xmlpp::Element(r);
        dom_extract(root, results, blacklist_tags);
        xmlFreeDoc(doc); // not sure if I have to free this memory
        delete root; // not sure if I have to free this memory
    }
    catch (exception& e)
    {
        // for now we just squash the error and return an empty vector
        parseTime += chrono::duration_cast<chrono::duration<double>>(chrono::steady_clock::now() - parseTimerStart).count();
        return results;
    }
    parseTime += chrono::duration_cast<chrono::duration<double>>(chrono::steady_clock::now() - parseTimerStart).count();
    return results;
}

// creating a bag of words from a string. This needs to be rewritten to work with ustrings
void tokenize(Glib::ustring const& str, vector<string>& vec)
{
    istringstream iss(str);
    vec.insert(vec.end(),istream_iterator<string>(iss),
        istream_iterator<string>());
}

// using boost tokenizer. Since this tokenizer works on ascii strings, I don't
// know what it does with non-ascii characters. In order to get around that, I would 
// need to write my own tokenizer or find a ustring aware tokenizer library.
vector<string> tokenize_boost(string const& str)
{
    auto results = vector<string>{};
    boost::split(results, str, boost::is_any_of(" `:.,?!()/\\-_[]{}\"\'\r\n\t"));
    // get rid of empty strings and long strings (> 30 characters)
    results.erase(remove_if(begin(results), end(results), [](string x){ return x.size() == 0 || x.size() > 30; }), end(results));
    return results;
}