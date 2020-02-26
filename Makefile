TARGETS=proxyMT

all: $(TARGETS)
clean:
	rm -f $(TARGETS)
proxyMT: proxyMain.cpp parseHttpRequest.cpp proxyServerGET.cpp proxyServerCONNECT.cpp proxyServerPOST.cpp
	g++ -std=c++11 -pthread -g -o  $@ $<
