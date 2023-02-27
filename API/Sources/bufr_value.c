/*
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

 * fichier : bufr_value.c
 *
 * author:  Vanh Souvanlasy 
 *
 * function: 
 *

*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>


#include "bufr_io.h"
#include "bufr_ieee754.h"
#include "bufr_value.h"
#include "private/bufr_priv_value.h"
#include "bufr_i18n.h"
#include "bufr_util.h"
#include "private/gcmemory.h"
#include "config.h"

static void * ValueINT8_gcmemory=NULL;
static void * ValueINT16_gcmemory=NULL;
static void * ValueINT32_gcmemory=NULL;
static void * ValueINT64_gcmemory=NULL;
static void * ValueFLT32_gcmemory=NULL;
static void * ValueFLT64_gcmemory=NULL;
static void * ValueSTRING_gcmemory=NULL;

/**
 * @english
 * @brief create a new BufrValue structure
 *
 * Note that you need to take care when creating values to ensure that the
 * chosen type can represent any of the values you might assign. In particular,
 * be aware that integer types represent -1 as "missing".
 * 
 * @param type the kind of value to create (see ValueType enum)
 * @return pointer to newly allocated BufrValue or NULL on failure
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @see ValueType, bufr_duplicate_value
 * @bug should be checking the results of malloc() calls
 * @ingroup descriptor
 */
BufrValue  *bufr_create_value( ValueType type )
   {
   BufrValue     *bv = NULL;

#if USE_GCMEMORY
   if (ValueINT8_gcmemory == NULL)
      {
      ValueINT8_gcmemory   = gcmem_new(GCMEM_VALINT8_SIZE, sizeof(ValueINT8) );
      ValueINT32_gcmemory  = gcmem_new(GCMEM_VALINT32_SIZE, sizeof(ValueINT32) );
      ValueINT64_gcmemory  = gcmem_new(GCMEM_VALINT64_SIZE, sizeof(ValueINT64) );
      ValueFLT32_gcmemory  = gcmem_new(GCMEM_VALFLT32_SIZE, sizeof(ValueFLT32) );
      ValueFLT64_gcmemory  = gcmem_new(GCMEM_VALFLT64_SIZE, sizeof(ValueFLT64) );
      ValueSTRING_gcmemory = gcmem_new(GCMEM_VALSTRING_SIZE, sizeof(ValueSTRING) );
      }
#endif

   switch( type )
      {
      case VALTYPE_INT8 :
         {
         ValueINT8    *v;

#if USE_GCMEMORY
         v         = gcmem_alloc(ValueINT8_gcmemory);
#else
         v         = (ValueINT8 *)malloc( sizeof(ValueINT8) );
#endif
         v->type   = type;
         v->value  = -1;
         v->af     = NULL;
         bv         = (BufrValue *)v;
         }
         break;
      case VALTYPE_INT32 :
         {
         ValueINT32    *v1;

#if USE_GCMEMORY
         v1         = gcmem_alloc(ValueINT32_gcmemory);
#else
         v1         = (ValueINT32 *)malloc( sizeof(ValueINT32) );
#endif
         v1->type   = type;
         v1->value  = -1;
         v1->af     = NULL;
         bv         = (BufrValue *)v1;
         }
         break;
      case VALTYPE_INT64 :
         {
         ValueINT64    *v2;

#if USE_GCMEMORY
         v2         = gcmem_alloc(ValueINT64_gcmemory);
#else
         v2         = (ValueINT64 *)malloc( sizeof(ValueINT64) );
#endif
         v2->type   = type;
         v2->value  = -1;
         v2->af     = NULL;
         bv         = (BufrValue *)v2;
         }
         break;
      case VALTYPE_FLT32 :
         {
         ValueFLT32    *v3;

#if USE_GCMEMORY
         v3         = gcmem_alloc(ValueFLT32_gcmemory);
#else
         v3         = (ValueFLT32 *)malloc( sizeof(ValueFLT32) );
#endif
         v3->type   = type;
         v3->value  = bufr_get_max_float();
         v3->af     = NULL;
         bv         = (BufrValue *)v3;
         }
         break;
      case VALTYPE_FLT64 :
         {
         ValueFLT64    *v4;

#if USE_GCMEMORY
         v4         = gcmem_alloc(ValueFLT64_gcmemory);
#else
         v4         = (ValueFLT64 *)malloc( sizeof(ValueFLT64) );
#endif
         v4->type   = type;
         v4->value  = bufr_get_max_double();
         v4->af     = NULL;
         bv         = (BufrValue *)v4;
         }
         break;
      case VALTYPE_STRING :
         {
         ValueSTRING   *v5;
#if USE_GCMEMORY
         v5         = gcmem_alloc(ValueSTRING_gcmemory);
#else
         v5         = (ValueSTRING *)malloc( sizeof(ValueSTRING) );
#endif
         v5->type   = type;
         v5->value  = NULL;
         v5->len    = 0;
         v5->af     = NULL;
         bv         = (BufrValue *)v5;
         }
         break;
      default :
         bv         = (BufrValue *)malloc( sizeof(BufrValue) );
         bv->type   = VALTYPE_UNDEFINE;
         bv->af     = NULL;
         break;
      }

   return bv;
   }

/**
 * @english
 * @brief duplicate a BufrValue structure
 * 
 * @param bv pointer to BufrValue structure to copy
 * @return pointer to newly allocated BufrValue or NULL on failure
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @see ValueType, bufr_create_value
 * @bug failure to check results of bufr_create_value
 * @ingroup descriptor
 */
BufrValue  *bufr_duplicate_value( const BufrValue *bv )
   {
   BufrValue     *dup;

   if (bv == NULL) 
      {
      bufr_print_debug( _("Error: cannot copy NULL in bufr_duplicate_value\n") );
      return NULL;
      }

   dup = bufr_create_value( bv->type );
   bufr_copy_value( dup, bv );

   if (bv->af)
      {
      dup->af = bufr_duplicate_af( bv->af );
      }
   return dup;
   }

/**
 * @english
 * @brief free a BufrValue structure
 * 
 * @param bv pointer to BufrValue structure to free
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @see ValueType, bufr_create_value, bufr_duplicate_value
 * @ingroup descriptor
 */
