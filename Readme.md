# Overview

This 32-bit application is designed to take a document and convert it into a inverted index structure. It is split into 4 modules: The Parser, Merger, the IndexBuilder, and the QueryEngine. All of these modules were written in C++ and built using MSVC 2019. These four modules follow the steps outlined in the course material:

1) Parse the documents, generate postings by inverting the words and document ids, sort the postings in RAM, and write them out to disk. Also write out the URL table.
2) Merge the postings documents using an I/O efficient merge sort
3) Scan the merged document and generate the compressed index and lexicon
4) Read in the index file and lexicon file and generate queries using DAAT conjunctive and disjunctive query processing.

# How to run
Inside the Release folder, there are 4 executables: Parser.exe, Mergesort.exe, InvIndexBuilder.exe, and QueryEngine.exe. Please see below for more details on usage for each executable. Please note that currently it is not possible to pipe the output from one executable to another.

# Parser.exe / Inverter (Source files found in the Indexer subfolder)

## Command line arguments

```
.\Parser.exe inputfile outputpath buffersize inverter windowsfile encode usetermids printout
    
    inputfile:  [string] the file to parse. Expecting a utf-16 encoding.
    outputpath: [string] the folder to place the intermediate posting documents
    buffersize: [int] the size of the buffer. Please note that this is not a hard rule, as the modules are not optimized for memory usage, so there are a lot of temporary data structures used.
    inverter:   [BSBI,SPIMI] Determines which inverter algorithm to use
    windowsfile:[0,1] A flag for whether the document was creating on Windows
    encode:     [0,1] A flag for whether to output the postings using varbyte encoding or use ASCII comma delimted file.
    printout:   [int] Will print out a message and time after processing x documents

```

## Output Files
- output0.bin...outputN.bin - output files containing the postings
- outputlist.txt - a new line delimited ascii file containing the paths to all the output files. Note that if you used provided relative paths for outputpath, the paths in this text file will also be relative (i.e. if you passed ../inputfile, the path will be written as ../output0). This is the file you feed to Mergesort.exe
- pages.db - a sqlite3 db with a PAGES table, which has (id, url, body, size) columns.

## Third party libraries
- [libxml2/libxml++](http://libxmlplusplus.sourceforge.net/) - used for HTML parsing
- [boost.algorithms.string](https://www.boost.org/) - used for splitting strings
- [sqlite3](https://www.sqlite.org/cintro.html) - used for storing the URL data (id, url, body, size)

## Description
The parser module does a simple loop to generate postings. More details can be found below.

1) Open a file stream to a file
2) Fetch a string that represents a document
3) Pass the document to libxml2/libxml++ to parse the HTML
4) Iterate over the HTML DOM to get text to add to the bag of words
5) Write an entry in our url db in the form (id, url, body, size)
6) Pass the bag of words and the document id to our inverter
7) Inverter will invert the bag of words and document id. Once its buffer is full (note that in neither SPIMI or BSBI implementations is the text counted as part of the buffer. They are held seperately in a map structure, and I don't keep track of that memory usage), it will collapse the postings to get a count, sort based on term and document id, and write the buffer out to a file. It can either be encoded using varbyte or as plain ASCII depending on the command line flag passed.
8) repeat for the next document until no more.

## Psuedocode
Main:
```
for document in file:
    parse document // returns a vector of words
    invert document id, words
```

BSBI Inverter:
```
For word in words:
    freq = sum of occurrences
    termid = termid_map[word]
    postings.push_back(posting{termid, docid, freq})
    if postings full:
        write_to_file(postings)
```

SPIMI Inverter:
```
For word in words:
    map[word].push_back(document id)
    if map full:
        for entry in map
            while document id == previous document id
                frequency++
            write_to_file(term, document id, frequency)
```
### Postings structure
Postings are written to a varbyte encoded file as (docid, frequency, size of string, string).

### URL table
URLs are written to a Sqlite3 database table with the command: 
```
CREATE TABLE PAGES(ID INT PRIMARY KEY NOT NULL,URL TEXT NOT NULL,BODY TEXT NOT NULL,SIZE INTEGER NOT NULL)
```

