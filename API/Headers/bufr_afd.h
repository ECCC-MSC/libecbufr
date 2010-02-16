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
 
 *  file      :  BUFR_AFD.H
 *
 *  author    :  Souvanlasy ViengSavanh
 *
 *  revision  :
 *
 *  status    :  DEVELOPMENT
 *
 *  language  :  C
 *
 *  object    :  HEADERS FILE FOR ASSOCIATED FIELDS DATA
 *
 *
 */


#ifndef _bufr_afd_h
#define _bufr_afd_h

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
   {
   int              nbits;
   char            *sig;  /* (BufrDescriptor *) */
   } AF_Definition;

typedef struct
   {
   int            count;
   AF_Definition *defs;
   } BufrAFD;

extern BufrAFD          *bufr_create_afd     ( const int *bdefs, int count );
extern BufrAFD          *bufr_duplicate_afd  ( const BufrAFD * );
extern void              bufr_free_afd       ( BufrAFD * );

#ifdef __cplusplus
}
#endif

#endif