void bufr_free_value( BufrValue *bv )
   {
   if (bv == NULL) return;

   if (bv->af)
      {
      bufr_free_af( bv->af );
      bv->af = NULL;
      }

   switch( bv->type )
      {
      case VALTYPE_INT8 :
#if USE_GCMEMORY
         gcmem_dealloc(ValueINT8_gcmemory, bv);
#else
         free( bv );
#endif
         break;
      case VALTYPE_INT32 :
#if USE_GCMEMORY
         gcmem_dealloc(ValueINT32_gcmemory, bv);
#else
         free( bv );
#endif
         break;
      case VALTYPE_INT64 :
#if USE_GCMEMORY
         gcmem_dealloc(ValueINT64_gcmemory, bv);
#else
         free( bv );
#endif
         break;
      case VALTYPE_FLT32 :
#if USE_GCMEMORY
         gcmem_dealloc(ValueFLT32_gcmemory, bv);
#else
         free( bv );
#endif
         break;
      case VALTYPE_FLT64 :
#if USE_GCMEMORY
         gcmem_dealloc(ValueFLT64_gcmemory, bv);
#else
         free( bv );
#endif
         break;
      case VALTYPE_STRING :
         {
         ValueSTRING *s = (ValueSTRING *)bv;

         if (s->value != NULL) 
            {
            free( s->value );
            s->value = NULL;
            s->len = 0;
            }
         }
#if USE_GCMEMORY
         gcmem_dealloc(ValueSTRING_gcmemory, bv);
#else
         free( bv );
#endif
         break;
      case VALTYPE_UNDEFINE :
      default :
#if (!USE_GCMEMORY)
        free( bv );
#endif
         break;
      }
   }

/**
 * @english
 * @brief copy the contents of one BufrValue to another
 *
 * Note that this copy may be lossy depending on the source and destination
 * types. Assigning a 64-bit floating point to an 8-bit integer, for
 * example, is usually a bad idea.
 * 
 * @param dest destination of copy
 * @param src source of copy
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @see ValueType, bufr_create_value
 * @bug no return value in case copy fails (NULL dest/src, or an UNDEFINED
 * value type?)
 * @bug should somehow generate an error when assigning incompatible values
 * (i.e. out-of-range 64-bit vals assigned to 32 or 8-bit integers)
 * @ingroup descriptor
 */
void bufr_copy_value( BufrValue *dest, const BufrValue *src )
   {
   if (dest == NULL) return;
   if (src == NULL)  return;

   switch( src->type )
      {
      case VALTYPE_INT8 :
         {
         ValueINT8 *vs=(ValueINT8 *)src;
         bufr_value_set_int32( dest, vs->value );
         }
         break;
      case VALTYPE_INT32 :
         {
         ValueINT32 *vs=(ValueINT32 *)src;
         bufr_value_set_int32( dest, vs->value );
         }
         break;
      case VALTYPE_INT64 :
         {
         ValueINT64 *vs=(ValueINT64 *)src;
         bufr_value_set_int64( dest, vs->value );
         }
         break;
      case VALTYPE_FLT32 :
         {
         ValueFLT32 *vs=(ValueFLT32 *)src;
         bufr_value_set_float( dest, vs->value );
         }
         break;
      case VALTYPE_FLT64 :
         {
         ValueFLT64 *vs=(ValueFLT64 *)src;
         bufr_value_set_double( dest, vs->value );
         }
         break;
      case VALTYPE_STRING :
         {
         ValueSTRING *vs=(ValueSTRING *)src;
         if (dest->type == VALTYPE_STRING)
            {
            bufr_value_set_string( dest, vs->value, vs->len );
            }
         }
         break;
      default :
         break;
      }
   }

/**
 * @english
 * @brief assign a character string to a BufrValue
 *
 * This assignment will fail if bv->type is not VALTYPE_STRING.
 * 
 * @param bv pointer to BufrValue structure to change
 * @param str character string to assign. If NULL, the existing string will
 * be padded/truncated to len.
 * @param len maximum number of bytes from str to copy. If less than len
 * bytes are available in str, the set string will be padded with spaces.
 * Note that len should normally be the datawidth of the descriptor. The
 * encoder will try to compensate if necessary, but results may vary.
 * @return zero on success, non-zero on failure
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @see bufr_value_get_string
 * @bug unchecked malloc
 * @ingroup descriptor
 */
int bufr_value_set_string
   ( BufrValue *bv, const char *str, int len )
   {
   int           i;
   char         *old_str = NULL;
   int           old_len=0;
   char         *value;
   ValueSTRING  *vstr=NULL;
   int           rtrn;
   int           string_is_missing;

   rtrn = -1;
   if (bv == NULL) return rtrn;

   if (bv->type == VALTYPE_STRING)
      {
      vstr = (ValueSTRING *)bv;
      if (vstr->value) 
         {
         old_str = vstr->value;
         old_len = vstr->len;
         vstr->value = NULL;
         }
      }
   else
      {
      return rtrn;
      }


   if ((old_len == len)&& old_str)
      {
      value = old_str;
      old_str = NULL;
      }
   else
      {
      value = (char *)malloc( (len+1) * sizeof(char) );
      }

   rtrn = 1;
   i = 0;
   string_is_missing = 1;

   if (str)
      {
      int  len2;

      len2 = strlen( str );
      if (len2 > len) 
         len2 = len;
      for (; i < len2 ; i++)
         {
         value[i] = str[i];
         if (str[i] != '\377')
            string_is_missing = 0;
         }
      }

/*
 * padding with blanks if has string
 * if not pad with missing
 */
   if (string_is_missing == 0)
      {
      for (; i < len ; i++)
         value[i] = ' ';
      }
   else
      {
      for (; i < len ; i++)
         value[i] = '\377';
      }
   value[len] = '\0';
   if ( old_str ) free( old_str );

   vstr->value = value;
   vstr->len   = len;

   return rtrn;
   }

/**
 * @english
 * @brief assign a 32-bit integer to a BufrValue
 * 
 * This assignment will not work for some types of BufrValue, and may
 * be lossy depending on ranges.
 * 
 * @param bv pointer to BufrValue structure to change
 * @param value 32-bit integer value to assign
 * @return 1 on success, -1 on failure
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @see bufr_value_get_int32
 * @bug should we be range checking to make sure the value fits?
 * @todo 1 and -1 are non-standard return codes
 * @ingroup descriptor
 */
