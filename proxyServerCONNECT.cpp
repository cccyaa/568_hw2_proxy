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
using namespace std;

class proxyServerCONNECT {
private:
  string hostServer;
  char * bufToServer;
  size_t endIdx;
  size_t httpResSize;
  string portNum;
  int clientFD;

public:
  proxyServerCONNECT(string hs, char * buftoserver, size_t eIdx, string portnum, int cFD): hostServer(hs), bufToServer(buftoserver), endIdx(eIdx), portNum(portnum), clientFD(cFD) {
  }

  // get sockaddr, IPv4 or IPv6:
  void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
      return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
  }

  void getServerName() {    // remove ":443"
    size_t colon = hostServer.find(":");
    hostServer = hostServer.substr(0, colon);
  }

  int tunnelRun() {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    // hints.ai_socktype = SOCK_STREAM;

    getServerName();
    cout << "hostserver: " << hostServer << endl;
    cout << "port number: " << portNum << endl;
    if ((rv = getaddrinfo(hostServer.c_str(), portNum.c_str(), &hints, &servinfo)) != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
      return 1;
    }

    // loop through all the results and connect to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
      if ((sockfd = socket(p->ai_family, p->ai_socktype,
                           p->ai_protocol)) == -1) {
        perror("client: socket");
        continue;
      }

      if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
        perror("client: connect");
        close(sockfd);
        continue;
      }

      break;
    }

    if (p == NULL) {
      fprintf(stderr, "client: failed to connect\n");
      return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
              s, sizeof s);
    printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo); // all done with this structure

    // up to here, connected to server
    const char * okMsg = "200 OK\r\n";
    send(clientFD, okMsg, strlen(okMsg), 0);
    while (true) {
      fd_set fdsets;
      FD_ZERO(&fdsets);
      FD_SET(clientFD, &fdsets);
      FD_SET(sockfd, &fdsets);
      int maxfd = max(clientFD, sockfd);
      select(maxfd + 1, &fdsets, NULL, NULL, NULL);
      int FDselected = (FD_ISSET(clientFD, &fdsets)) ? clientFD : sockfd;
      int FDsendto = (FD_ISSET(clientFD, &fdsets)) ? sockfd : clientFD;
      char buff[65535];
      int recvRes = recv(FDselected, buff, 65535, 0);

      // for test
      string recvfrom = (FDselected == clientFD) ? "client" : "server";
      cout << "This time receive from " << recvfrom << endl;
      cout << "recvRes: " << recvRes << endl;

      if(recvRes == 0) {
	close(clientFD);
	close(sockfd);
	break;
      }
      send(FDsendto, buff, recvRes, 0);

      // temp compromise
      //break;  
    }

    cout << "out!!!!!!!!!" << endl;

    return 0;
  }

  ~proxyServerCONNECT () {
  }
};