### HTML Parsing
Parsing of documents is done using the libxml2/libxml++ library. This library has limited ability to handle malformed html by attempting to insert tags where necessary. The parser creates a tree sructure representing the DOM. The application iterates through this tree structure recursively. It stops recursing down a branch if it enters a blacklisted tag (such as \<script\>, \<head\>, etc...). Any text node that it encounters is tokenized using the boost library, then added to our bag of words for the document. Once the entire DOM has been recursed, the bag of words is returned.

### BSBI Inverter
The BSBI inverter works by converting terms to termids, then generating postings. These postings are fully numeric, allowing for easy comparison and compression. Each posting is of the form (termid, docid, frequency). Note that the BSBI inverter counts all the word frequencies before generating the posting so that the postings are already compacted with frequency. Once it is ready to flush the postings to the file, it sorts the postings by termid, then docid, and writes them to a file.

### SPIMI Inverter
The SPIMI inverter uses a dimds std::map\<std::string,std::vector\<int\>\> to map terms to vectors of docids. Once it is ready to flush the postings to the file, it will iterate through the map, sorting each vector and compacting all docids to generate postings of the form (term, docid, frequency). Once each posting is generated, it is written to a filestream.

## Performance
- On the file provided, the SPIMI inverter takes about 2.5 hours. The BSBI inverter takes about 1.5 to 2 hours. While the BSBI inverter is faster, it also requires keeping the term/termid mapping in memory, which I was not able to do (as a 32-bit application, I only had access to 2 Gb memory). I would consider looking into more efficient hashing algorithms for storing the term/termid mapping. However, I think partly there are simply too many words being parsed. Even using a binary search tree would result in too much memory usage. About 50% of the time is spent on IO operations (reading from input file, writing postings to file). The second largest operation is tokenizing strings using the boost library. The third largest operation inserting into the inverter's data structures. BSBI was most likely signifigantly faster because it used an unordered_map instead of an ordered map, as well as perhaps better linearized memory usage.
- Compressed using varbyte, the postings took up about 14.4 GB of memory. With a buffersize of 256 Mb, this resulted in 57 files.
- Memory usage is roughly 2x buffersize. Most memory is used by the map that holds the terms and the vector that holds the postings.
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

This merge sort maintains a filestream to d files. It uses a priority queue of d entries, with each entry representing a file and that files smallest element (since our postings files are already sorted by our Parser, this is simply the head of the file). We simply loop through the priority queue, popping off the smallest element, writing the element out to a new file, and inserting the next smallest from the file that the smallest element came from. This continues until all filestreams have reached eof. It then reads in more files and starts the merge process again, writing to a new file, until no more files are available. Note that newly generated files are not automatically found, so if you want multiple passes, you would run Mergesort multiple times.

## Psuedocode
```
for each filestream:
    heap.push top entry

while heap !empty
    posting_i = heap.pop
    if posting's term and document id == previous term and document id:
        sum frequencies
    write posting to file
    heap.push next posting from file_i
```

## Performance
- The Mergesort took about 50 minutes to do a 57 way merge into a single 14.4 gb file.
- Memory utilization was quite small as the most of the memory was just the d filestream buffers and the priority queue with d posting elements.
- Changing the size of the filestream buffers had little to no impact on the performance. Given the somewhat poor performance of the reads and writes, it might be worthwhile to try switching to C style IO.

# InvIndexBuilder.exe (Source files found in the InvIndexBuilder subfolder)