int bufr_value_set_int32( BufrValue *bv, int value )
   {
   switch (bv->type)
      {
      case VALTYPE_FLT32 :
         {
         ValueFLT32 *v = (ValueFLT32 *)bv;

         v->value = (float)value;
         }
      break;
      case VALTYPE_FLT64 :
         {
         ValueFLT64 *v = (ValueFLT64 *)bv;

         v->value = (double)value;
         }
      break;
      case VALTYPE_INT32 :
         {
         ValueINT32 *v = (ValueINT32 *)bv;

         v->value = value;
         }
      break;
      case VALTYPE_INT8 :
         {
         ValueINT8 *v = (ValueINT8 *)bv;

         v->value = value;
         }
      break;
      case VALTYPE_INT64 :
         {
         ValueINT64 *v = (ValueINT64 *)bv;

         v->value = (int64_t)value;
         }
      break;
      default :
         return -1;
      break;
      }
   return 1;
   }

/**
 * @english
 * @brief assign a 64-bit integer to a BufrValue
 * 
 * This assignment will not work for some types of BufrValue, and may
 * be lossy depending on ranges or types.
 * 
 * @param bv pointer to BufrValue structure to change
 * @param value 64-bit integer value to assign
 * @return 1 on success, -1 on failure
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @see bufr_value_get_int64
 * @bug should we be range checking to make sure the value fits?
 * @todo 1 and -1 are non-standard return codes
 * @ingroup descriptor
 */
int bufr_value_set_int64( BufrValue *bv, int64_t value )
   {
   int   rtrn;

   rtrn = -1;
   if (bv->type == VALTYPE_FLT32)
      {
      ValueFLT32 *v = (ValueFLT32 *)bv;

      v->value = (float)value;
      rtrn = 1;
      }
   else if (bv->type == VALTYPE_FLT64)
      {
      ValueFLT64 *v = (ValueFLT64 *)bv;

      v->value = (double)value;
      rtrn = 1;
      }
   else if (bv->type == VALTYPE_INT8)
      {
      ValueINT8 *v = (ValueINT8 *)bv;

      v->value = (int8_t)value;
      rtrn = 1;
      }
   else if (bv->type == VALTYPE_INT32)
      {
      ValueINT32 *v = (ValueINT32 *)bv;

      v->value = (int32_t)value;
      rtrn = 1;
      }
   else if (bv->type == VALTYPE_INT64)
      {
      ValueINT64 *v = (ValueINT64 *)bv;

      v->value = value;
      rtrn = 1;
      }

   return rtrn;
   }

/**
 * @english
 * @brief assign a 32-bit float to a BufrValue
 * 
 * This assignment will not work for some types of BufrValue, and may
 * be lossy depending on ranges or types.
 * 
 * @param bv pointer to BufrValue structure to change
 * @param value 32-bit float value to assign
 * @return 1 on success, -1 on failure
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @see bufr_value_get_float
 * @bug should we be range checking to make sure the value fits?
 * @todo 1 and -1 are non-standard return codes
 * @ingroup descriptor
 */
int bufr_value_set_float( BufrValue *bv, float value )
   {
   int rtrn;

   rtrn = -1;
   if (bv->type == VALTYPE_FLT32)
      {
      ValueFLT32 *v = (ValueFLT32 *)bv;

      v->value = value;
      rtrn = 1;
      }
   else if (bv->type == VALTYPE_FLT64)
      {
      ValueFLT64 *v = (ValueFLT64 *)bv;

      v->value = (double)value;
      rtrn = 1;
      }
   else if (bv->type == VALTYPE_INT8)
      {
      ValueINT8 *v = (ValueINT8 *)bv;

      if (bufr_is_missing_float(value))
         v->value = -1;
      else
         v->value = (int8_t)value;
      rtrn = 1;
      }
   else if (bv->type == VALTYPE_INT32)
      {
      ValueINT32 *v = (ValueINT32 *)bv;

      if (bufr_is_missing_float(value))
         v->value = -1;
      else
         v->value = (int32_t)value;
      rtrn = 1;
      }
   else if (bv->type == VALTYPE_INT64)
      {
      ValueINT64 *v = (ValueINT64 *)bv;

      if (bufr_is_missing_float(value))
         v->value = -1;
      else
         v->value = (int64_t)value;
      rtrn = 1;
      }
   return rtrn;
   }

/**
 * @english
 * @brief assign a 64-bit floating point to a BufrValue
 * 
 * This assignment will not work for some types of BufrValue, and may
 * be lossy depending on ranges or types.
 * 
 * @param bv pointer to BufrValue structure to change
 * @param value floating point-bit value to assign
 * @return 1 on success, -1 on failure
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @see bufr_value_get_double
 * @bug should we be range checking to make sure the value fits?
 * @todo 1 and -1 are non-standard return codes
 * @ingroup descriptor
 */
int bufr_value_set_double( BufrValue *bv, double value )
   {
   int rtrn;

   rtrn = -1;
   if (bv->type == VALTYPE_FLT32)
      {
      ValueFLT32 *v = (ValueFLT32 *)bv;

      v->value = (float)value;
      rtrn = 1;
      }
   else if (bv->type == VALTYPE_FLT64)
      {
      ValueFLT64 *v = (ValueFLT64 *)bv;

      v->value = value;
      rtrn = 1;
      }
   else if (bv->type == VALTYPE_INT8)
      {
      ValueINT8 *v = (ValueINT8 *)bv;

      if (bufr_is_missing_double(value))
         v->value = -1;
      else
         v->value = (int8_t)value;
      rtrn = 1;
      }
   else if (bv->type == VALTYPE_INT32)
      {
      ValueINT32 *v = (ValueINT32 *)bv;

      if (bufr_is_missing_double(value))
         v->value = -1;
      else
         v->value = (int32_t)value;
      rtrn = 1;
      }
   else if (bv->type == VALTYPE_INT64)
      {
      ValueINT64 *v = (ValueINT64 *)bv;

      if (bufr_is_missing_double(value))
         v->value = -1;
      else
         v->value = (int64_t)value;
      rtrn = 1;
      }
   return rtrn;
   }

