BINARY=zhttp
LDFLAGS=-lboost_thread-mt -L/usr/lib/boost-1_42
CFLAGS=-O2
CXXFLAGS=$(CFLAGS)


all: $(BINARY)

zhttp: main.o server.o

clean:
	$(RM) -rf *.o zhttp
