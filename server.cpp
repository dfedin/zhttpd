/*
 *  server.cpp
 *  zhttpd
 *
 *  Created by dm on 12.02.12.
 *  Copyright (C) 2012 dm128. zhttpd is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  zhttpd is distributed in the hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  ary General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with the zhttpd; see the file COPYING
 *  If not, write to the Free Software Foundation, Inc., 59 Temple Place -
 *  Suite 330, Boston, MA 02111-1307, USA.
 *
 */
#include <stdio.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#if defined(__FreeBSD__)
#include <sys/types.h>
#endif
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#if (defined(__APPLE__) && defined(__MACH__)) || defined(__FreeBSD__)
#include <sys/event.h>
#include <sys/time.h>
#endif
#if defined(__linux__)
#include <string.h>
#include <sys/epoll.h>
#include <fcntl.h>
#endif

#include "server.h"

#define LISTEN_QUEUE 10
#define MAX_REQ_LEN 4096
//#define VERBOSE_DEBUG

namespace ZH {


TZHttp::TZHttp(int port, const char* documentRoot, int threads)
	: ThreadNumber(threads)
	, Port(port)
	, DocumentRoot(documentRoot)
{
}

void TZHttp::Start()
{
    struct sockaddr_in my_addr;
#if defined(__linux__)
    ListenSocket = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
#else
    ListenSocket = socket(PF_INET, SOCK_STREAM, 0);
#endif
    if (ListenSocket < 0) {
        perror("socker");
        exit(-1);
    }
// socket created non-blocked on linux
#if !defined(__linux__)
    int opt = 1;
    ioctl(ListenSocket, FIONBIO, &opt);
#endif
    memset(&my_addr, 0, sizeof(struct sockaddr_in));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(Port);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(ListenSocket, (struct sockaddr *) &my_addr,
        sizeof(struct sockaddr_in)) < 0)
    {
        perror("bind");
        exit(-1);
    }
    if (listen(ListenSocket, 10) < 0) {
        perror("listen");
        exit(-1);
    }
	for (int i = 0; i < ThreadNumber; ++i)
        Threads.create_thread(boost::bind(&TZHttp::ThreadFunc, this));
    Threads.join_all();
}

