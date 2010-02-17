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

 *  file      :  BUFR_AF.H
 *
 *  author    :  Souvanlasy ViengSavanh
 *
 *  revision  :
 *
 *  status    :  DEVELOPMENT
 *
 *  language  :  C
 *
 *  object    :  HEADERS FILE FOR ASSOCIATED FIELDS
 *
 *
 */


#ifndef _bufr_af_h
#define _bufr_af_h

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
   {
   uint8_t       len;
   uint8_t       shift;
   short         signify;
   } AF_Field;

/* 
 * maximum total of 64 bits per value, shoud be enough for current need 
 */
typedef struct
   {
   uint64_t         bits;   /* packed 64 bits */
   uint16_t         nbits;
   uint16_t         count;
   AF_Field        *fields;
   } BufrAF;

extern BufrAF           *bufr_create_af     ( const int *bdefs, int count );
extern BufrAF           *bufr_duplicate_af  ( const BufrAF * );
extern void              bufr_free_af       ( BufrAF * );
extern void              bufr_af_set_value  ( BufrAF *, int pos, int val );
extern int               bufr_af_get_value  ( const BufrAF *, int pos );
extern void              bufr_af_set_sig    ( BufrAF *, int pos, int val );
extern int               bufr_af_get_sig    ( const BufrAF *, int pos );
extern int               bufr_print_af      ( char *outstr, const BufrAF *af );

#ifdef __cplusplus
}
#endif

#endif
