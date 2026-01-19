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
 
 *  file      :  BUFR_IO.H
 *
 *  author    :  Souvanlasy ViengSavanh
 *
 *  revision  :
 *
 *  status    :  DEVELOPMENT
 *
 *  language  :  C
 *
 *  object    :  HEADERS FILE FOR BUFR INPUT/OUTPUT BASE FUNCTIONS
 *
 *
 */


#ifndef _bufr_io_h_
#define _bufr_io_h_

#include "bufr_tables.h"
#include "bufr_message.h"

#include <math.h>
#include <limits.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#define  BUFR_ERR_MSGS_LIMIT    10

extern void           bufr_skip_bits       ( BUFR_Message *bufr, int nbbits, int *errcode);
extern int            bufr_end_of_data     ( BUFR_Message *bufr );
extern void           bufr_putbits         ( BUFR_Message *bufr, uint64_t val, int nbbits );
extern uint64_t       bufr_getbits         ( BUFR_Message *bufr, int nbbits, int *errcode );
extern void           bufr_putstring       ( BUFR_Message *bufr, const char *str, int len );
extern void           bufr_put_padstring   ( BUFR_Message *bufr,
                                             const char *str, int len,
															int enclen);
extern int            bufr_getstring       ( BUFR_Message *bufr, char *str, int len );

extern int            bufr_decode_sect3    ( BUFR_Message *bufr );

extern void           bufr_alloc_sect4     ( BUFR_Message *bufr, unsigned int len );

/* TODO: FIND A USE OF BITSTREAM, PROBABLY FOR THE AF BITS >64 BITS UPTO 256 BITS */
/*       IMPLEMENTING THE bufr_get_bitstream() */
extern void           bufr_put_bitstream   ( BUFR_Message *bufr, const unsigned char *val, int nbbits );
/*
 * FUNCTIONS FOR ERRORS HANDLING AND DEBUGGING
 */
extern void           bufr_set_abort       ( void (*uabort)(const char *msg) );
extern void           bufr_set_debug       ( int mode );
extern void           bufr_set_verbose     ( int mode );
extern void           bufr_set_debug_file  ( const char *filename );
extern void           bufr_set_debug_handler( void (*udebug)(const char *msg) );
extern void           bufr_print_debug     ( const char *str );
extern void           bufr_vprint_debug    ( const char *format, ... );
extern void           bufr_set_output_handler( void (*uoutput)(const char *msg) );
extern void           bufr_set_output_file ( const char *filename );
extern void           bufr_print_output    ( const char *str );
extern void           bufr_vprint_output   ( const char* format, ...);
extern int            bufr_is_debug        ( void );
extern int            bufr_is_verbose      ( void );
extern int            bufr_errtype         ( void );
extern void           bufr_abort           ( const char * );
extern void           bufr_set_trimzero    ( int mode );
extern int            bufr_is_trimzero     ( void );

#ifdef __cplusplus
}
#endif

#endif
