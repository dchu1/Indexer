// Mergesort.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>
#include <vector>

int main(int argc, char* argv[])
{
    FILE* fin, * fout, * foutlist;
    vector<unsigned char> buf;        /* buffer holding records */
    int recSize, bufSize;      /* size of a record, and # of records in buffer */
    int numRecs, numFiles = 0; /* number of records, and # of output files */
    char filename[1024];
    int i;

    recSize = atoi(argv[1]);
    buf = (unsigned char*)malloc(atoi(argv[2]));
    bufSize = atoi(argv[2]) / recSize;

    ofstream os(argv[5], ofstream::binary);
    fin = fopen64(argv[3], "r");
    foutlist = fopen64(, "w");

    while (!feof(fin))
    {
        /* read data until buffer full or input file finished */
        for (numRecs = 0; numRecs < bufSize; numRecs++)
        {
            fread(&(buf[numRecs * recSize]), recSize, 1, fin);
            if (feof(fin))  break;
        }

        /* create output filename, store sorted data, then store the filename */
        if (numRecs > 0)
        {
            sprintf(filename, "%s%d", argv[4], numFiles);;
            fout = fopen64(filename, "w");
            qsort((void*)(buf), numRecs, recSize, intCompare);
            for (i = 0; i < numRecs; i++)
                fwrite(&buf[i * recSize], recSize, 1, fout);
            fclose(fout);
            fprintf(foutlist, "%s\n", filename);
            numFiles++;
        }
    }

    fclose(fin);
    fclose(foutlist);
    free(buf);
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
