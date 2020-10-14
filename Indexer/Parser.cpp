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
#include "varbyte.h"
#include <libxml++/libxml++.h>
#include <libxml/HTMLparser.h>
#include <libxml/HTMLtree.h>
#include <chrono>
#include <boost/algorithm/string.hpp>

using namespace std;

struct posting
{
    unsigned int data[3];

    unsigned int& docid() { return data[0]; }
    unsigned int& termid() { return data[1]; }
    unsigned int& frequency() { return data[2]; }
};

struct url_entry
{
    string url;
    unsigned int size;
};

constexpr auto BUFFER_SIZE = 134217728; // 1073741824; // 1 GB

// Memory Buffer
//char* read_buffer = new char[BUFFER_SIZE]();
//auto postings = vector<posting>{};

unsigned int curr_docid = 0;
unsigned int curr_termid = 0;
unsigned int curr_outputid = 0;

// Term to TermId map
unordered_map<string, unsigned int> term_termid_map;
unordered_map<unsigned int, string> termid_term_map;

// URL vector
auto url_table = vector<url_entry>{};

vector<unsigned char> postings;

void read_file(const string& filename);
vector<string> parse(string& str);
void print_node(const xmlpp::Node* node, unsigned int indentation = 0);
void tokenize(const string& str, vector<string>& vec);
//void count_words(vector<string> const& words, unordered_map<unsigned int, size_t>& wordCount);
//void create_postings(unsigned int docid, unordered_map<unsigned int, unsigned int>& wordCount, vector<posting>& vec);
unordered_map<unsigned int, unsigned int> count_words(vector<string> const& words);
void create_postings(unsigned int docid, unordered_map<unsigned int, unsigned int>& wordCount, vector<unsigned char>& vec);
void test_decode();
vector<string> tokenize_boost(string const& str);

void test()
{
    read_file("D:\\Users\\admin\\Documents\\NYU\\Fall_2020\\Search_Engines\\HW\\HW2\\example2.txt");
    test_decode();
}

void print_words(const string& filename)
{
    ofstream os(filename, ofstream::out);
    for (const auto& elem : term_termid_map)
    {
        os << elem.first << '\n';
    }
}


void print_termmap(const string& filename)
{
    ofstream os(filename, ofstream::binary);
    for (const auto& elem : term_termid_map)
    {
        os << elem.first << elem.second;
    }
}

int main(int argc, char* argv[])
{
    postings.reserve(BUFFER_SIZE);
    auto t1 = chrono::steady_clock::now();
    read_file("D:\\Users\\admin\\Documents\\NYU\\Fall_2020\\Search_Engines\\HW\\HW2\\fulldocs-new.trec");
    auto t2 = chrono::steady_clock::now();
    chrono::duration<double> time_span = chrono::duration_cast<chrono::duration<double>>(t2 - t1);
    std::cout << "Took " << time_span.count() << " seconds." << '\n';
}

void test_decode()
{
    ifstream is("D:\\Users\\admin\\Documents\\NYU\\Fall_2020\\Search_Engines\\HW\\HW2\\output" + to_string(curr_outputid-1), std::ios::binary);
    vector<char> vec;
    //vector<unsigned char> postings;
    //postings.reserve(BUFFER_SIZE);
    if (is) {
        // get length of file:
        is.seekg(0, is.end);
        int length = is.tellg();
        is.seekg(0, is.beg);
        vec.resize(length);
        is.read(&vec[0], length);
        auto decoded = varbyte::decode(vec);
        ostream_iterator<unsigned int> out_it(cout,", ");
        copy(decoded.begin(), decoded.end(), out_it);
    }
}

void output_buffer(const string& filename, const vector<unsigned char>& postings)
{
    ofstream os(filename, ofstream::binary);
    // print out the postings
    //for (auto const& p : postings)
    //{
    //    os << p.docid << ',' << p.termid << ',' << p.freq << endl;
    //}
    ostream_iterator<unsigned char> out_it(os);
    std::copy(postings.begin(), postings.end(), out_it);

    //os.write((const char*)postings[0], postings.size());
}

