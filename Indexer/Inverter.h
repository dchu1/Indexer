#pragma once

#include <vector>
#include <string>
#include "Util.h"
#include <fstream>
#include <iostream>
#include <map>
#include <unordered_map>
#include <algorithm>

class Inverter
{
public:
	virtual void invert(unsigned int, std::vector<std::string>&) = 0;
	virtual void flush_buffer() = 0;
    virtual void flush_all() = 0;
protected:
	std::string outpath_;
	std::string outprefix_;
	unsigned int curr_blockid_;
    unsigned int bufsize_;
    bool encode_;
    std::ofstream outputlist_;
};

class BSBI_Inverter :
    public Inverter
{
public:
    BSBI_Inverter(std::string outpath, std::string outprefix, const int bufsize, const bool encode, const bool use_termids)
    {
        _buf.reserve(bufsize/sizeof(Util::Posting));
        bufsize_ = bufsize;
        outpath_ = outpath;
        outprefix_ = outprefix;
        outputlist_.open(outpath_ + "\\outputlist.txt", std::ios::out | std::ios::app);
        encode_ = encode;
        _use_termids = use_termids;
    }
    void invert(unsigned int, std::vector<std::string>&);
    void flush_buffer();
    void flush_all();
private:
    unsigned int _curr_termid = 1;
    std::vector<Util::Posting> _buf;
    std::unordered_map<std::string, unsigned int> _term_termid_map;
    bool _use_termids;
};

class SPIMI_Inverter :
    public Inverter
{
public:
    SPIMI_Inverter(std::string outpath, std::string outprefix, const int bufsize)
    {
        bufsize_ = bufsize;
        outpath_ = outpath;
        outprefix_ = outprefix;
    }
    void invert(unsigned int, std::vector<std::string>&);
    void flush_buffer();
    void flush_all();
private:
    std::unordered_map<std::string, std::vector<unsigned int>> _term_buf;
    unsigned int _memused;
};
