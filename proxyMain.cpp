 #include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <utility>
#include <unordered_map>
#include <thread>
#include <mutex>
#include "proxyServerGET.cpp" 
#include "proxyServerGETold.hpp" 
#include "proxyServerCONNECT.cpp"
#include "proxyServerPOST.cpp"
#include "parseBuffer.hpp"
#include "logger.hpp"
#include "cache.hpp"
#include "cacheRes.hpp"

using namespace std;

mutex mtx;  // locker

class proxyMain{
  int proxySFD;
  int numOfRequests;
  // LRUCache cacheMain;
public:
  proxyMain(){
    proxySFD=getProxySFD();
    numOfRequests=100;
    //if (proxySFD==-1){
      
    //}
  }

  void sockaddrToIP(struct sockaddr * sa, char * s) {
    if (sa->sa_family == AF_INET) {
      inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr), s, INET6_ADDRSTRLEN);
    }
    else {
      inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr), s, INET6_ADDRSTRLEN);
    }
  }

  int mainThread(){
    LRUCache * cacheMain = new LRUCache(20);
    while(true){
      cout << "**Waiting for connection on port 12345**" << endl;
      struct sockaddr_storage socket_addr;
      socklen_t socket_addr_len = sizeof(socket_addr);
      int clientSFD;
      clientSFD = accept(proxySFD, (struct sockaddr *)&socket_addr, &socket_addr_len);
      char ip[INET6_ADDRSTRLEN];
      sockaddrToIP((struct sockaddr *)&socket_addr, ip);
      string clientIP=ip;
      cout << "**conected to client**" << endl;

      if (clientSFD == -1) {
      	cerr << "Error: cannot accept connection on socket" << endl;
      	return -1;
      }
      thread newthread(newThread,clientSFD,numOfRequests++,clientIP, cacheMain);
      newthread.detach();
    }
    return 1;
  }

  static void newThread(int clientSFD,int uniqueID,string clientIP, LRUCache* cache){
    cout<<"inside a new thread"<<endl;
    char * buffer=new char[65535];
    int numbytes;
    if((numbytes=recv(clientSFD,buffer,65535,0))==-1){
      cerr<<"Error: receive -1 bytes"<<endl;
      exit(1);
    }
    if(numbytes==0){
      exit(1);
    }
    logger log(uniqueID);
    parseBuffer pb(buffer);
    string hostname=pb.getHostName();
    string requestType=pb.getRequestType();
    string requestLine=pb.getFirstLine();
    string total=pb.getTotal();
    
    log.receiveNewRequest(clientIP,requestLine);
    //std::cout<<"***requestType:"<<requestType<<"***"<<endl;
    if (requestType=="GET"){
        //cout<<"***inside of GET***"<<endl;
      string request_Str=char_to_str(buffer,numbytes);
        //requestGET(clientSFD,request_Str,hostname,buffer,numbytes,&log);
      mtx.lock();
      cacheRes res = cache->get(requestLine);
      mtx.unlock();
      if (!res.isEmpty()) {
        cacheProcess(res, request_Str, clientSFD, hostname, buffer, numbytes, &log);
      }
      else {
        cout << "NOT USE CACHE, DIRECTLY TO SERVER" << endl;
        log.notInCache();
        proxyServerGET ps(hostname,buffer, numbytes,clientSFD,&log);
        ps.run();
        addToCache(ps.getBufFromServerStr(),requestLine, cache);   
      }
    }
      else if(requestType=="POST") {
        proxyServerGET ps(hostname,buffer, numbytes,clientSFD,&log);
        ps.run();
      }
      else if(requestType=="CONNECT"){
          proxyServerCONNECT ps(hostname,buffer,numbytes,clientSFD,&log);
          ps.run(); 
      }
      close(clientSFD);
    //delete[] buffer;
  }

  static string char_to_str(char * buffer, int numbytes){
    string ret;
    for(int i=0;i<numbytes;i++){
      ret.push_back(buffer[i]);
    }
    return ret;
  }


  static void makeValidate(cacheRes & res, string & req, int client_connection_fd, string hostname, char * buffer, int numbytes, logger * log) {
    // create new request, adding ifNoneMatch & ifModSince
    string etag = res.getEtag();   // W"aaa" or "aaabbbb"
    string ifNoneMatch = "If-None-Match: " + etag;
    string lastMod = res.getLastMod();
    string ifModSince = "If-Modified-Since: " + lastMod;
    size_t endIdx = req.find("\r\n\r\n");
    string newreq = req.substr(0, endIdx) + "\r\n" + ifNoneMatch + "\r\n" + ifModSince + "\r\n\r\n";
    size_t newEndIdx = newreq.find("\r\n\r\n");

    // validate with server
    // proxyServerGETold ps(getHostServer(), newreq.c_str(), newEndIdx, "80");
    proxyServerGET ps(hostname,buffer, numbytes,client_connection_fd,log);
  
    ps.sendToServer();
    ps.receiveFromServer();
    string httpResponse = ps.getBufFromServerStr();
    if (httpResponse.find("200") == string::npos) { // still valid, send back original content
      send(client_connection_fd, res.getTotal().c_str(), res.getTotal().size(), 0);
      cout << "THE CACHE IS STILL VALID, SEND CACHE BACK TO CLIENT" << endl;
    }
    else {  // new content
      send(client_connection_fd, httpResponse.c_str(), httpResponse.size(), 0);
      cout << "CACHE STALE, SEND NEW RESPONSE TO CLIENT" << endl;
    }
    // close(client_connection_fd);
  }

  // this function revalidate or directly send back cache content
   static void cacheProcess(cacheRes & res, string & req, int client_connection_fd, string hostname, char * buffer, int numbytes, logger * log) {
    long maxAge = res.retMaxAge();
    string date = res.getDate();
    string expires = res.getExp();
    string ctlContent = res.getCachaCtl();
    time_t curTime = time(NULL);   // convert EST to GMT
    cout << "curTime: " << curTime << endl;
    time_t expirTime;
    bool expTassigned = false;
    if (expires != "") {
      struct tm exp_struct;
      cout << "expires: " << expires << endl;
      strptime(expires.c_str(), "%a, %d %b %Y %H:%M:%S GMT", &exp_struct);
      cout << "tm_year: "<< exp_struct.tm_year << endl;
      cout << "tm_mon: " << exp_struct.tm_mon<< endl;
      cout << "tm_hour: " << exp_struct.tm_hour<< endl;
      expirTime = mktime(&exp_struct); 
      expTassigned = true;
      cout << "expirTime: " << expirTime << endl;
    }
    if (maxAge != -1 && date != "") {  // maxage exists
      struct tm date_struct;
      strptime(date.c_str(), "%a, %d %b %Y %H:%M:%S GMT", &date_struct);
      time_t createTime = mktime(&date_struct);
      cout << "createTime tm_year: "<< date_struct.tm_year << endl;
      cout << "createTime tm_mon: " << date_struct.tm_mon<< endl;
      cout << "createTime tm_hour: " << date_struct.tm_hour<< endl;
      expirTime = createTime + maxAge;
      expTassigned = true;

      cout << "MAXAGE: " << maxAge << endl;
      cout << "createTime: " << createTime << endl;
      cout << "expirTime: " << expirTime << endl;
    }

    // must revalidate
    if (res.isMustRevalidate() || (expTassigned && expirTime < curTime)) {
      cout << "MAKE REVALIDATION!!!!!!!!!!!!!!" << endl;
      if(res.isMustRevalidate()){
        log->revalidationCache();
      }else{
        log->expireCache(asctime(gmtime(&expirTime)));
      }
      makeValidate(res, req, client_connection_fd, hostname, buffer, numbytes, log);
    }
    else {  // directly send back original res to client
      send(client_connection_fd, res.getTotal().c_str(), res.getTotal().size(), 0);
      log->validCache();
      // close(client_connection_fd);
      cout << "FRESH, DIRECTLY SEND CACHE TO CLIENT" << endl;
    }
  }

  static void addToCache(string res, string req, LRUCache * cache) {
    lock_guard<mutex> lck(mtx);
    cacheRes maybeCache(res, false);
    string cacheCtlStr = maybeCache.getCacheCtlCnt();
    cout << "cacheCtlStr: " << cacheCtlStr << endl;
    if (cacheCtlStr != "no-store" && cacheCtlStr != "private") {
      cache->put(req, maybeCache);
      cout << "cache added:" << endl;
      cout << "key: " << req << endl;
      // cout << "value: " << getFirstLineReq(res) << endl;
    }
    else {
      cout << "NOT ADD TO CACHE" << endl;
    }
  }

  int getProxySFD(){
    int status;
    int socket_fd;
    struct addrinfo host_info;
    struct addrinfo *host_info_list;
    const char *hostname = NULL;
    const char *port     = "12345";

    memset(&host_info, 0, sizeof(host_info));

    host_info.ai_family   = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;
    host_info.ai_flags    = AI_PASSIVE;

    status = getaddrinfo(hostname, port, &host_info, &host_info_list);
    if (status != 0) {
      cerr << "Error: cannot get address info for host" << endl;
      cerr << "  (" << hostname << "," << port << ")" << endl;
      return -1;
    } //if

    socket_fd = socket(host_info_list->ai_family,
                       host_info_list->ai_socktype,
                       host_info_list->ai_protocol);
    if (socket_fd == -1) {
      cerr << "Error: cannot create socket" << endl;
      cerr << "  (" << hostname << "," << port << ")" << endl;
      return -1;
    } //if

    int yes = 1;
    status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
      cerr << "Error: cannot bind socket" << endl;
      cerr << "  (" << hostname << "," << port << ")" << endl;
      return -1;
    } //if

    status = listen(socket_fd, 100);
    if (status == -1) {
      cerr << "Error: cannot listen on socket" << endl;
      cerr << "  (" << hostname << "," << port << ")" << endl;
      return -1;
    } //if
    freeaddrinfo(host_info_list);
    return socket_fd;
  }
  
};

int main(){
  proxyMain proxymain;
  int res=proxymain.mainThread();
  if(res==-1){
    exit(1);
  }
}