void TZHttp::ThreadFunc()
{
    printf("thread started!\n");
    int nconn = 0;
    struct sockaddr_in addr;
    socklen_t len = sizeof(sockaddr_in);    
#if  (defined(__APPLE__) && defined(__MACH__)) || defined(__FreeBSD__)
    struct kevent chlist, evlist[LISTEN_QUEUE];
    int kv = kqueue();
    EV_SET(&chlist, ListenSocket, EVFILT_READ, EV_ADD, 0, 0, 0);
    (void) kevent(kv, &chlist, 1, (struct kevent*)0, 0, (struct timespec*)0);
#endif
#if defined (__linux__)
    struct epoll_event chlist, evlist[LISTEN_QUEUE];
    int ev = epoll_create(LISTEN_QUEUE);
    chlist.events = EPOLLIN | EPOLLET;
    chlist.data.fd = ListenSocket;
    if (epoll_ctl(ev, EPOLL_CTL_ADD, ListenSocket, &chlist) < 0){
        perror("epoll_ctl");
        exit(-1);
    }

#endif    
    char buf[MAX_REQ_LEN][LISTEN_QUEUE];
    int pos[LISTEN_QUEUE];

    for (;;) {
#if  (defined(__APPLE__) && defined(__MACH__)) || defined(__FreeBSD__)
        int nev = kevent(kv, (const struct kevent *)0, 0, evlist, LISTEN_QUEUE, (const timespec *)0);
#endif        
#if defined (__linux__)
        int nev = epoll_wait(ev, evlist, LISTEN_QUEUE, -1);
#endif
        for (int i = 0; i < nev; i++) {
#if  (defined(__APPLE__) && defined(__MACH__)) || defined(__FreeBSD__)
            int ident = evlist[i].ident;
#endif
#if defined (__linux__)
            int ident = evlist[i].data.fd;
#endif            
        	if (ident  == ListenSocket) {
            	// It's the FD we ran listen() on, it must be an incomming connection
                len = sizeof(sockaddr_in);    
            	int socketfd =
#if !defined (__linux__)                
					accept(ident, (struct sockaddr *)&addr, &len);
#else                    
					accept4(ident, (struct sockaddr *)&addr, &len, O_NONBLOCK);
#endif               
                if (socketfd < 0) {
                    perror("accept");
                    exit(-1);
                }
#ifdef VERBOSE_DEBUG
				printf("got connection from adderess %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
#endif                
#if  (defined(__APPLE__) && defined(__MACH__)) || defined(__FreeBSD__)
				EV_SET(&chlist, socketfd, EVFILT_READ, EV_ADD, 0, 0, 0);
				(void) kevent(kv, &chlist, 1, (struct kevent*)0, 0, (struct timespec*)0);
#endif                
#if defined (__linux__)
                chlist.events = EPOLLIN | EPOLLET;
                chlist.data.fd = socketfd;
                if (epoll_ctl(ev, EPOLL_CTL_ADD, socketfd, &chlist) < 0) {
                    perror("epoll_ctl");
                    exit(-1);
                }
#endif
				pos[i] = 0;
				nconn++;
#if  (defined(__APPLE__) && defined(__MACH__)) || defined(__FreeBSD__)
        	} else if (evlist[i].flags & EV_EOF) {
#endif
#if defined (__linux__)
            } else if ((evlist[i].events & EPOLLERR) || (evlist[i].events & EPOLLHUP) ||
                (!(evlist[i].events & EPOLLIN)))
            {
#endif
            	// connection closed...call close(2)
            	nconn--;
#ifdef VERBOSE_DEBUG
            	printf("con closed \n");
#endif                
            	close(ident);
            	pos[i] = 0;
			}
#if  (defined(__APPLE__) && defined(__MACH__)) || defined(__FreeBSD__)
			else if (evlist[i].flags & EVFILT_READ) {
#endif
#if defined(__linux__)
            else {
#endif            
				ssize_t readBytes = read(ident, buf[i] + pos[i], MAX_REQ_LEN - pos[i]);
            	char sendbuf[1024];
            	buf[i][pos[i] + readBytes] = 0;
#ifdef VERBOSE_DEBUG
            	printf("read %d bytes: %s", readBytes, buf[i] + pos[i]);
#endif                
            	pos[i] += readBytes;
            	if (memcmp(buf, "GET", 3))
            	{
#ifdef VERBOSE_DEBUG
                	printf("sending 400\n");
#endif                    
                	sprintf(sendbuf, "HTTP/1.0 400 Bad Request\r\n"
                                	"Content-Type: text/html; charset=UTF-8\r\n"
                                	"Content-Length: 0\r\n"
                                	"Date: Sun, 12 Feb 2012 16:12:58 GMT\r\n"
                                	"Server: zhttpd/0.1\r\n\r\n");
                	pos[i] = 0;
                	write(ident, sendbuf, strlen(sendbuf));
                	close(ident);
#ifdef VERBOSE_DEBUG
                	printf("con closed \n");
#endif                    
                	continue;
            	}
#if !defined(__linux__)                    
            	char* res = strnstr(buf[i], "\r\n\r\n", pos[i]);
#else                
            	char* res = strstr(buf[i], "\r\n\r\n");
#endif          
            	if (res == 0)
                	continue;
#ifdef VERBOSE_DEBUG
                printf("sending 200\n");
#endif                    
            	sprintf(sendbuf, "HTTP/1.0 200 OK   \r\n"
                            	"Content-Type: text/html; charset=UTF-8\r\n"
                            	"Content-Length: 30\r\n"
                            	"Date: Sun, 12 Feb 2012 16:12:58 GMT\r\n"
                            	"Server: zhttpd/0.1\r\n\r\n"
                            	"<html><H1>ZHTTP!!1</H1></html>");
            	write(ident, sendbuf, strlen(sendbuf));
                close(ident);
                pos[i] = 0;
#ifdef VERBOSE_DEBUG
                printf("con closed \n");
#endif                        
            } 
#if  (defined(__APPLE__) && defined(__MACH__)) || defined(__FreeBSD__)
            else {
                    	printf("unhandeled kevent\n");
            }
#endif                
		}
	}
}

}

