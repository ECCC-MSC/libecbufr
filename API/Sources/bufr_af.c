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

 * fichier : bufr_af.c
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

#include "bufr_io.h"
#include "bufr_af.h"
#include "bufr_afd.h"
#include "bufr_value.h"
#include "bufr_i18n.h"

static void bufr_copy_af( BufrAF *dest, const BufrAF *src );
static void bufr_copy_afd( BufrAFD *dest, const BufrAFD *src );

/**
 * @english
 * creates an instance of the BufrAF structure for Associated Field
 * @param   blens  array of bits len
 * @param   count  len of array
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 */
BufrAF  *bufr_create_af( const int *blens, int count )
   {
   BufrAF     *af;
   int                 nbits;
   int                 i;

   nbits = 0;
   for (i = 0; i < count ; i++)
      {
      nbits += blens[i];
      }

   if (nbits <= 0) return NULL;
   if (nbits > 64) 
      {
      char errmsg[256];
      snprintf( errmsg, 256, _("Warning: current implementation does not support >64 AF bits (%d)\n"), nbits );
      bufr_abort( errmsg );
      }

   af = (BufrAF *)malloc(sizeof(BufrAF));
   af->bits        = 0;
   af->nbits       = nbits;
   af->count       = count;

   af->fields      = (AF_Field *)malloc( count * sizeof(AF_Field));

   nbits = 0;
   for (i = count-1; i >= 0; i--)
      {
      af->fields[i].len     = blens[i];
      af->fields[i].shift   = nbits;
      af->fields[i].signify = 0;
      nbits += blens[i];
      }

   return af;
   }

/**
 * @english
 * creates an duplicate of an BufrAF structure
 * @param   dup     pointer to  BufrAF
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 */
BufrAF  *bufr_duplicate_af( const BufrAF *dup )
   {
   BufrAF     *af;
   int        blens[64];
   int        i;

   if (dup == NULL) 
      {
      bufr_print_debug( _("Error in bufr_duplicate_af(): cannot copy NULL in bufr_duplicate_af\n") );
      return NULL;
      }

   for (i = 0; i < dup->count ; i++)
      blens[i] = dup->fields[i].len;

   af = (BufrAF *)malloc(sizeof(BufrAF));
   af = bufr_create_af( blens, dup->count );
   bufr_copy_af( af, dup );
   return af;
   }

/**
 * @english
 * destroy an instance of BufrAF
 * @param  af    pointer to  BufrAF
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 */
void bufr_free_af( BufrAF *af )
   {
   if (af->fields)
      {
      free( af->fields );
      af->fields = NULL;
      }
   af->count =  0;
   af->nbits = 0;
   free( af );
   }

/**
 * @english
 * @param  outstr output string
 * @param  af     pointer to  BufrAF
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 */
int bufr_print_af ( char *outstr, const BufrAF *af )
   {
   if (af)
      {
      sprintf( outstr, "(0x%llx:%dbits)",
            af->bits, af->nbits );
      return 1;
      }
   return 0;
   }


/**
 * @english
 * creates an instance of the BufrAFD structure for Associated Field definition
 * @param   blens  array of bits len
 * @param   count  len of array
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 */
BufrAFD  *bufr_create_afd( const int *blens, int count )
   {
   BufrAFD     *afd;
   int                 nbits;
   int                 i;

   nbits = 0;
   for (i = 0; i < count ; i++)
      {
      nbits += blens[i];
      }

   if (nbits <= 0) return NULL;
   if (nbits > 64) 
      {
      char errmsg[256];

      snprintf( errmsg, 256, _n("Error in bufr_create_afd(): %d AF bit > 64\n", 
                                "Error in bufr_create_afd(): %d AF bits > 64\n", 
                                nbits), 
                nbits );
      bufr_abort( errmsg );
      }

   afd = (BufrAFD *)malloc(sizeof(BufrAFD));
   afd->count       = count;
   afd->defs = (AF_Definition *)malloc( count * sizeof(AF_Definition));

   nbits = 0;
   for (i = 0; i < count; i++)
      {
      afd->defs[i].nbits = blens[i];
      afd->defs[i].sig   = NULL;
      }

   return afd;
   }

