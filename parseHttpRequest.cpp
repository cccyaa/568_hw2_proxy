#include <stdio.h>

using namespace std;
class parseHttpRequest{
private:
  string requestTotal;
  string requestHead;
  string requestContent;
public:
  parseHttpRequest(char * buffer){
    requestTotal = buffer;
    cout << "*****HTTP request:*****" << endl << requestTotal << endl;
    cout << "**End of HTTP request**" << endl;
  }

  string getRequestType(){
    size_t firstBlank = requestTotal.find_first_of(" ");
    string ret = requestTotal.substr(0, firstBlank);
    return ret;
  }

  string getHostName(){
    size_t hostIdx = requestTotal.find("Host:");
    size_t hostServerIdx = hostIdx + 6;
    size_t retIdx = requestTotal.find_first_of("\r\n", hostIdx);
    string hostName = requestTotal.substr(hostServerIdx, retIdx - hostServerIdx);
    return hostName;
  }
};