/**
 * @english
 * @brief get a 32-bit integer from a BufrValue
 * 
 * This function will not work for some types of BufrValue, and may
 * be lossy depending on ranges or types.
 * 
 * @param bv pointer to BufrValue structure to query
 * @return -1 on failure, otherwise the integer value.
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @see bufr_value_set_int32
 * @bug should we be range checking to make sure the returned value fits
 * into 32 bits? And should we differentiate between a real missing value
 * and a range error? Should we use errno?
 * @bug should explicitly return bufr_missing_int() rather than -1.
 * @ingroup descriptor
 */
int32_t bufr_value_get_int32( const BufrValue *bv )
   {
   float  fval;
   double dval;

   if (bv == NULL)  return -1;

   switch( bv->type )
      {
      case VALTYPE_INT8 :
         {
         const ValueINT8 *v = (ValueINT8 *)bv;
         return (int32_t)v->value;
         }
      case VALTYPE_INT32 :
         {
         const ValueINT32 *v = (ValueINT32 *)bv;
         return (int32_t)v->value;
         }
      case VALTYPE_INT64 :
         return (int32_t)bufr_value_get_int64( bv );
      case VALTYPE_FLT32 :
         fval = bufr_value_get_float( bv );
         if (bufr_is_missing_float(fval)) return -1;
         return (int32_t)fval;
      case VALTYPE_FLT64 :
         dval = bufr_value_get_double( bv );
         if (bufr_is_missing_double(dval)) return -1;
         return (int32_t)dval;
      default :
         break;
      }
   return -1;
   }

/**
 * @english
 * @brief get a 64-bit integer from a BufrValue
 * 
 * This function will not work for some types of BufrValue, and may
 * be lossy depending on ranges or types.
 * 
 * @param bv pointer to BufrValue structure to query
 * @return -1 on failure, otherwise the integer value.
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @see bufr_value_set_int64
 * @bug should we be range checking to make sure the returned value converts
 * to an integer? And should we differentiate between a real missing value
 * and a range error? Should we use errno?
 * @bug should explicitly return bufr_missing_int() rather than -1.
 * @ingroup descriptor
 */
int64_t bufr_value_get_int64( const BufrValue *bv )
   {
   float  fval;
   double dval;

   if (bv == NULL)  return -1;

   switch( bv->type )
      {
      case VALTYPE_INT64 :
         {
         const ValueINT64 *v = (ValueINT64 *)bv;
         return (int64_t)v->value;
         }
      case VALTYPE_INT8 :
      case VALTYPE_INT32 :
         return (int64_t)bufr_value_get_int32( bv );
      case VALTYPE_FLT32 :
         fval = bufr_value_get_float( bv );
         if (bufr_is_missing_float(fval)) return -1;
         return (int64_t)fval;
      case VALTYPE_FLT64 :
         dval = bufr_value_get_double( bv );
         if (bufr_is_missing_double(dval)) return -1;
         return (int64_t)dval;
      default :
         break;
      }
   return -1;
   }

/**
 * @english
 * @brief get a 32-bit float from a BufrValue
 * 
 * This function will not work for some types of BufrValue, and may
 * be lossy depending on ranges or types.
 * 
 * @param bv pointer to BufrValue structure to query
 * @return max float on failure, otherwise the floating point value.
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @see bufr_get_max_float, bufr_value_set_float
 * @bug should we be range checking to make sure the returned value fits
 * into a float? And should we differentiate between a real missing value
 * and a range error? Should we use errno?
 * @bug should be checking integers against bufr_missing_int() rather than -1
 * @bug missing double value needs to be converted to missing max_float.
 * @bug should be using bufr_missing_float() rather than bufr_max_float().
 * @ingroup descriptor
 */
float bufr_value_get_float( const BufrValue *bv )
   {
   if (bv == NULL) return bufr_get_max_float();

   switch( bv->type )
      {
      case VALTYPE_FLT32 :
         {
         const ValueFLT32 *v = (ValueFLT32 *)bv;
         return (float)v->value;
         }
      case VALTYPE_FLT64 :
         {
         double dval = bufr_value_get_double( bv );

         if ( bufr_is_missing_double(dval) ) return bufr_get_max_float();
         return (float)dval;
         }
      case VALTYPE_INT8 :
      case VALTYPE_INT32 :
         {
         int32_t  ival;

         ival = bufr_value_get_int32( bv );
         if (ival == -1) return bufr_get_max_float();
         return (float)ival;
         }
      case VALTYPE_INT64 :
         {
         int64_t  ival;

         ival = bufr_value_get_int64( bv );
         if (ival == -1) return bufr_get_max_float();
         return (float)ival;
         }
      default :
         break;
      }
   return bufr_get_max_float();
   }

/**
 * @english
 * @brief get a double from a BufrValue
 * 
 * This function will not work for some types of BufrValue (i.e. strings).
 * 
 * @param bv pointer to BufrValue structure to query
 * @return max double on failure, otherwise the floating point value.
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @see bufr_get_max_double, bufr_value_set_double
 * @bug should we differentiate between a real missing value
 * and a range error? Should we use errno?
 * @bug should be checking integers against bufr_missing_int() rather than -1
 * @bug missing float value needs to be converted to missing max double.
 * @bug should be using bufr_missing_double() rather than bufr_max_float().
 * @ingroup descriptor
 */
double bufr_value_get_double( const BufrValue *bv )
   {
   if (bv == NULL) return bufr_get_max_double();

   switch( bv->type )
      {
      case VALTYPE_FLT64 :
         {
         const ValueFLT64 *v = (ValueFLT64 *)bv;
         return (double)v->value;
         }
      case VALTYPE_FLT32 :
         {
         float fval = bufr_value_get_float( bv );

         if ( bufr_is_missing_float(fval) ) return bufr_get_max_double();
         return (double)fval;
         }
      case VALTYPE_INT8 :
      case VALTYPE_INT32 :
         {
         int32_t  ival;

         ival = bufr_value_get_int32( bv );
         if (ival == -1) return bufr_get_max_double();
         return (double)ival;
         }
      case VALTYPE_INT64 :
         {
         int64_t  ival;

         ival = bufr_value_get_int64( bv );
         if (ival == -1) return bufr_get_max_double();
         return (double)ival;
         }
      default :
         break;
      }
   return bufr_get_max_double();
   }

