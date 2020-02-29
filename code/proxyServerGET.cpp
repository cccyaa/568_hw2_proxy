#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <error.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <vector>
#include "logger.hpp"
#include "parseBuffer.hpp"
#include "exceptions.hpp"
using namespace std;

getException gException;

class proxyServerGET {
private:
  string hostname;
  char * request;
  size_t requestLen;
  char * response;
  size_t responseLen;
  int clientSFD;
  int serverSFD;
  logger* log;
  string responseLine;

public:
  proxyServerGET(string hn, char * buf, size_t rl,int csfd,logger * lg): \
  hostname(hn), request(buf), requestLen(rl),clientSFD(csfd),log(lg) {
    response = nullptr;
  }

  // get sockaddr, IPv4 or IPv6:
  void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
      return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
  }
  //should not return
  //should throw exception
  void sendToServer(){
    struct addrinfo hints, *servinfo, *p;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    //get host info
    int gr;
    if ((gr = getaddrinfo(hostname.c_str(), "80", &hints, &servinfo)) != 0) {
      // fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(gr));
      // return 1;
      log->errorMessage("failed to get server address");
      throw gException;
    }

    //connect to host and get serverSFD
    for (p = servinfo; p != NULL; p = p->ai_next) {
      if ((serverSFD = socket(p->ai_family, p->ai_socktype,
                           p->ai_protocol)) == -1) {
        perror("client: socket");
        continue;
      }

      if (connect(serverSFD, p->ai_addr, p->ai_addrlen) == -1) {
        perror("client: connect");
        close(serverSFD);
        continue;
      }
      break;
    }

    if (p == NULL) {
      // perror("Error: failed to connectserver");
      // return 2;
      log->errorMessage("failed to connectserver");
      throw gException;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
              s, sizeof s);
    printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo); // all done with this structure

    int sr = send(serverSFD, request, requestLen, 0);

    if (sr == -1) {
      // perror("Error: cannot send request to server");
      // exit(1);
      log->errorMessage("cannot send request to server");
      throw gException;
    }
    cout<<"***send request to server***"<<endl;
    //log request from server
    log->requestRequest(hostname);
  }

  void addCharArray(char * ori, int length,vector<char>& res){
      for (int i=0;i<length;i++){
        res.push_back(ori[i]);
      }
  }

  void receiveFromServer(){
    int firstLen;
    char * tmp = new char[65535];
    if ((firstLen = recv(serverSFD, tmp, 65535, 0)) == -1) {
      perror("recv");
      exit(1);
    }
    cout<<"***received response from server***"<<endl;
    //tmp[firstLen] = '\0';
    parseBuffer pb(tmp);
    responseLine=pb.getFirstLine();
    log->receiveResponse(hostname,responseLine);
    //check chunk 
    if(pb.ifChunk()){
      //cout<<"it is a chunk situation"<<endl;
      log->noteMessage("received chunked content");
      vector<char> res;
      addCharArray(tmp,firstLen,res);
      delete[] tmp;
   
      int count=2;//for test
      while(true){
        //cout<<"inside chunk loop"<<endl;
        char * tmpP=new char[65535];
        int recvByte;
        if((recvByte=recv(serverSFD,tmpP,65535,0))==-1){
            // perror("Error: failed to get response from server[chunk]");
            // exit(1);
            log->errorMessage("failed to get response from server[chunk]");
            throw gException;
        }
        //cout<<"recv "<<count<<" response from server"<<endl;
        addCharArray(tmpP,recvByte,res);
        //cout<<"recvbytes:"<<recvByte<<endl;
        //count++;
        if (tmpP[0]=='0'){
          delete[] tmpP;
          break;
        }
        delete[] tmpP;
      }
      responseLen=res.size();

      response = new char[responseLen];
      for(int i=0;i<responseLen;i++){
          response[i]=res[i]; 
      }
    }
    else{
      long bodyLen=pb.getBodyLen();
      long headLen=pb.getHeadLen();
      responseLen=bodyLen+headLen;
      long leftLen = bodyLen - (firstLen - headLen);
      leftLen = bodyLen + headLen;
      response = new char[leftLen + 1];
      for (int i = 0 ; i < firstLen; ++i) {
        response[i] = tmp[i];
      }
      free(tmp);
      int recvByte;
      char * tmpP = response + firstLen;   // tmpP is the start point for recving remaining content

      int count = 0;
      while (leftLen > 0 && (recvByte = recv(serverSFD, tmpP, leftLen, 0)) > 0) {
        count ++;
        tmpP += recvByte;
        leftLen -= recvByte;
      }
      if (leftLen==0){
        cout<<"leftlen==0!!!"<<endl;
        //return 0;
      }
      tmpP[0] = '\0';
    }
    close(serverSFD);
  }

  void sendToClient(){
    cout<<"***inside of proxyServerGET.sendToClient()***"<<endl;
    int res;
    if ((res = send(clientSFD, response, responseLen, 0)) == -1) {
      // perror("Error: failed to send reponse to client");
      // exit(1);
      log->errorMessage("failed to send to client");
      throw gException;
    }
    log->respondResponse(responseLine);
  }

    ~proxyServerGET(){
    delete[] response;
  }

  string getBufFromServerStr(){
    string ret;
    for(int i=0;i<responseLen;i++){
      ret.push_back(response[i]);
    }
    return ret;
  }

  void run(){
    sendToServer();
    receiveFromServer();
    sendToClient();
    //cout<<"***end of proxyServerGET.run()***"<<endl;
  }
};
