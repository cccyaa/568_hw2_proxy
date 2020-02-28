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
#include "parseBuffer.hpp"
#include "logger.hpp"
using namespace std;

class proxyServerCONNECT {
private:
  string hostname;
  char * request;
  size_t requestLen;
  char * response;
  size_t responseLen;
  int clientSFD;
  int serverSFD;
  logger* log;
public:
  proxyServerCONNECT(string hn, char * buf, size_t rl,int csfd,logger * lg): \
  hostname(hn), request(buf), requestLen(rl),clientSFD(csfd) ,log(lg){
    size_t colon = hostname.find(":");
    hostname = hostname.substr(0, colon);
    response = nullptr;
  }
  // get sockaddr, IPv4 or IPv6:
  void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
      return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
  }

  int connectToServer(){
    struct addrinfo hints, *servinfo, *p;
    char s[INET6_ADDRSTRLEN];
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    //get host info
    int gr;
    if ((gr = getaddrinfo(hostname.c_str(), "443", &hints, &servinfo)) != 0) {
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
    setTunnel();
    return 0;
  }

  void setTunnel(){
    const char * okMsg = "200 OK\r\n";
    send(clientSFD, okMsg, strlen(okMsg), 0);
    log->respondResponse("200 OK");


    while (true) {
      fd_set fdsets;
      FD_ZERO(&fdsets);
      FD_SET(clientSFD, &fdsets);
      FD_SET(serverSFD, &fdsets);
      int maxfd = max(clientSFD, serverSFD);
      select(maxfd + 1, &fdsets, NULL, NULL, NULL);
      int FDselected = (FD_ISSET(clientSFD, &fdsets)) ? clientSFD : serverSFD;
      int FDsendto = (FD_ISSET(clientSFD, &fdsets)) ? serverSFD : clientSFD;
      char buff[65535];
      int recvRes = recv(FDselected, buff, 65535, 0);

      // for test
      //string recvfrom = (FDselected == clientSFD) ? "client" : "server";
      //cout << "This time receive from " << recvfrom << endl;
      //cout << "recvRes: " << recvRes << endl;

      if(recvRes == 0) {
        close(clientSFD);
        close(serverSFD);
        break;
      }
      send(FDsendto, buff, recvRes, 0);

      // temp compromise
      //break;  
    }
    //cout << "***Connection ends***" << endl;
    log->tunnelClose();
  }

  void run(){
    //cout<<"***inside of proxyServerGET.run()***"<<endl;
    int res;
    if(res=connectToServer()!=0){
      exit(1);
    }
    close(clientSFD);
    close(serverSFD);
  }
};
