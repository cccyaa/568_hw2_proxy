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

class proxyServerPOST {
private:
  string hostServer;
  char * bufToServer;
  size_t endIdx;
  char * bufFromServer;
  size_t httpResSize;
  string portNum;

public:
  proxyServerPOST(string hs, char * buftoserver, size_t eIdx, string portnum): hostServer(hs), bufToServer(buftoserver), endIdx(eIdx), portNum(portnum) {
    bufFromServer = nullptr;
  }

  // get sockaddr, IPv4 or IPv6:
  void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
      return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
  }

  int getHttpResponse() {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

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

    int sendsta = send(sockfd, bufToServer, endIdx, 0);
    if (sendsta == -1) {
      perror("second send");
      exit(1);
    }

    int numbytes1st;
    char * tmp = new char[65535];
    if ((numbytes1st = recv(sockfd, tmp, 65535, 0)) == -1) {
      perror("recv");
      exit(1);
    }

    // for test
    // for (int i = 0; i < numbytes1st; i++) {
    //   if (tmp[i] == '\0') {
    //     cout << "index: " << i << " is \\0" << endl;
    //   }
    // }

    tmp[numbytes1st] = '\0';

    // for test
    cout << "first recv: " << tmp << "\n\n\n\n\n\n" << endl;
    cout << "numbytes1st: " << numbytes1st << endl;


    long contentLen = parseContentLength(tmp);

    int headLen = parseResHeadLength(tmp);
    cout << "headLen: " << headLen << endl;

    long bodyremainLen = contentLen - (numbytes1st - headLen);
    cout << "bodyremainLen: " << bodyremainLen << endl;

    httpResSize = contentLen + headLen;
    bufFromServer = new char[httpResSize + 1];
    for (int i = 0 ; i < numbytes1st; ++i) {
      bufFromServer[i] = tmp[i];
    }
    free(tmp);
    int recvByte;
    char * tmpP = bufFromServer + numbytes1st;   // tmpP is the start point for recving remaining content

    int count = 0;
    while (bodyremainLen > 0 && (recvByte = recv(sockfd, tmpP, bodyremainLen, 0)) > 0) {
      count ++;
      cout << "recvByte " << count <<  ": " << recvByte << endl;
      tmpP += recvByte;
      bodyremainLen -= recvByte;
    }
    tmpP[0] = '\0';

    // for test
    cout << "bufFromServer length: " << strlen(bufFromServer) << endl;

    //printf("client: received\n '%s'\n", bufFromServer);
    close(sockfd);
    return 0;
  }

  size_t getHttpResSize() {
    return httpResSize;
  }

  char * getBufFromServer() {
    return bufFromServer;
  }

  long parseContentLength(char * resp) {
    string response = resp;    // response only contain chars before first '\0'
    cout << "string response size: " << response.size() << endl;
    size_t idx1 = response.find("Content-Length");
    size_t idx2 = response.find_first_of("\r\n", idx1);
    string str_length = response.substr(idx1 + 16, idx2 - (idx1 + 16));

    cout << "content-length: " << str_length << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;

    long length = stol(str_length);
    return length;
  }

  int parseResHeadLength(char * resp) {
    string response = resp;
    size_t idx1 = response.find("\r\n\r\n");
    return idx1 + 4;
  }

  ~proxyServerPOST () {
    delete[] bufFromServer;
  }
};