## Command line arguments
```
.\InvIndexBuilder.exe inputfile outputpath

    inputfile: the merged postings file (expected to be var byte encoded)
    outputpath: the path where the inverted index and lexicon will be written
```
## Output Files
- lexicon.bin - this file contains the lexicon. It is a binary file varbyte encoded. Each entry is in the format: (byte position in the inverted index, # of docids, size of term in bytes, term).
- index.bin - this file contains the varbyte encoded inverted index. It is made up of chunks. Each chunk has the format (unencoded last docid in chunk, unencoded number of bytes in chunk, encoded docids (stored as difference from previous docid), encoded frequencies).

## Description
The InvIndexBuilder builds an inverted list file and lexicon file. Because of memory limitations, it uses a sparse lexicon, where terms that occur in fewer than 50 documents or more than 500 postings have been read since the last full lexicon entry are written to the inverted list file instead of the lexicon file. It reads all the postings for a term into memory. Once it encounters a new term, it will write to the inverted list file and lexicon file. Since the merge file that it takes as input is already sorted by term and docid, all the index builder has to do is read each entry sequentially, building the chunk in memory. It does not keep the lexicon in memory. Since the input file is sorted by term, once you've seen a term you will not see it again, so it is flushed to disk along with the position that it starts and the number of docids in it's inverted index.

## Psuedocode
```
for Posting in Postings:
    if Posting term is same as previous term:
        docids.push_back(Posting.docid)
        frequencies.push_back(Posting.frequency)
    else
        split_into_chunks_and_encode(docids, frequencies)
        write_to_index()
        write_to_lexicon()
```
### Inverted Index File Structure
The inverted index has two types of entries. A normal entry is of the form:

 (# of chunks, metadata array of [last docid of chunk 0, size of chunk 0...last docid of chunk N, size of chunk N], N chunks of \[docids..., frequencies...\]). 
 
 A sparse entry is of the form: 
 
 (0, # of documents containing term, # of chars in term, term, normal entry). 
 
 It uses a 0 byte to indicate that it is a sparse entry rather than a normal entry (since a normal entry would never have 0 chunks). This allows us to determine whether we are about to read a normal or sparse entry.

### Lexicon File Structure
The lexicon file has entries of the form:

(position in inverted index, # of documents containing term, # of chars in term, term). 

Note that sparse entries that are written to the index file might also have an entry in the lexicon, except the term is replaced with a " ". This happens for the first sparse entry after and full entry. This is because I used the position in the inverted index of the next entry in the lexicon to determine the ending position of the previous term.

## Performance
- It took about 23 minutes to turn a 14.4 Gb merged postings file into a 3 Gb index and 475 Mb lexicon. However, when writing a sparse lexicon, it takes about 1 hour.
- Using a sparse lexicon, it takes abotu 30 minutes to turn a 14.4 Gb merged postings file into a 3.5 Gb index and a 10 Mb lexicon. 

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
4) Search through the document ids
   1) If conjunctive, iterate through the smallest inverted list using the nextGEQ function as stated in the slides. Score each document, and place it in a priority queue. Once the priority queue is full, check whether a new documents have a larger score than the smallest score in the priority queue. If so, replace it.
   2) If disjunctive, use a priority queue to hold the smallest document id from each term's inverted list. Pop off the smallest docid, replace it with the nextGEQ from all the ivnerted lists who had this smallest docid. Score the doc. If there are terms that are not present in the document, then they are not considered for the scoring (i.e. in BM25 they would just contribute 0 score). Like in the conjunctive case, a priority queue is used to store documents.
5) Once the top K documents are found, generate snippets by splitting each document on new lines, then scoring each sentence using BM25.
6) Print out the Url and Snippets to the console

## Psuedocode
nextGEQ(ListPointer, int k):
```
chunk metadata.find(chunk whose last docid <= k)
if !found: return int.MAX

if !currently decoded:
    decode chunk

for each docid in chunk:
    if docid >= k:
        break

return docid
```

getTopKConjunctive(query terms, int k)
```
lp_vector = loadList(query terms)
sort lp_vector
curr_docid = lp_vector[0]
while curr_docid <= lp_vector[0].max_docid:
    args.push_back(curr_docid.size)
    for (i = 1; i < lp_vector.size' i++):
        if d = nextGEQ(lp_vector[i], curr_docid) != curr_docid
            curr_docid = nextGEQ(lp_vector[0], d)
            break;
        args.push_back(lp_vector[i].fdt, getFreq(lp_vector[i]))
    score(args)
    heap.push(score, curr_docid)

return heap.pop() x 10
```

