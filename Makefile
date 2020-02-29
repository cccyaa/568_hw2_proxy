TARGETS=proxy

all: $(TARGETS)
clean:
	rm -f $(TARGETS)
proxy: proxyMain.cpp proxyServerGET.cpp proxyServerCONNECT.cpp proxyServerGETold.hpp proxyServerPOST.cpp parseBuffer.hpp logger.hpp cache.hpp cacheRes.hpp
	g++ -std=c++11 -pthread -g -o  $@ $<
