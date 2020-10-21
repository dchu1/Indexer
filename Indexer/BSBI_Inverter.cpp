#include "Inverter.h"

void BSBI_Inverter::invert(unsigned int docid, std::vector<std::string>& tokens)
{
    auto wordCount = std::unordered_map<unsigned int, unsigned int>{};
    // count words using range operator (c++ 11 feature)
    for (auto const& word : tokens)
    {
        // check if the word is in our term_termid_map. If not, add it
        if (_term_termid_map[word] == 0) _term_termid_map[word] = _curr_termid++;
        ++wordCount[_term_termid_map[word]];
    }

    // iterate over map using range operator (c++ 11 feature) and insert into
    // our buffer
    for (const auto& it : wordCount) {
        if (_buf.size() == _buf.capacity()) flush_buffer();
        _buf.push_back(Util::Posting{ it.first, docid, it.second });
    }
}

void BSBI_Inverter::flush_buffer()
{
    std::string filepath = outpath_ + "\\" + outprefix_ + std::to_string(curr_blockid_) + ".bin";
    ++curr_blockid_;
    std::cout << filepath << std::endl;

    // sort our buffer
    std::sort(begin(_buf), end(_buf));

    std::ofstream os(filepath, std::ios::out | std::ios::binary);

    // should we use termids
    if (_use_termids)
    {
        if (encode_)
        {
            std::vector<unsigned char> v = Util::encode((unsigned int*)_buf.data(), _buf.size() * 3);
            // write it out
            std::ostream_iterator<unsigned char> out_it(os);
            std::copy(v.begin(), v.end(), out_it);
        }
        else
        {
            // write it out. untested path
            os.write((char*)_buf.data(), _buf.size() * (size_t)sizeof(Util::Posting));
        }
    }   
    else
    {
        // create a reversed mapping of rev_term_termid_map
        std::unordered_map<unsigned int, std::string> rev_term_termid_map;
        for (const auto& it : _term_termid_map)
        {
            rev_term_termid_map[it.second] = it.first;
        }
        if (encode_)
        {
            for (const auto& it : _buf)
            {
                // write out the binary in the form of docid, freq, # of bytes of string, string
                // this is kind of dangerous as i'm mixing size_t and unsigned int which are not
                // guaranteed to be the same size of bytes. Should standardize on one or the other
                std::string& s = rev_term_termid_map[it.termid];
                size_t size = s.size(); 
                std::vector<unsigned char> v = Util::encode(&it.docid, 2);
                os.write((char*)v.data(), v.size());
                v = Util::encode(&size, 1);
                os.write((char*)v.data(), v.size());
                os.write(s.c_str(), size);
            }
        }
        else
        {
            for (const auto& it : _buf)
            {
                // write out the binary in the form of docid, freq, # of bytes of string, string
                // this is kind of dangerous as i'm mixing size_t and unsigned int which are not
                // guaranteed to be the same size of bytes. Should standardize on one or the other
                std::string& s = rev_term_termid_map[it.termid];
                size_t size = s.size();
                os.write((char*)&it.docid, sizeof(it.docid));
                os.write((char*)&it.frequency, sizeof(it.frequency));
                os.write((char*)&size, sizeof(size));
                os.write(s.c_str(), size);
            }
        }
        
        // https://stackoverflow.com/questions/42114044/how-to-release-unordered-map-memory
        std::unordered_map<std::string, unsigned int> empty;
        std::swap(_term_termid_map, empty);
    }
    
    // clear our buffer
    _buf.clear();

    // write filepath to file
    outputlist_ << filepath << '\n';
}

void BSBI_Inverter::flush_all()
{
    flush_buffer();
    if (_use_termids)
    {
        std::string filepath = outpath_ + "\\termmap.bin";
        std::ofstream os(filepath, std::ios::out | std::ios::binary);
        for (const auto& elem : _term_termid_map)
        {
            // write out the binary in the form of # of bytes of string, string, word count
            size_t size = elem.first.size();
            os.write((char*)&size, sizeof(size));
            os.write(elem.first.c_str(), size);
            os.write((char*)&elem.second, sizeof(elem.second));
        }
    }
}