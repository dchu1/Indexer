// Indexer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <unordered_map> 
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

using namespace std;

//constexpr auto BUFFER_SIZE = 134217728; // 1073741824; // 1 GB
constexpr auto STR_BUFFER_SIZE = 1048576; // 1 Mb
// Memory Buffer
//char* read_buffer = new char[BUFFER_SIZE]();
//auto postings = vector<posting>{};

unsigned int curr_docid = 0;
unsigned int curr_termid = 0;
unsigned int curr_outputid = 0;

// Term to TermId map
vector<string> parse(Glib::ustring& str);
void dom_extract(const xmlpp::Node* node, vector<string>& results, vector<string>& blacklist_tags);
void tokenize(const string& str, vector<string>& vec);
vector<string> tokenize_boost(string const& str);

// ./parser infile outpath buffersize windows_file[0,1], encode[0,1], use_termids[0,1]
int main(int argc, char* argv[])
{
    wstring line;
    Glib::ustring buf;
    string input_file(argv[1]);
    string output_path(argv[2]);
    bool in_text = false;
    bool windows_file = stoi(argv[4]) == 1;
    bool encode = stoi(argv[5]) == 1;
    bool use_termids = stoi(argv[6]) == 1;
    auto url_table = vector<pair<string, unsigned int>>{};
    ofstream url_os(output_path + "\\urltable.bin", ios::out | ofstream::binary);
    Inverter *inverter;
    inverter = new BSBI_Inverter(output_path, "output", stoi(argv[3]), encode, use_termids);
    // https://stackoverflow.com/questions/11040703/convert-unicode-to-char/11040983#11040983
    // xmllib2 only works with utf8 strings, so need to convert from utf16 to utf8 (on windows file is UTF-16LE)
    // htmlReadDoc has an encoding parameter, but I couldn't figure out how to get it to correctly decode utf-16 to utf-8,
    // so i had to do it myself before passing it to htmlReadDoc
    wstring_convert<codecvt_utf8_utf16<wchar_t>, wchar_t> convert;

    //read_file("D:\\Users\\admin\\Documents\\NYU\\Fall_2020\\Search_Engines\\HW\\HW2\\fulldocs-new.trec");
    wifstream is(input_file, ios::in);

    // This line is necessary for utf-16 documents made on Windows. not sure why...
    if (windows_file) 
        is.imbue(locale(is.getloc(), new codecvt_utf16<wchar_t, 0x10ffff, consume_header>));

    // for now these two strings are unbounded in size because in order to parse a document,
    // I need to be able to read the entire document into memory to pass to the parser
    line.reserve(STR_BUFFER_SIZE);
    buf.reserve(STR_BUFFER_SIZE);

    auto timer1 = chrono::steady_clock::now();
    try {
        // Check that we opened the input stream
        if (is) {
            auto t1 = chrono::steady_clock::now();
            auto t2 = chrono::steady_clock::now();
            // for now we are just going to look for text between <TEXT> and </TEXT> tags
            // and sends that to our HTML parser. We are also assuming that the text will fit
            // in memory, so we can process a full document at a time.
            while (getline(is, line, L'<'))
            {
                // If we are in text, pass what we have gotten to our parser and generate postings
                if (in_text)
                {
                    buf += convert.to_bytes(line);
                    // Look ahead and check what tag we are at
                    getline(is, line, L'>');

                    // if we are in the /TEXT tag, we will parse our data
                    if (line.compare(L"/TEXT") == 0)
                    {
                        in_text = false;
                        auto bow = parse(buf);
                        //url_table[curr_docid].second = bow.size();
                        // write it to our url file
                        inverter->invert(curr_docid, bow);

                        if (curr_docid % 1000 == 0)
                        {
                            t2 = chrono::steady_clock::now();
                            chrono::duration<double> time_span = chrono::duration_cast<chrono::duration<double>>(t2 - t1);
                            cout << "Read docid: " << curr_docid << "(" << time_span.count() << " sec)" << endl;
                            t1 = chrono::steady_clock::now();
                        }
                        ++curr_docid;
                        buf.clear();
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

                        // Get current position
                        //int len = is.tellg();
                        getline(is, line);

                        // Return to position before "Read line".
                        //is.seekg(len, ios_base::beg);

                        // write out the url
                        //url_table.push_back(pair<string, unsigned int>{ convert.to_bytes(line), 0 });
                        url_os << convert.to_bytes(line) << '\n';
                        buf += convert.to_bytes(line);
                    }
                }
            }
        }
    }
    catch (exception& e)
    {
        ofstream os(output_path + "error.txt", ios::out);
        os << "docid: " << curr_docid << "exception: " << e.what() << "line: " << buf << '\n';
    }
    inverter->flush_all();
    delete inverter;

    auto timer2 = chrono::steady_clock::now();
    chrono::duration<double> time_span = chrono::duration_cast<chrono::duration<double>>(timer2 - timer1);
    std::cout << "Took " << time_span.count() << " seconds." << '\n';
}

