/***
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

 * fichier : bufr_meta.c
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
#include "bufr_meta.h"
#include "bufr_value.h"


/**
 * @english
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
BufrRTMD  *bufr_create_rtmd( int count )
   {
   BufrRTMD     *bm;
   int           i;

   bm = (BufrRTMD *)malloc(sizeof(BufrRTMD));
   bm->nb_nesting = count;
   bm->nesting = (int *)malloc( count * sizeof(int) );
   for (i = 0; i < bm->nb_nesting ; i++)
      bm->nesting[i] = 0;

   bm->tlc = NULL;
   bm->nb_tlc = 0;

   bm->pos_template = -1;
   bm->len_expansion = 0;

   return bm;
   }

/**
 * @english
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
BufrRTMD  *bufr_duplicate_rtmd( BufrRTMD *dup )
   {
   BufrRTMD     *bm;

   if (dup == NULL) 
      {
      return NULL;
      }

   bm = bufr_create_rtmd( dup->nb_nesting );
   bufr_copy_rtmd( bm, dup );

   return bm;
   }

/**
 * @english
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
void bufr_copy_rtmd( BufrRTMD *dest, BufrRTMD *src )
   {
   int i;

   if (dest->nb_nesting != src->nb_nesting)
      {
      if ( dest->nesting ) free( dest->nesting );
      dest->nesting = (int *)malloc( src->nb_nesting * sizeof(int) );
      dest->nb_nesting = src->nb_nesting;
      }

   for (i = 0; i < src->nb_nesting ; i++ )
      dest->nesting[i] = src->nesting[i];

   if (dest->nb_tlc != src->nb_tlc)
      {
      if ( dest->tlc ) free( dest->tlc );
      dest->tlc = (LocationValue *)malloc( src->nb_tlc * sizeof(LocationValue) );
      dest->nb_tlc = src->nb_tlc;
      }

   for (i = 0; i < src->nb_tlc ; i++ )
      dest->tlc[i] = src->tlc[i];
   }

/**
 * @english
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
void bufr_free_rtmd( BufrRTMD *bm )
   {
   if (bm->nesting)
      {
      free( bm->nesting );
      bm->nesting = NULL;
      }
   bm->nb_nesting = 0;
   if (bm->tlc)
      {
      free( bm->tlc );
      bm->tlc = NULL;
      }
   bm->nb_tlc = 0;

   free( bm );
   }

/**
 * @english
 *  
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup debug
 */
void bufr_print_rtmd_repl( char *outstr, BufrRTMD *bm )
   {
   int i;
   char buf[128];

   if (bm == NULL) return;

   outstr[0] = '\0';
   strcat( outstr, "{" );
   if (bm->nesting)
      {
      strcat( outstr, "R=" );
      for (i = 0; i < bm->nb_nesting ; i++ )
         {
         if (i >= 1) strcat( outstr, "." );
         sprintf( buf, "%d", bm->nesting[i] );
         strcat( outstr, buf );
         }
      }
   strcat( outstr, "}" );
   }

/**
 * @english
 * This call prints out metadata which includes replication values, heights
 * of sensors, time allocation, values that define the loops (need
 * example), (not first-order statistics, or qualifiers). field “buf”
 * is a string long enough to hold 256 bytes.
 * @return void
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup io meta debug
 */
void bufr_print_rtmd_data( char *outstr, BufrRTMD *bm )
   {
   int i;
   char buf[128];
   char buf1[128];

   if (bm == NULL) return;

   bufr_print_rtmd_repl(  outstr, bm );

   if (bm->nb_tlc)
      {
      strcat( outstr, "{" );
      for (i = 0; i < bm->nb_tlc ; i++ )
         {
         if (i >= 1) strcat( outstr, "," );
         bufr_print_float( buf1, bm->tlc[i].value );
         sprintf( buf, "%d=%s", bm->tlc[i].descriptor, buf1 );
         strcat( outstr, buf );
         }
      strcat( outstr, "}" );
      }
   }

/**
 * @english
 * This call prints out metadata of a descriptor which includes replication values, heights
 * of sensors, time allocation, values that define the loops (need
 * example), (not first-order statistics, or qualifiers). field “buf”
 * is a string long enough to hold 256 bytes.
 * @return void
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup io meta debug
 */
void bufr_print_rtmd_location( char *outstr, int desc, BufrRTMD *bm )
   {
   int i;
   char buf[128];
   char buf1[128];

   if (bm == NULL) return;

   if (bm->nb_tlc)
      {
      for (i = 0; i < bm->nb_tlc ; i++ )
         {
         if (bm->tlc[i].descriptor == desc )
            {
            strcat( outstr, "{" );
            bufr_print_float( buf1, bm->tlc[i].value );
            sprintf( buf, "%d=%s", bm->tlc[i].descriptor, buf1 );
            strcat( outstr, buf );
            strcat( outstr, "}" );
            return;
            }
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
 * @ingroup decode descriptor
 */
float bufr_fetch_rtmd_location( int descriptor, BufrRTMD *bm )
   {
   int i;

   if (bm == NULL) 
      return bufr_missing_float();

   if (bm->nb_tlc)
      {
      for (i = 0; i < bm->nb_tlc ; i++ )
         {
         if (bm->tlc[i].descriptor == descriptor)
            return bm->tlc[i].value;
         }
      }
   return bufr_missing_float();
   }
