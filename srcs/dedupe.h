#ifndef DEDUPE_H_INCLUDED
#define DEDUPE_H_INCLUDED

#include <algorithm>
#include <vector>

template<class T>
void SortAndDedupe(std::vector<T> &v) {
  std::sort(v.begin(), v.end());
  v.erase(std::unique(v.begin(), v.end()), v.end());
}

#endif  // ndef DEDUPE_H_INCLUDED
