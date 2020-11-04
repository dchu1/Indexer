// Mergesort.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <queue>
#include <memory>
#include <chrono>
#include "Util.h"
#include "Mergesort.h"

using namespace std;

//void read_urltable(const string& filepath)
//{
//    auto url_table = vector<pair<string, unsigned int>>{};
//    ifstream is(filepath, ios::in | ios::binary);
//    size_t stringsize;
//    string url;
//    unsigned int size;
//    while (is.read((char*)&stringsize, sizeof(size_t)))
//    {
//        url.resize(stringsize);
//        is.read((char*)url.c_str(), stringsize);
//        is.read((char*)&size, sizeof(unsigned int));
//        cout << url << ", " << size << '\n';
//        url_table.push_back(pair<string, unsigned int>{ url, size });
//    }
//}

void write_to_file(ofstream& os, Util::Posting_str& ps, bool encode)
{
    // This code adapted from SPIMI_Inverter. Might want to make this a util function
    if (encode)
    {
        // write out the binary in the form of docid, freq, # of bytes of string, string
        // this is kind of dangerous as i'm mixing size_t and unsigned int which are not
        // guaranteed to be the same size of bytes. Should standardize on one or the other
        size_t size = ps.term.size();
        std::vector<unsigned char> v = Util::encode(&ps.docid, 2);
        os.write((char*)v.data(), v.size());
        v = Util::encode(&size, 1);
        os.write((char*)v.data(), v.size());
        os.write(ps.term.c_str(), size);
    }
    else
    {
        // write out the binary in the form of docid, freq, # of bytes of string, string
        // this is kind of dangerous as i'm mixing size_t and unsigned int which are not
        // guaranteed to be the same size of bytes. Should standardize on one or the other
        size_t size = ps.term.size();
        os.write((char*)&ps.docid, sizeof(ps.docid));
        os.write((char*)&ps.frequency, sizeof(ps.frequency));
        os.write((char*)&size, sizeof(size));
        os.write(ps.term.c_str(), size);
    }
}

ifstream& nextPostingStr(ifstream& is, Util::Posting_str& posting)
{
    try {
        int counter = 0;
        // read in docid, freq, # of bytes of string
        std::vector<unsigned int>vec;
        vec.reserve(3);
        if (Util::decode(is, vec, 3))
        {
            posting.docid = vec[0];
            posting.frequency = vec[1];

            // read in string
            char* str = new char[vec[2] + 1];
            is.read(str, vec[2]);
            str[vec[2]] = '\0';
            posting.term = str;
            delete[] str;
        }
    }
    catch (exception& e)
    {
        // for now i'm just going to print some exception happened
        //cout << "Exception occurred: " << e.what() << endl;
        throw e;
    }
    return is;
}

// ./mergesort finlist foutpath degree
int main(int argc, char* argv[])
{
    auto timer1 = chrono::steady_clock::now();
    string line;
    int maxDegree, degree;
    int counter = 0;
    maxDegree = stoi(argv[3]);
    vector<ifstream> ifstream_vec;
    ifstream_vec.reserve(maxDegree);
    ifstream filelist_is(argv[1], ios::in);
    string outpath = argv[2];
    ofstream outfilelist_os(outpath + "\\mergefiles.txt", ios::out);

    // Read in the file that lists our input files
    if (filelist_is)
    {
        //os->rdbuf()->pubsetbuf(bufSpace + (maxDegree * bufSize), memSize - (maxDegree * bufSize));
        //os->open(argv[2], ios::out | ios::binary);

        // read from our input file list file
        while (getline(filelist_is, line))
        {
            degree = 0;
            string filepath = outpath + "\\merge" + to_string(counter) + ".bin";
            auto os = ofstream(filepath, ios::out | ios::binary);
            do
            {
                //unique_ptr<ifstream> temp(new ifstream(line, ios::in | ios::binary));
                //temp->rdbuf()->pubsetbuf(bufSpace+(degree*bufSize), bufSize);
                //temp->open(line, ios::in | ios::binary);
                try {
                    ifstream_vec.push_back(ifstream(line, ios::in | ios::binary));
                    ++degree;
                }
                catch (exception& e) {
                    cout << "Error reading file: " + line + ", Exception: " << e.what() << endl;
                }
            } while (degree < maxDegree && getline(filelist_is, line));
            
            // populate our priority queue
            priority_queue<heap_entry<Util::Posting_str>, vector<heap_entry<Util::Posting_str>>, pq_comparator<heap_entry<Util::Posting_str>>> pq(pq_comparator<heap_entry<Util::Posting_str>>(true));
            try {
                for (int i = 0; i < ifstream_vec.size(); i++)
                {
                    Util::Posting_str ps;
                    if (nextPostingStr(ifstream_vec[i], ps))
                        pq.push(heap_entry<Util::Posting_str>{i, ps});
                    //else
                    //{
                    //    // if we were unable to read from the file, remove it from our list
                    //    //ifstreams[i].close();
                    //    ifstream_vec.erase(begin(ifstream_vec) + i);
                    //}
                }
                
                // do the merge by popping off our priority queue
                // keep the last seen posting so we can merge if necessary (i.e.
                // if a term and docid were split across two files, we can merge
                // them into one posting).
                Util::Posting_str cached_posting;
                Util::Posting_str temp2;
                auto temp = pq.top();
                cached_posting = temp.obj;
                pq.pop();
                if (nextPostingStr(ifstream_vec[temp.from_file_index], temp2))
                {
                    pq.push(heap_entry<Util::Posting_str>{temp.from_file_index, temp2});
                }
                while (pq.size() > 0)
                {
                    auto he = pq.top();
                    auto idx = he.from_file_index;
                    if (cached_posting == he.obj)
                        cached_posting.frequency += he.obj.frequency;
                    else
                    {
                        write_to_file(os, cached_posting, true);
                        cached_posting = he.obj;
                    }
                    Util::Posting_str ps;
                    pq.pop();
                    if (nextPostingStr(ifstream_vec[idx], ps))
                    {
                        pq.push(heap_entry<Util::Posting_str>{idx, ps});
                    }
                }
                write_to_file(os, cached_posting, true);
            }
            catch (exception& e)
            {
                os.flush();
                throw e;
            }
            counter++;
            outfilelist_os << filepath << '\n';
        }
    }
    std::cout << "Total time: " << chrono::duration_cast<chrono::duration<double>>(chrono::steady_clock::now() - timer1).count() << " seconds." << endl;
    return 0;
}