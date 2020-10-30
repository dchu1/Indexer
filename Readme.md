# Overview

This 32-bit application is designed to take a document and convert it into a inverted index structure. It is split into 3 modules: The Parser, Merger, and the IndexBuilder. All of these modules were written in C++ and built using MSVC 2019. These three modules follow the steps outlined in the course material:

1) Parse the documents, generate postings by inverting the words and document ids, sort the postings in RAM, and write them out to disk. Also write out the URL table.
2) Merge the postings documents using an I/O efficient merge sort
3) Scan the merged document and generate the compressed index and lexicon

# How to run
Inside the Release folder, there are 3 executables: Parser.exe, Mergesort.exe, and InvIndexBuilder.exe. Please see below for more details on usage for each executable. Please note that currently it is not possible to pipe the output from one executable to another.

# Parser.exe / Inverter (Src files found in the Indexer subfolder)

## Command line arguments

```
.\Parser.exe inputfile outputpath buffersize inverter windowsfile encode usetermids printout
    
    inputfile:  [string] the file to parse. Expecting a utf-16 encoding.
    outputpath: [string] the folder to place the intermediate posting documents
    buffersize: [int] the size of the buffer. Please note that this is not a hard rule, as the modules are not optimized for memory usage, so there are a lot of temporary data structures used.
    inverter:   [BSBI,SPIMI] Determines which inverter algorithm to use
    windowsfile:[0,1] A flag for whether the document was creating on Windows
    encode:     [0,1] A flag for whether to output the postings using varbyte encoding or use ASCII comma delimted file.
    usetermids: [0,1] A flag for whether to use termids. Only affects BSBI. Will probably be removed as it was just a stop gap until SPIMI was implemented.
    printout:   [int] Will print out a message and time after processing x documents

```

## Output Files
- output0.bin...outputN.bin - output files containing the postings
- outputlist.txt - a new line delimited ascii file containing the paths to all the output files. Note that if you used provided relative paths for outputpath, the paths in this text file will also be relative. This is the file you feed to Mergesort.exe
- urltable.txt - a new line and comma delimited ASCII file containing urls and word counts.

## Third party libraries
- [libxml2/libxml++](http://libxmlplusplus.sourceforge.net/) - used for HTML parsing
- [boost.algorithms.string](https://www.boost.org/) - used for splitting strings

## Description
The parser module does a simple loop to generate postings

1) Open a file stream to a file
2) Fetch a document
3) Pass the document to libxml2/libxml++ to parse the HTML
4) Iterate over the HTML DOM to get text to add to the bag of words
5) Write an entry in our url table as well as word count
6) Pass the bag of words and the document id to our inverter
7) Inverter will invert the bag of words and document id. Once its buffer is full (note that in neither SPIMI or BSBI implementations is the text counted as part of the buffer. They are held seperately in a map structure, and I don't keep track of that memory usage), it will collapse the postings to get a count, sort based on term and document id, and write the buffer out to a file. It can either be encoded using varbyte or as plain ASCII depending on the command line flag passed.
8) repeat for the next document until no more.

## Performance
- On the file provided, the SPIMI inverter takes about 2.5 hours. The BSBI inverter takes about 1.5 to 2 hours. While the BSBI inverter is faster, it also requires keeping the term/termid mapping in memory, which I was not able to do (as a 32-bit application, I only had access to 2 Gb memory). I would consider looking into more efficient hashing algorithms for storing the text. A lot of the time is spent on IO operations and inserting into the inverter's data structures. BSBI was most likely signifigantly faster because it used an unordered_map instead of an ordered map, as well as perhaps better linearized memory usage.
- Compressed using varbyte, the postings took up about 14.4 GB of memory. With a buffersize of 256 Mb, this resulted in 57 files.
- Memory usage is roughly 2x buffersize. This is mostly due to the map that holds the terms.
- Changing the size of the filestream buffers had little to no impact on the performance. Given the somewhat poor performance of the reads and writes, it might be worthwhile to try switching to C style IO.
  
## Known Issues
- I was unsure how to handle encoding properly. As a result, I converted everything to UTF-8, but c++ strings work on 7 bit ASCII characters. I treated everything as single byte strings with the assumption that it would still be encoded as UTF-8.
- Currently there is an assumption that the entire document will be read into memory. There's no requirement for this, it was just a simplyfying assumption I made. The parser should still work even if this was not the case, I would just need to adjust how document ids are handled slightly.

# Mergesort.exe (Source files found the in Mergesort subfolder)
```
./mergesort finlist foutpath degree

    finlist:  file that contains all the documents to be merged (input files are expected to be varbyte encoded)
    foutpath: path to place the resultant document(s)
    degree:   max number of files to merge at once
```
## Output Files
- merge0.bin...mergeN.bin - output files containing the merged postings (files are varbyte encoded)
- mergefiles.txt - a new line delimited ascii file containing the paths to all the merge files. Note that if you used provided relative paths for outputpath, the paths in this text file will also be relative. This is the file you feed back to Mergesort if you need multiple passes. Note that if you do that, you should specify a different foutpath as the files will be overwritten.
  
## Description
The Mergesort does an IO efficient merge of files. It is heavily based on the [sample](http://engineering.nyu.edu/~suel/cs6913/msort/mergephase.c) provided by the professor. However, mine is less memory efficient as it uses the default file stream buffers and std priority queue. Still, It uses very little memory.

Like the sample, this merge sort uses a priority queue of d entries, with each entry representing a file and that files smallest element. We simply loop through the priority queue, popping off the smallest element and inserting the next smallest from the file that the smallest element came from.

## Performance
- The Mergesort took about 50 minutes to do a 57 way merge into a single 14.4 gb file.
- Memory utilization was quite small as the most of the memory was just the d filestream buffers and the priority queue with d posting elements.
- Changing the size of the filestream buffers had little to no impact on the performance. Given the somewhat poor performance of the reads and writes, it might be worthwhile to try switching to C style IO.

# InvIndexBuilder.exe (Src files found in the InvIndexBuilder subfolder)
```
.\InvIndexBuilder.exe inputfile outputpath chunksize

    inputfile: the merged postings file (expected to be var byte encoded)
    outputpath: the path where the inverted index and lexicon will be written
    chunksize: the size of the chunk in terms of # of docids per chunk.
```
## Output Files
- lexicon.bin - this file contains the lexicon. It is a binary file varbyte encoded. Each entry is in the format: (byte position in the inverted index, # of docids, size of term in bytes, term).
- index.bin - this file contains the varbyte encoded inverted index. It is made up of chunks. Each chunk has the format (unencoded last docid in chunk, unencoded number of bytes in chunk, encoded docids (stored as difference - 1 from previous docid), encoded frequencies).

## Description
The InvIndexBuilder is very simple. It builds a chunk in memory. Once either the chunk is full, or a new word is read, it will flush the chunk to the index file. Since the merge file that it takes as input is already sorted by term and docid, all the index builder has to do is read each entry sequentially, building the chunk in memory. The only things it needs to keep track of between chunks is some file positions, and some counts of how many docids have been seen for the current term. It does not keep the lexicon in memory. Since the input file is sorted by term, once you've seen a term you will not see it again, so it is flushed to disk along with the position that it starts and the number of docids in it's inverted index.

## Performance
- It took about 23 minutes to turn a 14.4 Gb merged postings file into a 3.2 Gb index and 475 Mb lexicon.