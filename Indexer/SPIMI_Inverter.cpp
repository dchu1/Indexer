#include "Inverter.h"

void SPIMI_Inverter::invert(unsigned int docid, std::vector<std::string>& tokens)
{
    for (auto const& word : tokens)
    {
        _term_buf[word].push_back(docid);
    }
}

void SPIMI_Inverter::flush_buffer()
{

}

void SPIMI_Inverter::flush_all()
{

}