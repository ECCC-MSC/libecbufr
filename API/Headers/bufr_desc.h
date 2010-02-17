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
 
 *  file      :  BUFR_CODE.H (to be renamed BUFR_DESC.H)
 *
 *  author    :  Souvanlasy ViengSavanh
 *
 *  revision  :
 *
 *  status    :  DEVELOPMENT
 *
 *  language  :  C
 *
 *  object    :  HEADER FILE FOR BUFR DECRIPTORS
 *
 *
 */


#ifndef _bufr_desc_h
#define _bufr_desc_h

#include  <stdio.h>
#include  <inttypes.h>
#include  "bufr_tables.h"
#include  "bufr_afd.h"
#include  "bufr_value.h"
#include  "bufr_meta.h"
#include  "bufr_array.h"

#ifdef __cplusplus
extern "C" {
#endif

#define   FLAG_CLASS31      0x1
#define   FLAG_EXPANDED     0x2
#define   FLAG_SKIPPED      0x4
#define   FLAG_CLASS33      0x8

typedef struct bufr_desc
   {
   int                  descriptor;
   BufrValue           *value;
   BufrValueEncoding    encoding;
   unsigned char        flags;
   BufrAFD             *afd;
   BufrRTMD            *meta;
   int                  s_descriptor;
   } BufrDescriptor ;


typedef ArrayPtr  BufrDescriptorArray;

extern BufrDescriptor *bufr_create_descriptor         ( BUFR_Tables *tbls, int desc );
extern BufrDescriptor *bufr_dupl_descriptor           ( BufrDescriptor *dup );
extern void            bufr_free_descriptor           ( BufrDescriptor * );
extern void            bufr_copy_descriptor           ( BufrDescriptor *dest, BufrDescriptor *src );
extern BufrValue      *bufr_mkval_for_descriptor      ( BufrDescriptor * );
extern void            bufr_print_descriptor          ( char *str, BufrDescriptor *bdsc );
extern int             bufr_print_dscptr_value        ( char *outstr, BufrDescriptor *cb );
extern ValueType       bufr_encoding_to_valtype       ( BufrValueEncoding * );
extern ValueType       bufr_datatype_to_valtype       ( BufrDataType type, int nbits, int scale );

extern int             bufr_descriptor_get_range      ( BufrDescriptor *bdsc, double *min, double *max );

extern int             bufr_descriptor_set_fvalue     ( BufrDescriptor *bdsc,  float  val );
extern int             bufr_descriptor_set_dvalue     ( BufrDescriptor *bdsc,  double val );
extern int             bufr_descriptor_set_ivalue     ( BufrDescriptor *bdsc,  int    val );
extern int             bufr_descriptor_set_svalue     ( BufrDescriptor *bdsc,  const char  *val );
extern int             bufr_descriptor_set_bitsvalue  ( BufrDescriptor *bdsc,  uint64_t ival );

extern float           bufr_descriptor_get_fvalue     ( BufrDescriptor *bdsc );
extern double          bufr_descriptor_get_dvalue     ( BufrDescriptor *bdsc );
extern int             bufr_descriptor_get_ivalue     ( BufrDescriptor *bdsc );
extern char           *bufr_descriptor_get_svalue     ( BufrDescriptor *bdsc, int *len );
extern void            bufr_set_value_af              ( BufrValue *bv, const BufrDescriptor *bc );

extern float           bufr_descriptor_get_location   ( BufrDescriptor *bdsc, int desc );

#ifdef __cplusplus
}
#endif

#endif
