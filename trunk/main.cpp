#include <stdio.h>
#include "server.h"

using namespace ZH;

int main (int argc, char * const argv[]) {
	
    const char* docRoot = ".";
    const int port = 8081;
	TZHttp server(port, docRoot, 10);
    printf("Starting server at port %d, docroot %s\n", port, docRoot);
    server.Start();
}