/**
 * @english
 * @brief get a string from a BufrValue
 * 
 * This function will only work for string types of BufrValue. Note that
 * this returns a pointer to the internal string value, which may change or
 * become invalidated due to changes elsewhere. Note that BUFR strings
 * are stored space-filled to the descriptor data width.
 * 
 * @param bv pointer to BufrValue structure to query
 * @param len pointer into which to return the length of the string
 * @return NULL on failure, otherwise a pointer to the string.
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @see bufr_value_set_string
 * @bug should be using errno
 * @bug doesn't test for len==NULL
 * @bug should len be a size_t rather than int?
 * @ingroup descriptor
 */
const char *bufr_value_get_string( const BufrValue *bv, int *len )
   {
   if (bv == NULL) 
      {
      *len = 0;
      return NULL;
      }
   switch( bv->type )
      {
      case VALTYPE_STRING :
         {
         const ValueSTRING *strval = (ValueSTRING *)bv;

         *len = strval->len;
         return strval->value;
         }
      default :
         break;
      }
   *len = 0;
   return NULL;
   }

/**
 * @english
 * @brief determine the "missing" integer value for a specific bit length
 * 
 * @param nbits missing value for the number of bits
 * @return appropriate missing value
 * @endenglish
 * @francais
 * @brief retourne la valeur manquante correspondant au nombre de bits
 * @param nbits nombre de bits
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @see bufr_value_is_missing
 * @bug should be using errno (ERANGE, in particular)
 * @ingroup descriptor
 */
uint64_t bufr_missing_ivalue( int nbits )
   {
   static uint64_t msng_values[65];
   static int      initialized=0;

   if (initialized == 0)
      {
      int i;
      msng_values[0] = 0;
      for (i = 1; i <= 64 ; i++)
         {
         msng_values[i] = (1ULL << i) - 1L;
         }
      initialized=1;
      }

   if (nbits <= 0) return 0;
   if (nbits >= 64) return msng_values[64];
   return msng_values[nbits];
//   uint64_t v;
//   if ((nbits >= 64)||(nbits <= 0)) return -1;
//   v = (1ULL << nbits) - 1L;
//   return v;
   }

/**
 * @english
 * 
 * @param nbits value for the number of bits
 * @return ?
 * @todo translate to English
 * @endenglish
 * @francais
 * @brief retourne une valeur correspondant au nombre de bits
 * @param nbits nombre de bits
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @bug should be using errno (ERANGE?)
 * @todo Vanh, this one isn't clear...
 * @author  Vanh Souvanlasy
 * @ingroup descriptor
 */
int64_t bufr_cvt_ivalue( uint64_t value, int nbits )
   {
   uint64_t         missing;
   int64_t         signbit;

   missing = bufr_missing_ivalue( nbits );
   if (value == missing) return -1;

   signbit = 1ULL << (nbits-1);
   if (value & signbit)
      {
      value = (value % signbit) * -1;
      }
   return value;
   }

/**
 * @english
 * 
 * @param nbits value for the number of bits
 * @return ?
 * @todo translate to English
 * @endenglish
 * @francais
 * @brief retourne une valeur negative correspondant au nombre de bits
 * @param nbits nombre de bits
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @bug should be using errno (ERANGE?)
 * @bug silently cuts max number of bits to 64
 * @todo Vanh, this one isn't clear...
 * @author  Vanh Souvanlasy
 * @ingroup descriptor
 */
uint64_t bufr_negative_ivalue( int64_t value, int nbits )
   {
   uint64_t  v;
   int       minbits;

   if (value >= 0) return value;

   if (nbits > 64) 
      nbits = 64;
   else if (nbits <= 0) 
      return -1;

   minbits = bufr_value_nbits( value );
   if (minbits > nbits)
      {
      char errmsg[256];

      sprintf( errmsg, _n("Warning: %lld needs at least %d bit for storage\n", 
                          "Warning: %lld needs at least %d bits for storage\n", minbits), 
               (long long)value, minbits );
      bufr_print_debug( errmsg );
      }

   v = abs( value );
   v |= (1ULL << (nbits-1));
   return v;
   }

/**
 * @english
 * This call prints the BufrValue structure as a formatted
 * value into the string passed as a parameter.
 * @return Int, if 0 there is no value, if 1, there was something to print.
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @code
 * bufr_print_value( errmsg, bcv->value )
 * @endcode
 * @bug caller should be able to pass a max length of outstr
 * @bug don't really need a temporary variable, do we?
 * @ingroup descriptor io debug
 */
int bufr_print_value( char *outstr, const BufrValue *bv )
   {
   int status;

   status = bufr_print_scaled_value( outstr, bv, INT_MAX );
   return status;
   }

/**
 * @english
 * This call prints the BufrValue structure as a formatted
 * value into the string passed as a parameter using scale from Table B
 * @return 0 if no value, 1 if there was something to print.
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @code
 * bufr_print_scaled_value( errmsg, bcv->value, scale );
 * @endcode
 * @bug caller should be able to pass a max length of outstr
 * @ingroup descriptor io debug
 */
