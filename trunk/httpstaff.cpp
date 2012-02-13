/*
 *  httpstaff.cpp
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

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#include "params.h"

#include "httpstaff.h"
// 400th
const char* BAD_REQUEST_400="400 Bad Request";
const char* REQUEST_ENTRY_TOO_LARGE_413="413 Request Entity Too Large";
const char* URL_TOO_LONG_414="414 Request-URL Too Long";
// 500th
const char* INTERNAL_SERVER_ERROR_500="500 Internal Server Error";
const char* NOT_SUPPORTED_501="501 Not Implemented";

namespace ZH {

bool THTTPUtils::GetHttpReq(THttpRequest& res, const char *buf, size_t bufSize)
{
   if (!strncmp(buf, "GET ", 4) ) {
       res.Method = GET;
       buf += 4;
   }
   else if ( !strncmp(buf, "HEAD ", 5) ) {
       res.Method = HEAD;
       buf += 5;
   }
   else {
       res.Method = UNSUPPORTED;
       res.Status = 501;
       res.StatusString = NOT_SUPPORTED_501;
       return false;
   }  
   // (\s*)
   while ( *buf && isspace(*buf) )
       buf++;
   const char* endptr = strchr(buf, ' ');
   int len = endptr ? endptr - buf : 0;
   if (endptr == 0 || len == 0) {
       res.Status = 400;
       res.StatusString = BAD_REQUEST_400;
       return false;
   }
   if (len > MAX_URI_LEN) {
       res.Status = 414;
       res.StatusString = URL_TOO_LONG_414;
       return false;
   }
   res.Req = (char*)calloc(len + 1, sizeof(char));
   strncpy(res.Req, buf, len);
   buf += len;
   // (\s*)
    while ( *buf && isspace(*buf) )
       buf++;
    int verMaj = 0;
#ifdef VERBOSE_DEBUG
    printf("HTTP version %s", buf);
#endif

    int verMin = 0;
    if (sscanf(buf, "HTTP/%1d.%1d", &verMaj, &verMin) != 2) {
        // do not allow HTTP/0.9
       res.Status = 400;
       res.StatusString = BAD_REQUEST_400;
       return false;
    }    
    return true;
}

}
