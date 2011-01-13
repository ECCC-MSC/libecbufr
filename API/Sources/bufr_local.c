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

 * fichier : bufr_local.c
 *
 * author:  Vanh Souvanlasy 
 *
 * function: 
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>


#include "bufr_array.h"
#include "bufr_io.h"
#include "bufr_api.h"
#include "bufr_sequence.h"
#include "bufr_dataset.h"
#include "bufr_tables.h"
#include "bufr_local.h"
#include "bufr_util.h"
#include "bufr_i18n.h"

static int             *bufr_sequence_2intarr  ( BUFR_Sequence *bsq, int *len );
static EntryTableBArray bufr_sequence_2TBarray ( BUFR_Sequence *sequence, BUFR_Tables *tbls );
static void             fill_line              ( char *line, int len, char *str, int slen );
static void             split_lines            ( char *line1,int ,char *line2,int ,char *str,int );

/**
 * @english
 * This call stores local table updates used in the dataset into a file
 * (a BUFR message, actually).
 * If no local table is defined in the dataset, no message is written.
 * @warning Need to discuss these Expert functions as a group.
 * @warning Not thread-safe
 * @param fp   a file pointer of output BUFR file
 * @param dts  pointer to a BUFR_Dataset containing local tables (if any)
 * @return number of elements written from local table B and D (may be zero)
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup io tables advanced
 */
