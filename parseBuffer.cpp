#include <stdio.h>

using namespace std;
class parseBuffer{
private:
  string total;
  string head;
  string content;
public:
  parseBuffer(char * buffer){
    total = buffer;
    //cout << "*****HTTP request:*****" << endl << total << endl;
    //cout << "**End of HTTP request**" << endl;
  }

  string getRequestType(){
    size_t firstBlank = total.find_first_of(" ");
    string ret = total.substr(0, firstBlank);
    return ret;
  }
  
  string getFirstLine(){
    size_t idx=total.find_first_of("\r\n");
    string firstLine=total.substr(0,idx);
    return firstLine;
  }

  string getHostName(){
    size_t hostIdx = total.find("Host:");
    size_t hostServerIdx = hostIdx + 6;
    size_t retIdx = total.find_first_of("\r\n", hostIdx);
    string hostName = total.substr(hostServerIdx, retIdx - hostServerIdx);
    return hostName;
  }

  long getBodyLen(){
    size_t idx1 = total.find("Content-Length");
    size_t idx2 = total.find_first_of("\r\n", idx1);
    string bodyLenStr= total.substr(idx1 + 16, idx2 - (idx1 + 16));
    long bodyLen = stol(bodyLenStr);
    return bodyLen;
  }

  int getHeadLen(){
    size_t idx = total.find("\r\n\r\n");
    return idx + 4;
  }
};