int bufr_print_scaled_value( char *outstr, const BufrValue *bv, int scale )
   {
   const char   *str;
   float   fval;
   double  dval;
   int     ival;
   int64_t lval;
   int     len;
   int     hasvalue=0;

   if (bv == NULL) return 0;
   if (outstr == NULL) return 0;

   outstr[0] = '\0';

   switch (bv->type)
      {
      case VALTYPE_STRING :
         str = bufr_value_get_string( bv, &len );
         if (str)
            {
            char *str1;

            str1 = (char *)malloc( (len + 1)*sizeof(char) );
            strncpy( str1, str, len );
            str1[len] = '\0';

            sprintf( outstr, "\"%s\"", str1 );
            free( str1 );
            }
         else
            {
            strcat(  outstr, "MSNG" );
            }
         hasvalue = 1;
         break;
      case VALTYPE_INT8  :
         ival = bufr_value_get_int32( bv );
         if ( ival == -1 )
            {
            strcat(  outstr, "MSNG" );
            }
         else
            {
            sprintf(  outstr, "%d", ival );
            }
         hasvalue = 1;
         break;
      case VALTYPE_INT32  :
         ival = bufr_value_get_int32( bv );
         if ( ival == -1 )
            {
            strcat(  outstr, "MSNG" );
            }
         else
            {
            sprintf(  outstr, "%d", ival );
            }
         hasvalue = 1;
         break;
      case VALTYPE_INT64  :
         lval = bufr_value_get_int64( bv );
         if ( lval == -1 )
            {
            strcat(  outstr, "MSNG" );
            }
         else
            {
            sprintf(  outstr, "%lld", (long long)lval );
            }
         hasvalue = 1;
         break;
      case VALTYPE_FLT32  :
         fval = bufr_value_get_float( bv );
         if (bufr_is_missing_float(fval))
            {
            strcat(  outstr, "MSNG" );
            }
         else
            {
            if (scale == INT_MAX)
               {
               if ((fval < 0.00001 )||(fval > INT_MAX))
                  sprintf(  outstr, "%.14E", fval );
               else
                  {
                  bufr_print_float( outstr, fval );
                  }
               }
            else
               {
               bufr_print_scaled_float( outstr, fval, scale );
               }
            }
         hasvalue = 1;
         break;
      case VALTYPE_FLT64  :
         dval = bufr_value_get_double( bv );
         if (bufr_is_missing_double(dval))
            strcat(  outstr, "MSNG" );
         else
            {
            if (scale == INT_MAX)
               {
               if ((dval < 0.00001 )||(dval > INT_MAX))
                  sprintf(  outstr, "%.14E", dval );
               else
                  {
                  bufr_print_double( outstr, dval );
                  }
               }
            else
               {
               bufr_print_scaled_double( outstr, dval, scale );
               }
            }
         hasvalue = 1;
         break;
      default :
         break;
      }
   return hasvalue;
   }

/**
 * @english
 * tests a double value to determine if it represents a missing BUFR value
 * @return non-zero if it's the missing value
 * @warn will also return non-zero if the double is NaN.
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @see bufr_value_is_missing
 * @ingroup descriptor
 */
int bufr_is_missing_double( double d )
   {
   if (isnan( d )) return 1;
   if (isinf( d )) return 1;


   if ( d == bufr_get_max_double() ) return 1;

   return 0;
   }

/**
 * @english
 * tests a float value to determine if it represents a missing BUFR value
 * @return non-zero if it's the missing value
 * @warn will also return non-zero if the float is NaN.
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @see bufr_value_is_missing
 * @ingroup descriptor
 */
int bufr_is_missing_float( float f )
   {
   if (isnan( f )) return 1;
   if (isinf( f )) return 1;

   if ( f == bufr_get_max_float() ) return 1;

   return 0;
   }

/**
 * @english
 * tests an integer value to see if it represents the "missing" value
 * @return non-zero if it's the missing value
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @see bufr_missing_int, bufr_value_is_missing
 * @bug should be testing against bufr_missing_int()
 * @ingroup descriptor
 */
int bufr_is_missing_int( int i )
   {
   if ( i == -1 ) return 1;

   return 0;
   }

/**
 * @english
 * tests a string value to determine if it represents a missing BUFR value
 * @return non-zero if it's the missing value
 * @warn will also return non-zero if the double is NaN.
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @see bufr_value_is_missing
 * @ingroup descriptor
 */
int bufr_is_missing_string( const char *str, int len )
   {
   int  i;
   unsigned char c;
/*
 * skip trailing blanks if any
 */
   for (i = len; i > 1 ; i--)
      {
      c = (unsigned char)str[i-1];
      if (c != 32) break;
      }
   len = i;
   for (i = 0; i < len ; i++)
      {
      c = (unsigned char)str[i];
      if (c != 255) return 0;
      }

   return 1;
   }

/**
 * @english
 * return the representation of a missing double
 * @return value representing MISSING in double
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @see bufr_is_missing_double, bufr_value_is_missing
 * @ingroup descriptor
 */
double bufr_missing_double(void)
   {
   return bufr_get_max_double();
   }

/**
 * @english
 * return the representation of a missing float
 * @return value representing MISSING in float
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @see bufr_is_missing_float, bufr_value_is_missing
 * @ingroup descriptor
 */
float bufr_missing_float(void)
   {
   return bufr_get_max_float();
   }

/**
 * @english
 * return the representation of a missing integer. This value is
 * identical for all sizes of integers.
 * @return value representing MISSING in integer
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @see bufr_is_missing_int, bufr_value_is_missing
 * @ingroup descriptor
 */
int bufr_missing_int(void)
   {
   return -1;
   }

/**
 * @english
 * set the representation of a missing string. This value is
 * set as  -1 in every characters of a string
 * @return none
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @see bufr_is_missing_int, bufr_value_is_missing
 * @ingroup descriptor
 */
void bufr_missing_string(char *str, int len)
   {
   int i;
   char cmsng;

   cmsng = (char)255;
   for (i = 0; i < len ; i++)
      str[i] = cmsng;
   }
/**
 * @english
 *
 * This call checks any type of value to see if it contains a "missing"
 * BUFR value appropriate for that type.
 *
 * @return zero if the value is not the BUFR missing value
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @code
 * BufrValue* bv;
 * ...
 * if( bufr_value_is_missing( bv ) ) continue;
 * @endcode
 * @bug also returns "missing" if the type of BUFR value isn't handled,
 * which should really be an error (i.e. set errno).
 * @ingroup descriptor
 */
int bufr_value_is_missing( BufrValue* bv )
	{
	switch( bv->type )
		{
		case VALTYPE_INT8:
		case VALTYPE_INT32:
			return bufr_missing_int() == bufr_value_get_int32(bv);
		case VALTYPE_INT64:
			return bufr_missing_int() == bufr_value_get_int64(bv);
		case VALTYPE_FLT32:
			return bufr_is_missing_float( bufr_value_get_float(bv) );
		case VALTYPE_FLT64:
			return bufr_is_missing_double( bufr_value_get_double(bv) );
		case VALTYPE_STRING:
			{
			int l;
         const char *str;

         str = bufr_value_get_string( bv, &l ); 
         return  bufr_is_missing_string( str, l );
			}
		default:
			break;
		}

	/* If we don't recognize the type, assume it's missing */
	return 1;
	}

/**
 * @english
 * Determine if two BufrValue structures are equal
 * @param bv1 first value to compare
 * @param bv2 second value to compare
 * @param eps floating point values closer than this value are considered equal
 * @return zero if values are equal, allowing for some type conversions
 * @warn results may be odd if values are of incompatible types
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @bug string values use strncmp, but numeric values only ever return 0 or -1
 * @bug no enforcing that values are compatible types
 * @bug not checking for NULL when handling strings... strncmp() will core
 * @bug no checking for missing value; can two "missing" values be
 * considered equal?
 * @ingroup descriptor
 */
