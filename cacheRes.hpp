#ifndef CACHERES
#define CACHERES

#include <stdio.h>
#include <string>

using namespace std;
class cacheRes {
private:
  string total;
  string head;
  // string content;
  string cacheControl;
  string date;
  string expires;
  string etag;
  string lastMod;
  long maxAge;
  bool empty;
  bool mustRevalidate;

public:
  cacheRes():total(""), head(""), cacheControl(""), date(""), expires(""), etag(""), lastMod(""), maxAge(-1), empty(true), mustRevalidate(false) {}

  cacheRes(string buffer, bool isEmpty) {
    total = buffer;
    cacheControl = cacheControlStr(total);
    maxAge = calcMaxAge(total);
    empty = isEmpty;
    date = calcDate();
    expires = calcExp();
    mustRevalidate = needRevalidate();
    etag = calcEtag();
    lastMod = calcLastMod();
  }

  string getCacheCtlCnt() {
    return cacheControl;
  }

  string getTotal() {
    return total;
  }

  string getLastMod() {
    return lastMod;
  }

  string calcLastMod() {
    size_t lastModIdx = total.find("Last-Modified");
    if(lastModIdx == string::npos) {
      return "";
    }
    size_t nextNewLine = total.find("\r\n", lastModIdx);
    return total.substr(lastModIdx + 15, nextNewLine - (lastModIdx + 15));
  }

  string getEtag() {
    return etag;
  }

  string calcEtag() {
    size_t etagIdx = total.find("ETag");
    if(etagIdx == string::npos) {
      return "";
    }
    size_t nextNewLine = total.find("\r\n", etagIdx);
    return total.substr(etagIdx + 6, nextNewLine - (etagIdx + 6));
  }

  bool isMustRevalidate() {
    return mustRevalidate;
  }

  bool needRevalidate() {
    return (cacheControl == "no-cache" || cacheControl == "must-revalidate");
  }

  string getExp() {
    return expires;
  }

  string calcExp() {
    size_t expIdx = total.find("Expires");
    string ret;
    if (expIdx == string::npos) {
      ret = "";
    }
    else {
      size_t nextNewLine = total.find("\r\n", expIdx);
      ret = total.substr(expIdx + 9, nextNewLine - (expIdx + 9));
    }
    return ret;
  }

  string getDate() {
    return date;
  }

  string calcDate() {
    size_t dateIdx = total.find("Date");
    string ret;
    if (dateIdx == string::npos) {
      ret = "";
    }
    else {
      size_t nextNewLine = total.find("\r\n", dateIdx);
      ret = total.substr(dateIdx + 6, nextNewLine - (dateIdx + 6));
    }
    return ret;
  }

  bool isEmpty() {
    return empty;
  }

  string getCachaCtl() {
    return cacheControl;
  }

  int retMaxAge() {
    return maxAge;
  }

  int calcMaxAge(string res) {
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

  // return content of cache-control
  string cacheControlStr(string req) {
    size_t cachControlIdx = req.find("Cache-Control");
    if (cachControlIdx == string::npos) {return "";}
    size_t nextNewLine = req.find("\r\n", cachControlIdx);
    size_t sz = strlen("Cache-control: ");
    return req.substr(cachControlIdx + sz, nextNewLine - (cachControlIdx + sz));
  }

  string getRequestType() {
    size_t firstBlank = total.find_first_of(" ");
    string ret = total.substr(0, firstBlank);
    return ret;
  }

  string getHostName() {
    size_t hostIdx = total.find("Host:");
    size_t hostServerIdx = hostIdx + 6;
    size_t retIdx = total.find_first_of("\r\n", hostIdx);
    string hostName = total.substr(hostServerIdx, retIdx - hostServerIdx);
    return hostName;
  }

  long getBodyLen() {
    size_t idx1 = total.find("Content-Length");
    size_t idx2 = total.find_first_of("\r\n", idx1);
    string bodyLenStr = total.substr(idx1 + 16, idx2 - (idx1 + 16));
    long bodyLen = stol(bodyLenStr);
    return bodyLen;
  }

  int getHeadLen() {
    size_t idx = total.find("\r\n\r\n");
    return idx + 4;
  }
};

#endif