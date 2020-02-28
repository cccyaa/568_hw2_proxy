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
#include "proxyServerGET.cpp" 
#include "proxyServerCONNECT.cpp"
#include "proxyServerPOST.cpp"
#include "parseHttpRequest.cpp"

using namespace std;

class proxyMain{
  int proxySFD;
  int numOfRequests;
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
    while(true){
      cout << "**Waiting for connection on port 4444**" << endl;
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
      thread newthread(newThread,clientSFD,numOfRequests++,clientIP);
      newthread.detach();
    }
  }

  static void newThread(int clientSFD,int uniqueID,string clientIP){
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
    //get client address
    // struct sockaddr addr;
    // socklen_t addrlen;
    // int res;
    // res=getpeername(clientSFD, &addr, &addrlen); 
    // if(res==-1){
    //   cerr<<"Error: failed to get client's infomation";
    // }
    // string clientAddr=addr.sa_data;
    log.receiveNewRequest(clientIP,requestLine);
    //std::cout<<"***requestType:"<<requestType<<"***"<<endl;
    if (requestType=="GET"||requestType=="POST"){
        //cout<<"***inside of GET***"<<endl;
        //client address??
        //log receive new request
        //log.receiveNewRequest(clientAddr,requestLine);
        proxyServerGET ps(hostname,buffer, numbytes,clientSFD,&log);
        ps.run();
    }else if(requestType=="CONNECT"){
        //log.receiveNewRequest(clientAddr,requestLine);
        proxyServerCONNECT ps(hostname,buffer,numbytes,clientSFD,&log);
        ps.run(); 
    }
    //delete[] buffer;
  }
    
  int getProxySFD(){
    int status;
    int socket_fd;
    struct addrinfo host_info;
    struct addrinfo *host_info_list;
    const char *hostname = NULL;
    const char *port     = "4444";

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
