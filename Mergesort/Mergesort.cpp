// Mergesort.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "Util.h"

using namespace std;

struct filebuf {
    ifstream is;

};
void read_urltable(const string& filepath)
{
    auto url_table = vector<pair<string, unsigned int>>{};
    ifstream is(filepath, ios::in | ios::binary);
    size_t stringsize;
    string url;
    unsigned int size;
    while (is.read((char*)&stringsize, sizeof(size_t)))
    {
        url.resize(stringsize);
        is.read((char*)url.c_str(), stringsize);
        is.read((char*)&size, sizeof(unsigned int));
        cout << url << ", " << size << '\n';
        url_table.push_back(pair<string, unsigned int>{ url, size });
    }
}

//Util::Posting nextPosting(ifstream* is)
//{
//    int counter = 0;
//
//}

// ./mergesort finlist foutlist degree memsize
int main(int argc, char* argv[])
{
    string line;
    int memSize;
    int maxDegree, degree;
    int numFiles = 0;

    memSize = stoi(argv[4]);
    maxDegree = stoi(argv[3]);
    char* bufSpace = new char[memSize];
    int bufSize = memSize / (maxDegree + 1);
    vector<ifstream*> ifstreams;
    ifstreams.reserve(maxDegree);
    ifstream is(argv[1], ios::in);
    if (is)
    {
        auto os = new ofstream();
        os->rdbuf()->pubsetbuf(bufSpace + (maxDegree * bufSize), memSize - (maxDegree * bufSize));
        os->open(argv[2], ios::out | ios::binary);
        degree = 0;
        while (getline(is, line))
        {
            auto temp = new ifstream();
            temp->rdbuf()->pubsetbuf(bufSpace+(degree*bufSize), bufSize);
            temp->open(line, ios::in | ios::binary);
            ifstreams.push_back(temp);
            ++degree;
            // if we've read in all our merge files, start merging
            if (degree == (maxDegree - 1))
            {

            }
        }
        
    }

    
    //FILE* finlist, * foutlist;  /* files with lists of in/output file names */
    //int memSize;             /* available memory for merge buffers (in bytes) */
    //int maxDegree, degree;   /* max allowed merge degree, and actual degree */
    //int numFiles = 0;        /* # of output files that are generated */
    //char* bufSpace;
    //char filename[1024];
    //void heapify();
    //void writeRecord();
    //int nextRecord();
    //int i;

    //recSize = atoi(argv[1]);
    //memSize = atoi(argv[2]);
    //bufSpace = (unsigned char*)malloc(memSize);
    //maxDegree = atoi(argv[3]);
    //ioBufs = (buffer*)malloc((maxDegree + 1) * sizeof(buffer));
    //heap.arr = (int*)malloc((maxDegree + 1) * sizeof(int));
    //heap.cache = (void*)malloc(maxDegree * recSize);

    //finlist = fopen64(argv[4], "r");
    //foutlist = fopen64(argv[6], "w");

    //while (!feof(finlist))
    //{
    //    for (degree = 0; degree < maxDegree; degree++)
    //    {
    //        fscanf(finlist, "%s", filename);
    //        if (feof(finlist)) break;
    //        ioBufs[degree].f = fopen64(filename, "r");
    //    }
    //    if (degree == 0) break;

    //    /* open output file (output is handled by the buffer ioBufs[degree]) */
    //    sprintf(filename, "%s%d", argv[5], numFiles);
    //    ioBufs[degree].f = fopen64(filename, "w");

    //    /* assign buffer space (all buffers same space) and init to empty */
    //    bufSize = memSize / ((degree + 1) * recSize);
    //    for (i = 0; i <= degree; i++)
    //    {
    //        ioBufs[i].buf = &(bufSpace[i * bufSize * recSize]);
    //        ioBufs[i].curRec = 0;
    //        ioBufs[i].numRec = 0;
    //    }

    //    /* initialize heap with first elements. Heap root is in heap[1] (not 0) */
    //    heap.size = degree;
    //    for (i = 0; i < degree; i++)  heap.arr[i + 1] = nextRecord(i);
    //    for (i = degree; i > 0; i--)  heapify(i);

    //    /* now do the merge - ridiculously simple: do 2 steps until heap empty */
    //    while (heap.size > 0)
    //    {
    //        /* copy the record corresponding to the minimum to the output */
    //        writeRecord(&(ioBufs[degree]), heap.arr[1]);

    //        /* replace minimum in heap by the next record from that file */
    //        if (nextRecord(heap.arr[1]) == -1)
    //            heap.arr[1] = heap.arr[heap.size--];     /* if EOF, shrink heap by 1 */
    //        if (heap.size > 1)  heapify(1);
    //    }

    //    /* flush output, add output file to list, close in/output files, and next */
    //    writeRecord(&(ioBufs[degree]), -1);
    //    fprintf(foutlist, "%s\n", filename);
    //    for (i = 0; i <= degree; i++)  fclose(ioBufs[i].f);
    //    numFiles++;
    //}

    //fclose(finlist);
    //fclose(foutlist);
    //free(ioBufs);
    //free(heap.arr);
    //free(heap.cache);
    return 0;
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
