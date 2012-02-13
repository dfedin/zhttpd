/*
 *  server.h
 *  zhttpd
 *
 *  Created by dm on 12.02.12.
 *  Copyright (C) 2012 dm128. zhttpd is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  zhttpd is distributed in the hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with the zhttpd; see the file COPYING
 *  If not, write to the Free Software Foundation, Inc., 59 Temple Place -
 *  Suite 330, Boston, MA 02111-1307, USA.
 *
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
        ssize_t ReadAll(int fs, char* buf, const size_t readMaxBytes);
	public:
		TZHttp(int port, const char* documentRoot, int threads = 10);
        void Start();
	};
}
