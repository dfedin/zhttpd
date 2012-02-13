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

#include "params.h"
#include "httpstaff.h"

#include "server.h"


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
    int on = 1;
    if (setsockopt(ListenSocket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)  {
          perror("setsockopt(SO_REUSEADDR) failed");
          exit(-1);
    }
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
    char ntoa_buf[MAX_REQ_LEN];

    for (;;) {
#if  (defined(__APPLE__) && defined(__MACH__)) || defined(__FreeBSD__)
        int nev = kevent(kv, (const struct kevent *)0, 0, evlist, LISTEN_QUEUE, (const timespec *)0);
        if (nev < 0) {
            perror("kevent");
            exit(-1);
        }
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
                memset(&addr, 0, sizeof(struct sockaddr_in));

                int socketfd =
#if !defined (__linux__)
                    accept(ident, (struct sockaddr *)&addr, &len);
#else
                    accept4(ident, (struct sockaddr *)&addr, &len, O_NONBLOCK);
#endif
                if (socketfd < 0) {
                    // someone alreadu handle this
                    if (errno == EAGAIN)
                        continue;
                    perror("accept");
                    exit(-1);
                }
#ifdef VERBOSE_DEBUG
#if !defined(__linux__)
                printf("th:%lu: got connection from adderess %s:%d\n", pthread_self(), inet_ntoa_r(addr.sin_addr, ntoa_buf, MAX_REQ_LEN), ntohs(addr.sin_port));
#else
                printf("th:%lu: got connection from adderess %s:%d\n", pthread_self(), inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
#endif
#endif
#if (defined(__APPLE__) && defined(__MACH__)) || defined(__FreeBSD__)
                EV_SET(&chlist, socketfd, EVFILT_READ, EV_ADD|EV_CLEAR, 0, 0, 0);
                if (kevent(kv, &chlist, 1, (struct kevent*)0, 0, (struct timespec*)0) < 0) {
                    perror("kevent");
                    exit(-1);
                }
#endif
#if defined (__linux__)
                chlist.events = EPOLLIN | EPOLLET;
                chlist.data.fd = socketfd;
                if (epoll_ctl(ev, EPOLL_CTL_ADD, socketfd, &chlist) < 0) {
                    perror("epoll_ctl");
                    exit(-1);
                }
#endif
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
                printf("th:%lu: con closed \n", pthread_self());
#endif
                close(ident);
            }
#if  (defined(__APPLE__) && defined(__MACH__)) || defined(__FreeBSD__)
            else if (evlist[i].flags & EVFILT_READ) {
#endif
#if defined(__linux__)
            else {
#endif
                char buf[MAX_REQ_LEN];

                ssize_t readBytes = ReadAll(ident, buf, MAX_REQ_LEN);
                if (readBytes < 0) {

                    SendStatus(ident, 413, REQUEST_ENTRY_TOO_LARGE_413);
                    close(ident);
                    continue;
                }
                buf[readBytes] = 0;
#ifdef VERBOSE_DEBUG
                printf("th:%lu: read %ld bytes: %s", pthread_self(), readBytes, buf);
#endif
                THttpRequest req;
                if (!THTTPUtils::GetHttpReq(req, buf, readBytes)) {
                    SendStatus(ident, req.Status, req.StatusString);
                    close(ident);
#ifdef VERBOSE_DEBUG
                    printf("th:%lu: con closed \n", pthread_self());
#endif
                    continue;
                }
#ifdef VERBOSE_DEBUG
                printf("th:%lu: req uri %s\n", pthread_self(), req.Req);
#endif
                SendResponse(ident, req);
                close(ident);
#ifdef VERBOSE_DEBUG
                printf("th:%lu: con closed \n", pthread_self());
#endif
            }
#if  (defined(__APPLE__) && defined(__MACH__)) || defined(__FreeBSD__)
            else {
                printf("unhandeled kevent %d socket %d\n", evlist[i].flags, evlist[i].ident);
            }
#endif
        }
    }
}
ssize_t TZHttp::ReadAll(int fd, char* buf, const size_t readMaxBytes)
{
    size_t pos = 0;
    while (true) {
        if (readMaxBytes == pos)
            return -1;
        ssize_t readBytes = read(fd, buf + pos, readMaxBytes - pos);
        if (readBytes < 0) {
            if (errno != EAGAIN) {
                perror("read");
                exit(-1);
            }
            // continue reading later
            continue;
        }
        buf[pos + readBytes] = 0;
        if (strstr(buf, "\r\n\r\n") != 0)
            return (pos + readBytes);
        pos += readBytes;
    }
}

void TZHttp::SendStatus(int fd, int status, const char* statusString)
{
#ifdef VERBOSE_DEBUG
    printf("th:%lu: sending %d\n", pthread_self(), status);
#endif
    char sendbuf[1024];
    int chars = sprintf(sendbuf, "HTTP/1.0 %s\r\n"
        "Content-Type: text/html;\r\n"
        "Server: zhttpd/0.1\r\n\r\n", statusString);
    write(fd, sendbuf, chars);
}

void TZHttp::SendResponse(int fd, THttpRequest& req)
{
#ifdef VERBOSE_DEBUG
    printf("th:%lu: sending 200\n", pthread_self());
#endif
    // get content from somewhere by req
    std::string content = "<html><H1>ZHTTP!!1</H1></html>";
    char sendbuf[1024];
    int chars = sprintf(sendbuf, "HTTP/1.0 200 OK   \r\n"
        "Content-Type: text/html;\r\n"
        "Content-Length: %lu\r\n"
        "Server: zhttpd/0.1\r\n\r\n", content.length());
    memcpy(sendbuf + chars, content.c_str(), content.length());
    size_t sizeToSend = chars + content.length();
    ssize_t sizeWritten = write(fd, sendbuf, sizeToSend);
    free(req.Req);
#ifdef VERBOSE_DEBUG
    printf("th:%lu: has to send %d bytes, sent %d\n", pthread_self(), sizeToSend, sizeWritten);
#endif
}

}

