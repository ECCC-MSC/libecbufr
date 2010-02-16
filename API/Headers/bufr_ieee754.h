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
 
 *  file      :  BUFR_IEEE754.H
 *
 *  author    :  Souvanlasy ViengSavanh
 *
 *  revision  :
 *
 *  status    :  DEVELOPMENT
 *
 *  language  :  C
 *
 *  object    :  HEADERS FILE FOR IEEE 754 FLOATING POINT REPRESENTATION
 *               IN BUFR, AS OF VERSION 5, ONLY SINGLE(32) AND DOUBLE(64) 
 *               BITS FLOAT ARE SUPPORTED
 *
 */


#ifndef _bufr_ieee_h
#define _bufr_ieee_h

#include <inttypes.h>
#include <math.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int           bufr_use_C_ieee754      ( int use );

extern double        bufr_ieee_decode_double ( uint64_t ivalue );
extern float         bufr_ieee_decode_single ( uint32_t ivalue );

extern uint64_t      bufr_ieee_encode_double ( double   fvalue );
extern uint32_t      bufr_ieee_encode_single ( float    fvalue );

extern void          bufr_init_limits        ( void );

#ifdef __cplusplus
}
#endif

#endif
