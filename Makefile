TARGETS=proxy

all: $(TARGETS)
clean:
	rm -f $(TARGETS)
proxy: proxyMain.cpp parseHttpRequest.cpp proxyServerGET.cpp proxyServerCONNECT.cpp proxyServerPOST.cpp
	g++ -std=c++11 -pthread -g -o  $@ $<
