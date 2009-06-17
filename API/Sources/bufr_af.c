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


static void bufr_copy_af( BufrAF *dest, const BufrAF *src );
static void bufr_copy_afd( BufrAFD *dest, const BufrAFD *src );

/**
 * bufr_create_af
 * @english
 * creates an instance of the BufrAF structure for Associated Field
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_af.c
  * name: bufr_create_af
 *
 * author:  Vanh Souvanlasy
 *
 * function: creates an instance of the BufrAF structure for Associated Field
 *
 * parametres:
 *
 *    blens  :  array of bits len
 *    count  :  len of array

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
      snprintf( errmsg, 256, "Warning: current implementation do not support >64 AF bits (%d)\n", nbits );
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
 * bufr_duplicate_af
 * @english
 * creates an duplicate of an BufrAF structure
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_af.c
  * name: bufr_duplicate_af
 *
 * author:  Vanh Souvanlasy
 *
 * function: creates an duplicate of an BufrAF structure
 *
 * parametres:
 *
 *   dup     :  pointer to  BufrAF
 *

 */
BufrAF  *bufr_duplicate_af( const BufrAF *dup )
   {
   BufrAF     *af;
   int        blens[64];
   int        i;

   if (dup == NULL) 
      {
      bufr_print_debug( "Error in bufr_duplicate_af(): cannot copy NULL in bufr_duplicate_af\n" );
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
 * bufr_free_af
 * @english
 * destroy an instance of BufrAF
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_af.c
  * name: bufr_free_af
 *
 * author:  Vanh Souvanlasy
 *
 * function: destroy an instance of BufrAF
 *
 * parametres:
 *
 *   af    :  pointer to  BufrAF

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
 * bufr_print_af
 * @english
 *  
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_af.c
  * name: bufr_print_af
 *
 * author:  Vanh Souvanlasy
 *
 * function: 
 *
 * parametres:
 *
 *   outstr :  output string
 *   af     :  pointer to  BufrAF

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
 * bufr_create_afd
 * @english
 * creates an instance of the BufrAFD structure for Associated Field definition
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_af.c
  * name: bufr_create_afd
 *
 * author:  Vanh Souvanlasy
 *
 * function: creates an instance of the BufrAFD structure for Associated Field definition
 *
 * parametres:
 *
 *    blens  :  array of bits len
 *    count  :  len of array

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

      snprintf( errmsg, 256, "Error in bufr_create_afd(): AF bits %d > 64\n", nbits );
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
 * bufr_duplicate_afd
 * @english
 * creates an duplicate of an BufrAFD structure
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_af.c
  * name: bufr_duplicate_afd
 *
 * author:  Vanh Souvanlasy
 *
 * function: creates an duplicate of an BufrAFD structure
 *
 * parametres:
 *
 *   dup     :  pointer to  BufrAFD

 */
BufrAFD  *bufr_duplicate_afd( const BufrAFD *dup )
   {
   BufrAFD     *afd;
   int        blens[64];
   int        i;

   if (dup == NULL) 
      {
      bufr_print_debug( "Error: cannot copy NULL in bufr_duplicate_af\n" );
      return NULL;
      }

   for (i = 0; i < dup->count ; i++)
      blens[i] = dup->defs[i].nbits;

   afd = (BufrAFD *)malloc(sizeof(BufrAFD));
   afd = bufr_create_afd( blens, dup->count );
   bufr_copy_afd( afd, dup );
   return afd;
   }

/**
 * bufr_free_afd
 * @english
 *  
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_af.c
  * name: bufr_free_afd
 *
 * author:  Vanh Souvanlasy
 *
 * function: 
 *
 * parametres:
 *
 *   afd     :  pointer to  BufrAFD

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
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_af.c
  * name: bufr_af_get_sig
 *
 * author:  Vanh Souvanlasy
 *
 * function: obtain the significance some bits at a position of the bits block
 *
 * parametres:
 *
 *   af     :  pointer to  BufrAF

 */
int bufr_af_get_sig  ( const BufrAF *af, int pos )
   {
   if ((pos < 0)||(pos >= af->count)) return -1;

   return af->fields[pos].signify;
   }

/**
 * bufr_af_set_sig
 * @english
 *  
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_af.c
  * name: bufr_af_set_sig
 *
 * author:  Vanh Souvanlasy
 *
 * function: 
 *
 * parametres:
 *

 */
void bufr_af_set_sig  ( BufrAF *af, int pos, int val )
   {
   if ((pos < 0)||(pos >= af->count)) return;

   af->fields[pos].signify = val;
   }

/**
 * bufr_af_get_value
 * @english
 *  
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_af.c
  * name: bufr_af_get_value
 *
 * author:  Vanh Souvanlasy
 *
 * function: 
 *
 * parametres:
 *

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
 * bufr_af_set_value
 * @english
 *  
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_af.c
  * name: bufr_af_set_value
 *
 * author:  Vanh Souvanlasy
 *
 * function: 
 *
 * parametres:
 *

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
 * @ingroup bufr_af.c
  * name: bufr_copy_af
 *
 * author:  Vanh Souvanlasy
 *
 * function: 
 *
 * parametres:
 *

 */
static void bufr_copy_af( BufrAF *dest, const BufrAF *src )
   {
   int i;

   if (dest == NULL) return;
   if (src  == NULL) return;
   if ((dest->count != src->count) || (dest->nbits != src->nbits))
      {
      bufr_print_debug( "Warning: cannot copy different AF\n" );
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
 * bufr_copy_afd
 * @english
 *  
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_af.c
  * name: bufr_copy_afd
 *
 * author:  Vanh Souvanlasy
 *
 * function: 
 *
 * parametres:
 *

 */
static void bufr_copy_afd( BufrAFD *dest, const BufrAFD *src )
   {
   int i;

   if (dest == NULL) return;
   if (src  == NULL) return;
   if (dest->count != src->count)
      {
      char errmsg[256];

      snprintf( errmsg, 256, "Warning: cannot copy different AF: dest=%d != src=%d\n", 
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

