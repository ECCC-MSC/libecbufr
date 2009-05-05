/***
Copyright Her Majesty The Queen in Right of Canada, Environment Canada, 2009.
Copyright Sa Majesté la Reine du Chef du Canada, Environnement Canada, 2009.

This file is part of libECBUFR.

    libECBUFR is free software: you can redistribute it and/or modify
    it under the terms of the Lesser GNU General Public License,
    version 3, as published by the Free Software Foundation.

    libECBUFR is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    Lesser GNU General Public License for more details.

    You should have received a copy of the Lesser GNU General Public
    License along with libECBUFR.  If not, see <http://www.gnu.org/licenses/>.

 * fichier:  bufr_sio.c
 *
 * author:  Vanh Souvanlasy (avril 1996)
 *
 * function: for writing or reading BUFR message through a file descriptor
 *
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>

#include "bufr_api.h"
#include "bufr_io.h"
#include "bufr_message.h"

/*
 * name: bufr_swrite_fn
 *
 * author:  Chris Beauregard
 *
 * function: internal callback to write to a byte-oriented buffer sink
 *
 * parameters: 
 *      client_data : file descriptor to output file or socket stream
 *      len : number of bytes to write
 *      buffer : data buffer to write from
 *
 * returns:
 *      number of bytes written. Zero or negative means
 *      some kind of error.
 */
static ssize_t bufr_swrite_fn( void *client_data, size_t len,
                               const char *buffer)
	{
	/* write to a file descriptor, handling short writes correctly */
	int wrote = 0;
	while( len > 0 )
		{
		ssize_t rc = write( (int) client_data, buffer, len );
		if( rc <= 0 )
			{
			/* EAGAIN happens when non-blocking I/O is used, EINTR
			 * happens when a signal triggers. Neither of them should
			 * interrupt writing.
			 */
			if( errno != EINTR && errno != EAGAIN ) break;
			errno = 0;
			continue;
			}
		wrote += rc;
		len -= rc;
		buffer += rc;
		}
	return wrote;
	}

/*
 * name: bufr_swrite_message
 *
 * author:  Vanh Souvanlasy
 *
 * function: write a BUFR message sections into a file descriptor or socket
 *           stream
 *
 * parameters: 
 *      fd   : file descriptor opened for writing
 *      bufr : pointer to BUFR data structure
 */
int bufr_swrite_message(int  fd, BUFR_Message *bufr)
   {
		return bufr_callback_write_message( bufr_swrite_fn, (void*) fd, bufr );
   }

/*
 * name: bufr_sread_fn
 *
 * author:  Chris Beauregard
 *
 * function: internal callback to read from a byte-oriented buffer source
 *
 * parameters: 
 *      client_data : file descriptor to input file or socket stream
 *      len : number of bytes to read
 *      buffer : data buffer to read into
 *
 * returns:
 *      number of bytes read. Zero means end-of-file, negative means
 *      some kind of error.
 */
static ssize_t bufr_sread_fn( void* client_data, size_t len, char* buffer )
	{
	/* read from a file descriptor, handling short reads correctly */
	int got = 0;
	while( len > 0 )
		{
		ssize_t rc = read( (int) client_data, buffer, len );
		if( rc == 0 ) break;	/* end-of-file */
		if( rc < 0 )
			{
			/* EAGAIN happens when non-blocking I/O is used, EINTR
			 * happens when a signal triggers. Neither of them should
			 * interrupt reading.
			 */
			if( errno != EINTR && errno != EAGAIN ) break;
			errno = 0;
			continue;
			}
		got += rc;
		len -= rc;
		buffer += rc;
		}
	return got;
	}

/*
 * name: bufr_sread_message
 *
 * author:  Vanh Souvanlasy
 *
 * function: read a BUFR report from a socket
 *
 * parameters: 
 *      fd   : file descriptor to input file or socket stream
 */
int bufr_sread_message( int fd, BUFR_Message **rtrn )
   {
	return bufr_callback_read_message( bufr_sread_fn, (void*) fd, rtrn );
   }

