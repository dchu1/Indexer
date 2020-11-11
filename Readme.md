# Overview

This 32-bit application is designed to take a document and convert it into a inverted index structure. It is split into 4 modules: The Parser, Merger, the IndexBuilder, and the QueryEngine. All of these modules were written in C++ and built using MSVC 2019. These four modules follow the steps outlined in the course material:

1) Parse the documents, generate postings by inverting the words and document ids, sort the postings in RAM, and write them out to disk. Also write out the URL table.
2) Merge the postings documents using an I/O efficient merge sort
3) Scan the merged document and generate the compressed index and lexicon
4) Read in the index file and lexicon file and generate queries using DAAT conjunctive and disjunctive query processing.

# How to run
Inside the Release folder, there are 4 executables: Parser.exe, Mergesort.exe, InvIndexBuilder.exe, and QueryEngine.exe. Please see below for more details on usage for each executable. Please note that currently it is not possible to pipe the output from one executable to another.

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
- pages.db - a sqlite3 db with a PAGES table, which has (id, url, body, size) columns.

## Third party libraries
- [libxml2/libxml++](http://libxmlplusplus.sourceforge.net/) - used for HTML parsing
- [boost.algorithms.string](https://www.boost.org/) - used for splitting strings
- [sqlite3](https://www.sqlite.org/cintro.html) - used for storing the URL data (id, url, body, size)

## Description
The parser module does a simple loop to generate postings. More details can be found below.

1) Open a file stream to a file
2) Fetch a document
3) Pass the document to libxml2/libxml++ to parse the HTML
4) Iterate over the HTML DOM to get text to add to the bag of words
5) Write an entry in our url db in the form (id, url, body, size)
6) Pass the bag of words and the document id to our inverter
7) Inverter will invert the bag of words and document id. Once its buffer is full (note that in neither SPIMI or BSBI implementations is the text counted as part of the buffer. They are held seperately in a map structure, and I don't keep track of that memory usage), it will collapse the postings to get a count, sort based on term and document id, and write the buffer out to a file. It can either be encoded using varbyte or as plain ASCII depending on the command line flag passed.
8) repeat for the next document until no more.

### HTML Parsing
Parsing of documents is done using the libxml2/libxml++ library. This library has limited ability to handle malformed html by attempting to insert tags where necessary. The parser creates a tree sructure representing the DOM. The application iterates through this tree structure recursively. It stops recursing down a branch if it enters a blacklisted tag (such as \<script\>, \<head\>, etc...). Any text node that it encounters is tokenized using the boost library, then added to our bag of words for the document. This bag of words is passed to the inverter to be turned into postings.

### BSBI Inverter
The BSBI inverter works by converting terms to termids, then generating postings. These postings are fully numeric, allowing for easy comparison and compression. Each posting is of the form (termid, docid, frequency). Note that the BSBI inverter counts all the word frequencies before generating the posting so that the postings are already compacted with frequency. Once it is ready to flush the postings to the file, it sorts the postings by termid, then docid, and writes them to a file.

### SPIMI Inverter
The SPIMI inverter uses a dimds std::map\<std::string,std::vector\<int\>\> to map terms to vectors of docids. Once it is ready to flush the postings to the file, it will iterate through the map, sorting each vector and compacting all docids to generate postings of the form (term, docid, frequency). Once each posting is generated, it is written to a filestream.

## Performance
- On the file provided, the SPIMI inverter takes about 2.5 hours. The BSBI inverter takes about 1.5 to 2 hours. While the BSBI inverter is faster, it also requires keeping the term/termid mapping in memory, which I was not able to do (as a 32-bit application, I only had access to 2 Gb memory). I would consider looking into more efficient hashing algorithms for storing the term/termid mapping. However, I think partly there are simply too many words being parsed. Even using a binary search tree would result in too much memory usage. About 50% of the time is spent on IO operations (reading from input file, writing postings to file). The second largest operation is tokenizing strings using the boost library. The third largest operation inserting into the inverter's data structures. BSBI was most likely signifigantly faster because it used an unordered_map instead of an ordered map, as well as perhaps better linearized memory usage.
- Compressed using varbyte, the postings took up about 14.4 GB of memory. With a buffersize of 256 Mb, this resulted in 57 files.
- Memory usage is roughly 2x buffersize. This is mostly due to the map that holds the terms.
- Changing the size of the filestream buffers had little to no impact on the performance. Given the somewhat poor performance of the reads and writes, it might be worthwhile to try switching to C style IO.
  
## Known Issues
- I was unsure how to handle encoding properly. As a result, I converted everything to UTF-8, but c++ strings work on 7 bit ASCII characters. I treated everything as single byte strings with the assumption that it would still be encoded as UTF-8.
- Currently there is an assumption that the entire document will be read into memory. There's no requirement for this, it was just a simplyfying assumption I made. The parser should still work even if this was not the case, I would just need to adjust how document ids are handled slightly.

# Mergesort.exe (Source files found the in Mergesort subfolder)

