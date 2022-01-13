#ifndef UTIL_H
#define UTIL_H

#include <unordered_set>

namespace Util
{
    template<typename T>
    bool set_contains(std::unordered_set<T>* set, T value)
    {
        auto found = set->find(value);
        return found != set->end();
    };

};

template bool Util::set_contains<int>(std::unordered_set<int> *set, int value);

#endif