int bufr_store_tables
( FILE *fp, BUFR_Dataset *dts )
   {
   EntryTableB    *e1, *e2, *e3, *e4;
   int             descriptor;
   EntryTableD    *d1;
   IntArray        desc_list;
   BUFR_Sequence   *sequence = NULL;
   EntryTableBArray clist;
   int             i, j, tcount, dcount;
   int             f,x,y;
   int             blen;
   char            line1[256], line2[256];
   BUFR_Message      *bufr; 
   BUFR_Tables    *tbls;
   char           *cat_desc; 
   int             category;
   int             debug=bufr_is_debug();
   char            errmsg[256];
   int             errflg = 0;

   if (debug)
      bufr_print_debug( _("### Checking if local Table Update Message is needed\n") );

   if (dts == NULL) 
      {
      if (debug)
         bufr_print_debug( _("###  no, Dataset is null\n") );
      return 0;
      }

   if (dts->tmplte == NULL) 
      {
      if (debug)
         bufr_print_debug( _("###  no, Dataset has no template\n") );
      return 0;
      }

   tbls = dts->tmplte->tables;
   if (tbls == NULL) 
      {
      if (debug)
         bufr_print_debug( _("###  no, Dataset has no local tables\n") );
      return 0;
      }

   tcount = arr_count( tbls->local.tableB );
   dcount = arr_count( tbls->local.tableD );
   if ((tcount <= 0)&&(dcount <= 0)) 
      {
      if (debug)
         bufr_print_debug( _("###  no, Local tables are empty\n") );
      return 0;
      }

   if (debug)
      bufr_print_debug( _("### Creating Local Table Update BUFR Message\n") );

   bufr = bufr_create_message( dts->tmplte->edition );
   bufr_set_gmtime( &(bufr->s1) );

   cat_desc = tbls->data_cat_desc;
   category = tbls->data_cat;
/*
 * BUFR tables, complete replacement or update 
 */
   BUFR_SET_MSGDTYPE( bufr, MSGDTYPE_TABLES_REPLACE_UPDATE );  
/*
 * other data, non-compressed 
 */
   BUFR_SET_UNCOMPRESSED( bufr );
   BUFR_SET_OTHER_DATA( bufr );
   bufr_begin_message( bufr );
   bufr_alloc_sect4( bufr, 8192 );
/*
 * construire la liste descripteurs de la section 3
 */
   desc_list = bufr->s3.desc_list;
   clist = NULL;
   BUFR_SET_NB_DATASET(bufr, 1);

   if (tcount > 0) 
      {
      descriptor = 103000;                    
      arr_add( desc_list, (char *)&descriptor );
      descriptor = 31001;                    
      arr_add( desc_list, (char *)&descriptor );
      e1 = bufr_fetch_tableB( tbls, 31001 );
      bufr_putbits( bufr, 1, e1->encoding.nbits ); /* il n'y a qu'un */

      descriptor = 1;                    
      arr_add( desc_list, (char *)&descriptor );
      descriptor = 2;                    
      e1 = bufr_fetch_tableB( tbls, descriptor ); 
      arr_add( desc_list, (char *)&descriptor );
      descriptor = 3;                    
      e2 = bufr_fetch_tableB( tbls, descriptor ); 
      arr_add( desc_list, (char *)&descriptor );
/*
 * inscrire la categorie des donnees de cette table

*/
      if ( category < 0 ) category = - category;
      category = category % 256;
      sprintf( line1, "%.3d", category );
      bufr_putstring( bufr, line1, 3 );
      split_lines( line1, e1->encoding.nbits/8, line2, e2->encoding.nbits/8,
                        cat_desc, strlen(cat_desc) ); 
      bufr_putstring( bufr, line1, e1->encoding.nbits/8 );
      bufr_putstring( bufr, line2, e2->encoding.nbits/8 );

      descriptor = 101000;                      /* replicateur en attente */
      arr_add( desc_list, (char *)&descriptor );
      descriptor = (tcount < 256) ? 31001 : 31002 ;
      e1 = bufr_fetch_tableB( tbls, descriptor ); 
      arr_add( desc_list, (char *)&descriptor );
      bufr_putbits( bufr, tcount, e1->encoding.nbits );
      descriptor = 300004;
      arr_add( desc_list, (char *)&descriptor );
      sequence = bufr_expand_descriptor( descriptor, OP_RM_XPNDBL_DESC, tbls, &errflg );
      if ((sequence == NULL)||(errflg != 0))
         {
         sprintf( errmsg, _("Error expanding descriptor: %d\n"), descriptor );
         bufr_print_debug( errmsg );
         return -1;
         }

      clist = bufr_sequence_2TBarray( sequence, tbls );
      if (debug)
         {
         sprintf( errmsg, _n("### Local tables contain %d table B entry\n", 
                             "### Local tables contain %d table B entries\n", 
                             tcount ), 
                  tcount );
         bufr_print_debug( errmsg );
         }
      }

/*
 * la sequence de la table D est optionnel

*/
   if (dcount > 0) 
      {
      descriptor = FXY_TO_DESC(1,1,dcount);
      arr_add( desc_list, (char *)&descriptor );
      descriptor = 300010;
      arr_add( desc_list, (char *)&descriptor );
      if (debug)
         {
         sprintf( errmsg, _n("### Local tables contain %d table D entry\n", 
                             "### Local tables contain %d table D entries\n", 
                             dcount), 
                  dcount );
         bufr_print_debug( errmsg );
         }
      }


/*
 * ecrire les valeurs de la table B

*/
   if (tcount > 0) 
      {
      EntryTableB **ptr = (EntryTableB **)arr_get( clist, 0 );
      e1 = (EntryTableB *) ptr[3];
      e2 = (EntryTableB *) ptr[4];
      e3 = (EntryTableB *) ptr[5];
      for ( i = 0 ; i < tcount ; i++ ) 
         {
         ptr = (EntryTableB **)arr_get( tbls->local.tableB, i );
         e4 = *ptr;
         bufr_descriptor_to_fxy( e4->descriptor, &f, &x, &y );   
         sprintf( line1, "%.1d", f );
         bufr_putstring( bufr, line1, 1 );
         sprintf( line1, "%.2d", x );
         bufr_putstring( bufr, line1, 2 );
         sprintf( line1, "%.3d", y );
         bufr_putstring( bufr, line1, 3 );

         split_lines( line1, e1->encoding.nbits/8, line2, e2->encoding.nbits/8,
                           e4->description, strlen(e4->description) ); 
         bufr_putstring( bufr, line1, e1->encoding.nbits/8 );
         bufr_putstring( bufr, line2, e2->encoding.nbits/8 );

         split_lines( line1, e3->encoding.nbits/8, NULL, 0,
                           e4->unit, strlen(e4->unit) ); 
         bufr_putstring( bufr, line1, e3->encoding.nbits/8 );

         line1[0] = ( e4->encoding.scale >= 0 ) ? '+' : '-' ;
         bufr_putstring( bufr, line1, 1 );
         sprintf( line1, "%3d", abs(e4->encoding.scale) );
         bufr_putstring( bufr, line1, 3 );
         line1[0] = ( e4->encoding.reference >= 0 ) ? '+' : '-' ;
         bufr_putstring( bufr, line1, 1 );
         sprintf( line1, "%10d", abs(e4->encoding.reference) );
         bufr_putstring( bufr, line1, 10 );
         sprintf( line1, "%3d", e4->encoding.nbits );
         bufr_putstring( bufr, line1, 3 );
         }
      arr_free( &clist );
      }
/*
 * ecrire les valeurs de la table D

*/
   if (dcount > 0) 
      {
      EntryTableD **ptr;
      e1 = bufr_fetch_tableB( tbls, 31001 ); 
      e2 = bufr_fetch_tableB( tbls, 30 ); 
      blen = e2->encoding.nbits/8;
      for ( i = 0 ; i < dcount ; i++ ) 
         {
         ptr = (EntryTableD **)arr_get( tbls->local.tableD, i );
         d1 = *ptr;
         bufr_descriptor_to_fxy( d1->descriptor, &f, &x, &y );   
         sprintf( line1, "%.1d", f );
         bufr_putstring( bufr, line1, 1 );
         sprintf( line1, "%.2d", x );
         bufr_putstring( bufr, line1, 2 );
         sprintf( line1, "%.3d", y );
         bufr_putstring( bufr, line1, 3 );
  
         bufr_putbits( bufr, d1->count, e1->encoding.nbits );
         for ( j = 0 ; j < d1->count ; j++ ) {
            sprintf( line1, "%.6d", d1->descriptors[j] );
            fill_line( line2, blen , line1, 6 );
            bufr_putstring( bufr, line2, blen );
            }
         }
      }

   bufr_end_message( bufr );
   bufr_write_message( fp, bufr );
   bufr_free_message( bufr );
   bufr_free_sequence( sequence );
   return 0;
   }

