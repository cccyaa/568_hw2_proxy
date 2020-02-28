TARGETS=proxy

all: $(TARGETS)
clean:
	rm -f $(TARGETS)
proxy: proxyMain.cpp proxyServerGET.cpp proxyServerCONNECT.cpp proxyServerPOST.cpp parseBuffer.hpp logger.hpp
	g++ -std=c++11 -pthread -g -o  $@ $<