int bufr_compare_value( const BufrValue *bv1, const BufrValue *bv2, double eps )
   {
   switch( bv1->type )
      {
      case VALTYPE_INT8 :
      case VALTYPE_INT32 :
         {
         int32_t  i1, i2;

         i1 = bufr_value_get_int32( bv1 );
         i2 = bufr_value_get_int32( bv2 );
         if (i1 == i2) return 0;
         }
         break;
      case VALTYPE_INT64 :
         {
         int64_t  i1, i2;

         i1 = bufr_value_get_int64( bv1 );
         i2 = bufr_value_get_int64( bv2 );
         if (i1 == i2) return 0;
         }
         break;
      case VALTYPE_FLT32 :
         {
         float  f1, f2;

         f1 = bufr_value_get_float( bv1 );
         f2 = bufr_value_get_float( bv2 );
         if (fabsf(f1-f2) <= eps) return 0;
          }
         break;
      case VALTYPE_FLT64 :
         {
         double  f1, f2;

         f1 = bufr_value_get_double( bv1 );
         f2 = bufr_value_get_double( bv2 );
         if (fabs(f1-f2) <= eps) return 0;
/*         if (f1 == f2) return 0; */
         }
         break;
      case VALTYPE_STRING :
         {
         const char  *s1, *s2;
         int    len1,len2;
         int    len;

         s1 = bufr_value_get_string( bv1, &len1 );
         s2 = bufr_value_get_string( bv2, &len2 );
         len = (len1 < len2) ? len1 : len2;
         return strncmp ( s1, s2, len );
         }
      default :
         break;
      }
   return -1;
   }

/**
 * @english
 * Determine if a BufrValue falls in between two others.
 *
 * Note that a value is considered in between if it equals either value.
 * @param bv1 first value to compare
 * @param bv value to test for betweeness
 * @param bv2 second value to compare
 * @return negative on error, 1 if bv1 <= bv <= bv2.
 * @warn values must be of compatible types
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @bug string values use strncmp, but numeric values only ever return 0 or -1
 * @bug no enforcing that values are compatible types
 * @bug not checking for NULL when handling strings... strcmp() will core
 * @bug string comparison seems incorrect. It probably should be testing
 * for strcmp(s1,s) <= strcmp(s,s2) ?
 * @bug no checking for missing value; does it mean anything to compare
 * against a missing value
 * @ingroup descriptor
 */
int bufr_between_values( const BufrValue *bv1, const BufrValue *bv, const BufrValue *bv2 )
   {
   if ((bv1->type != bv->type)||(bv2->type != bv->type)) return -1;

   switch( bv1->type )
      {
      case VALTYPE_INT8 :
      case VALTYPE_INT32 :
         {
         int32_t  i1, i2, ii;

         i1 = bufr_value_get_int32( bv1 );
         i2 = bufr_value_get_int32( bv2 );
         ii = bufr_value_get_int32( bv );
         if ((i1 <= ii)&&(ii <= i2)) return 1;
         }
         break;
      case VALTYPE_INT64 :
         {
         int64_t  i1, i2, ii;

         i1 = bufr_value_get_int64( bv1 );
         i2 = bufr_value_get_int64( bv2 );
         ii = bufr_value_get_int64( bv );
         if ((i1 <= ii)&&(ii <= i2)) return 1;
         }
         break;
      case VALTYPE_FLT32 :
         {
         float  f1, f2, ff;

         f1 = bufr_value_get_float( bv1 );
         f2 = bufr_value_get_float( bv2 );
         ff = bufr_value_get_float( bv );
         if ((f1 <= ff)&&(ff <= f2)) return 1;
         }
         break;
      case VALTYPE_FLT64 :
         {
         double  f1, f2, ff;

         f1 = bufr_value_get_double( bv1 );
         f2 = bufr_value_get_double( bv2 );
         ff = bufr_value_get_float( bv );
         if ((f1 <= ff)&&(ff <= f2)) return 1;
         }
         break;
      case VALTYPE_STRING :
         {
         const char  *s1, *s2, *s;
         int    len1,len2;
         int    len;

         s1 = bufr_value_get_string( bv1, &len1 );
         s2 = bufr_value_get_string( bv2, &len2 );
         s  = bufr_value_get_string( bv, &len );
         if (strcmp ( s1, s ) == 0) return 1;
         if (strcmp ( s2, s ) == 0) return 1;
         }
         break;
      default :
         break;
      }
   return 0;
   }

/**
 * @english
 * format a float value as compactly as possible (i.e. with trailing zeros
 * removed).
 *
 * @param str output string
 * @param fval value to print
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @bug should be checking str for NULL
 * @bug should take a max buffer length and use snprintf
 * @bug sprintf("%hg",fval) would be equivalent
 * @ingroup descriptor
 */
void bufr_print_float( char *str, float fval )
   {
   sprintf( str, "%f", fval );
   if (bufr_is_trimzero())
      str_trimchar( str, '0' );
   }

/**
 * @english
 * format a float value with precision matching with scale
 *
 * @param str output string
 * @param fval value to print
 * @param scale number of decimal digits
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @bug should be checking str for NULL
 * @bug should take a max buffer length and use snprintf
 * @bug sprintf("%.*hg",scale,fval) would be equivalent where scale>=0,
 * rather than dynamically building the format string
 * @ingroup descriptor
 */
void bufr_print_scaled_float( char *str, float fval, int scale )
   {
   int  len;
   char format[256];

   if (scale < 0)
      {
      strcpy ( format, "%.1f" );
      }
   else
      {
      sprintf( format, "%%.%df", scale );
      }
   sprintf( str, format, fval );
   }

/**
 * @english
 * check if a string is in binary form
 *
 * @param str input string
 * @return nonzero if the string is a binary number
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @bug should be checking str for NULL
 * @bug equivalent: n = strspn(str,"01"); return n>0 && str[n+strspn(str," ")]==0;
 * or could also be replaced with strtoll(str,&end,2)...
 * @ingroup descriptor
 */
