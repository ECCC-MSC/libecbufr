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
 
 *  file      :  BUFR_META.H
 *
 *  author    :  Souvanlasy ViengSavanh
 *
 *  revision  :
 *
 *  status    :  DEVELOPMENT
 *
 *  language  :  C
 *
 *  object    :  HEADERS FILE FOR BUFR METADATA
 *
 *
 */

#ifndef _bufr_meta_h
#define _bufr_meta_h

#include "bufr_array.h"

typedef struct
   {
   int   descriptor;
   float value;
   float value0;
   int   idescriptor;
   float increment;
   int   npos;
   } LocationEncoding;

typedef struct
   {
   int   descriptor;
   float value;
   } LocationValue;

typedef struct
   {
   int           *nesting;
   int            nb_nesting;
   LocationValue *tlc;
   int            nb_tlc;
   int            pos_template;
   int            len_expansion;
   } BufrRTMD;

typedef ArrayPtr LocationEncodingArray;
typedef ArrayPtr LocationValueArray;

extern BufrRTMD     *bufr_create_rtmd     ( int count );
extern BufrRTMD     *bufr_duplicate_rtmd  ( BufrRTMD * );
extern void          bufr_copy_rtmd       ( BufrRTMD *dest, BufrRTMD *src );
extern void          bufr_free_rtmd       ( BufrRTMD * );

extern void          bufr_print_rtmd_data      ( char *outstr, BufrRTMD *bm );
extern void          bufr_print_rtmd_repl      ( char *outstr, BufrRTMD *bm );
extern void          bufr_print_rtmd_location  ( char *outstr, int desc, BufrRTMD *bm );
extern float         bufr_fetch_rtmd_location ( int descriptor, BufrRTMD *bm );

#endif
