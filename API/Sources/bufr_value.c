/*
Copyright Her Majesty The Queen in Right of Canada, Environment Canada, 2009.
Copyright Sa Majest� la Reine du Chef du Canada, Environnement Canada, 2009.

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

 * fichier : bufr_valuedata.c
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


/**
 * @english
 *  
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 */
BufrValue  *bufr_create_value( ValueType type )
   {
   BufrValue     *bv = NULL;

   switch( type )
      {
      case VALTYPE_INT8 :
         {
         ValueINT8    *v;

         v         = (ValueINT8 *)malloc( sizeof(ValueINT8) );
         v->type   = type;
         v->value  = -1;
         v->af     = NULL;
         bv         = (BufrValue *)v;
         }
         break;
      case VALTYPE_INT32 :
         {
         ValueINT32    *v1;

         v1         = (ValueINT32 *)malloc( sizeof(ValueINT32) );
         v1->type   = type;
         v1->value  = -1;
         v1->af     = NULL;
         bv         = (BufrValue *)v1;
         }
         break;
      case VALTYPE_INT64 :
         {
         ValueINT64    *v2;

         v2         = (ValueINT64 *)malloc( sizeof(ValueINT64) );
         v2->type   = type;
         v2->value  = -1;
         v2->af     = NULL;
         bv         = (BufrValue *)v2;
         }
         break;
      case VALTYPE_FLT32 :
         {
         ValueFLT32    *v3;

         v3         = (ValueFLT32 *)malloc( sizeof(ValueFLT32) );
         v3->type   = type;
         v3->value  = bufr_get_max_float();
         v3->af     = NULL;
         bv         = (BufrValue *)v3;
         }
         break;
      case VALTYPE_FLT64 :
         {
         ValueFLT64    *v4;

         v4         = (ValueFLT64 *)malloc( sizeof(ValueFLT64) );
         v4->type   = type;
         v4->value  = bufr_get_max_double();
         v4->af     = NULL;
         bv         = (BufrValue *)v4;
         }
         break;
      case VALTYPE_STRING :
         {
         ValueSTRING   *v5;

         v5         = (ValueSTRING *)malloc( sizeof(ValueSTRING) );
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
 *  
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 */
BufrValue  *bufr_duplicate_value( const BufrValue *bv )
   {
   BufrValue     *dup;

   if (bv == NULL) 
      {
      bufr_print_debug( "Error: cannot copy NULL in bufr_duplicate_value\n" );
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
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
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
      case VALTYPE_INT32 :
      case VALTYPE_INT64 :
      case VALTYPE_FLT32 :
      case VALTYPE_FLT64 :
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
         break;
      case VALTYPE_UNDEFINE :
      default :
         break;
      }
   free( bv );
   }

/**
 * @english
 *  
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
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
 *  
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
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
   if (str == NULL) str = old_str; /* reuse old string if new string is not available */
   if (str)
      {
      int  len2;

      len2 = strlen( str );
      if (len2 > len) 
         len2 = len;
      for (; i < len2 ; i++)
         value[i] = str[i];
      }

   for (; i < len ; i++)
      value[i] = ' ';
   value[len] = '\0';
   if ( old_str ) free( old_str );

   vstr->value = value;
   vstr->len   = len;

   return rtrn;
   }

/**
 * @english
 *  
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
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
 *  
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
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
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
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
 *  
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
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
 *  
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
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
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
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
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
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
         return (float)bufr_value_get_double( bv );
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
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
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
         return (float)bufr_value_get_float( bv );
      case VALTYPE_INT8 :
      case VALTYPE_INT32 :
         {
         int32_t  ival;

         ival = bufr_value_get_int32( bv );
         if (ival == -1) return bufr_get_max_float();
         return (double)ival;
         }
      case VALTYPE_INT64 :
         {
         int64_t  ival;

         ival = bufr_value_get_int64( bv );
         if (ival == -1) return bufr_get_max_float();
         return (double)ival;
         }
      default :
         break;
      }
   return bufr_get_max_double();
   }

/**
 * @english
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
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
 * @todo translate
 * @endenglish
 * @francais
 * retourne la valeur manquante correspondant au nombre de bits
 * @param       nbits : nombre de bits
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 */
uint64_t bufr_missing_ivalue( int nbits )
   {
   uint64_t v;

   if ((nbits >= 64)||(nbits <= 0)) return -1;

   v = (1ULL << nbits) - 1L;
   return v;
   }

/**
 * @english
 * @todo translate
 * @endenglish
 * @francais
 * retourne une valeur correspondant au nombre de bits
 * @param       nbits : nombre de bits
 * @endfrancais
 * @author Vanh Souvanlasy
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
 * @todo translate
 * @endenglish
 * @francais
 * retourne une valeur negative correspondant au nombre de bits
 * @param       nbits : nombre de bits
 * @endfrancais
 * @author Vanh Souvanlasy
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

      sprintf( errmsg, "Warning: %lld need at least %d bits for storage\n", value, minbits );
      bufr_print_debug( errmsg );
      }

   v = abs( value );
   v |= (1ULL << (nbits-1));
   return v;
   }

/**
 * @english
 *    bufr_print_value( errmsg, bcv->value )
 *    (char *str, BufrValue *)
 * This call prints the “BufrValue” structure and prints a formatted
 * value into the string passed as a parameter.
 * @return Int, if 0 there is no value, if 1, there was something to print.
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
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
 *    bufr_print_scaled_value( errmsg, bcv->value, scale )
 *    (char *str, BufrValue *, int scale)
 * This call prints the “BufrValue” structure and prints a formatted
 * value into the string passed as a parameter using scale from Table B
 * @return Int, if 0 there is no value, if 1, there was something to print.
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
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

   if (bv->af)
      {
      bufr_print_af( outstr, bv->af );
      }

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
            sprintf(  outstr, "%lld", lval );
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
            sprintf(  outstr, "%E", dval );
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
 *    bufr_is_missing_double( value )
 *    Idouble)
 * This call compares the values only if maximum double value allowed.
 * There is a similar routine as for floating point.
 * @return Int, TRUE=1, FALSE=0
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @ingroup descriptor
 */
int bufr_is_missing_double( double d )
   {
   if (isnan( d )) return 1;

   if ( d == bufr_get_max_double() ) return 1;

   return 0;
   }

/**
 * @english
 *    bufr_is_missing_float( value )
 *    (float)
 * This is the same as bufr_is_missing_double regular floating-point.
 * @return int, TRUE=1, FALSE=0
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @ingroup descriptor
 */
int bufr_is_missing_float( float f )
   {
   if (isnan( f )) return 1;

   if ( f == bufr_get_max_float() ) return 1;

   return 0;
   }

/**
 * @english
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 */
int bufr_is_missing_int( int i )
   {
   if ( i == -1 ) return 1;

   return 0;
   }

/**
 * @english
 * @return value representing MISSING in double
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 */
double bufr_missing_double(void)
   {
   return bufr_get_max_double();
   }

/**
 * @english
 * @return value representing MISSING in float
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 */
float bufr_missing_float(void)
   {
   return bufr_get_max_float();
   }

/**
 * @english
 * @return return value representing MISSING in int
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 */
int bufr_missing_int(void)
   {
   return -1;
   }

/**
 * @english
 *    BufrValue* bv;
 *    if( bufr_value_is_missing( bv ) ) continue;
 *
 * This call checks any type of value to see if it contains a "missing"
 * BUFR value appropriate for that type.
 *
 * @return zero if the value is not the BUFR missing value
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
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
			int l = 0;
			return NULL==bufr_value_get_string( bv, &l );
			}
		default:
			break;
		}

	/* If we don't recognize the type, assume it's missing */
	return 1;
	}

/**
 * @english
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
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

/*
 * name: bufr_between_values
 *
 * author:  Vanh Souvanlasy
 *
 * function: 
 *
 * parametres:
 *

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
 * print a float value with minimal string
 * @param  str  : output string
 * @param  fval : value to print
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 */
void bufr_print_float( char *str, float fval )
   {
   int  len;

   sprintf( str, "%f", fval );
   len = strlen( str ) - 1;
   while (str[len] == '0')
      --len;
   if ( str[len] == '.')
      str[len] = '\0';
   else
      str[len+1] = '\0';
   }

/**
 * @english
 * print a scaled float value with precision matching with scale
 * @param  str  : output string
 * @param  fval : value to print
 * @param  scale : no of decimal digit 
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
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
 * @param  str  : input string
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
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
      if ((str[i] == '0')||(str[i] == '1'))
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
 * @param  str  : input string
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
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
 * print a value in binary form
 * @param  str  : output string
 * @param  ival : value to print
 * @param  nbit : number of bits
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor debug
 */
void bufr_print_binary ( char *outstr, int64_t  ival, int nbit )
   {
   uint64_t  len;
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