int bufr_str_is_binary( const char *str )
   {
   int  i, len;

   len = strlen( str );
/*
 * remove trailing spaces if any

*/
   for (i = len-1; i > 0 ; i-- )
      if (isspace(str[i])) --len;
/*
 * accept only '1' or '0'

*/
   for (i = 0; i < len ; i++ )
      {
      if ((str[i] == '0')||(str[i] == '1')||((i==0)&&(str[i]=='b')))
         {
         }
      else
         return 0;
      }
   return 1;
   }

/**
 * @english
 * convert a binary value to int
 *
 * @param str input string
 * @return integer value corresponding to the binary input
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @bug should be checking str for NULL
 * @bug could be replaced with strtoll(str,&end,2)...
 * @ingroup descriptor
 */
int64_t bufr_binary_to_int( const char *str )
   {
   uint64_t  i, len;
   uint64_t  ival;
   uint64_t  bval;

   if (bufr_str_is_binary( str ) == 0) return -1;

   len = strlen( str );
   bval = 1;
   for (i = 0; i < len ; i++)
      bval = bval << 1 ;
   bval = bval >> 1;

   ival = 0;
   for (i = 0; i < len ; i++ )
      {
      if (str[i] == '1')
         {
         ival += bval;
         }
      bval = bval >> 1;
      }
   return (int64_t)ival;
   }


/**
 * @english
 * format a value in binary
 *
 * @param outstr output string
 * @param ival value to format
 * @param nbit number of bits to format
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @bug should be checking outstr for NULL
 * @bug should range check nbit
 * @ingroup descriptor debug
 */
void bufr_print_binary ( char *outstr, int64_t  ival, int nbit )
   {
   int  len;
   uint64_t  bval;

   outstr[0] = '\0';

   if (ival < 0)
      {
      len = nbit;
      while (len > 0)
         {
         strcat( outstr, "1" );
         --len;
         }
      return;
      }

   len = 0;
   for (bval = 1; (bval-1) < ival ; ) 
      {
      len += 1;
      bval = bval << 1 ;
      }
   bval = bval >> 1;
   len = nbit - len;
   while (len > 0)
      {
      strcat( outstr, "0" );
      --len;
      }

   while ( ival > 0 )
      {
      if (bval > ival)
         strcat( outstr, "0" );
      else
         {
         strcat( outstr, "1" );
         ival = ival - bval;
         }
      bval = bval >> 1;
      }

   if (bval >= 1) 
      {
      while (bval >= 1)
         {
         strcat( outstr, "0" );
         bval = bval >> 1;
         }
      }
   }

/**
 * @english
 * format a double value as compactly as possible (i.e. with trailing zeros
 * removed).
 *
 * @param str output string
 * @param dval value to print
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @bug should be checking str for NULL
 * @bug should take a max buffer length and use snprintf
 * @bug sprintf("%hg",dval) would be equivalent
 * @ingroup descriptor
 */
void bufr_print_double( char *str, double dval )
   {
   sprintf( str, "%f", dval );
   if (bufr_is_trimzero())
      str_trimchar( str, '0' );
   }

/**
 * @english
 * format a double value with precision matching with scale
 *
 * @param str output string
 * @param dval value to print
 * @param scale number of decimal digits
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author  Vanh Souvanlasy
 * @bug should be checking str for NULL
 * @bug should take a max buffer length and use snprintf
 * @bug sprintf("%.*hg",scale,dval) would be equivalent where scale>=0,
 * rather than dynamically building the format string
 * @ingroup descriptor
 */
void bufr_print_scaled_double( char *str, double dval, int scale )
   {
   int  len;
   char format[256];

   if (scale < 0)
      {
      strcpy ( format, "%.1f" );
      }
   else
      {
      sprintf( format, "%%.%df", scale );
      }
   sprintf( str, format, dval );
   }

/**
 * @english
 * @brief free internal garbage collector 
 *   free memory allocation used to create garbage collector of BufrValue
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 * @warning should be called by bufr_end_api
 */
void bufr_value_end(void)
   {
#if USE_GCMEMORY
   int size, isize;
   char  errmsg[256];

   isize = gcmem_blk_size( ValueINT8_gcmemory );
   size = gcmem_delete( ValueINT8_gcmemory );
   ValueINT8_gcmemory = NULL;
   if (gcmem_is_verbose())
      {
      sprintf( errmsg, "GCMEM used %d ValueINT8, blocs size=%d\n", size, isize );
      bufr_print_output( errmsg );
      }
   isize = gcmem_blk_size( ValueINT32_gcmemory );
   size = gcmem_delete( ValueINT32_gcmemory );
   ValueINT32_gcmemory = NULL;
   if (gcmem_is_verbose())
      {
      sprintf( errmsg, "GCMEM used %d ValueINT32, blocs size=%d\n", size, isize );
      bufr_print_output( errmsg );
      }
   isize = gcmem_blk_size( ValueINT64_gcmemory );
   size = gcmem_delete( ValueINT64_gcmemory );
   ValueINT64_gcmemory = NULL;
   if (gcmem_is_verbose())
      {
      sprintf( errmsg, "GCMEM used %d ValueINT64, blocs size=%d\n", size, isize );
      bufr_print_output( errmsg );
      }
   isize = gcmem_blk_size( ValueFLT32_gcmemory );
   size = gcmem_delete( ValueFLT32_gcmemory );
   ValueFLT32_gcmemory = NULL;
   if (gcmem_is_verbose())
      {
      sprintf( errmsg, "GCMEM used %d ValueFLT32, blocs size=%d\n", size, isize );
      bufr_print_output( errmsg );
      }
   isize = gcmem_blk_size( ValueFLT64_gcmemory );
   size = gcmem_delete( ValueFLT64_gcmemory );
   ValueFLT64_gcmemory = NULL;
   if (gcmem_is_verbose())
      {
      sprintf( errmsg, "GCMEM used %d ValueFLT64, blocs size=%d\n", size, isize );
      bufr_print_output( errmsg );
      }
   isize = gcmem_blk_size( ValueSTRING_gcmemory );
   size = gcmem_delete( ValueSTRING_gcmemory );
   ValueSTRING_gcmemory = NULL;
   if (gcmem_is_verbose())
      {
      sprintf( errmsg, "GCMEM used %d ValueSTRING, blocs size=%d\n", size, isize );
      bufr_print_output( errmsg );
      }
#endif
   }
