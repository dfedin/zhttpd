/*
 *  server.h
 *  zhttpd
 *
 *  Created by dm on 12.02.12.
 *  Copyright 2012 __MyCompanyName__. All rights reserved.
 *
 */
#include <string>
#include <boost/thread/thread.hpp>

    

namespace ZH {
	class TZHttp {
	private:
		int ThreadNumber;
		int Port;
		std::string DocumentRoot;
        void ThreadFunc();
        boost::thread_group Threads;
        int ListenSocket;
        int Kqueue;
	public:
		TZHttp(int port, const char* documentRoot, int threads = 10);
        void Start();
	};
}