/**
 * @english
 * creates an duplicate of an BufrAFD structure
 * @param  dup     pointer to  BufrAFD
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 */
BufrAFD  *bufr_duplicate_afd( const BufrAFD *dup )
   {
   BufrAFD     *afd;
   int        blens[64];
   int        i;

   if (dup == NULL) 
      {
      bufr_print_debug( _("Error: cannot copy NULL in bufr_duplicate_af\n") );
      return NULL;
      }

   for (i = 0; i < dup->count ; i++)
      blens[i] = dup->defs[i].nbits;

   afd = bufr_create_afd( blens, dup->count );
   bufr_copy_afd( afd, dup );
   return afd;
   }

/**
 * @english
 * @param  afd     pointer to  BufrAFD
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 */
void bufr_free_afd( BufrAFD *afd )
   {
   if (afd->defs)
      {
      free( afd->defs );
      afd->defs = NULL;
      }
   afd->count =  0;
   free( afd );
   }


/**
 * bufr_af_get_sig
 * @english
 * obtain the significance some bits at a position of the bits block
 * @param  af     :  pointer to  BufrAF
 * @param  pos    :  position of Ass.Field
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 *
 */
int bufr_af_get_sig  ( const BufrAF *af, int pos )
   {
   if ((pos < 0)||(pos >= af->count)) return -1;

   return af->fields[pos].signify;
   }

/**
 * @english
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 *
 */
void bufr_af_set_sig  ( BufrAF *af, int pos, int val )
   {
   if ((pos < 0)||(pos >= af->count)) return;

   af->fields[pos].signify = val;
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
int bufr_af_get_value  ( const BufrAF *af, int pos )
   {
   int           shift, len;
   uint64_t      vmask;
   uint64_t      mask;
   uint64_t      value;

   if ((pos < 0)||(pos >= af->count)) return 0;

   shift = af->fields[pos].shift;
   len = af->fields[pos].len;
  
   vmask = (len < 64) ? (1ULL<<len) - 1 : ~0L;
   mask = vmask << shift;
   value = (af->bits & mask) >> shift;

   if (value == vmask) 
      return -1;
   else
      return value;
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
void bufr_af_set_value  ( BufrAF *af, int pos, int val )
   {
   int           shift, len;
   uint64_t      vmask;
   uint64_t      mask;

   if ((pos < 0)||(pos >= af->count)) return;

   shift = af->fields[pos].shift;
   len = af->fields[pos].len;

   if (val < 0)
      {
      val = bufr_missing_ivalue( len );
      }

   vmask = (len < 64) ? (1ULL<<len) - 1 : ~0L;
   mask = vmask << shift;
   af->bits &= ~mask; /* clear current value */
   mask &= val;
   vmask = (mask << shift) & vmask;
   af->bits |= vmask;
   }

/**
 * bufr_copy_af
 * @english
 *  
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static void bufr_copy_af( BufrAF *dest, const BufrAF *src )
   {
   int i;

   if (dest == NULL) return;
   if (src  == NULL) return;
   if ((dest->count != src->count) || (dest->nbits != src->nbits))
      {
      bufr_print_debug( _("Warning: cannot copy different AF\n") );
      return;
      }

   dest->bits = src->bits;
   for (i = 0; i < dest->count ; i++)
      {
      dest->fields[i].len     = src->fields[i].len;
      dest->fields[i].shift   = src->fields[i].shift;
      dest->fields[i].signify = src->fields[i].signify;
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
 * @ingroup internal
 */
static void bufr_copy_afd( BufrAFD *dest, const BufrAFD *src )
   {
   int i;

   if (dest == NULL) return;
   if (src  == NULL) return;
   if (dest->count != src->count)
      {
      char errmsg[256];

      snprintf( errmsg, 256, _("Warning: cannot copy different AF: dest=%d != src=%d\n"), 
            dest->count, src->count );
      bufr_print_debug( errmsg );
      return;
      }

   for (i = 0; i < dest->count ; i++)
      {
      dest->defs[i].nbits     = src->defs[i].nbits;
      dest->defs[i].sig     = src->defs[i].sig;
      }
   }

