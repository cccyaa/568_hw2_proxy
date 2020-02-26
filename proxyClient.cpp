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

using namespace std;

class proxyClient {
private:
  char *  buffer;   // store http request from client
  string hostServer;
  size_t endIdx;
  int ownFD;

public:
  proxyClient() {
    buffer = nullptr;
    hostServer = "";
    endIdx = 0;
    ownFD = -1;
  }

  string parseType(string total) {
    size_t firstBlank = total.find_first_of(" ");
    string ret = total.substr(0, firstBlank);
    return ret;
  }

  string parseHttpRequest() {
    string total = "";
    for (int i = 0; buffer[i] != 0; i++) {
      total.push_back(buffer[i]);
    }
    cout << "HTTP request:" << endl << total << endl;
    cout << "**********************" << endl;
    size_t hostIdx = total.find("Host:");
    size_t hostServerIdx = hostIdx + 6;
    size_t retIdx = total.find_first_of("\r\n", hostIdx);
    hostServer = total.substr(hostServerIdx, retIdx - hostServerIdx);
    endIdx = total.find("\r\n\r\n");
    return total;
  }

  size_t getEndIdx() {
    return endIdx;
  }


  // create own socket and listen to it
  int initOwnFD() {
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

  int mainThread(){
    int initRes = initOwnFD();
    if (initRes == -1) {
      return -1;
    }
    else {
      ownFD = initRes;
    }

    while(true){
      cout << "Waiting for connection on port 4444" << endl;
      struct sockaddr_storage socket_addr;
      socklen_t socket_addr_len = sizeof(socket_addr);
      int client_connection_fd;
      client_connection_fd = accept(ownFD, (struct sockaddr *)&socket_addr, &socket_addr_len);

      cout << "conected with client!!!!!!!!!!!" << endl;

      if (client_connection_fd == -1) {
	cerr << "Error: cannot accept connection on socket" << endl;
	return -1;
      } //if
      std::thread newthread(newThread,client_connection_fd);
    }
    return 0;
  }
  
  void newThread(int client_connection_fd){
    int numbytes;   // bytes reveived from client
    if (buffer != nullptr) {
      delete[] buffer;
    }
    buffer = new char[65535];
    if ((numbytes = recv(client_connection_fd, buffer, 65535, 0)) == -1) {
      perror("recv");
      exit(1);
    }
    //if(numbytes == 0) {continue;}

    buffer[numbytes] = '\0';
    string total = parseHttpRequest();    // http request
    string type = parseType(total);

    switch (type.size()) {
    case 3:    // GET
      requestGET(client_connection_fd);
      // deal with cache !!!
      break;
    case 4:    // POST
      cout << "numbytes received from client of POST: " << numbytes << endl;
      requestPOST(client_connection_fd, numbytes);
      break;
    case 7:    // CONNECT
      requestCONNECT(client_connection_fd);
      break;
    }
  }

  int getHttpRequest() {
    int initRes = initOwnFD();
    if (initRes == -1) {
      return -1;
    }
    else {
      ownFD = initRes;
    }

    while (true) {
      cout << "Waiting for connection on port 4444" << endl;
      struct sockaddr_storage socket_addr;
      socklen_t socket_addr_len = sizeof(socket_addr);
      int client_connection_fd;
      client_connection_fd = accept(ownFD, (struct sockaddr *)&socket_addr, &socket_addr_len);

      cout << "conected with client!!!!!!!!!!!" << endl;

      if (client_connection_fd == -1) {
        cerr << "Error: cannot accept connection on socket" << endl;
        return -1;
      } //if

      int numbytes;   // bytes reveived from client
      if (buffer != nullptr) {
        delete[] buffer;
      }
      buffer = new char[65535];
      if ((numbytes = recv(client_connection_fd, buffer, 65535, 0)) == -1) {
        perror("recv");
        exit(1);
      }
      if(numbytes == 0) {continue;}

      buffer[numbytes] = '\0';
      string total = parseHttpRequest();    // http request
      string type = parseType(total);

      switch (type.size()) {
      case 3:    // GET
        requestGET(client_connection_fd);
        // deal with cache !!!
        break;
      case 4:    // POST
        cout << "numbytes received from client of POST: " << numbytes << endl;
        requestPOST(client_connection_fd, numbytes);
        break;
      case 7:    // CONNECT
        requestCONNECT(client_connection_fd);
        break;
      }
    }

    return 0;
  }


  void requestCONNECT(int client_connection_fd) {
    proxyServerCONNECT ps(getHostServer(), getBuffer(), getEndIdx(), "443", client_connection_fd);
    ps.tunnelRun();
  }

  void requestPOST(int client_connection_fd, int numbytes) {
    proxyServerPOST ps(getHostServer(), getBuffer(), numbytes, "80");
    int res2 = ps.getHttpResponse();
    if (res2 > 0) {
      cout << "res2: " << res2;
      //      exit(1);
    }
    char * httpResponse = ps.getBufFromServer();

    // for test
    //    cout << "HTTP RESPONSE RECEIVED: \n" << *httpResponse << endl;

    int bytesSend = send(client_connection_fd, httpResponse, ps.getHttpResSize(), 0);
    cout << "bytesSend: " << bytesSend << endl;
    cout << "httpResponse length: " << strlen(httpResponse) + 1 << endl;
  }

  void requestGET(int client_connection_fd) {
    proxyServerGET ps(getHostServer(), getBuffer(), getEndIdx(), "80");
    int res2 = ps.getHttpResponse();
    if (res2 > 0) {
      cout << "res2: " << res2;
      //      exit(1);
    }
    char * httpResponse = ps.getBufFromServer();

    // for test
    //    cout << "HTTP RESPONSE RECEIVED: \n" << *httpResponse << endl;

    int bytesSend = send(client_connection_fd, httpResponse, ps.getHttpResSize(), 0);
    cout << "bytesSend: " << bytesSend << endl;
    cout << "httpResponse length: " << strlen(httpResponse) + 1 << endl;
  }

  ~proxyClient () {
    delete[] buffer;
    close(ownFD);
  }

  char* getBuffer() {
    return buffer;
  }

  string getHostServer() {
    return hostServer;
  }
};

int main() {
  proxyClient ctop;
  int res = ctop.getHttpRequest();
  if (res == -1) {
    exit(1);
  }
  return 1;
}

