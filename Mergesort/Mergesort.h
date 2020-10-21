#pragma once

template <typename T>
struct heap_entry
{
    int from_file_index;
    T obj;
    bool operator<(const heap_entry<T>& y) const {
        return this->obj < y.obj;
    }
    bool operator>(const heap_entry<T>& y) const {
        return this->obj > y.obj;
    }
};

template <typename T>
class pq_comparator
{
private:
    bool reverse;
public:
    pq_comparator(const bool& revparam = false) :
        reverse{ revparam }
    {
    }
    bool operator() (const T& lhs, const T& rhs) const
    {
        if (reverse) return (lhs > rhs);
        else return (lhs < rhs);
    }
};