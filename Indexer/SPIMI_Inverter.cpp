#include "Inverter.h"
#include <chrono>

// inverts given docid and vector of strings 
void SPIMI_Inverter::invert(unsigned int docid, std::vector<std::string>& tokens)
{
    // iterate over each word and insert the docid into the associated vector
    for (auto const& word : tokens)
    {
        // if we have gotten docids to definitely gone over the desired
        // buffer size, flush the buffer. Note this is not an accurate measure
        // of used memory. It is just a rough estimate, but is actually only about
        // 50%
        if (_count*sizeof(unsigned int) == bufsize_) flush_buffer();

        // look to see if the word exists in our map. If not, we need to create a
        // new entry and vector
        auto it = _term_buf.find(word); 
        if (it != _term_buf.end())
            _term_buf[word].push_back(docid);
        else // otherwise just push the docid onto the vector
            _term_buf[word] = std::vector<unsigned int>{ docid };
        ++_count;
    }
}

// prints out the intermediate postings
void SPIMI_Inverter::flush_buffer()
{
    std::string filepath = outpath_ + "\\" + outprefix_ + std::to_string(curr_blockid_++) + ".bin";
    std::ofstream os(filepath, std::ios::out | std::ios::binary);
    std::cout << filepath << std::endl;

    // Increasing buffer size taken from:
    // https://stackoverflow.com/questions/11563963/how-to-write-a-large-buffer-into-a-binary-file-in-c-fast/39097696#39097696
    //const size_t bufsize = 1024 * 1024;
    //std::unique_ptr<char[]> buf(new char[bufsize]);
    //os.rdbuf()->pubsetbuf(buf.get(), bufsize);

    _count = 0;
    unsigned int counter = 0;
    
    // timers
    auto timer1 = std::chrono::steady_clock::now();
    auto writeTimer1 = std::chrono::steady_clock::now();
    double writeTime = 0, totalTime = 0;
    for (auto& it : _term_buf)
    {
        // special case for if we only have 1 entry, we don't need to do
        // anything but write it (this would probably be the majority)
        if (it.second.size() == 1)
        {
            writeTimer1 = std::chrono::steady_clock::now();
            write_to_file(os, it.first, it.second[0], 1);
            writeTime += std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - writeTimer1).count();
            continue;
        }

        // sort our term vector. This step isn't necessary given that our vectors
        // would already be sorted, but i have the code here just in case
        std::sort(begin(it.second), end(it.second));

        // compact our vector to get frequencies and write them out (every occurrence of
        // a word in a document generated an entry in our vector. We compact them here to
        // get a count.
        unsigned int curr_docid = it.second[0];
        for (auto const& i : it.second)
        {
            // if we are still looking at the same docid, increment our counter
            if (i == curr_docid)
                counter++;
            else // once we've reached a new docid, write our previous docid's posting
            {
                writeTimer1 = std::chrono::steady_clock::now();
                write_to_file(os, it.first, curr_docid, counter);
                writeTime += std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - writeTimer1).count();
                counter = 1;
                curr_docid = i;
            }
        }
        // the range loop misses the last docid, so need to write it out here
        writeTimer1 = std::chrono::steady_clock::now();
        write_to_file(os, it.first, curr_docid, counter);
        writeTime += std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - writeTimer1).count();
        counter = 0;
    }
    // clear term buf
    _term_buf.clear();
    outputlist_ << filepath << '\n';

    totalTime = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - timer1).count();
    std::cout << "Total Time: " << totalTime << "sec" << std::endl;
    std::cout << "Write Time: " << writeTime << "sec" << std::endl;
    std::cout << "Non-write Time: " << totalTime - writeTime << "sec" << std::endl;
}

void SPIMI_Inverter::write_to_file(std::ofstream& os, const std::string& s, const unsigned int& docid, const unsigned int& frequency)
{
    if (encode_)
    {
        // write out the binary in the form of docid, freq, # of bytes of string, string
        // this is kind of dangerous as i'm mixing size_t and unsigned int which are not
        // guaranteed to be the same size of bytes. Should standardize on one or the other
        size_t size = s.size();
        std::vector<unsigned char> v = Util::encode(&docid, 1);
        os.write((char*)v.data(), v.size());
        v = Util::encode(&frequency, 1);
        os.write((char*)v.data(), v.size());
        v = Util::encode(&size, 1);
        os.write((char*)v.data(), v.size());
        os.write(s.c_str(), size);
    }
    else
    {
        // write comma delimited ascii
        os << s << ',' << docid << ',' << frequency << '\n';
    }
}

void SPIMI_Inverter::flush_all()
{
    flush_buffer();
}