## Command line arguments
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

This merge sort maintains a filestream to d files. It uses a priority queue of d entries, with each entry representing a file and that files smallest element (since our postings files are already sorted by our Parser, this is simply the head of the file). We simply loop through the priority queue, popping off the smallest element and inserting the next smallest from the file that the smallest element came from. This continues until all filestreams have reached eof. It then continues until no more files are available. Note that newly generated files are not automatically found, so if you want multiple passes, you would run Mergesort multiple times.

## Performance
- The Mergesort took about 50 minutes to do a 57 way merge into a single 14.4 gb file.
- Memory utilization was quite small as the most of the memory was just the d filestream buffers and the priority queue with d posting elements.
- Changing the size of the filestream buffers had little to no impact on the performance. Given the somewhat poor performance of the reads and writes, it might be worthwhile to try switching to C style IO.

# InvIndexBuilder.exe (Source files found in the InvIndexBuilder subfolder)

## Command line arguments
```
.\InvIndexBuilder.exe inputfile outputpath chunksize

    inputfile: the merged postings file (expected to be var byte encoded)
    outputpath: the path where the inverted index and lexicon will be written
    chunksize: the size of the chunk in terms of # of docids per chunk.
```
## Output Files
- lexicon.bin - this file contains the lexicon. It is a binary file varbyte encoded. Each entry is in the format: (byte position in the inverted index, # of docids, size of term in bytes, term).
- index.bin - this file contains the varbyte encoded inverted index. It is made up of chunks. Each chunk has the format (unencoded last docid in chunk, unencoded number of bytes in chunk, encoded docids (stored as difference from previous docid), encoded frequencies).

## Description
The InvIndexBuilder builds an inverted list file and lexicon file. Because it of memory limitations, it uses a sparse lexicon, where terms that occur in fewer than 100 documents are written to the inverted list file instead of the lexicon file. It reads all the postings for a term into memory. Once it encounters a new term, it will write to the inverted list file and lexicon file. Since the merge file that it takes as input is already sorted by term and docid, all the index builder has to do is read each entry sequentially, building the chunka in memory. It does not keep the lexicon in memory. Since the input file is sorted by term, once you've seen a term you will not see it again, so it is flushed to disk along with the position that it starts and the number of docids in it's inverted index.

### Inverted Index File Structure
The inverted index has two types of entries. A normal entry is of the form (# of chunks, metadata array of [last docid of chunk 0, size of chunk 0...last docid of chunk N, size of chunk N], chunks of \[docids..., frequencies...\]). A sparse entry is of the form (0, # of documents containing term, # of chars in term, term, normal entry). It uses a 0 to indicate that it is a sparse entry rather than a normal entry (since a normal entry would never have 0 chunks).

### Lexicon File Structure
The lexicon file has entries of the form (position in inverted index, # of documents containing term, # of chars in term, term). Note that sparse entries that are written to the index file might also have an entry in the lexicon, except the term is replaced with a " ". This happens for the first sparse entry after and full entry. This is because I used the position in the inverted index of the next entry in the lexicon to determine the ending position of the previous term.

## Performance
- It took about 23 minutes to turn a 14.4 Gb merged postings file into a 3.2 Gb index and 475 Mb lexicon. However, when writing a sparse lexicon, it takes about 1 hour.

# QueryEngine.exe (Source files found in the QueryEngine subfolder)

## Command line arguments
```
.\QueryEngine.exe indexfile lexiconfile dbfile

    indexfile: the inverted list file
    lexiconfile: the lexicon file
    dbfile: the sqlite3 db for urls and pages
```

## Description
Read in the index file and lexicon file and generate queries using DAAT conjunctive and disjunctive query processing. It then gets the top 10 urls and generates snippets by scoring each sentence using BM25 and returning the top three sentences. The flow can be described as:

1) Read a query from the command prompt
2) Read whether it should be a conjunctive or disjunctive search
3) Open the relevant list pointers
   1) If conjunctive, iterate through the smallest inverted list using the nextGEQ function as stated in the slides. Score each document, and place it in a priority queue. Once the priority queue is full, check whether a new documents have a larger score than the smallest score in the priority queue. If so, replace it.
   2) If disjunctive, use a priority queue to hold the smallest document id from each term's inverted list. Pop off the smallest docid, replace it with the nextGEQ from all the ivnerted lists who had this smallest docid. Score the doc. If there are terms that are not present in the document, then they are not considered for the scoring (i.e. in BM25 they would just contribute 0 score). Like in the conjunctive case, a priority queue is used to store documents.
4) Once the top K documents are found, generate snippets by splitting each document on new lines, then scoring each sentence using BM25.
5) Print out the Url and Snippets to the console

### List Pointers
List Pointers contain all the relevant data about a term, including the term's compressed inverted list, an uncompressed chunk, and the # of documents that contain the term. These list pointers are cached using a Least Recently Used policy.

### Scoring
The QueryEngine uses BM25 scoring for both documents and sentences.