BINARY=zhttpd
#LDFLAGS=-lboost_thread-mt -L/usr/lib/boost-1_42
#CFLAGS=-O2
LDFLAGS=-lboost_thread -L/usr/local/lib
CFLAGS=-O2 -I/usr/local/include
CXXFLAGS=$(CFLAGS)

all: $(BINARY)

zhttpd: main.o server.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $? 

clean:
	$(RM) *.o zhttp
