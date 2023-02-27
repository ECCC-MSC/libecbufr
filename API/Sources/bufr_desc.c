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

 * fichier : bufr_codedata.c
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
#include <assert.h>

#include "bufr_io.h"
#include "bufr_ddo.h"
#include "bufr_ieee754.h"
#include "bufr_desc.h"
#include "bufr_tables.h"
#include "bufr_linklist.h"
#include "bufr_array.h"
#include "bufr_value.h"
#include "bufr_i18n.h"
#include "private/gcmemory.h"
#include "config.h"

static void * BufrDescriptor_gcmemory=NULL;


static int bufr_check_class31_set( BufrDescriptor *cb );
static void print_set_value_error( BufrDescriptor *cb, char *valstr );

/**
 * @english
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor debug
 * @warning assumes outstr is large enough to represent the code
 */
void bufr_print_descriptor( char *outstr, BufrDescriptor *cb )
   {
   char buffer[256];

   if (outstr == NULL) return;

   if (cb->flags & FLAG_SKIPPED)
      sprintf( outstr, "#%.6d :", cb->descriptor );
   else
      sprintf( outstr, "%.6d :", cb->descriptor );

   switch (cb->encoding.type)
      {
      case TYPE_CCITT_IA5 :
         sprintf( buffer, "CCITT IA5 " );
         strcat( outstr, buffer );
         break;
      case TYPE_IEEE_FP :
         sprintf( buffer, "IEEE FLPT " );
         strcat( outstr, buffer );
         break;
      case TYPE_NUMERIC :
         sprintf( buffer, "NUMERIC   " );
         strcat( outstr, buffer );
         break;
      case TYPE_CODETABLE :
         sprintf( buffer, "CODETABLE " );
         strcat( outstr, buffer );
         break;
      case TYPE_FLAGTABLE :
         sprintf( buffer, "FLAGTABLE " );
         strcat( outstr, buffer );
         break;
      case TYPE_CHNG_REF_VAL_OP :
         sprintf( buffer, "CHNGE REF " );
         strcat( outstr, buffer );
         break;
      case TYPE_OPERATOR :
         strcat( outstr, "TABLE C " );
         break;
      case TYPE_SEQUENCE :
         strcat( outstr, "TABLE D " );
         break;
      case TYPE_REPLICATOR :
         strcat( outstr, "REPLICATOR " );
         break;
      default :
         strcat( outstr, "UNDEFINED " );
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
BufrDescriptor  *bufr_create_descriptor( BUFR_Tables *tbls, int desc )
   {
   BufrDescriptor     *d;

#if USE_GCMEMORY
   if (BufrDescriptor_gcmemory == NULL)
      {
      BufrDescriptor_gcmemory = gcmem_new(GCMEM_DESCRIPTOR_SIZE, sizeof(BufrDescriptor) );
      }

   d = gcmem_alloc(BufrDescriptor_gcmemory);
#else
   d = (BufrDescriptor *)malloc(sizeof(BufrDescriptor));
#endif

   d->descriptor         = desc;
   d->s_descriptor       = 0;
   d->encoding.type      = TYPE_UNDEFINED;
   d->encoding.reference = 0;
   d->encoding.scale     = 0;
   d->encoding.nbits     = -1;
   d->encoding.af_nbits  = 0;
   d->afd                = NULL;
   d->value              = NULL;
   d->flags              = 0;
   d->meta               = NULL;
   d->etb                = NULL;
   d->repl_rank          = 0;

   if (tbls != NULL)
      {
		const EntryTableB *e;

		e = bufr_fetch_tableB( tbls, desc );
		if( e )
			{
         memcpy( &d->encoding, &e->encoding, sizeof(d->encoding) );
         d->etb = (EntryTableB *)e;
			}
      }
   return d;
   }

/**
 * @english
 * @brief This call creates copies of a BUFR code structure.
 *
 * This is used to copy
 * templates before expansion for encoding and decoding.. A pointer to a
 * BUFR code structure is returned if there is no error, in case of error a
 * NULL be returned if no more memory or if the data is invalid.
 *
 * @param dup code to duplicate
 * @return BufrDescriptor pointer, to be cleared with bufr_free_descriptor
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 *
 *    BufrDescriptor *cb = bufr_dupl_descriptor( dup );
 *
 * @see bufr_free_descriptor
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 */
BufrDescriptor  *bufr_dupl_descriptor( BufrDescriptor *dup )
   {
   BufrDescriptor     *code;

   if (dup == NULL) 
      {
      if (bufr_is_debug())
         bufr_print_debug( _("Error in bufr_dupl_descriptor(): cannot copy NULL in bufr_dupl_descriptor\n") );
      return NULL;
      }

#if USE_GCMEMORY
   code = gcmem_alloc(BufrDescriptor_gcmemory);
#else
   code = (BufrDescriptor *)malloc(sizeof(BufrDescriptor));
#endif
   code->afd                = NULL;
   code->value              = NULL;
   code->meta               = NULL;
   bufr_copy_descriptor( code, dup );
   return code;
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
void bufr_free_descriptor( BufrDescriptor *code )
   {
   if (code == NULL) return;

   if (code->afd)
      {
      bufr_free_afd( code->afd );
      code->afd = NULL;
      }

   if (code->meta)
      {
      bufr_free_rtmd( code->meta );
      code->meta = NULL;
      }

   if (code->value)
      {
      bufr_free_value( code->value );
      code->value = NULL;
      }

#if USE_GCMEMORY
   gcmem_dealloc(BufrDescriptor_gcmemory, code );
#else
   free( code );
#endif
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
void bufr_copy_descriptor( BufrDescriptor *dest, BufrDescriptor *src )
   {
   dest->descriptor         = src->descriptor;
   dest->flags              = src->flags;
   memcpy( &(dest->encoding), &(src->encoding), sizeof(BufrValueEncoding) );
   dest->repl_rank          = src->repl_rank;
   dest->s_descriptor       = src->s_descriptor;

   if ( src->value )
      {
      if (dest->value) bufr_free_value( dest->value );
      dest->value = bufr_duplicate_value( src->value );
      }

   if (dest->afd)
      bufr_free_afd( dest->afd );
   if (src->afd)
      dest->afd = bufr_duplicate_afd( src->afd );
   else
      dest->afd = NULL;

   if (dest->meta)
      bufr_free_rtmd( dest->meta );
   if (src->meta)
      dest->meta = bufr_duplicate_rtmd( src->meta );
   else
      dest->meta = NULL;

   dest->etb = src->etb;
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
BufrValue *bufr_mkval_for_descriptor ( BufrDescriptor *bc )
   {
   ValueType vt;
   BufrValue *bv = NULL;

   switch (bc->encoding.type)
      {
      case TYPE_CCITT_IA5 :
         vt = bufr_encoding_to_valtype( &(bc->encoding) );
         bv = (BufrValue *)bufr_create_value( vt );
         bufr_value_set_string( bv, NULL, bc->encoding.nbits / 8 );
         if (bc->value)
            bufr_copy_value( bv, bc->value );
         break;
      case TYPE_IEEE_FP   :
      case TYPE_NUMERIC   :
      case TYPE_CODETABLE :
      case TYPE_FLAGTABLE :
      case TYPE_CHNG_REF_VAL_OP :
         vt = bufr_encoding_to_valtype( &(bc->encoding) );
         bv = (BufrValue *)bufr_create_value( vt );
         if (bc->value)
            bufr_copy_value( bv, bc->value );
      break;
      default :
      break;
      }

   bufr_set_value_af( bv, bc );
   return bv;
   }

/**
 * @english
 * @brief set a descriptor as the associated field value
 *
 * @param bv value to update
 * @param bc descriptor to use as the source
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @warning this function is really meant to be used internally
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 */
void bufr_set_value_af( BufrValue *bv, const BufrDescriptor *bc )
   {
   if ( bc->afd && bv )
      {
      int  blens[256];
      int  i;

      if (bv->af != NULL) return;

      for (i = 0; i < bc->afd->count ; i++)
         {
         blens[i] = bc->afd->defs[i].nbits;
         }
      bv->af = bufr_create_af( blens, bc->afd->count );

		/* After we create the value, we need to initialize all the
		 * signifier values, too.
		 */
		for (i = 0; i < bc->afd->count ; i++)
			{
			BufrDescriptor* sigbcv = (BufrDescriptor*) bc->afd->defs[i].sig;
			bufr_af_set_sig( bv->af, i, bufr_value_get_int32( sigbcv->value ) );
			}
      }
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
ValueType bufr_encoding_to_valtype( BufrValueEncoding *be )
   {
   switch (be->type)
      {
      case TYPE_CCITT_IA5 :
         return VALTYPE_STRING;
      case TYPE_IEEE_FP :
         if (be->nbits == 64)
            return VALTYPE_FLT64;
         else
            return VALTYPE_FLT32;
      case TYPE_NUMERIC :
			 /* FIXME: we can't allow negative ref values because we're presently
			  * using -1 as "missing". See
			  * https://github.com/ECCC-MSC/libecbufr/issues/15
			  * But there's no reason positive integer values can't be
			  * represented entirely with INT32, as long as we count it in
			  * the right place.
			  */
          if ( be->scale == 0 && be->reference >= 0 ) 
            {
				int rb;
            if (be->reference != 0)
               rb = be->ref_nbits > 0 ? be->ref_nbits : bufr_value_nbits(be->reference);
            else
               rb = 0;
            if (be->nbits + rb <= 8)
               return VALTYPE_INT32; 
            else if (be->nbits + rb <= 32)
               return VALTYPE_INT32;
            else if(be->nbits + rb <= 64)
               return VALTYPE_INT64;
            }
			return VALTYPE_FLTDEFAULT;
      case TYPE_CODETABLE :
      case TYPE_FLAGTABLE :
         if (be->nbits <= 8)
            return VALTYPE_INT32;
         else if (be->nbits <= 32)
            return VALTYPE_INT32;
         else
				{
				assert(be->nbits <= 64);
            return VALTYPE_INT64;
				}
      case TYPE_CHNG_REF_VAL_OP :
         return VALTYPE_INT32;
      default :
      break;
      }
   return VALTYPE_UNDEFINE;
   }


/*
 * name: bufr_datatype_to_valtype
 *
 * author:  Vanh Souvanlasy
 *
 * function: 
 *
 * parametres:

 */
ValueType bufr_datatype_to_valtype( BufrDataType type, int nbits, int scale )
   {
   switch (type)
      {
      case TYPE_CCITT_IA5 :
         return VALTYPE_STRING;
      case TYPE_IEEE_FP :
         if (nbits == 64)
            return VALTYPE_FLT64;
         else
            return VALTYPE_FLT32;
      case TYPE_NUMERIC :
         if (scale == 0)
            {
            if (nbits <= 8)
               return VALTYPE_INT8;
            else if (nbits <= 32)
               return VALTYPE_INT32;
            else
               return VALTYPE_INT64;
            }
         else
            return VALTYPE_FLTDEFAULT;
      case TYPE_CODETABLE :
      case TYPE_FLAGTABLE :
         if (nbits <= 8)
            return VALTYPE_INT8;
         else if (nbits <= 32)
            return VALTYPE_INT32;
         else
            return VALTYPE_INT64;
      case TYPE_CHNG_REF_VAL_OP :
         return VALTYPE_INT32;
      default :
      break;
      }
   return VALTYPE_UNDEFINE;
   }

/**
 * @english
 * @brief This function returns the minima and maxima allowed for an element
 * (used more in encoding than in decoding).
 * 
 *    int rc = bufr_descriptor_get_range( &cb, &flmin, &flmax )
 *
 * Based on an element scale,
 * reference, and number of bits, it can determine the minimum and maximum
 * value that can be placed within this limit.
 *
 * @warning This is only valid for non-string fields.
 * @param cb element to get range for
 * @param min pointer to value to hold the minimum
 * @param max pointer to value to hold the maximum
 * @return int, It returns a value of 1 if there is no error, and returns 0
 * or -1 in case of error.
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @bug should check if min or max are NULL
 * @ingroup descriptor
 */                                                                             
int bufr_descriptor_get_range ( BufrDescriptor *cb, double *min, double *max )
   {
   double    scale_factor;
   int64_t   imax;
   int       x;

   if (cb == NULL) return -1;

   switch (cb->encoding.type)
      {
      case TYPE_IEEE_FP :
         if (cb->encoding.nbits == 64)
            {
            *max = bufr_get_max_double();
            *min = - bufr_get_max_double();
            }
         else
            {
            *max = bufr_get_max_float();
            *min = - bufr_get_max_float();
            }
         return 1;
      case TYPE_CHNG_REF_VAL_OP :
         imax = (1ULL << (cb->encoding.nbits-1)) - 1;
         *max = imax;
         *min = -imax;
         return 1;
      case TYPE_NUMERIC :
      case TYPE_CODETABLE :
      case TYPE_FLAGTABLE :
         break;
      case TYPE_CCITT_IA5 :
      case TYPE_OPERATOR :
      case TYPE_SEQUENCE :
      default :
         *min = nan("char-sequence");
         *max = *min;
         return 0;
      }

   scale_factor = pow(10.0,(double)cb->encoding.scale);
   imax = (1ULL << cb->encoding.nbits) - 1;

   x = DESC_TO_X( cb->descriptor );
   if (x == 31)
      *max = ( imax + cb->encoding.reference ) / scale_factor;
   else
      *max = ( imax - 1 + cb->encoding.reference ) / scale_factor;
   *min = cb->encoding.reference / scale_factor;
   return 1;
   }

/**
 * @english
 * @brief This function stores a floating point value with a code structure.
 *
 * The descriptor of the code should be valid for the storage of floating
 * point values. There is range-checking in the call and also
 * type-checking.
 * @param cb set value for this code
 * @param val new floating point value
 * @return int, Return codes are less than zero if there is an error.
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 *
 * @see bufr_descriptor_set_dvalue, bufr_descriptor_set_ivalue,
 * bufr_descriptor_set_svalue, bufr_descriptor_set_bitsvalue, bufr_descriptor_set_fvalue,
 * bufr_descriptor_get_dvalue, bufr_descriptor_get_ivalue, bufr_descriptor_get_svalue,
 * bufr_descriptor_get_fvalue
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 */
int bufr_descriptor_set_fvalue ( BufrDescriptor *cb , float fval )
   {
   int rtrn;
   double  dmin, dmax;
   float   min, max;


   rtrn = -1;

   if (bufr_check_class31_set( cb )) return rtrn;

   if (cb->value == NULL)
      {
      cb->value = bufr_mkval_for_descriptor( cb );
      }

   if (cb->value == NULL)
      {
      if (bufr_is_debug())
         {
         char errmsg[256];

         sprintf( errmsg, _("Warning: cannot assign a value to descriptor %d\n"), cb->descriptor );
         bufr_print_debug( errmsg );
         }
      return rtrn;
      }

   if (bufr_is_missing_float(fval))
      {
      rtrn = bufr_value_set_float( cb->value, fval );
      return rtrn;
      }

   if (bufr_descriptor_get_range( cb, &dmin, &dmax ) > 0 )
      {
      min = (float)dmin;
      max = (float)dmax;
      if ((fval >= min)&&(fval <= max))
         {
         rtrn = bufr_value_set_float( cb->value, fval );
         }
      else
         {
         rtrn = bufr_value_set_float( cb->value, bufr_get_max_float() );
         if (bufr_is_verbose())
            {
            char errmsg[256];

            sprintf( errmsg, _("Warning: The value %f of descriptor %d is out of range [%f,%f]\n"), 
                     fval, cb->descriptor, min, max );
            bufr_print_debug( errmsg );
            }
         rtrn = -1;
         }
      }
   else
      {
      rtrn = bufr_value_set_float( cb->value, bufr_get_max_float() );
      }

   if ((rtrn < 0) && bufr_is_verbose())
      {
      char errmsg[256];

      sprintf( errmsg, "%f", fval );
      print_set_value_error( cb, errmsg );
      }

   if (bufr_is_verbose() || bufr_is_debug())
      bufr_print_debug( NULL );

   return rtrn;
   }

/**
 * @english
 * @brief This function stores a double-precision value with a BUFR code
 * structure.
 *
 * The descriptor of the code should be valid for the storage of
 * double-precision values. There is range-checking in the call and also
 * type-checking.
 * @param cb set value for this code
 * @param val new double-precision value
 * @return int, Return codes are less than zero if there is an error.
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 *
 * @see bufr_descriptor_set_dvalue, bufr_descriptor_set_ivalue,
 * bufr_descriptor_set_svalue, bufr_descriptor_set_bitsvalue, bufr_descriptor_set_fvalue,
 * bufr_descriptor_get_dvalue, bufr_descriptor_get_ivalue, bufr_descriptor_get_svalue,
 * bufr_descriptor_get_fvalue
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 */
int bufr_descriptor_set_dvalue ( BufrDescriptor *cb , double dval )
   {
   int rtrn;
   double  min, max;
   char errmsg[256];

   rtrn = -1;

   if (bufr_check_class31_set( cb )) return rtrn;

   if (cb->value == NULL)
      {
      cb->value = bufr_mkval_for_descriptor( cb );
      }

   if (cb->value == NULL)
      {
      if (bufr_is_debug())
         {
         sprintf( errmsg, _("Warning: cannot assign a value to descriptor %d\n"), cb->descriptor );
         bufr_print_debug( errmsg );
         }
      return rtrn;
      }

   if (bufr_is_missing_double(dval))
      {
      rtrn = bufr_value_set_double( cb->value, dval );
      return rtrn;
      }

   if (bufr_descriptor_get_range( cb, &min, &max ) > 0 )
      {
      if ((dval >= min)&&(dval <= max))
         {
         rtrn = bufr_value_set_double( cb->value, dval );
         }
      else
         {
         bufr_value_set_double( cb->value, bufr_get_max_double() );
         sprintf( errmsg, _("Warning: The value %f of descriptor %d is out of range [%f,%f]\n"), 
                  dval, cb->descriptor, min, max );
         bufr_print_debug( errmsg );
         }
      } 
   else
      {
      bufr_value_set_double( cb->value, bufr_get_max_double() );
      }


   if (rtrn < 0)
      {
      sprintf( errmsg, "%E", dval );
      print_set_value_error( cb, errmsg );
      }

   return rtrn;
   }


/**
 * @english
 * @brief This function stores an integer value with a BUFR code structure.
 *
 * The descriptor of the code should be valid for the storage of integer
 * values. There is range-checking in the call and also type-checking.
 * @warning For a BUFR class 31 BUFR code (delayed replication count) the
 * value can only be set once before expansion of the replicator occurs â~@~S
 * this is a safety mechanism to avoid causing execution errors in
 * replication.
 * @param cb set value for this code
 * @param val new integer value
 * @return int, Return codes are less than zero if there is an error.
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 *
 * @see bufr_descriptor_set_dvalue, bufr_descriptor_set_ivalue,
 * bufr_descriptor_set_svalue, bufr_descriptor_set_bitsvalue, bufr_descriptor_set_fvalue,
 * bufr_descriptor_get_dvalue, bufr_descriptor_get_ivalue, bufr_descriptor_get_svalue,
 * bufr_descriptor_get_fvalue
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 */
int bufr_descriptor_set_ivalue ( BufrDescriptor *cb , int32_t ival )
   {
   int rtrn;
   double  dmin, dmax;
   char    errmsg[256];
   int32_t  min, max;

   rtrn = -1;

   if (bufr_check_class31_set( cb )) return rtrn;

   if (cb->value == NULL)
      {
      cb->value = bufr_mkval_for_descriptor( cb );
      }

   if (cb->value == NULL)
      {
      if (bufr_is_debug())
         {
         sprintf( errmsg, _("Warning: cannot assign a value to descriptor %d\n"), cb->descriptor );
         bufr_print_debug( errmsg );
         }
      return rtrn;
      }

   if (bufr_is_missing_int(ival))
      {
      rtrn = bufr_value_set_int32( cb->value, ival );
      return rtrn;
      }

   if (bufr_descriptor_get_range( cb, &dmin, &dmax ) > 0 )
      {
      min = (int32_t) dmin;
      max = (int32_t) dmax;
/*
 * for descriptor 20011, it is valid to use missing=max+1 as value
 */
      if (cb->descriptor == 20011) max += 1;
      if ((ival >= min)&&(ival <= max))
         {
         rtrn = bufr_value_set_int32( cb->value, ival );
         }
      else
         {
         rtrn = bufr_value_set_int32( cb->value, -1 );
         if ( ival != -1 )
            {
            sprintf( errmsg, _("Warning: The value %d of descriptor %d is out of range [%d,%d]\n"), 
                     ival, cb->descriptor, min, max );
            bufr_print_debug( errmsg );
            }
         }
      }
   else
      {
      rtrn = bufr_value_set_int32( cb->value, -1 );
      }

   if (rtrn < 0)
      {
      sprintf( errmsg, "%d", ival );
      print_set_value_error( cb, errmsg );
      }

   return rtrn;
   }

/**
 * @english
 * see if class31 value is already set and expanded
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor internal
 */
static int bufr_check_class31_set( BufrDescriptor *cb )
   {
/*
 * delayed replication count cannot be changed once it is set and expanded
 * the FLAG_CLASS31 is set only when this code is preceded by a 1YY000

*/
   switch (cb->descriptor)
      {
      case 31000 :
      case 31001 :
      case 31002 :
         if ((cb->flags & FLAG_CLASS31)&&(cb->value != NULL))
            {
            int32_t v = bufr_value_get_int32( cb->value );
            if ((v > 0)&&(cb->flags & FLAG_EXPANDED))
               return 1;
            }
      break;
      }
   return 0;
   }

/**
 * @english
 * turn bits into value according to code type
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 * @bug where's bufr_code_get_bitsvalue?
 */
int bufr_descriptor_set_bitsvalue ( BufrDescriptor *cb , uint64_t ival )
   {
   int64_t   iv;
   uint64_t  msng;

   if (cb->flags & FLAG_SKIPPED) return -1;

   switch ( cb->encoding.type )
      {
      case TYPE_NUMERIC :
      case TYPE_CODETABLE :
      case TYPE_FLAGTABLE :
      case TYPE_CHNG_REF_VAL_OP :
      break;
      default :
         return 0;
      }

   msng = bufr_missing_ivalue( cb->encoding.nbits );

   if (ival == msng)
      {
/*
 * special case for descriptor 31000 where 1 is 1 not -1
 */
      if ((cb->descriptor == 31000)&&(cb->encoding.nbits == 1))
         iv = 1;
      else
         iv = -1 ;
      }
   else 
      iv = ival;

   if (cb->value == NULL)
      cb->value = bufr_mkval_for_descriptor( cb );

   if (cb->encoding.type == TYPE_NUMERIC)
      {
      if ((cb->value->type == VALTYPE_INT32)||(cb->value->type == VALTYPE_INT64))
         {
         if (iv != -1)
            iv += cb->encoding.reference;
         bufr_value_set_int64( cb->value, iv );
         }
      else if (cb->value->type == VALTYPE_FLT32)
         {
         float fval = bufr_cvt_i32_to_fval( &(cb->encoding), ival );
         bufr_value_set_float( cb->value, fval );
         }
      else if (cb->value->type == VALTYPE_FLT64)
         {
         double dval = bufr_cvt_i64_to_dval( &(cb->encoding), ival );
         bufr_value_set_double( cb->value, dval );
         }
      }
   else if (cb->encoding.type == TYPE_CHNG_REF_VAL_OP)
      {
      iv = bufr_cvt_ivalue( ival, cb->encoding.nbits );
      bufr_value_set_int64( cb->value, iv );
      }
   else
      {
      bufr_value_set_int64( cb->value, iv );
      }
   return 1;
   }

/**
 * @english
 * @brief This function stores a string value with a BUFR code structure.
 *
 * The descriptor of the code should be valid for the storage of strings. It
 * checks that the string passed is not wider than the space to hold it as
 * per the code descriptor.
 * @param cb set value for this code
 * @param val new string value
 * @return int, Return codes are less than zero if there is an error.
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @see bufr_descriptor_set_dvalue, bufr_descriptor_set_ivalue,
 * bufr_descriptor_set_svalue, bufr_descriptor_set_bitsvalue, bufr_descriptor_set_fvalue,
 * bufr_descriptor_get_dvalue, bufr_descriptor_get_ivalue, bufr_descriptor_get_svalue,
 * bufr_descriptor_get_fvalue
 * @bug should be "const char *"
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 */
int bufr_descriptor_set_svalue ( BufrDescriptor *cb , const char *sval )
   {
   int rtrn;
   char errmsg[256];

   rtrn = -1;

   if (cb->value == NULL)
      {
      cb->value = bufr_mkval_for_descriptor( cb );
      }

   if (cb->value == NULL)
      {
      if (bufr_is_debug())
         {
         sprintf( errmsg, _("Warning: cannot assign a value to descriptor %d\n"), cb->descriptor );
         bufr_print_debug( errmsg );
         }
      return rtrn;
      }

   rtrn = bufr_value_set_string( cb->value, sval, cb->encoding.nbits/8 );

   if (rtrn < 0)
      {
      sprintf( errmsg, "[%s]", sval );
      print_set_value_error( cb, errmsg );
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
 * @ingroup error internal
 */
static void print_set_value_error( BufrDescriptor *cb, char *valstr )
   {
   char errmsg[256];

   strcpy( errmsg, _("Warning: cannot set value for descriptor:") );
   bufr_print_descriptor( errmsg, cb );
   bufr_print_debug( errmsg );
   sprintf( errmsg, _("with value=%s\n"), valstr );
   bufr_print_debug( errmsg );
   }

/**
 * @english
 * @brief This is a floating-point version of bufr_descriptor_get_dvalue.
 *
 * @warning Before using this function one should check the data type.
 * @return Float, if no value then the value returned will be a missing
 * (maximum) value.
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @see bufr_descriptor_set_dvalue, bufr_descriptor_set_ivalue,
 * bufr_descriptor_set_svalue, bufr_descriptor_set_bitsvalue, bufr_descriptor_set_fvalue,
 * bufr_descriptor_get_dvalue, bufr_descriptor_get_ivalue, bufr_descriptor_get_svalue,
 * bufr_descriptor_get_fvalue
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 */
float bufr_descriptor_get_fvalue ( BufrDescriptor *cb )
   {
   if (cb->value == NULL) return bufr_get_max_float();

   return (bufr_value_get_float( cb->value ));
   }

/**
 * @english
 *    bufr_descriptor_get_dvalue( bcv)
 *    (BufrDescriptor *cb)
 * @brief This is a double-precision version of bufr_descriptor_get_fvalue.
 *
 * Currently there is no user of the double version.
 *
 * @warning The feature to use IEEE floating-point (64 bits) may be used in
 * BUFR edition 5 and isn't available in earlier versions. Before using
 * this function one should check the data type.
 *
 * @return double
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @see bufr_descriptor_set_dvalue, bufr_descriptor_set_ivalue,
 * bufr_descriptor_set_svalue, bufr_descriptor_set_bitsvalue, bufr_descriptor_set_fvalue,
 * bufr_descriptor_get_dvalue, bufr_descriptor_get_ivalue, bufr_descriptor_get_svalue,
 * bufr_descriptor_get_fvalue
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 */
double bufr_descriptor_get_dvalue ( BufrDescriptor *cb )
   {
   if (cb->value == NULL) return bufr_get_max_double();

   return (bufr_value_get_double( cb->value ));
   }

/**
 * @english
 * @brief Extracts an integer value from the element code.
 *
 * Extracts an integer value from the element code structure by obtaining
 * the value field within the code (example needed).
 * @return int, Return values is a decoded value if there is no error; if
 * there is no value defined, a value of -1 is returned. value of zero is a
 * legitimate return value.
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @see bufr_descriptor_set_dvalue, bufr_descriptor_set_ivalue,
 * bufr_descriptor_set_svalue, bufr_descriptor_set_bitsvalue, bufr_descriptor_set_fvalue,
 * bufr_descriptor_get_dvalue, bufr_descriptor_get_ivalue, bufr_descriptor_get_svalue,
 * bufr_descriptor_get_fvalue
 * @bug A missing value of -1 can be also a legitimate value
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 */
int32_t bufr_descriptor_get_ivalue ( BufrDescriptor *cb )
   {
   if (cb->value == NULL) return -1;

   return (bufr_value_get_int32( cb->value ));
   }

/**
 * @english
 * @brief This call returns a pointer to a string BUFR Value.
 *
 * @warning The type of the value should be checked first or if the value
 * is not a string, it will return NULL as an address of "len"
 * @return char*, if not a string or there is no value, this will return NULL
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @see bufr_descriptor_set_dvalue, bufr_descriptor_set_ivalue,
 * bufr_descriptor_set_svalue, bufr_descriptor_set_bitsvalue, bufr_descriptor_set_fvalue,
 * bufr_descriptor_get_dvalue, bufr_descriptor_get_ivalue, bufr_descriptor_get_svalue,
 * bufr_descriptor_get_fvalue
 * @bug should probably return "const char*"
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 */
char *bufr_descriptor_get_svalue ( BufrDescriptor *cb, int *len )
   {
   if (cb->value == NULL) 
      {
      *len = 0;
      return NULL;
      }

   return ((char *)bufr_value_get_string( cb->value, len ));
   }

/**
 * @english
 * return the incremented time associated with this instance of BufrDescriptor
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 */
float bufr_descriptor_get_location ( BufrDescriptor *cb, int desc )
   {
   if (cb->meta == NULL) return bufr_missing_float();

   return ( bufr_fetch_rtmd_location( desc, cb->meta ) );
   }

/**
 * @english
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup debug descriptor
 * @warning assumes outstr is large enough to represent the code
 */
int bufr_print_dscptr_value( char *outstr, BufrDescriptor *cb )
   {
   int32_t    ival;

   if (outstr == NULL) return 0;

   if (cb->flags & FLAG_SKIPPED) return 0;

   switch (cb->encoding.type)
      {
      case TYPE_NUMERIC :
         bufr_print_scaled_value( outstr, cb->value, cb->encoding.scale );
         break;
      case TYPE_FLAGTABLE :
         ival = bufr_value_get_int32( cb->value );
         if (ival < 0)
            {
            strcpy( outstr, "MSNG" );
            }
         else
            {
            bufr_print_binary( outstr, ival, cb->encoding.nbits );
            }

         break;
      default :
         bufr_print_value( outstr, cb->value );
      break;
      }
   return 1;
   }

/**
 * @english
 * @brief free internal garbage collector
 *   free memory allocation used to create garbage collector of BufrDescriptor
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 * @warning should be called by bufr_end_api
 */
void bufr_desc_end(void)
   {
#if USE_GCMEMORY
   int size, isize;

   isize = gcmem_blk_size( BufrDescriptor_gcmemory );
   size = gcmem_delete( BufrDescriptor_gcmemory );
   BufrDescriptor_gcmemory = NULL;
   if (gcmem_is_verbose())
      {
      char errmsg[256];

      sprintf( errmsg, "GCMEM used %d BufrDescriptor, blocs size=%d\n", size, isize );
      bufr_print_output( errmsg );
      }
#endif
   }