/**
 * @english
 * This call extracts table information (usually local table elements) and
 * then returns a BUFR tables object. A message read can contain local
 * table updates. The extracted tables are stored in the BUFR table objects
 * as local table objects (tbls which contains master table space and
 * local table space). Local table space may be saved and merged (see
 * bufr_merge_tables) where this is not true of master table space.
 * @warning We need to review whether this is a good practice or not.
 * Examples have been noted by Jose Garcia (using a local table dictionary
 * on the side); countries may be sending local data with their messages
 * and including the means to decode the local elements contained -
 * originating centre AFJY 775 satellite data (look up).
 * @param dts pointer to Dataset
 * @return BUFR_Table, or NULL on failure or if the message doesn't contain
 any tables.
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup dataset tables advanced
 */
BUFR_Tables *bufr_extract_tables( BUFR_Dataset *dts )
   {
   int            len;
   DataSubset    *subset;
   int           sscount, cvcount;
   int           i, j;
   BufrDescriptor      *bcv;
   const char    *str;
   int            f=0, x=0, y=0;
   char           desc[512];
   EntryTableB    eb, *e1;
   EntryTableD    *etd;
   BUFR_Tables   *tbls;
   int            descriptor = 0;
   int            *codes = NULL;
   int            count_tableD=0;
   int            c = 0;
   int            cat = 0;
   char           errmsg[256];
   int            debug=bufr_is_debug();

   if (!bufr_contains_tables( dts ))
      return NULL;

   if (debug)
      bufr_print_debug( _("### Extracting Local Table Update from BUFR Message\n") );

   tbls = bufr_create_tables();
   tbls->local.tableD = (EntryTableDArray)arr_create( 100, sizeof(EntryTableD *), 100 );
   tbls->local.tableB = (EntryTableBArray)arr_create( 100, sizeof(EntryTableB *), 100 );

   eb.description = NULL;
   eb.unit = NULL;

   desc[0] = '\0';
   sscount = bufr_count_datasubset( dts );
   for (i = 0; i < sscount ; i++)
      {
      subset = bufr_get_datasubset( dts, i );
      cvcount = bufr_datasubset_count_descriptor( subset );
      for (j = 1; j < cvcount ; j++)
         {
         bcv = bufr_datasubset_get_descriptor( subset, j );

         switch ( bcv->descriptor )
            {
            case 1  :
               str = bufr_value_get_string( bcv->value, &len );
               if (str) cat = atoi( str );
               break;
            case 2  :
               str = bufr_value_get_string( bcv->value, &len );
               if (str) 
                  strcpy( desc, str );
               break;
            case 3  :
               str = bufr_value_get_string( bcv->value, &len );
               if (str) 
                  strcat( desc, str );
               bufr_set_tables_category( tbls, cat, desc );
               break;
            case 10 :
               str = bufr_value_get_string( bcv->value, &len );
               if (str) f = atoi( str );
               break;
            case 11 :
               str = bufr_value_get_string( bcv->value, &len );
               if (str) x = atoi( str );
               break;
            case 12 :
               str = bufr_value_get_string( bcv->value, &len );
               if (str) 
                  {
                  y = atoi( str );
                  descriptor = bufr_fxy_to_descriptor( f, x, y );
                  eb.descriptor = descriptor;
                  }
               break;
            case 13 :
               str = bufr_value_get_string( bcv->value, &len );
               strcpy( desc, str );
               break;
            case 14 :
               str = bufr_value_get_string( bcv->value, &len );
               strcat( desc, str );
               if (eb.description)
                  {
                  free( eb.description );
                  eb.description = NULL;
                  }
               eb.description = strimdup( NULL, desc, 65 );
               break;
            case 15 :
               str = bufr_value_get_string( bcv->value, &len );
               if (eb.unit)
                  {
                  free( eb.unit );
                  eb.unit = NULL;
                  }
               eb.unit = strimdup( NULL, str, 65 );
               break;
            case 16 :
               str = bufr_value_get_string( bcv->value, &len );
               eb.encoding.scale = (str[0] == '-') ? -1 : 1 ;
               break;
            case 17 :
               str = bufr_value_get_string( bcv->value, &len );
               eb.encoding.scale *= atoi( str );
               break;
            case 18 :
               str = bufr_value_get_string( bcv->value, &len );
               eb.encoding.reference = (str[0] == '-') ? -1 : 1 ;
               break;
            case 19 :
               str = bufr_value_get_string( bcv->value, &len );
               eb.encoding.reference *= atoi( str );
               break;
            case 20 :
               str = bufr_value_get_string( bcv->value, &len );
               eb.encoding.nbits = atoi( str );
               eb.encoding.type = bufr_unit_to_datatype( eb.unit );
               e1 = bufr_new_EntryTableB();
               bufr_copy_EntryTableB( e1, &eb );
               if (bufr_is_debug())
                  {
                  sprintf( errmsg, _n("Extracted TableB %d : (%d bit) %s\n", 
                                      "Extracted TableB %d : (%d bits) %s\n", e1->encoding.nbits), 
                           e1->descriptor, e1->encoding.nbits, e1->description );
                  bufr_print_debug( errmsg );
                  }
               arr_add( tbls->local.tableB, (char *)&e1 );
               if (eb.description)
                  {
                  free( eb.description );
                  eb.description = NULL;
                  }
               if (eb.unit)
                  {
                  free( eb.unit );
                  eb.unit = NULL;
                  }
               break;
            case 31001 :
               count_tableD = bufr_value_get_int32( bcv->value );
               if (codes != NULL)
                  {
                  free( codes );
                  if (c > 0)
                     {
                     sprintf( errmsg, _("Warning: Table D descriptor %d is incomplete, got only %d of %d, rejected.\n"), 
                           descriptor, c, count_tableD );
                     bufr_print_debug( errmsg );
                     }
                  }
               codes = (int *)malloc( count_tableD * sizeof(int) );
               c = 0;
               break;
            case 30 :
               str = bufr_value_get_string( bcv->value, &len );
               if ( c < count_tableD )
                  {
                  codes[c++] = atoi( str );
                  }
               
               if ( c == count_tableD )
                  {
                  if (bufr_is_debug())
                     {
                     sprintf( errmsg, _n("Extracted TableD %d : %d item\n", 
                                         "Extracted TableD %d : %d items\n", count_tableD), 
                              descriptor, count_tableD );
                     bufr_print_debug( errmsg );
                     }
                  etd = bufr_new_EntryTableD( descriptor, NULL, 0, codes, count_tableD );
                  free( codes );
                  codes = NULL;
                  count_tableD = 0;
                  c = 0;
                  arr_add( tbls->local.tableD, (char *)&etd );
                  }
               break;
            default :
               break;
            }
         }
      }

   if (eb.description)
      {
      free( eb.description );
      eb.description = NULL;
      }
   if (eb.unit)
      {
      free( eb.unit );
      eb.unit = NULL;
      }

   if (codes)
      {
      free( codes );
      codes = NULL;
      }

   if (debug)
      bufr_print_debug( NULL );

   return tbls;
   }

