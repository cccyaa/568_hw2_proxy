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
#include "parseBuffer.cpp"
using namespace std;


class proxyServerGET {
private:
  string hostname;
  char * request;
  size_t requestLen;
  char * response;
  size_t responseLen;
  int clientSFD;
  int serverSFD;

public:
  proxyServerGET(string hn, char * buf, size_t rl,int csfd): \
  hostname(hn), request(buf), requestLen(rl),clientSFD(csfd) {
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
  int sendToServer(){
    struct addrinfo hints, *servinfo, *p;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    //get host info
    int gr;
    if ((gr = getaddrinfo(hostname.c_str(), "80", &hints, &servinfo)) != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(gr));
      return 1;
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
      perror("Error: failed to connectserver");
      return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
              s, sizeof s);
    printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo); // all done with this structure

    int sr = send(serverSFD, request, requestLen, 0);

    if (sr == -1) {
      perror("Error: cannot send request to server");
      exit(1);
    }
    cout<<"***send request to server***"<<endl;
    return 0;
  }

  int receiveFromServer(){
    int firstLen;
    char * tmp = new char[65535];
    if ((firstLen = recv(serverSFD, tmp, 65535, 0)) == -1) {
      perror("recv");
      exit(1);
    }
    cout<<"***received response from server***"<<endl;
    tmp[firstLen] = '\0';

    // for test
    cout << "*****first recv***** " <<endl<< tmp <<endl<< "**first receive end**" << endl;
    cout << "***first receive length: " << firstLen <<"***"<< endl;
    
    //string firstResponse=tmp;
    parseBuffer pb(tmp);
    long bodyLen=pb.getBodyLen();
    long headLen=pb.getHeadLen();
    long responseLen=bodyLen+headLen;

    cout << "***headLen: " << headLen <<"***"<< endl;

    long leftLen = bodyLen - (firstLen - headLen);
    cout << "***leftLen: " << leftLen <<"***"<< endl;

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
      cout << "recvByte " << count <<  ": " << recvByte << endl;
      tmpP += recvByte;
      leftLen -= recvByte;
    }
    if (leftLen==0){
        return 0;
    }
    tmpP[0] = '\0';

    // for test
    cout << "***response length: " << strlen(response) <<"***"<< endl;

    int sr2;
    if ((sr2 = send(clientSFD, response, responseLen, 0)) == -1) {
      perror("Error: failed to send reponse to client");
      exit(1);
    }
  }

  // void sendToClient(){
  //   cout<<"***inside of proxyServerGET.sendToClient()***"<<endl;
  //   int res;
  //   if ((res = send(clientSFD, response, responseLen, 0)) == -1) {
  //     perror("Error: failed to send reponse to client");
  //     exit(1);
  //   }
  // }

  void run(){
    //cout<<"***inside of proxyServerGET.run()***"<<endl;
    int rs;
    if(rs=sendToServer()!=0){
      exit(1);
    }
    int rr;
    if(rr=receiveFromServer()!=0){
      exit(1);
    };
    //sendToClient();
    cout<<"***end of proxyServerGET.run()***"<<endl;
    close(clientSFD);
    close(serverSFD);
  }
};
