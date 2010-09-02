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
 
 *  file      :  BUFR_VALUE.H
 *
 *  author    :  Souvanlasy ViengSavanh
 *
 *  revision  :
 *
 *  status    :  DEVELOPMENT
 *
 *  language  :  C
 *
 *  object    :  HEADERS FILE FOR BUFR VALUES
 *
 *
 */

#ifndef _bufr_value_h
#define _bufr_value_h

#include <inttypes.h>
#include "bufr_af.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
   {
   VALTYPE_UNDEFINE=0,
   VALTYPE_INT8=1,
   VALTYPE_INT32=2,
   VALTYPE_INT64=3,
   VALTYPE_FLT32=4,
   VALTYPE_FLT64=5,
   VALTYPE_STRING=6
   } ValueType;

typedef struct 
   {
   ValueType     type;
   BufrAF            *af;
   } BufrValue ;


extern BufrValue     *bufr_create_value         ( ValueType type );
extern BufrValue     *bufr_duplicate_value      ( const BufrValue *dup );
extern void           bufr_free_value           ( BufrValue * );
extern void           bufr_copy_value           ( BufrValue *dest, const BufrValue *src );
extern int            bufr_compare_value        ( const BufrValue *bv1, const BufrValue *bv2, double epsilon );
extern int            bufr_between_values       ( const BufrValue *bv1, const BufrValue *bv, const BufrValue *bv2 );

extern int            bufr_value_set_string     ( BufrValue *bv, const char *str, int len );
extern int            bufr_value_set_double     ( BufrValue *bv, double val );
extern int            bufr_value_set_float      ( BufrValue *bv, float val );
extern int            bufr_value_set_int32      ( BufrValue *bv, int32_t  val );
extern int            bufr_value_set_int64      ( BufrValue *bv, int64_t  val );

extern const char    *bufr_value_get_string     ( const BufrValue *bv, int *len );
extern double         bufr_value_get_double     ( const BufrValue *bv );
extern int64_t        bufr_value_get_int64      ( const BufrValue *bv );
extern float          bufr_value_get_float      ( const BufrValue *bv );
extern int32_t        bufr_value_get_int32      ( const BufrValue *bv );


extern int            bufr_print_value          ( char *str, const BufrValue * );
extern int            bufr_print_scaled_value   ( char *outstr, const BufrValue *bv, int scale );


extern uint64_t       bufr_missing_ivalue       ( int nbits );
extern uint64_t       bufr_negative_ivalue      ( int64_t value, int nbits );
extern int64_t        bufr_cvt_ivalue           ( uint64_t value, int nbits );


extern int            bufr_value_is_missing     ( BufrValue *bv );
extern int            bufr_is_missing_float     ( float );
extern int            bufr_is_missing_double    ( double );
extern int            bufr_is_missing_int       ( int i );
extern int            bufr_is_missing_string    ( const char *, int );
extern double         bufr_missing_double       ( void );
extern float          bufr_missing_float        ( void );
extern int            bufr_missing_int          ( void );
extern void           bufr_missing_string       ( char *, int );


extern double         bufr_get_max_double       ( void );
extern float          bufr_get_max_float        ( void );


extern void           bufr_print_float          ( char *str, float fval );
extern void           bufr_print_scaled_float   ( char *str, float fval, int scale );
extern void           bufr_print_binary         ( char *outstr, int64_t  ival, int nbit );
extern int64_t        bufr_binary_to_int        ( const char *str );
extern int            bufr_str_is_binary        ( const char *str );

#ifdef __cplusplus
}
#endif

#endif