getTopKDisjunctive(query terms, int k)
```
loadList(query terms)
for each ListPointer:
    heap.push nextGEQ(ListPointer, 1)

while heap !empty:
    curr_docid = heap.pop()
    args.push_back(curr_docid.size)
    for each ListPointer
        if ListPointer.curr_docid == curr_docid:
            args.push_back(lp_vector[i].fdt, getFreq(lp_vector[i]))
    score(args)
    heap.push(score, curr_docid)

return heap.pop() x 10
```

### Initializing the Query Engine
1) Load the Lexicon into memory. Lexicon is stored in two vectors. One vector\<string\>, which holds the terms. One vector\<LexiconMetadata\> which holds an object that represents the metadata about the term (position in index file, # of docids, position of next entry in index file). To find a term's metadata we do a binary search of our string vector (which is sorted since we insert into the vector the same order as the lexicon file, which is sorted) to get the index in our term metadata
2) Load the SQLite3 DB. Make an SQL query to cache the # of rows and to cache the size of each document id in a vector\<unsigned int\>.

### Open the relevant list pointers
List Pointers contain all the relevant data about a term, including the term's compressed inverted list, an uncompressed chunk, metadata about each chunk (largest docid, size of chunk) and the # of documents that contain the term. These list pointers are cached using a Least Recently Used policy, with a cache size of 100 ListPointers. When we process a query, we call loadList(words), which will load the word's list pointers into the cache if not already present.

### nextGEQ(ListPointer, int k) function
We find document ids using the nextGEQ function. This function first looks at a ListPointer's chunk metadata to find the chunk whose last document id is greater than or equal to k. It then checks whether that chunk is the currently decoded chunk. If not, decode the correct chunk into the ListPointer's chunk cache. Finally iterate through the decoded chunk until we reach a document id that is >= to k. If not such chunk exists, return the maximum value.

### Getting Top K Conjunctive Results
We create a vector of ListPointers from our ListPointer cache. We then sort the vector by inverted list size so that the first element is the ListPointer with the smallest inverted list. We then get the smallest docid in the smallest ListPointer, curr_docid. We search through the rest of the ListPointer vector for a ListPointer who has a docid greater than or equal to curr_docid using the nextGEQ() function. If it finds a document id, d, that is larger than curr_docid, we search for a docid in our smallest list that is greater than or equal to d. This repeats until we find a document id that is common among all the lists.

Once we have found a common document id, we will score that document id, and push the document id into a min-heap (ordered by score). If the size of the heap is greater than k, we will replace the minimum docid.

Finally we reverse the heap and return it.

### Getting Top K Disjunctive Results
We create a min heap of document ids. From each term's ListPointer, we push onto the heap the smallest docid. We then pop the smallest docid in the heap, curr_docid. We then score the curr_docid using the terms that occur in that docid.

We then push the document id into a min-heap (ordered by score). We then get the next document id from the ListPointers that contributed to the previous curr_docid using nextGEQ().

Finally we reverse the heap and return it.

### Scoring
The QueryEngine uses BM25 scoring for both documents and sentences.

### Snippet Generation
Documents are split into "sentences" by splitting on new line characters ('\r\n'). There are probably better ways to split, but given our data this seemed a decent compromise. We then carry out the BM25 scoring function, with sentences taking the place of documents. So instead of N being # of documents, it is # of sentences. |Davg| is average sentence length. |D| is the number of words in the document. Ft is the frequency of a term in a sentence. Fdt is the number of sentences that contain the term. Finally, we get the top scoring sentence, as well as 4 surrounding sentences.

## Performance
Starting the query engine takes a little less than 10 minutes on my laptop. Most of the startup time is spent on making the queries to the Sqlite3 db and creating our cache.

## Known or Suspected issues
- 
- The exception handling is quite messy, as it wasn't until recently that I understood how to properly do try/catch in c++. Therefore it is possible that certain excpetions I thought were caught actually do not catch the exceptions, causing the application to crash.