/******************************************************************************
  Copyright (c) 2013 Morten Houmøller Nygaard - www.mortz.dk - admin@mortz.dk
 
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
******************************************************************************/

#ifndef _INCLUDES_H
#define _INCLUDES_H

#include <pthread.h> 			/* pthread_create, pthread_t, pthread_attr_t 
								   pthread_mutex_init */
#include <ctype.h> 				/* isdigit, isblank */
#include <stdio.h> 				/* printf, fflush(stdout), sprintf */
#include <stdlib.h> 			/* atoi, malloc, free, realloc */
#include <stdint.h> 			/* uint32_t */
#include <errno.h> 				/* errno */
#include <string.h> 			/* strerror, memset, strncpy, memcpy */
#include <strings.h> 			/* strncasecmp */
#include <unistd.h> 			/* close */
#include <math.h> 				/* log10 */
#include <signal.h> 			/* sigaction */

#include <sys/types.h> 			/* socket, setsockopt, accept, send, recv */
#include <sys/socket.h> 		/* socket, setsockopt, inet_ntoa, accept */
#include <netinet/in.h> 		/* sockaddr_in, inet_ntoa */
#include <arpa/inet.h> 			/* htonl, htons, inet_ntoa */

#define KEYSIZE 16
#define HOSTS 5
#define BUFFERSIZE 8192


#define DEBUG_MESSAGES 0
#define DEBUG_HANDSHAKE 1

#endif