pair<string, string> next_document(wifstream& is)
{
    wstring line;
    string buf;
    string url;
    bool in_text = false;

    // https://stackoverflow.com/questions/11040703/convert-unicode-to-char/11040983#11040983
    // xmllib2 only works with utf8 strings, so need to convert from utf16 to utf8 (on windows file is UTF-16LE)
    // htmlReadDoc has an encoding parameter, but I couldn't figure out how to get it to correctly decode utf-16 to utf-8,
    // so i had to do it myself before passing it to htmlReadDoc
    wstring_convert<codecvt_utf8_utf16<wchar_t>, wchar_t> convert;

    // Check that we opened the input stream
    if (is) {
        auto t1 = chrono::steady_clock::now();
        auto t2 = chrono::steady_clock::now();
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
                    return pair<string, string>{url, buf};
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
    }
}

// extracts the text starting from an xmlpp root node
// based on https://developer.gnome.org/libxml++-tutorial/stable/chapter-parsers.html#idm46
void dom_extract(const xmlpp::Node* node, vector<string>& results, vector<string>& blacklist_tags)
{
    const xmlpp::ContentNode* nodeContent = dynamic_cast<const xmlpp::ContentNode*>(node);
    const xmlpp::Element* nodeElement = dynamic_cast<const xmlpp::Element*>(node);

    // if its an element node, we want to check if the tag is blacklisted. If so, return
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
        // There seems to have some kind of memory leak here...
        xmlpp::Node::NodeList list = node->get_children();
        for (xmlpp::Node::NodeList::iterator iter = list.begin(); iter != list.end(); ++iter)
        {
            dom_extract(*iter, results, blacklist_tags); //recursive
            delete *iter;
        }
        
    }
}

// creates a word of bags from the html page
vector<string> parse(Glib::ustring& str)
{
    vector<string> blacklist_tags({});
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
        ofstream os("D:\\Users\\admin\\Documents\\NYU\\Fall_2020\\Search_Engines\\HW\\HW2\\error.txt", ios_base::out);
        // for now we just squash the error and return an emtpy vector
        os << "parse func, " << "docid: " << curr_docid << "exception: " << e.what() << "string: " << str << '\n';
        return results;
    }
    return results;
}

// creating a bag of words from a string
void tokenize(Glib::ustring const& str, vector<string>& vec)
{
    //istringstream iss(str);
    //vec.insert(vec.end(),istream_iterator<string>(iss),
    //    istream_iterator<string>());
}

vector<string> tokenize_boost(string const& str)
{
    auto results = vector<string>{};
    boost::split(results, str, boost::is_any_of(" .,?!()/-_[]\"\'\r\n\t"));
    // get rid of empty strings and long strings
    //results.erase(remove(begin(results), end(results), ""), end(results));
    results.erase(remove_if(begin(results), end(results), [](string x){ return x.size() == 0 || x.size() > 30; }), end(results));
    return results;
}