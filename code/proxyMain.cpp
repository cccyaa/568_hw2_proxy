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
#include "proxyServerCONNECT.cpp"
#include "parseBuffer.hpp"
#include "logger.hpp"
#include "cache.hpp"
#include "cacheRes.hpp"
#include "exceptions.hpp"

using namespace std;

mutex mtx;  // locker
mainException mException;

class proxyMain{
  int proxySFD;
  int numOfRequests;
public:
  proxyMain(){
    proxySFD=getProxySFD();
    numOfRequests=100;
  }

  void sockaddrToIP(struct sockaddr * sa, char * s) {
    if (sa->sa_family == AF_INET) {
      inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr), s, INET6_ADDRSTRLEN);
    }
    else {
      inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr), s, INET6_ADDRSTRLEN);
    }
  }

  void mainThread(){
    LRUCache * cacheMain = new LRUCache(20);
    while(true){
      cout << "Waiting for connection on port 12345" << endl;
      struct sockaddr_storage socket_addr;
      socklen_t socket_addr_len = sizeof(socket_addr);
      int clientSFD;
      clientSFD = accept(proxySFD, (struct sockaddr *)&socket_addr, &socket_addr_len);
      char ip[INET6_ADDRSTRLEN];
      sockaddrToIP((struct sockaddr *)&socket_addr, ip);
      string clientIP=ip;
      cout << "Conected to client" << endl;

      if (clientSFD == -1) {
        throw mException;
      }

      thread newthread(newThread,clientSFD,numOfRequests++,clientIP, cacheMain);
      newthread.detach();

    }
  }

  static void newThread(int clientSFD,int uniqueID,string clientIP, LRUCache* cache){
    cout<<"inside a new thread"<<endl;
    char * buffer=new char[65535];
    int numbytes;
    if((numbytes=recv(clientSFD,buffer,65535,0))==-1){
      cerr<<"Error: receive -1 bytes"<<endl;
      return;
    }
    if(numbytes==0){
      cerr<<"Error: receive 0 bytes"<<endl;
      return;
    }
    logger log(uniqueID);
    parseBuffer pb(buffer);
    string hostname=pb.getHostName();
    string requestType=pb.getRequestType();
    string requestLine=pb.getFirstLine();
    string total=pb.getTotal();
    
    log.receiveNewRequest(clientIP,requestLine);
    if (requestType=="GET"){
      string request_Str=char_to_str(buffer,numbytes);
      mtx.lock();
      cacheRes res = cache->get(requestLine);
      mtx.unlock();
      if (!res.isEmpty()) {
        cacheProcess(res, request_Str, clientSFD, hostname, buffer, numbytes, &log);
      }
      else {
        log.noteMessage("not use cache, directly to server");
        log.notInCache();
        try{
          proxyServerGET ps(hostname,buffer, numbytes,clientSFD,&log);
          ps.run();
          addToCache(ps.getBufFromServerStr(),requestLine, cache,&log); 
        }catch(exception e){
          cerr<<e.what();
          return;
        }
        log.noteMessage("add to cache");
      }
      close(clientSFD);
    }
    else if(requestType=="POST") {
      try{
       proxyServerGET ps(hostname,buffer, numbytes,clientSFD,&log);
       ps.run();
     }catch(exception e){
      cerr<<e.what();
      return;
    }
    
    close(clientSFD);
  }
  else if(requestType=="CONNECT"){
    try{
      proxyServerCONNECT ps(hostname,buffer,numbytes,clientSFD,&log);
      ps.run(); 
    }catch(exception e){
      cerr<<e.what();
      return;
    }
  }
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
    string etag = res.getEtag();   // W"aaa" or "aaabbb"
    string ifNoneMatch = "If-None-Match: " + etag;
    string lastMod = res.getLastMod();
    string ifModSince = "If-Modified-Since: " + lastMod;
    size_t endIdx = req.find("\r\n\r\n");
    string newreq = req.substr(0, endIdx) + "\r\n" + ifNoneMatch + "\r\n" + ifModSince + "\r\n\r\n";
    size_t newEndIdx = newreq.find("\r\n\r\n");
    // validate with server
    proxyServerGET ps(hostname,buffer, numbytes,client_connection_fd,log);

    ps.sendToServer();
    ps.receiveFromServer();
    string httpResponse = ps.getBufFromServerStr();
    if (httpResponse.find("200") == string::npos) { // still valid, send back original content
      send(client_connection_fd, res.getTotal().c_str(), res.getTotal().size(), 0);
      log->noteMessage("the cache is still valid, sent it back to client");
    }
    else {  // new content
      send(client_connection_fd, httpResponse.c_str(), httpResponse.size(), 0);
      log->noteMessage("cache stale, send new response to client");
    }
  }

  // this function revalidate or directly send back cache content
  static void cacheProcess(cacheRes & res, string & req, int client_connection_fd, string hostname, char * buffer, int numbytes, logger * log) {
    long maxAge = res.retMaxAge();
    string date = res.getDate();
    string expires = res.getExp();
    string ctlContent = res.getCachaCtl();
    time_t curTime = time(NULL);   // convert EST to GMT
    time_t expirTime;
    bool expTassigned = false;
    if (expires != "") {
      struct tm exp_struct;
      size_t lstBlk = expires.find_last_of(" ");
      strptime(expires.substr(0, lstBlk).c_str(), "%a, %d %b %Y %H:%M:%S", &exp_struct);
      expirTime = mktime(&exp_struct); 
      expTassigned = true;
    }
    if (maxAge != -1 && date != "") {  // maxage exists
      struct tm date_struct;
      size_t lstBlk = expires.find_last_of(" ");
      strptime(date.substr(0, lstBlk).c_str(), "%a, %d %b %Y %H:%M:%S", &date_struct);
      time_t createTime = mktime(&date_struct) - 3600 * 5;   // reduce 5 hours
      expirTime = createTime + maxAge;
      expTassigned = true;
    }

    // must revalidate
    if (res.isMustRevalidate() || (expTassigned && expirTime < curTime)) {
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
      log->noteMessage("fresh, directly send cache to client");
    }
  }

  static void addToCache(string res, string req, LRUCache * cache,logger * log) {
    lock_guard<mutex> lck(mtx);
    cacheRes maybeCache(res, false);
    string cacheCtlStr = maybeCache.getCacheCtlCnt();
    log->noteMessage("cache control: "+cacheCtlStr);
    if (cacheCtlStr != "no-store" && cacheCtlStr != "private") {
      cache->put(req, maybeCache);
      if(maybeCache.getExp() != "") {
         log->cacheButExpires(maybeCache.getExp());
      }
      else if(maybeCache.retMaxAge() != -1 && maybeCache.getDate() != ""){
        long maxAge = maybeCache.retMaxAge();
        string date = maybeCache.getDate();
        struct tm date_struct;
        size_t lstBlk = date.find_last_of(" ");
        strptime(date.substr(0, lstBlk).c_str(), "%a, %d %b %Y %H:%M:%S", &date_struct);
        time_t createTime = mktime(&date_struct) - 3600 * 5;   // reduce 5 hours
        time_t expirTime = createTime + maxAge;
        log->cacheButExpires(asctime(gmtime(&expirTime)));
      }
      else if(maybeCache.isMustRevalidate()) {
        log->cacheButValidation();
      }
    }
    else {
      log->notCacheable(cacheCtlStr);
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
      throw mException;
    } 

    socket_fd = socket(host_info_list->ai_family,
     host_info_list->ai_socktype,
     host_info_list->ai_protocol);
    if (socket_fd == -1) {
      cerr << "Error: cannot create socket" << endl;
      throw mException;
    } 

    int yes = 1;
    status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
      cerr << "Error: cannot bind socket" << endl;
      throw mException;
    } 

    status = listen(socket_fd, 100);
    if (status == -1) {
      cerr << "Error: cannot listen on socket" << endl;
      throw mException;
    } 
    freeaddrinfo(host_info_list);
    return socket_fd;
  }
  
};

int main(){
  try{
    proxyMain proxymain;
    proxymain.mainThread();
  }catch(mainException e){
    cout<<e.what();
  }
}