void read_file(const string& filename)
{
    wifstream is(filename);
    
    // This line is necessary for utf-16 documents made on Windows. not sure why...
    //is.imbue(locale(is.getloc(), new codecvt_utf16<wchar_t, 0x10ffff, consume_header>));

    wstring line;
    line.reserve(BUFFER_SIZE);
    string buf;
    buf.reserve(BUFFER_SIZE);
    bool in_text = false;
    //auto wordCount = unordered_map<unsigned int, unsigned int>{};
    //vector<string> bow;

    auto t1 = chrono::steady_clock::now();
    auto t2 = chrono::steady_clock::now();

    // https://stackoverflow.com/questions/11040703/convert-unicode-to-char/11040983#11040983
    // xmllib2 only works with utf8 strings, so need to convert from utf16 to utf8 (on windows file is UTF-16LE)
    // htmlReadDoc has an encoding parameter, but I couldn't figure out how to get it to correctly decode utf-16 to utf-8,
    // so i had to do it myself before passing it to htmlReadDoc
    wstring_convert<codecvt_utf8_utf16<wchar_t>, wchar_t> convert;
    try {
        if (is) {
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
                        parse(buf);
                        auto bow = parse(buf);
                        auto wordCount = count_words(bow);
                        
                        url_table[curr_docid].size = bow.size();
                        create_postings(curr_docid, wordCount, postings);
                        //wordCount.clear();

                        // if we need to write our buffer out to a file, do so
                        if (postings.size() > BUFFER_SIZE)
                        {
                            string filename = "output" + to_string(curr_outputid++);
                            cout << "Writing postings out to " + filename << endl;
                            output_buffer("D:\\Users\\admin\\Documents\\NYU\\Fall_2020\\Search_Engines\\HW\\HW2\\" + filename, postings);
                            print_words("D:\\Users\\admin\\Documents\\NYU\\Fall_2020\\Search_Engines\\HW\\HW2\\words.txt");
                            postings.clear();
                        }
                        if (curr_docid % 1000 == 0)
                        {
                            t2 = chrono::steady_clock::now();
                            chrono::duration<double> time_span = chrono::duration_cast<chrono::duration<double>>(t2 - t1);
                            cout << "Read docid: " << curr_docid << "(" << time_span.count() << " sec)" << "unique words: " << term_termid_map.size() << '\n';
                            t1 = chrono::steady_clock::now();
                        }
                        ++curr_docid;
                        buf.clear();
                        //bow.clear();
                        // TESTING
                        //term_termid_map.clear();
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

                        url_table.push_back(url_entry{ convert.to_bytes(line),0 });
                        buf += convert.to_bytes(line);
                    }
                }
            }
        }
        output_buffer("D:\\Users\\admin\\Documents\\NYU\\Fall_2020\\Search_Engines\\HW\\HW2\\output" + to_string(curr_outputid++), postings);
    }
    catch (exception& e)
    {
        ofstream os("D:\\Users\\admin\\Documents\\NYU\\Fall_2020\\Search_Engines\\HW\\HW2\\error.txt", ios_base::out);
        os << "docid: " << curr_docid << "exception: " << e.what() << "line: " << buf << '\n';
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

        // avoid word counting and directly add to our posting vector
        //vector<unsigned char> temp;
        //for (const string& word : tokenize_boost(nodeText->get_content()))
        //{
        //    if (term_termid_map.find(word) == term_termid_map.end())
        //    {
        //        term_termid_map[word] = curr_termid++;
        //    }
        //    temp = varbyte::encode(term_termid_map[word]);
        //    postings.insert(end(postings), begin(temp), end(temp));
        //    temp = varbyte::encode(curr_docid);
        //    postings.insert(end(postings), begin(temp), end(temp));
        //    
        //}
    }

    if (!nodeContent)
    {
        // Recurse through child nodes:
        // There seems to have some kind of memory leak here...
        xmlpp::Node::NodeList list = node->get_children();
        for (xmlpp::Node::NodeList::iterator iter = list.begin(); iter != list.end(); ++iter)
        {
            dom_extract(*iter, results, blacklist_tags); //recursive
        }
    }
}

// creates a word of bags from the html page
vector<string> parse(string& str)
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
void tokenize(string const& str, vector<string>& vec)
{
    istringstream iss(str);
    vec.insert(vec.end(),istream_iterator<string>(iss),
        istream_iterator<string>());
}

vector<string> tokenize_boost(string const& str)
{
    auto results = vector<string>{};
    boost::split(results, str, boost::is_any_of(" .,?!()/-_\"\'\r\n\t"));
    // get rid of empty strings and long strings
    results.erase(remove_if(begin(results), end(results), [](string x){ return x.size() > 0 && x.size() < 30; }), end(results));
    return results;
}
unordered_map<unsigned int, unsigned int> count_words(vector<string> const& words)
{
    auto wordCount = unordered_map<unsigned int, unsigned int>{};
    // count words using range operator (c++ 11 feature)
    for (auto const& word : words)
    {
        // check if the word is in our term_termid_map
        if (term_termid_map.find(word) == term_termid_map.end())
        {
            term_termid_map[word] = curr_termid++;
        }
        ++wordCount[term_termid_map[word]];
    }
    return wordCount;
}

void create_postings(unsigned int docid, unordered_map<unsigned int, unsigned int>& wordCount, vector<unsigned char>& postings)
{
    // resize our array if necessary
    //postings.reserve(postings.size() + wordCount.size());
    //postings.reserve(postings.size() + wordCount.size()*3);

    vector<unsigned char> temp;

    // iterate over map using range operator (c++ 11 feature)
    for (const auto& it : wordCount) {
        //vec.emplace_back(posting{ docid, it.first, it.second });
        temp = varbyte::encode(docid);
        postings.insert(end(postings), begin(temp), end(temp));
        temp = varbyte::encode(it.first);
        postings.insert(end(postings), begin(temp), end(temp));
        temp = varbyte::encode(it.second);
        postings.insert(end(postings), begin(temp), end(temp));
    }
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
