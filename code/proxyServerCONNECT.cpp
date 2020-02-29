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
#include "exceptions.hpp"
using namespace std;

connectException cException;

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

  void connectToServer(){
    struct addrinfo hints, *servinfo, *p;
    char s[INET6_ADDRSTRLEN];
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    //get host info
    int gr;
    if ((gr = getaddrinfo(hostname.c_str(), "443", &hints, &servinfo)) != 0) {
      log->errorMessage("failed to get server address");
      throw cException;
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
      log->errorMessage("failed to connectserver");
      throw cException;
    }
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
              s, sizeof s);
    cout<<"Connect with Server"<<endl;
    freeaddrinfo(servinfo); // all done with this structure
    setTunnel();
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
      if(recvRes == 0) {
        close(clientSFD);
        close(serverSFD);
        break;
      }
      int sendbytes;
      if((sendbytes=send(FDsendto, buff, recvRes, 0))==-1){
        log->errorMessage("failed to send to client");
        close(clientSFD);
        close(serverSFD);
        throw cException;
      }
    }
    log->tunnelClose();
    cout<<"Tunnel close"<<endl;
  }

  void run(){
    connectToServer();
  }
};
