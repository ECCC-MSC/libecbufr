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
#include "bufr_desc.h"
#include "bufr_value.h"


/**
 * @english
 *
 * Constructor for the BufrRTMD (BUFR runtime meta data) object.
 * @warning This is an internal function, normal usage should never need to call this function directly. 
 *
 * @param  count number of nesting level
 * @return (BufrRTMD *)
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

   bm = (BufrRTMD *)calloc(1,sizeof(BufrRTMD));
	if( bm == NULL ) return NULL;
	if( count > 0 )
		{
		bm->nb_nesting = count;
		bm->nesting = (int *)calloc( count, sizeof(int) );
		if( bm->nesting == NULL )
			{
			free(bm);
			return NULL;
			}
		}

   bm->pos_template = -1;

   return bm;
   }

/**
 * @english
 *
 * Make a duplicate of an BufrRTMD object
 *
 * @warning This is an internal function, normal usage should never need to call this function directly. 
 * @param  dup  the original of the object to duplicate
 * @return (BufrRTMD *)
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
 *
 * Copy the content of a BufrRTMD object to another
 *
 * @param src  input
 * @param desc output
 * @return void
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
		{
      dest->tlc[i] = src->tlc[i];
		}

	if( dest->nb_qualifiers != src->nb_qualifiers )
		{
      if ( dest->qualifiers ) free( dest->qualifiers );
      dest->qualifiers = (BufrDescriptor**)calloc( src->nb_qualifiers,
			sizeof(BufrDescriptor*) );
      dest->nb_qualifiers = src->nb_qualifiers;
		}

   for (i = 0; i < src->nb_qualifiers ; i++ )
		{
      dest->qualifiers[i] = src->qualifiers[i];
		}
   }

/**
 * @english
 *
 * Destructor for the BufrRTMD object.
 *
 * @param bm input
 * @return void
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
void bufr_free_rtmd( BufrRTMD *rtmd )
   {
   if (rtmd->nesting)
      {
      free( rtmd->nesting );
      rtmd->nesting = NULL;
      }
   rtmd->nb_nesting = 0;
   if (rtmd->tlc)
      {
      free( rtmd->tlc );
      rtmd->tlc = NULL;
      }
   rtmd->nb_tlc = 0;
   if (rtmd->qualifiers)
      {
      free( rtmd->qualifiers );
      rtmd->qualifiers = NULL;
      }
   rtmd->nb_qualifiers = 0;

   free( rtmd );
   }

/**
 * @english
 *  
 * Print out the replication count of an BufrRTMD object into a string buffer
 * @warning  Make sure that output string buffer size is big enough to hold the entire printout. A 4k string should be enough for most RTMD.
 *
 * @return void
 * @param outstr  output string buffer (need at least 4k)
 * @param rtmd pointer to an BufrRTMD
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup debug
 */
void bufr_print_rtmd_repl( char *outstr, BufrRTMD *rtmd )
   {
   int i;
   char buf[128];

   if (rtmd == NULL) return;

   outstr[0] = '\0';
   strcat( outstr, "{" );
   if (rtmd->nesting)
      {
      strcat( outstr, "R=" );
      for (i = 0; i < rtmd->nb_nesting ; i++ )
         {
         if (i >= 1) strcat( outstr, "." );
         sprintf( buf, "%d", rtmd->nesting[i] );
         strcat( outstr, buf );
         }
      }
   strcat( outstr, "}" );
   }

void bufr_print_rtmd_data( char *outstr, BufrRTMD *rtmd )
   {
   int i;
   char buf[128];
   char buf1[128];

   if (rtmd == NULL) return;

   bufr_print_rtmd_repl(  outstr, rtmd );

   if (rtmd->nb_tlc)
      {
      strcat( outstr, "{" );
      for (i = 0; i < rtmd->nb_tlc ; i++ )
         {
         if (i >= 1) strcat( outstr, "," );
         bufr_print_float( buf1, rtmd->tlc[i].value );
         sprintf( buf, "%d=%s", rtmd->tlc[i].descriptor, buf1 );
         strcat( outstr, buf );
         }
      strcat( outstr, "}" );
      }
   }

/**
 * @english
 *
 * This call prints out all active qualifiers for the descriptor.
 * 
 * @return void
 * @param outstr  output string buffer (need at least 4k)
 * @param rtmd pointer to an BufrRTMD
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Chris Beauregard
 * @ingroup io meta debug
 */
void bufr_print_rtmd_qualifiers( char *outstr, BufrRTMD *rtmd )
	{
   int i;
   char buf[128];
   char buf1[128];

   if (rtmd->nb_qualifiers>0)
      {
      strcat( outstr, "{" );
      for (i = 0; i < rtmd->nb_qualifiers ; i++ )
         {
         if (i >= 1) strcat( outstr, "," );
			bufr_print_value( buf1, rtmd->qualifiers[i]->value );
         sprintf( buf, "%d=%s", rtmd->qualifiers[i]->descriptor, buf1 );
         strcat( outstr, buf );
         }
      strcat( outstr, "}" );
      }
	}

/**
 * @english
 *
 * Prints out the runtime metadata of a descriptor 
 *
 * @param outstr  output string buffer (need at least 4k)
 * @param desc  the descriptor to look for
 * @param rtmd pointer to an BufrRTMD
 * @return void
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup io meta debug
 */
void bufr_print_rtmd_location( char *outstr, int desc, BufrRTMD *rtmd )
   {
   int i;
   char buf[128];
   char buf1[128];

   if (rtmd == NULL) return;

   if (rtmd->nb_tlc)
      {
      for (i = 0; i < rtmd->nb_tlc ; i++ )
         {
         if (rtmd->tlc[i].descriptor == desc )
            {
            strcat( outstr, "{" );
            bufr_print_float( buf1, rtmd->tlc[i].value );
            sprintf( buf, "%d=%s", rtmd->tlc[i].descriptor, buf1 );
            strcat( outstr, buf );
            strcat( outstr, "}" );
            return;
            }
         }
      }
   }

/**
 * @english
 *
 * Fetch the value of a descriptor's runtime metadata
 *
 * @param desc  the descriptor to look for
 * @param rtmd pointer to an BufrRTMD
 * @return float value of metadata
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup decode descriptor
 */
float bufr_fetch_rtmd_location( int descriptor, BufrRTMD *rtmd )
   {
   int i;

   if (rtmd == NULL) 
      return bufr_missing_float();

   if (rtmd->nb_tlc)
      {
      for (i = 0; i < rtmd->nb_tlc ; i++ )
         {
         if (rtmd->tlc[i].descriptor == descriptor)
            return rtmd->tlc[i].value;
         }
      }
   return bufr_missing_float();
   }
