/*
 *  params.h
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
#pragma once
#define THREAD_COUNT 10
#define HTTP_PORT 8081
#define LISTEN_QUEUE 50
#define LISTEN_BACKLOG 500
#define MAX_REQ_LEN 4096
#define MAX_URI_LEN 2048 // U are IE or go away

#if  (defined(__APPLE__) && defined(__MACH__))
#define __darwin__ 
#endif

//#define VERBOSE_DEBUG