/**
 * @english
 * @brief test if the dataset contains local tables B or D updates
 *
 * If the value returned is TRUE, then this dataset does not contain data
 * but only local tables definition. It may contain Table B, Table D, or
 * both. This is used in order to make a decision as to whether Tables can
 * be extracted to help decoding subsequent messages.
 * @param dts pointer to a DataSet
 * @return zero if the dataset contains no BUFR table updates
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup dataset tables advanced
 */
int bufr_contains_tables( BUFR_Dataset *dts )
   {
   return (dts->s1.msg_type == MSGDTYPE_TABLES_REPLACE_UPDATE);
   }


/**
 * @english
 * copy a string into a fixed length line and fill with blank 
 * @param     line  : line of string to fill
 * @param     len   : maxlength of line
 * @param     str   : string to copy
 * @param     slen  : length of string
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal

 */
static void fill_line(char *line, int len, char *str, int slen)
   {
   int i;

   if (slen > len ) slen = len;
   for ( i = 0 ; i < slen ; i++ ) { 
      line[i] = str[i];
      }
   for ( ; i < len ; i++ ) { 
      line[i] = ' ';
      }
   }

/**
 * @english
 * cut a string into 2 lines and fill remaining with blank
 * @param     line1 : string line 1
 * @param     len1  : length of line1
 * @param     line2 : string line 2
 * @param     len2  : length of line2
 * @param     str   : source string to split
 * @param     len   : length of str
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static void split_lines
(char *line1, int len1, char *line2, int len2, char *str, int len)
   {
   int i;

   if (len > len1) 
      {
      strncpy( line1, str, len1 );
      len -= len1;
      if (len > len2) len = len2;

      strncpy( line2, str+len1, len );
      for ( i = len ; i < len2 ; i++ ) line2[i] = ' ';
      } 
   else 
      {
      strncpy( line1, str, len );
      for ( i = len ; i < len1 ; i++ ) line1[i] = ' ';
      for ( i = 0 ; i < len2 ; i++ ) line2[i] = ' ';
      }
   }

/**
 * @english
 * Convert a sequence of BUFR descriptors to an array of EntryTableBArray
 * @param sequence BUFR descriptors to convert
 * @param tbls BUFR tables needed to lookup meta-data
 * @return EntryTableBArray containing Table B entries for the sequence
 * @warn arr_free must be called on the resulting array to cleanup.
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static EntryTableBArray bufr_sequence_2TBarray( BUFR_Sequence *sequence, BUFR_Tables *tbls )
   {
   EntryTableBArray arr;
   int   count;
   EntryTableB *tb;
   ListNode  *node;
   BufrDescriptor  *cb;

   if (sequence == NULL) return NULL;
   count = lst_count( sequence->list );
   arr = (EntryTableBArray)arr_create( count, sizeof(EntryTableB *), 100 );

   node = lst_firstnode( sequence->list );
   while ( node )
      {
      cb = (BufrDescriptor *)node->data;

      if (cb) 
         {
         if (!(cb->flags & FLAG_SKIPPED))
            {
            tb = bufr_fetch_tableB( tbls, cb->descriptor );
            arr_add( arr, (char *)&tb );
            }
         }

      node = lst_nextnode( node );
      }

   return arr;
   }

/**
 * @english
 * Convert a sequence of BUFR descriptors to an array of integers, returning
 * the length.
 * @param bsq BUFR descriptor sequence to convert
 * @param len pointer to place to store the number of descriptors
 * @return point to array of ints, or NULL on failure
 * @warn return must be freed be caller, using free()
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 * @bug unchecked malloc result
 */
static int *bufr_sequence_2intarr( BUFR_Sequence *bsq, int *len )
   {
   int   count, i;
   ListNode *node;
   int         *rtrn;
   BufrDescriptor    *cb;
   
   count = lst_count( bsq->list );
   rtrn = (int *)malloc( count * sizeof(int) );
   node = lst_firstnode( bsq->list );
   i = 0;
   while ( node )
      {
      cb = (BufrDescriptor *)node->data;
      rtrn[i++] = cb->descriptor;
      node = lst_nextnode(node);
      }
   *len = i;
   return rtrn;
   }

