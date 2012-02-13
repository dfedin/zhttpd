BINARY=zhttpd
LDFLAGS=-lboost_thread-mt -L/usr/lib/boost-1_42
CFLAGS=-O2
CXXFLAGS=$(CFLAGS)

all: $(BINARY)

zhttpd: main.o server.o httpstaff.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $? 

clean:
	$(RM) *.o zhttp
