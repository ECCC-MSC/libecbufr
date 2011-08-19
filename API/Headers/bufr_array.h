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
 
 *  file      :  ARRAY.H
 *
 *  author    :  Souvanlasy ViengSavanh
 *
 *  revision  :  
 *
 *  status    :  DEVELOPMENT
 *
 *  language  :  C
 *
 *  object    :  HEADERS FILE FOR DYNAMIC ARRAYS
 *
 *
 */

#ifndef include_ARRAY
#define include_ARRAY

#ifdef __cplusplus
extern "C" {
#endif

typedef char  *ArrayPtr ;
typedef void  *ArrayItemPtr ;

typedef ArrayPtr IntArray;

 extern int          arr_add        ( ArrayPtr , const void * );
 extern int          arr_count      ( ArrayPtr );
 extern ArrayPtr     arr_create     ( int, int, int );
 extern int          arr_del        ( ArrayPtr, int );
 extern void         arr_free       ( ArrayPtr * );
 extern ArrayItemPtr arr_get        ( ArrayPtr, int );
 extern int          arr_inc        ( ArrayPtr, int );
 extern void         arr_reduce     ( ArrayPtr );
 extern ArrayPtr     arr_share      ( ArrayPtr );
 extern int          arr_size       ( ArrayPtr );
 extern ArrayItemPtr arr_search     ( ArrayPtr, const void *, int (*compar)(const void *, const void *) );
 extern ArrayItemPtr arr_find     ( ArrayPtr, const void *, int (*compar)(const void *, const void *) );
 extern void         arr_sort       ( ArrayPtr, int (*compar)(const void *, const void *) );
 extern int          arr_set        ( ArrayPtr, int pos, const void *elem );
 extern void         arr_free_string( ArrayPtr * );
 extern int          arr_find_string( ArrayPtr, const char *string );
 extern void         arr_add_string ( ArrayPtr, const char *string );

 extern int          arr_floatcmp   ( const void *c1, const void *c2 );
 extern int          arr_intcmp     ( const void *c1, const void *c2 );
 extern int          arr_floatdcmp  ( const void *c1, const void *c2 );
 extern int          arr_intDescCmp ( const void *c1, const void *c2 );

#ifdef __cplusplus
}
#endif

#endif /* include_ARRAY */
