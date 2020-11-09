#include <fstream>
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
#include "Tokenizer.h"
#include "Parser.h"
using namespace std;

// gets the next document from the input stream
wifstream& Parser::next_document(wifstream& is, Document& p)
{
    auto getDocTimerStart = chrono::steady_clock::now();
    wstring line;
    Glib::ustring buf, url;
    bool in_text = false;
    wifstream::streampos start_pos, end_pos;

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
            end_pos = is.tellg();
            // If we are in text, read until we reach the /TEXT tag
            if (in_text)
            {
                buf += convert.to_bytes(line);
                // Look ahead and check what tag we are at
                getline(is, line, L'>');

                // if we are in the </TEXT> tag, return
                if (line.compare(L"/TEXT") == 0)
                {
                    //p = Document { url, buf, start_pos, end_pos };
                    p = Document{ url, buf };
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
                    // chomp the new line character
                    getline(is, line);
                    getline(is, line);

                    // store the url
                    url = convert.to_bytes(line);
                    boost::algorithm::trim(url);
                    buf += url + " ";
                    
                    // store the starting position
                    start_pos = is.tellg();
                }
            }
        }
    }
    // if we reach this point, we did not find a closing TEXT tag
    return is;
}

// extracts the text starting from an xmlpp root node
// based on https://developer.gnome.org/libxml++-tutorial/stable/chapter-parsers.html#idm46
void Parser::dom_extract(const xmlpp::Node* node, vector<string>& results, vector<string>& blacklist_tags)
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
        vector<string> tokens = _tok->tokenize(nodeText->get_content());
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
vector<string> Parser::parse(Glib::ustring& str)
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
        return results;
    }
    return results;
}