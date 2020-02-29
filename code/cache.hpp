#ifndef CACHE
#define CACHE

#include <unordered_map>
#include <list>
#include <cstring>
#include <string>
#include <time.h>

#include "cacheRes.hpp"

using namespace std;

class LRUCache {
    int size;
    list<string> lst;
    unordered_map<string, cacheRes> kv;    // first line & response object
    unordered_map<string, list<string>::iterator> mp;
public:
    LRUCache(): size(0) {
    }

    LRUCache(int capacity) {
        size = capacity;
    }

    cacheRes get(string key) {
        if (kv.count(key) == 0) {
            cacheRes ret("", true);
            return ret;
        }
        updateLst(key);
        return kv[key];
    }

    void put(string key, cacheRes value) {
        if (lst.size() == size && kv.count(key) == 0) {
            evict();
        }
        updateLst(key);
        kv[key] = value;
    }

    void updateLst(string key) {
        if (kv.count(key) == 1) {
            lst.erase(mp[key]);
        }
        lst.push_front(key);
        mp[key] = lst.begin();
    }

    void evict() {
        mp.erase(lst.back());
        kv.erase(lst.back());
        lst.pop_back();
    }

    string cacheControlStr(string req) {
        size_t cachControlIdx = req.find("Cache-control");
        if (cachControlIdx == string::npos) {return "";}
        size_t nextNewLine = req.find("\r\n", cachControlIdx);
        size_t sz = strlen("Cache-control: ");
        return req.substr(cachControlIdx + sz, nextNewLine - (cachControlIdx + sz));
    }

    int getMaxAge(string res) {
        size_t maxAgeIdx = res.find("max-age");
        int maxAge;
        if (maxAgeIdx == string::npos) {
            maxAge = -1;
        }
        else {
            size_t nextNewLine = res.find("\r\n", maxAgeIdx);
            maxAge = stoi(res.substr(maxAgeIdx + 8, nextNewLine - (maxAgeIdx + 8)));
        }
        return maxAge;
    }

    


};

#endif








