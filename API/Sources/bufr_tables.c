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

 * fichier:  bufr_api.c
 *
 * author:  Vanh Souvanlasy (avril 1996)
 *
 * function: outils pour ecrire les fichiers BUFR
 *
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "bufr_array.h"
#include "bufr_util.h"
#include "bufr_io.h"
#include "bufr_ieee754.h"
#include "bufr_value.h"
#include "bufr_tables.h"
#include "bufr_sequence.h"
#include "bufr_i18n.h"

static int          bad_descriptor=0;
static BufrValueEncoding bufr_errtbe;
static int          bufr_errcode= BUFR_NOERROR;
static int          bufr_minimum_reference=0;
static int          bufr_minimum_nbits=0;


static int             bufr_load_tableB       ( BUFR_Tables *, BufrTablesSet *tbls, const char *filename, int local );
static int             bufr_load_tableD       ( BUFR_Tables *, BufrTablesSet *tbls, const char *filename );

static EntryTableD    *bufr_tabled_fetch_entry( EntryTableDArray addr_tabled, int desc);

static EntryTableBArray        bufr_tableb_read       ( EntryTableBArray addr_tableb, const char *filename, int local,
                                                char *desc, int *cat, int *version );
static EntryTableDArray       bufr_tabled_read       ( EntryTableDArray addr_tabled, const char *filename);

static int             compare_tableb         ( const void *p1, const void *p2);
static int             compare_tabled         ( const void *p1, const void *p2);
static int             compare_tabled_seq     (const void *p1, const void *p2);

static int             bufr_check_desc_tableD ( BUFR_Tables *tbls, int desc , char *array );
static int             bufr_check_loop_tableD ( BUFR_Tables *tbls, BufrTablesSet *tbl );
static void            bufr_merge_tableB      ( EntryTableBArray table1, EntryTableBArray table2 );
static void            bufr_merge_tableD      ( EntryTableDArray table1, EntryTableDArray table2 );
static void            bufr_copy_EntryTableD  ( EntryTableD *r, const char *desc, int desclen,
                                                int *descriptors, int count);
static void            bufr_merge_TablesSet   ( BufrTablesSet *tbls1, BufrTablesSet *tbls2 );
static int             strtlen                (char *Str);

#define  DEBUG   0
#if DEBUG
static void test_print_tableD( char *tabled );
#endif

/**
 * @english
 *    file_tables = bufr_create_tables()
 *    (void)
 * Create an instance of the BUFR table objects as a placeholder for the
 * master and local tables. This does not load the tables, it only loads
 * the object; there is separate code to load the table. This can handle
 * the master and local tables, both Table B and Table D.
 * @warning The storage needs to be freed by bufr_free_tableswhen it is no
 * longer needed, but other dependent objects must be de-referenced first,
 * thus that call needs to be the last to be made.
 * @return BUFR_Tables
 * @endenglish
 * @francais
 * @brief constructeur de la structure BUFR_Tables
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup tables
 */
BUFR_Tables *bufr_create_tables(void)
   {
   BUFR_Tables *t;

   t = (BUFR_Tables *)malloc(sizeof(BUFR_Tables));
   t->master.version = 0;   /* not defined */
   t->master.tableB = NULL;
   t->master.tableD = NULL;
   t->master.tableBtype = TYPE_REFERENCED;
   t->master.tableDtype = TYPE_REFERENCED;
   t->local.version = 0;   /* not defined */
   t->local.tableB = NULL;
   t->local.tableD = NULL;
   t->local.tableBtype = TYPE_REFERENCED;
   t->local.tableDtype = TYPE_REFERENCED;
   bufr_set_tables_category( t, 0, NULL );

   t->tableB_cache = NULL;
   t->last_searched = NULL;
   return t;
   }

/**
 * @english
 *    bufr_free_tables( file_tables )
 *    (BUFR_Tables *)
 * The routine bufr_free_tablesis called when tables allocated by
 * bufr_create_tablesare no longer needed.
 * @warning Other dependent objects (templates and datasets) must be
 * de-referenced first, thus this call needs to be the last to be made.
 * @return void
 * @endenglish
 * @francais
 * @brief destructeur de la structure BUFR_Tables
 * @todo translate to French
 * @param  tbls: BUFR Tables to be freed
 * @endfrancais
 * @ingroup tables
 */
void  bufr_free_tables( BUFR_Tables *tbls )
   {
   if ( tbls == NULL ) return;

   if (tbls->master.tableB)
      {
      if (tbls->master.tableBtype == TYPE_ALLOCATED)
         bufr_tableb_free( tbls->master.tableB );
      tbls->master.tableB = NULL;
      }

   if (tbls->master.tableD)
      {
      if (tbls->master.tableDtype == TYPE_ALLOCATED)
         bufr_tabled_free( tbls->master.tableD );
      tbls->master.tableD = NULL;
      }

   if (tbls->local.tableB)
      {
      if (tbls->local.tableBtype == TYPE_ALLOCATED)
         bufr_tableb_free( tbls->local.tableB );
      tbls->local.tableB = NULL;
      }

   if (tbls->local.tableD)
      {
      if (tbls->local.tableDtype == TYPE_ALLOCATED)
         bufr_tabled_free( tbls->local.tableD );
      tbls->local.tableD = NULL;
      }

   if (tbls->tableB_cache)
      arr_free( &(tbls->tableB_cache) );

   free( tbls );
   }

/**
 * @english
 * set Tables category description
 * @param     tbls   : pointer to BUFR tables structure
 * @param     cat    : category number
 * @param     desc   : string describing the category (max 64 char.)
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup encode tables
 */
void bufr_set_tables_category( BUFR_Tables *tbls, int cat, const char *desc )
   {
   int i, len;

   if ((cat >= 0)&&(cat < 256))
      {
      tbls->data_cat = cat;
      }

   i = 0;
   if (desc)
      {
      len = strlen ( desc );
      if (len > 64) len = 64;
      for (; i < len ; i++)
         {
         if (isspace(desc[i]))
            tbls->data_cat_desc[i] = ' ';
         else
            tbls->data_cat_desc[i] = desc[i];
         }
      }

   for ( ; i < 64 ; i++)
      tbls->data_cat_desc[i] = ' ';

   tbls->data_cat_desc[64] = '\0';
   }

/**
 * @english
 * If bufr_extract_tablesis successful, one merges these local table
 * elements with the standard BUFR tables in use.
 * @warning Ensure that the destination structure has sufficient allocation
 * prior to this call. If not, nothing will be copied and 0 will be
 * returned. When merging at the end of the destination structure, this
 * will append data, This may be really acting as an append.
 * @param tbls1 pointer to dest bufr tables structure
 * @param tbls2 pointer to src  bufr tables structure
 * @return int, If error is -1, else the number of items copied.
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @ingroup tables
 */
void bufr_merge_tables( BUFR_Tables *tbls1, BUFR_Tables *tbls2 )
   {
   if ( tbls1 == NULL ) return;
   if ( tbls2 == NULL ) return;
/*
 * master tables are never copied on merged, only referenced

*/
   if (tbls2->master.tableB)
      {
      if (tbls1->master.tableBtype == TYPE_ALLOCATED)
         bufr_tableb_free( tbls1->master.tableB );
      tbls1->master.tableB = tbls2->master.tableB;
      tbls1->master.version = tbls2->master.version;
      tbls1->master.tableBtype = TYPE_REFERENCED;
      }

   if (tbls2->master.tableD)
      {
      if (tbls1->master.tableDtype == TYPE_ALLOCATED)
         bufr_tabled_free( tbls1->master.tableD );
      tbls1->master.tableD = tbls2->master.tableD;
      tbls1->master.tableDtype = TYPE_REFERENCED;
      }
/*
 * only local tables are merged and copied

*/
   bufr_merge_TablesSet( &(tbls1->local), &(tbls2->local) );
   }

/**
 * @english
 * copy bufr tables from tbls2 into tbls1, merging and overwriting
 *           duplicates
 * @param     tbls1  : pointer to dest BufrTablesSet structure
 * @param     tbls2  : pointer to src  BufrTablesSet structure
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static void bufr_merge_TablesSet( BufrTablesSet *tbls1, BufrTablesSet *tbls2 )
   {
/*
 * merged tables are always allocated

*/
   if (tbls1->tableBtype == TYPE_REFERENCED)
      tbls1->tableB = NULL;

   if (tbls1->tableB == NULL)
      tbls1->tableB = (EntryTableBArray)arr_create( arr_count(tbls2->tableB),
            sizeof(EntryTableB *), 100 );
   tbls1->tableBtype = TYPE_ALLOCATED;

   if (tbls1->tableDtype == TYPE_REFERENCED)
      tbls1->tableD = NULL;
   if (tbls1->tableD == NULL)
      tbls1->tableD = (EntryTableDArray)arr_create( arr_count(tbls2->tableD),
            sizeof(EntryTableD *), 100 );
   tbls1->tableDtype = TYPE_ALLOCATED;

   bufr_merge_tableB( tbls1->tableB, tbls2->tableB );
   bufr_merge_tableD( tbls1->tableD, tbls2->tableD );

   arr_sort( tbls1->tableB, compare_tableb );
   arr_sort( tbls1->tableD, compare_tabled );

   if (tbls1->version < tbls2->version ) 
      tbls1->version = tbls2->version ;
   }

/**
 * @english
 *    bufr_load_m_tableB(tables, filename)
 *    (BUFR_Tables *, char *filename)
 * This call is used to load non-CMC Table B versions inside the master
 * table set.
 * @warning The format of the files is still the same as the CMC Format.
 * @return int
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @ingroup io tables
 */
int bufr_load_m_tableB( BUFR_Tables *tbls, const char *filename )
   {
   return bufr_load_tableB ( tbls, &(tbls->master), filename, 1 );
   }

/**
 * @english
 *    bufr_load_l_tableB( file_tables, str_ltableb )
 *    (BUFR_Tables *, char *filename)
 * This call used for loading local Table B, and it can also contain WMO
 * elements as part of the table
 * @warning Local elements loaded here will override master table element
 * definitions. The format of the files is still the same as the CMC
 * Format.
 * @return int
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @ingroup tables
 */
int bufr_load_l_tableB( BUFR_Tables *tbls, const char *filename )
   {
   return bufr_load_tableB ( tbls, &(tbls->local), filename, 1 );
   }

/**
 * @english
 * load a table B file
 * @param       tbls      : the Set of tables to load into
 * @param       filename : string refering to table B
 * @param       local    : if local descriptors should be included
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup tables internal
 */
static int bufr_load_tableB( BUFR_Tables *tables, BufrTablesSet *tbls, const char *filename, int local )
   {
   char  data_cat_desc[65];
   int   data_cat;
   int   version;

   tbls->tableBtype = TYPE_ALLOCATED;

   data_cat_desc[0] = '\0';
   data_cat = -1;
   if (tbls->tableB == NULL)
      {
      tbls->tableB = bufr_tableb_read( NULL, filename, local, data_cat_desc, &data_cat, &version );
      tbls->version = version;
      if (bufr_is_debug())
         {
         char buf[128];

         sprintf( buf, _("Info:  Loaded Table B: %s  version=%d\n"), filename, version );
         bufr_print_debug( buf );
         }
      }
   else
      {
      char *tableB;

      tableB = bufr_tableb_read( NULL, filename, local, data_cat_desc, 
                                 &data_cat, &version );
      bufr_merge_tableB( tbls->tableB, tableB );
      bufr_tableb_free( tableB );
      tbls->version = version;
      if (bufr_is_debug())
         {
         char buf[128];

         sprintf( buf, _("Info:  Merged Table B: %s  version=%d\n"), filename, version );
         bufr_print_debug( buf );
         }
      }

   if (local)
      {
      bufr_set_tables_category( tables, data_cat, data_cat_desc );
      }

   if (tbls->tableB == NULL) return -1;
   arr_sort( tbls->tableB, compare_tableb );
   return 0;
   }

/**
 * @english
 *    bufr_load_m_tableD(tables, filename)
 *    (BUFR_Tables *, char *filename)
 * This call is used to load non-CMC Table D versions inside the master
 * table set. This along with the table B call (bufr_load_m_tableB) is an
 * alternative to bufr_load_cmc_tables.
 * @warning The format of the files is still the same as the CMC Format.
 * @return int
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @ingroup tables
 */
int bufr_load_m_tableD( BUFR_Tables *tbls, const char *filename )
   {
   return bufr_load_tableD ( tbls, &(tbls->master), filename );
   }

/**
 * @english
 *    bufr_load_l_tableD( file_tables, str_ltabled )
 *    (BUFR_Tables *, char *filename)
 * This call is used to load non-CMC Table D versions inside the local
 * table set.
 * @warning The local files are looked at first. If the descriptor cannot
 * be found in the local files, then the master one will be searched. The
 * format of the files is still the same as the CMC Format.
 * @return int
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @ingroup tables
 */
int bufr_load_l_tableD( BUFR_Tables *tbls, const char *filename )
   {
   return bufr_load_tableD ( tbls, &(tbls->local), filename );
   }

/**
 * @english
 * load table D from a file
 * @param       tbls     : pointer to BUFR_Tables structure containing tables
 * @param       tbl      : the Set of tables to load into
 * @param       filename : string refering to table D
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static int bufr_load_tableD( BUFR_Tables *tbls, BufrTablesSet *tbl, const char *filename )
   {
   int  rtrn;

   tbl->tableDtype = TYPE_ALLOCATED;

   if (tbl->tableD == NULL)
      {
      tbl->tableD = bufr_tabled_read( NULL, filename );
      if (bufr_is_debug())
         {
         char buf[128];

         sprintf( buf, _("Info:  Loaded Table D: %s\n"), filename );
         bufr_print_debug( buf );
         }
      }
   else
      {
      EntryTableBArray tableD = bufr_tabled_read( NULL, filename );
      bufr_merge_tableD( tbl->tableD, tableD );
      bufr_tabled_free( tableD );
      if (bufr_is_debug())
         {
         char buf[128];

         sprintf( buf, _("Info:  Merged Table D: %s\n"), filename );
         bufr_print_debug( buf );
         }
      }

   arr_sort( tbl->tableD, compare_tabled );

#if DEBUG
   test_print_tableD( tbl->tableD );
#endif
/*
 * check for any circular loop in table D

*/
   rtrn = bufr_check_loop_tableD( tbls, tbl );

   return rtrn;
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
static void bufr_merge_tableD ( EntryTableDArray table1, EntryTableDArray table2 )
   {
   EntryTableD *e1, *e2;
   EntryTableD **pe;
   int   count, i;
   int   len2;

   count = arr_count( table2 );
   for (i = 0; i < count ; i++)
      {
      pe = (EntryTableD **)arr_get( table2, i );
      e2 = (pe)? *pe : NULL;
      if (e2)
         {
         len2 = e2->description ? strlen( e2->description ) : 0 ;
         e1 = bufr_tabled_fetch_entry( table1, e2->descriptor );
         if (e1)
            {
            bufr_copy_EntryTableD( e1, e2->description,  len2, e2->descriptors, e2->count );
            }
         else
            {
            e1 = bufr_new_EntryTableD( e2->descriptor, e2->description, len2, e2->descriptors, e2->count );
            arr_add( table1, (char *)&e1 );
            }
         }
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
static void bufr_merge_tableB ( EntryTableBArray table1, EntryTableBArray table2 )
   {
   EntryTableB *e1, *e2;
   EntryTableB **pe;
   int   count, i;

   count = arr_count( table2 );
   for (i = 0; i < count ; i++)
      {
      pe = (EntryTableB **)arr_get( table2, i );
      e2 = (pe)? *pe : NULL;
      if (e2)
         {
         e1 = bufr_tableb_fetch_entry( table1, e2->descriptor );
         if (e1)
            {
            bufr_copy_EntryTableB( e1, e2 );
            }
         else
            {
            e1 = bufr_new_EntryTableB();
            bufr_copy_EntryTableB( e1, e2 );
            arr_add( table1, (char *)&e1 );
            }
         }
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
static int bufr_check_loop_tableD( BUFR_Tables *tbls, BufrTablesSet *tbl )
   {
   int  i, t;
   int  count;
   EntryTableD  *etd, **p_etd;
   char   *array;
   int     has_error=0;

   array = (IntArray)arr_create( 100, sizeof(int), 100 );

   count = arr_count( tbl->tableD );
   for (t = 0; t < count ; t++)
      {
      p_etd = (EntryTableD **)arr_get( tbl->tableD, t );
      etd = p_etd[0];
      for (i = 0; i < etd->count ; i++ )
         {
         if (bufr_check_desc_tableD( tbls, etd->descriptors[i], array ) < 0)
            {
            has_error = -1;
            }
         }
      }

   arr_free( &array );
   return has_error;
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
static int bufr_check_desc_tableD( BUFR_Tables *tbls, int desc , char *array )
   {
   int  f;
   EntryTableD  *etd;
   int  i;
   int  count;
   int  *pival, ival;
   char  errmsg[256];

   f = DESC_TO_F( desc );
   if (f == 3)
      {
      count = arr_count( array );
      for (i = 0; i < count ; i++ )
         {
         pival = (int *)arr_get( array, i );
         if (pival)
            {
            ival = *pival;
            if (ival == desc)
               {
               sprintf( errmsg, _("Warning: Table D descriptor : %d is in a circular loop\n"), desc );
               bufr_print_debug( errmsg );
               return -1;
               }
            }
         }
      arr_add( array, (char *)&desc );

      etd = bufr_fetch_tableD( tbls, desc );
      if (etd == NULL)
         {
         sprintf( errmsg, _("Warning: invalid Table D descriptor : %d\n"), desc );
         bufr_print_debug( errmsg );
         return -1;
         }
      for (i = 0; i < etd->count ; i++ )
         {
         if (bufr_check_desc_tableD( tbls, etd->descriptors[i], array ) < 0) return -1;
         }

      arr_del( array, 1 );
      }
   return 1;
   }

/**
 * @english
 * find and return a table B entry
 * @note Checks both the master and local tables no irrespective of whether
 * it's a master or local descriptor.
 * @param  tbls: target BUFR Tables for the search
 * @param  desc: descriptor to be found
 * @endenglish
 * @francais
 * chercher et retourner une entree de la table B
 * @param       desc : descripteur a chercher
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup tables
 */
EntryTableB *bufr_fetch_tableB(BUFR_Tables *tbls, int desc)
   {
   int          f;
   EntryTableB *e;

	if( tbls == NULL ) return errno=EINVAL, NULL;

   if (tbls->last_searched)
      {
      if (desc == tbls->last_searched->descriptor)
         return tbls->last_searched;
      }

   if (tbls->tableB_cache)
      {
      e = bufr_tableb_fetch_entry( tbls->tableB_cache, desc );
      if (e != NULL)
         {
         tbls->last_searched = e;
         return e;
         }
      }
   else
      {
      tbls->tableB_cache = (EntryTableBArray)arr_create( 100, sizeof(EntryTableB *), 100 );
      }

   f = DESC_TO_F( desc );
   switch( f )
      {
      case 1 :
      case 2 :
      case 3 :
         return NULL;
      default :
         break;
      }

   e=NULL;

   if (tbls->local.tableB)
      e = bufr_tableb_fetch_entry( tbls->local.tableB, desc );
   if (e == NULL)
      e = bufr_tableb_fetch_entry( tbls->master.tableB, desc );
   if (e == NULL)
      {
      bufr_errcode = BUFR_TB_NOTFOUND;
      if (bufr_is_debug())
         {
         char buf[128];

         sprintf( buf, _("Warning: Unknown BUFR descriptor: %d\n"), desc );
         bufr_print_debug( buf );
         }
      return NULL;
      }

   arr_add( tbls->tableB_cache, (char *)&e );
   arr_sort( tbls->tableB_cache, compare_tableb );

   return e;
   }

/**
 * @english
 * find and return a Table D entry
 * @note Checks both the master and local tables no irrespective of whether
 * it's a master or local descriptor.
 * @param  tbls: target BUFR Tables for the search
 * @param  desc : descriptor to be found
 * @endenglish
 * @francais
 * chercher et retourner une entree de la table D
 * @param       desc : descripteur a chercher
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup tables
 */
EntryTableD *bufr_fetch_tableD(BUFR_Tables *tbls, int desc)
   {
   EntryTableD *e=NULL;
   int   f;

	if( tbls == NULL ) return errno=EINVAL, NULL;

   f = DESC_TO_F( desc );
   switch( f )
      {
      case 3 :
         break;
      case 2 :
      case 1 :
      default :
         return NULL;
      }

   if (tbls->local.tableD)
      e = bufr_tabled_fetch_entry( tbls->local.tableD, desc );
   if (e == NULL)
      e = bufr_tabled_fetch_entry( tbls->master.tableD, desc );
   if (e == NULL)
      {
      char buf[128];
      sprintf( buf, _("Warning: Table D Code unknown: %d\n"), desc );
      bufr_print_debug( buf );
      }
   return e;
   }

/**
 * @english
 * @brief map a sequence of descriptors to a table D entry
 * @param tbls tables to search
 * @param ndesc number of descriptors in sequence to match
 * @param desc array of sequences to match
 * @return matching EntryTableD, or NULL on failure
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Christophe Beauregard
 * @ingroup tables
 */
EntryTableD *bufr_match_tableD_sequence  ( BUFR_Tables * tbls,
                                           int ndesc, int desc[] )
   {
   EntryTableD *e=NULL;

	if( tbls == NULL ) return errno=EINVAL, NULL;
	if( 0 == ndesc ) return errno=EINVAL, NULL;

   if (tbls->local.tableD)
      e = bufr_tabled_match_sequence( tbls->local.tableD, ndesc, desc );
   if (e == NULL)
      e = bufr_tabled_match_sequence( tbls->master.tableD, ndesc, desc );
   if (e == NULL)
      bufr_print_debug( _("Warning: Table D sequence not found\n") );
   return e;
   }

/**
 * @english
 * find the length of a string without trailing white space
 * @param       Str : the string to evaluate
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Jean-Philippe Gauthier
 * @ingroup internal
 */
static int strtlen(char *Str)
   {
   register int len=strlen(Str);

   while(len && *(Str+len-1)==' ') len--;
      return(len);
   }

/**
 * bufr_tableb_read
 * @english
 * read Table B from an ASCII file
 * @param     filename    : Table B file 
 * @param     addr_tableb : already allocated table b array to load in or merge with
 * @param     local       : if loading should restrict range to local descriptors only 
 *                    x=[1,47] y=[1,192]
 * @param     ltds        returned string describing the local table in the file
 * @param     cat         returned local table category description in the file
 * @param     version     returned version of table B
 * @return     EntryTableBArray : A Table B array
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 *
 */
static EntryTableBArray bufr_tableb_read
   (EntryTableBArray addr_tableb, const char *filename, int local, 
    char *ltds, int *cat, int *version)
   {
   FILE         *fp ;
   char          ligne[256] ;
   char          buf[256] ;
   EntryTableB  *etb;
   int           column[7], count;
   int           desc;
   int           desclen, len;
   int           ver = -1;
   char          errmsg[256];
   int           isdebug=bufr_is_debug();
   int           lineno=0;

   if (filename == NULL) return NULL;
/*
 * on va lire la premiere ligne, car elle ne fait pas partie du fichier

*/

   fp = fopen ( filename, "rb" ) ;
   if (fp == NULL)
      {
      sprintf( buf, _("Warning: can't open Table B file %s\n"), filename );
      bufr_print_debug( buf );
      return NULL;
      }

   if (addr_tableb == NULL)
      addr_tableb = (EntryTableBArray)arr_create( 100, sizeof(EntryTableB *), 100 );
/*
   if ( fgets(ligne,256,fp) == NULL ) return NULL;
   count = bufr_parse_columns(ligne, column, 7);
   if (count < 6)
      {
      column[0] = 0; column[1] = 8; column[2] = 52;
      column[3] = 63; column[4] = 66; column[5] = 78;
      count = 6;
      }
*/
   memset( ligne, (int)' ', 256 );
/**
 * on ne lit que les lignes ayant un descripteur commencant par 0
 * et ce, jusqu'au premier element local ou la fin du fichier.
 **/
   lineno = 0;
   while ( fgets(ligne,256,fp) != NULL )
      {
      ++lineno;
      if ( lineno==1 )
         {
         count = bufr_parse_columns(ligne, column, 7);
         if (count < 6)
            {
            column[0] = 0; column[1] = 8; column[2] = 52;
            column[3] = 63; column[4] = 66; column[5] = 78;
            count = 6;
            desclen = column[2] - column[1];
            }
         else
            {
            desclen = column[2] - column[1];
            continue;
            }
         }

      if ( strncmp( ligne, "DATA_CATEGORY=", 14 ) == 0) 
         /* for local tables only */
         {
         *cat = atoi ( &ligne[14] ) ;
         continue;
         }
      else if ( strncmp( ligne, "DATA_DESCRIPTION=", 17 ) == 0) 
         /* for local tables only */
         {
         len = strlen( &ligne[17] );

         if (ligne[len-1] == '\n')
            {
            ligne[len-1] = '\0';
            len -= 1;
            }
         if (len > 64) len = 64;
         if (len > 0)
            strncpy ( ltds,   &ligne[17],  len ) ;
         ltds[len] = '\0';
         continue;
         }

      if ( ligne[0] == '#' ) continue ; /* comments */
      if ((ver < 0)&&(strncmp(ligne, "** VERSION", 10) == 0))
         ver = atoi( &ligne[11] ) ;
      if ( ligne[0] == '*' ) continue ; /* comments */

      if ( ligne[0] != '0' ) continue ; /* for table B, F=0  */

      if (strlen( ligne ) < 82) continue; /* line is incomplete */

      desc = atoi ( &ligne[0] ) ;

      if (!bufr_is_table_b( desc )) 
         {
#if DEBUG
         if (isdebug)
            {
            sprintf( buf, _("Skipped invalid descriptor: %s\n"), ligne );
            bufr_print_debug( buf );
            }
#endif
         continue;
         }

      if (local == 1)
         {
         if ((count == 7)&&(ligne[column[6]] == '-')) continue;
         }
      else
         {
         if (bufr_is_local_descriptor( desc )) 
            {
#if DEBUG
            if (isdebug)
               {
               sprintf( buf, _("Skipped local descriptor: %s\n"), ligne );
               bufr_print_debug( buf );
               }
#endif
            continue;
            }
         }

      /*Supprimer les espaces a la fin du descripteur*/
      len=desclen;
      while(len && ligne[column[1]+len-1]==' ') len--;

      etb = bufr_new_EntryTableB();
      etb->description = (char *)malloc((len+1)*sizeof(char));
      strncpy ( etb->description,   &ligne[column[1]],  len ) ;
      etb->description[len]   = '\0' ;
      etb->description[strtlen(etb->description)] = '\0' ;
      strncpy( buf, &(ligne[column[2]]), 11 );
      buf[11] = '\0';
      etb->unit = strimdup( NULL, buf, 11 );
      etb->descriptor           = atoi ( &ligne[column[0]] ) ;
      etb->encoding.scale       = atoi ( &ligne[column[3]] ) ;
      etb->encoding.reference   = atoi ( &ligne[column[4]] ) ;
      etb->encoding.nbits       = atoi ( &ligne[column[5]] ) ;

      etb->encoding.type        = bufr_unit_to_datatype( etb->unit );
      if (etb->encoding.type == TYPE_UNDEFINED)
         {
         sprintf( errmsg, _("Warning: error while loading Table B file: %s\n"), 
            filename );
         bufr_print_debug( errmsg );
         sprintf( errmsg, _("Error reading descriptor: %d unit=\"%s\"\n"), 
                etb->descriptor, etb->unit );
         bufr_print_debug( errmsg );
         }
#if DEBUG
      if (isdebug)
         {
         sprintf( buf, _("Loaded descriptor: %s\n"), ligne );
         bufr_print_debug( buf );
         }
#endif
      arr_add( addr_tableb, (char *)&etb );
      }
   fclose ( fp ) ;

   *version = ver;
   return addr_tableb;
   }

/**
 * @english
 * search for a Table B entry
 * @param addr : an instance of Table
 * @param desc : the descriptor to be found 
 * @endenglish
 * @francais
 * rechercher une entree dans la table B
 * @param     addr  : une table B
 * @param     desc : le descripteur recherche
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup tables
 */
EntryTableB *bufr_tableb_fetch_entry(EntryTableBArray addr, int desc)
   {
   EntryTableB *ptr1, tb, **ptr;

   tb.descriptor = desc;
   ptr1 = &tb;
   ptr = (EntryTableB **)arr_search( addr, (char *)&ptr1, compare_tableb );
   if ( ptr != NULL)
      return *ptr;
   else
      return NULL;
   }

/**
 * @english
 * @todo translate
 * @endenglish
 * @francais
 * comparaison d'ordre des entrees de table B
 * @param     p1, p2 : l'addresse des 2 entrees
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
static int compare_tableb(const void *p1, const void *p2)
   {
   EntryTableB *r1 = *(EntryTableB **)p1;
   EntryTableB *r2 = *(EntryTableB **)p2;

   if (r1->descriptor < r2->descriptor) return -1;
   if (r1->descriptor > r2->descriptor) return 1;
   return 0;
   }

/**
 * @english
 * @todo translate
 * @endenglish
 * @francais
 * chercher et retourner une entree de la table B d'une description
 * @param       addr : une table B
 * @param       desc : Description a chercher
 * @endfrancais
 * @author Jean-Philippe Gauthier
 * @ingroup tables

 */
EntryTableB *bufr_tableb_fetch_entry_desc( EntryTableBArray addr, const char *desc )
   {
   EntryTableB **ptr;
   int i,cnt;
   char *cstring;

   cnt = arr_count( addr );
   for (i = 0; i < cnt ; i++)
      {
      ptr = (EntryTableB **)arr_get( addr, i );
      if (ptr)
         {
         cstring = ptr[0]->description;
         if (cstring )
            {
            if (strcmp( desc , cstring  ) == 0 )
               return (*ptr);
            }
         }
      }
   return (NULL);
   }

/**
 * @english
 * @todo translate
 * @endenglish
 * @francais
 * creer une nouvelle entree de table B
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup tables
 */
EntryTableB *bufr_new_EntryTableB(void)
   {
   EntryTableB *r;

   r = (EntryTableB *)malloc(sizeof(EntryTableB));
   r->descriptor         = 0;
   r->encoding.af_nbits  = 0;
   r->encoding.nbits     = 0;
   r->encoding.scale     = 0;
   r->encoding.reference = 0;
   r->encoding.type      = TYPE_NUMERIC;
   r->description        = NULL;
   r->unit               = NULL;
   return r;
   }

/**
 * @english
 * @todo translate
 * @endenglish
 * @francais
 * liberer une entree de table B
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup tables
 */
void bufr_free_EntryTableB( EntryTableB *r )
   {
   if (r != NULL)
      {
      if (r->description != NULL)  free( r->description );
      r->description = NULL;
      if (r->unit != NULL)  free( r->unit );
      r->unit = NULL;
      free( r );
      }
   }

/**
 * @english
 * @todo translate
 * @endenglish
 * @francais
 * creer une nouvelle entree de table B
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup tables
 */
void  bufr_copy_EntryTableB( EntryTableB *e1, EntryTableB *e2 )
   {
   if ((e1 == NULL)||(e2 == NULL))
      {
      return;
      }

   e1->descriptor = e2->descriptor;
   e1->encoding.scale = e2->encoding.scale;
   e1->encoding.reference = e2->encoding.reference;
   e1->encoding.nbits = e2->encoding.nbits;
   e1->encoding.type = e2->encoding.type;

   if (e1->unit)
      {
      free( e1->unit );
      e1->unit = NULL;
      }
   if (e2->unit)
      {
		e1->unit = e2->unit ? strdup(e2->unit) : NULL;
      }
   if (e1->description)
      {
      free( e1->description );
      e1->description = NULL;
      }
   if (e2->description)
      {
		e1->description = e2->description ? strdup(e2->description) : NULL;
      }
   }

/**
 * @english
 * @todo translate
 * @endenglish
 * @francais
 * detruire une table B
 * @param     tableb : la table a detruire
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup tables
 */
void bufr_tableb_free(EntryTableBArray tableb)
   {
   int count, i;
   EntryTableB *r;
   EntryTableB **pe;

   count = arr_count(tableb);
   for ( i = 0 ; i < count ; i++ )
      {
      pe = (EntryTableB **)arr_get( tableb, i );
      r = (pe)? *pe : NULL;
      if ( r != NULL)
         {
         bufr_free_EntryTableB( r );
         }
      }

   arr_free( &tableb );
   }

/**
 * @english
 * read Table D from a file
 * @param   filename : file containing table D
 * @param   addr_tabled : table D to be incremented (if relevant)
 * @endenglish
 * @francais
 * lire une table D d'un fichier
 * @param     filename : nom du fichier
 * @param     addr_tabled  : une table D a incrementer (s'il y a lieu)
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup tables
 */
static EntryTableDArray bufr_tabled_read (EntryTableDArray addr_tabled, const char *filename)
   {
   int desclen = 0;
   FILE *fp ;
   char ligne[4096] ;
   EntryTableD  *etb;
   int  count;
   int  descriptors[1024];
   char *tok;
   int  column[20];
	char description[sizeof(ligne)];

   if (filename == NULL) return NULL;

   description[0] = '\0';
   fp = fopen ( filename, "rb" ) ;
   if (fp == NULL)
		{
      sprintf( ligne, _("Warning: can't open Table D file %s\n"), filename );
      bufr_print_debug( ligne );
      return NULL;
      }

   if (addr_tabled == NULL)
      addr_tabled = (EntryTableDArray)arr_create( 50, sizeof(EntryTableD *), 10 );

   desclen = 0;
   while ( fgets(ligne,sizeof(ligne),fp) != NULL )
      {
      if ( ligne[0] == '*' )
         {
         if (strncmp( ligne, "*TABLED7890123456", 17 ) == 0)
            {
            count = bufr_parse_columns( ligne, column, 3 );
            if (count == 3)
               {
               desclen = column[1]-column[0];
               }
            else
               {
               desclen = 0;
               }
            }
			
			/* In practice, descriptions are actually in the comment
			 * section immediately before the actual template.
			 * Accumulate those comments as a description if it's not
			 * provided in another fashion.
			 */
			if( desclen == 0 )
				{
				size_t l = strspn(ligne,"* \t\r\n");
				char* p = ligne[l] ? &ligne[l] : NULL;
				if( p )
					{
					size_t s = strlen(description);
					size_t l = strlen(p);
					while(p[l-1]=='\n' || p[l-1]=='\r' ) p[(l--)-1] = 0;
					if( s + l + 2 < sizeof(description) )
						{
						if( description[0] ) strcat(description," ");
						strcat(description,p);
						}
					}
				else
					{
					description[0] = 0;
					}
				}

         continue ;
         }

      if ( ligne[0] == '#' ) continue ; /* comments */
      if ( ligne[0] != '3' ) continue ; /* for table D , F=3 */

      tok = strtok( ligne+desclen, " \t\n" );
      if (tok == NULL) continue;

      if ( tok[0] != '3') continue;
      count = 0;
      while ( tok )
         {
         descriptors[count++] = atoi(tok);
         tok = strtok( NULL, " \t\n" );
         }
      if (count > 1)
         {
         etb = bufr_new_EntryTableD( descriptors[0],
				desclen ? ligne : description,
				desclen ? desclen : strlen(description),
				descriptors+1, count-1 );
         arr_add( addr_tabled, (char *)&etb );
         }
			description[0] = 0;
      }
   fclose ( fp ) ;

   return addr_tabled;
   }

/**
 * @english
 * @todo translate
 * @endenglish
 * @francais
 * rechercher une entree dans la table D
 * @param     addr_tabled  : une table D
 * @param     code : le code recherche
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup tables
 */
static EntryTableD *bufr_tabled_fetch_entry(EntryTableDArray addr_tabled, int desc)
   {
   EntryTableD *ptr1, tb, **ptr;

   tb.descriptor = desc;
   ptr1 = &tb;
   ptr = (EntryTableD **)arr_search( addr_tabled,
                                     (char *)&ptr1, compare_tabled );
   if ( ptr != NULL)
      return *ptr;
   else
      return NULL;
   }

/**
 * @english
 * compare the sequence of descriptors in two table D entries
 * @param     p1, p2 : l'addresse des 2 entrees
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Christophe Beauregard
 * @ingroup tables
 * @todo translate both
 */
static int compare_tabled_seq(const void *p1, const void *p2)
   {
   EntryTableD *r1 = *(EntryTableD **)p1;
   EntryTableD *r2 = *(EntryTableD **)p2;
	int i;

	if( r1->count != r2->count ) return r1->count - r2->count;

	for( i = 0; i < r1->count; i ++ )
		{
		if( r1->descriptors[i] != r2->descriptors[i] )
			{
				return r1->descriptors[i] - r2->descriptors[i];
			}
		}

   return 0;
   }

/**
 * @english
 * @brief map a sequence of descriptors to a table D entry
 * @param tabled table to search
 * @param ndesc number of descriptors in sequence to match
 * @param desc array of sequences to match
 * @return matching EntryTableD, or NULL on failure
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Christophe Beauregard
 * @ingroup tables
 */
EntryTableD   *bufr_tabled_match_sequence  ( EntryTableDArray tabled,
                                             int ndesc, int desc[] )
   {
   EntryTableD tb, *ptb;
	int i, n;

	if( tabled == NULL ) return errno=EINVAL, NULL;
	if( ndesc == 0 ) return errno=EINVAL, NULL;

	tb.descriptors = &desc[0];
	tb.count = ndesc;
	tb.descriptor = 0;
	ptb = &tb;

	/* Note that we can't do an arr_search using our comparison function;
	 * that does a binary search, and the array is sorted on a different
	 * set of keys. So we need to do it the hard way.
	 */
	n = arr_count(tabled);
	for( i = 0; i < n; i ++ )
		{
		void* element = arr_get(tabled,i);
		if( NULL != element && 0 == compare_tabled_seq( element, &ptb ) )
			{
				return *(EntryTableD**) element;
			}
		}

	return NULL;
   }

/**
 * @english
 * @todo translate
 * @endenglish
 * @francais
 * comparaison d'ordre des entrees de table D
 * @param     p1, p2 : l'addresse des 2 entrees
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup tables
 */
static int compare_tabled(const void *p1, const void *p2)
   {
   EntryTableD *r1 = *(EntryTableD **)p1;
   EntryTableD *r2 = *(EntryTableD **)p2;

   if (r1->descriptor < r2->descriptor) return -1;
   if (r1->descriptor > r2->descriptor) return 1;
   return 0;
   }

/**
 * @english
 * @todo translate
 * @endenglish
 * @francais
 * creer une nouvelle entree de table D
 * @param     descriptor : le code a placer dans la Table D
 * @param     dsc : la description de l'element
 * @param     descriptors : les codes a placer dans l'entree
 * @param     count : compte des codes
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup tables
 */
EntryTableD *bufr_new_EntryTableD(int descriptor, const char *dsc, int dsclen, int *descriptors, int count)
   {
   EntryTableD *r;

   r = (EntryTableD *)malloc(sizeof(EntryTableD));
   r->description = NULL;
   r->descriptors = NULL;
   r->count = 0;
   r->descriptor = descriptor;
   bufr_copy_EntryTableD( r, dsc, dsclen, descriptors, count );
   return r;
   }

/**
 * @english
 * @todo translate
 * @endenglish
 * @francais
 * detruire une table D
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup tables
 */
static void bufr_copy_EntryTableD
   ( EntryTableD *r, const char *desc, int desclen, int *descriptors, int count)
   {
   if (r->description != NULL)
      {
      free( r->description );
      r->description = NULL;
      }

   if (r->descriptors != NULL)
      {
      free( r->descriptors );
      r->descriptors = NULL;
      r->count = 0;
      }

   if ((desclen > 0)&&(desc != NULL))
      {
      r->description = (char *)malloc((desclen+1)*sizeof(char));
      strncpy( r->description, desc, desclen );
      r->description[desclen] = '\0';
      }
   if (count > 0)
      {
      int i;

      r->descriptors = (int *)malloc(count*sizeof(int));
      r->count = count;
      for ( i = 0 ; i <  count ; i++ )
         {
         r->descriptors[i] = descriptors[i];
         }
      }
   else
      {
      r->descriptors = NULL;
      r->count = 0;
      }
   }

/**
 * @english
 * @todo translate
 * @endenglish
 * @francais
 * liberer une entree de table D
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup tables
 */
void bufr_free_EntryTableD( EntryTableD *r )
   {
   if (r != NULL)
      {
		if (r->description != NULL)  free( r->description );
		if (r->descriptors != NULL)  free( r->descriptors );
		free( r );
      }
   }

/**
 * @english
 * @todo translate
 * @endenglish
 * @francais
 * detruire une table D
 * @param     tabled : la table a detruire
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup tables
 */
void bufr_tabled_free(EntryTableDArray tabled)
   {
   int count, i;
   EntryTableD *r, **pe;

   count = arr_count(tabled);
   for ( i = 0 ; i < count ; i++ )
      {
      pe = (EntryTableD **)arr_get( tabled, i );
      r = pe ? *pe : NULL;
      if ( r != NULL)
         {
			bufr_free_EntryTableD( r );
         }
      }
   arr_free( &tabled );
   }

/**
 * @english
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup tables internal
 */
int bufr_parse_columns(const char *ligne, int *column, int limit)
   {
   int i, count;

   for ( i = 0, count = 0 ; i < 256 ; i++ )
      {
      switch( ligne[i] )
         {
         case '\n':case '\0':
            i = 256;
            break;
         case '*' :
            if (count < limit) 
               {
               column[count] = i;
               }
            ++count;
            break;
         default :
         break;
         }
      }
   return count;
   }

/**
 * @english
 * @todo translate
 * @endenglish
 * @francais
 * identifie le nombre de bits minimum pour contenir une value
 * @param       ival : la valeur
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 */
int bufr_value_nbits(int64_t val)
   {
   static uint64_t bmaxval[65] = {0};
   static uint64_t bnegval[65] = {0};
   int i;
   uint64_t        ival;

   if (bmaxval[0] == 0)
      {
      for ( i = 1 ; i <= 64 ; i++ )
         {
         bmaxval[i] = (1ULL<<i)-1L;
         bnegval[i] = 1ULL<<i;
         }
      }

   if (val >= 0)
      {
      ival = (uint64_t)val;
      for ( i = 1 ; i <= 64 ; i++ )
         if (bmaxval[i] > ival) break;
      }
   else
      {
      ival = abs(val);
      for ( i = 1 ; i <= 64 ; i++ )
         if (bnegval[i-1] > ival) break;
      }

   return i;
   }

/**
 * @english
 * @todo translate
 * @endenglish
 * @francais
 * identifie le bit le plus a gauche dans un mot
 * @param       val : la valeur
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
int bufr_leftest_bit(uint64_t val)
   {
   int i=0;

   while (val > 0)
      {
      ++i;
      val = val >> 1;
      }
   return i;
   }

/**
 * @english
 * @todo translate
 * @endenglish
 * @francais
 * convertir une valeur reel en entier avec l'entree
 *           de la table B
 * @param       code   : descripteur de la Table B
 * @param       be     : encodage de la table B
 * @param       fval   : la valeur a convertir
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
uint32_t bufr_cvt_fval_to_i32(int code, BufrValueEncoding *be, float fval)
   {
   uint32_t  ival, rem;
   uint64_t  maxval;
   uint64_t  missing;
   float     val_pow;
   int       ival_pow;
   double    val1;
   float     fmin, fmax;
   int       underflow=0, overflow=0;
   int       delta;

   if (be->nbits > 32)
      {
      char buffer[128];

      sprintf( buffer, _("Warning: unsupported bitwidth %d (max=%d) for descriptor %d"),
               be->nbits, 32, code );
      bufr_print_debug( buffer );
      return 0;
      }

   missing = bufr_missing_ivalue( be->nbits );
   if (bufr_is_missing_float( fval )) return missing;
/*
 * compute value range
 */
   maxval = (1ULL << be->nbits) - 1;
   ival_pow = val_pow = pow(10.0,(double)be->scale);
   fmin = be->reference / val_pow;
   fmax = ((maxval-1) + be->reference) / val_pow;

   if (fval > fmax)
      {
      int  x;

      overflow = 1;
/* Reg:94.1.5 NA to class 31, value==maxval */
      x = DESC_TO_X( code );
      if (x == 31)
         {
         ival = (int)fval;
         if (ival == maxval) overflow = 0;
         }
      }
   else if (fval < fmin)
      {
      underflow = 1;
      }
   else if (be->scale >= 0)
      {
      val1 = fval - (be->reference / val_pow);
      ival = round(val1 * val_pow);
      delta = maxval-ival;
      if ( delta < be->reference )
         {
         ival = val1;
         rem = round( (val1 - ival) * val_pow );
         ival = ival * val_pow + rem;
         }
      else
         {
         if (fval > 0.0)
            {
            uint32_t sval;
            ival = fval;
            sval = ival * ival_pow;
            rem = round( (fval - ival) * val_pow );
            ival = sval - be->reference;
            ival = ival + rem;
            }
         else
            {
            int32_t sval = round( fval * val_pow );
            ival = sval - be->reference;
            }
         }
      if (ival >= maxval) overflow = 1;
      }
   else
      {
      int sval = round(fval * val_pow);
      ival = sval - be->reference;
      }

   if (underflow)
      {
      char buffer[128];
      int32_t  minval;
      minval = rint(fval * val_pow);
      bufr_minimum_reference = minval - 1;
      bufr_errtbe = *be;
      bad_descriptor = code;
      bufr_errcode = BUFR_TB_UNDERFLOW;
      minval = (int32_t)ival - be->reference;
      sprintf( buffer, _("Warning: UNDERFLOW with element %d : value = %e, giving %d"),
               code, fval, minval );
      bufr_print_debug( buffer );
      ival = maxval;
      }
   else if (overflow)
      {
      char buffer[128];

      val1 = fval - fmax;
      bufr_minimum_nbits = be->nbits + bufr_value_nbits( val1 * val_pow );
      bufr_errtbe = *be;
      bad_descriptor = code;
      bufr_errcode = BUFR_TB_OVERFLOW;
      sprintf( buffer, _("Warning: OVERFLOW with element %d (max=%llu) : value = %e, giving %llu"),
               code, (unsigned long long)maxval, fval,
					(unsigned long long)missing );
      bufr_print_debug( buffer );
      ival = missing;
      }

   return ival;
   }


/**
 * @english
 * @todo translate
 * @endenglish
 * @francais
 * convertir un entier en valeur reel avec l'entree
 *           de la table B
 * @param       be      : une entree de la table B
 * @param       ival   : la valeur a convertir
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
double bufr_cvt_i64_to_dval(BufrValueEncoding *be, int64_t ival)
   {
   double fval;
   double val_pow;
   uint64_t  missing;

   missing = bufr_missing_ivalue( be->nbits );
   if ((ival < 0)||(ival == missing)) return bufr_get_max_double();

   val_pow = pow(10.0,(double)be->scale);

   if ((be->reference < 0) && (ival < (-be->reference)))
      {
      int64_t val = (int64_t)(ival + be->reference);
      fval = (double)val / val_pow;
      }
   else
      {
      fval = (double)(ival + be->reference) / val_pow ;
      }

   return fval;
   }

/**
 * @english
 * @todo translate
 * @endenglish
 * @francais
 * convertir un entier en valeur reel avec l'entree
 *           de la table B
 * @param       be      : une entree de la table B
 * @param       ival   : la valeur a convertir
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
float bufr_cvt_i32_to_fval(BufrValueEncoding *be, uint32_t ival)
   {
   float     fval;
   float     val_pow;
   uint32_t  missing;

   missing = bufr_missing_ivalue( be->nbits );
   if (ival == missing) return bufr_get_max_float();

   val_pow = pow(10.0,(double)be->scale);

   if ((be->reference < 0) && (ival < (-be->reference)))
      {
      int32_t val = (int32_t)(ival + be->reference);
      fval = (float)val / val_pow;
      }
   else
      {
      fval = (float)(ival + be->reference) / val_pow ;
      }

   return fval;
   }


/**
 * @english
 * @todo translate
 * @endenglish
 * @francais
 * convertir une valeur reel en entier avec l'entree
 *           de la table B
 * @param       code   : descripteur de la table B
 * @param       be     : encodage de la table B
 * @param       fval   : la valeur a convertir
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup internal
 */
uint64_t bufr_cvt_dval_to_i64(int code, BufrValueEncoding *be, double fval)
   {
   uint64_t  ival, rem;
   uint64_t  maxval;
   uint64_t  missing;
   double    val_pow;
   int       ival_pow;
   double    val1;
   double    fmin, fmax;
   int       underflow=0, overflow=0;
   int       delta;

   if (be->nbits > 32)
      {
      char buffer[128];

      sprintf( buffer, _("Warning: unsupported bitwidth %d (max=%d) for descriptor %d"),
               be->nbits, 32, code );
      bufr_print_debug( buffer );
      return 0;
      }

   missing = bufr_missing_ivalue( be->nbits );
   if (bufr_is_missing_double( fval )) return missing;
/*
 * compute value range
 */
   maxval = (1ULL << be->nbits) - 1;
   ival_pow = val_pow = pow(10.0,(double)be->scale);
   fmin = be->reference / val_pow;
   fmax = ((maxval-1) + be->reference) / val_pow;

   if (fval > fmax)
      {
      int  x;

      overflow = 1;
/* Reg:94.1.5 NA to class 31, value==maxval */
      x = DESC_TO_X( code );
      if (x == 31)
         {
         ival = (int)fval;
         if (ival == maxval) overflow = 0;
         }
      }
   else if (fval < fmin)
      {
      underflow = 1;
      }
   else if (be->scale >= 0)
      {
      val1 = fval - (be->reference / val_pow);
      ival = round(val1 * val_pow);
      delta = maxval-ival;
      if ( delta < be->reference )
         {
         ival = val1;
         rem = round( (val1 - ival) * val_pow );
         ival = ival * val_pow + rem;
         }
      else
         {
         if (fval > 0.0)
            {
            uint64_t sval;
            ival = fval;
            sval = ival * ival_pow;
            rem = round( (fval - ival) * val_pow );
            ival = sval - be->reference;
            ival = ival + rem;
            }
         else
            {
            int64_t sval = round( fval * val_pow );
            ival = sval - be->reference;
            }
         }
      if (ival >= maxval) overflow = 1;
      }
   else
      {
      int64_t sval = round(fval * val_pow);
      ival = sval - be->reference;
      }

   if (underflow)
      {
      char buffer[128];
      int64_t  minval;
      minval = rint(fval * val_pow);
      bufr_minimum_reference = minval - 1;
      bufr_errtbe = *be;
      bad_descriptor = code;
      bufr_errcode = BUFR_TB_UNDERFLOW;
      minval = (int64_t)ival - be->reference;
      sprintf( buffer, _("Warning: UNDERFLOW with element %d : value = %e, giving %lld"),
               code, fval, (long long)minval );
      bufr_print_debug( buffer );
      ival = maxval;
      }
   else if (overflow)
      {
      char buffer[128];

      val1 = fval - fmax;
      bufr_minimum_nbits = be->nbits + bufr_value_nbits( val1 * val_pow );
      bufr_errtbe = *be;
      bad_descriptor = code;
      bufr_errcode = BUFR_TB_OVERFLOW;
      sprintf( buffer, _("Warning: OVERFLOW with element %d (max=%llu) : value = %e, giving %llu"),
               code, (unsigned long long)maxval, fval,
					(unsigned long long)missing );
      bufr_print_debug( buffer );
      ival = missing;
      }

   return ival;
   }


/**
 * @english
 * @todo translate
 * @endenglish
 * @francais
 * retourne les conditions d'erreur de table B
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup tables error
 */
int bufr_get_tberror(BufrValueEncoding *be, int *reference, int *nbits)
   {
   *reference = bufr_minimum_reference;
   *nbits = bufr_minimum_nbits;
   *be = bufr_errtbe;

   return bad_descriptor;
   }

/**
 * @english
 * @todo translate
 * @endenglish
 * @francais
 * retourne le code d'erreur du logiciel
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup error
 */
int bufr_errtype(void)
   {
   return bufr_errcode;
   }

/**
 * bufr_unit_to_datatype
 * @english
 * 
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup tables
 */
BufrDataType bufr_unit_to_datatype ( const char *unit )
   {
   BufrDataType  type;
   char     *uc_unit;
   int       i;

	uc_unit = unit ? strdup(unit) : NULL;
	if( NULL == uc_unit )
		return TYPE_UNDEFINED;

   for (i = 0; uc_unit[i] != '\0'; i++)
      uc_unit[i] = toupper( uc_unit[i] );

   if (strncmp( uc_unit, "NUMERI", 6 ) == 0)
      type       = TYPE_NUMERIC;
   else if ((strncmp( uc_unit, "FLAG TABLE", 10 ) == 0)||
            (strncmp( uc_unit, "TABLE FLAG", 10 ) == 0)||
            (strncmp( uc_unit, "TABLEFLAG", 9 ) == 0)||
            (strncmp( uc_unit, "MARQUEURS", 9 ) == 0)||
            (strncmp( uc_unit, "FLAGTABLE", 9 ) == 0))
      type       = TYPE_FLAGTABLE;
   else if ((strncmp( uc_unit, "TABLE CODE", 10 ) == 0)||
            (strncmp( uc_unit, "TABLECODE", 9 ) == 0)||
            (strncmp( uc_unit, "CODE TABLE", 10 ) == 0)||
            (strncmp( uc_unit, "CODETABLE", 9 ) == 0))
      type       = TYPE_CODETABLE;
   else if ((strncmp( uc_unit, "CCITT IA5", 9 ) == 0)||
            (strncmp( uc_unit, "CCITTIA5", 8 ) == 0))
      type       = TYPE_CCITT_IA5;
   else
      type       = TYPE_NUMERIC;

   free( uc_unit );
   return type;
   }

/**
 * @english
 * 
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup tables
 */
BufrDataType bufr_descriptor_to_datatype   ( BUFR_Tables *tbls, EntryTableB *e, int code, int *len )
   {
   int f, x, y;
      
   *len = 0;
   f = DESC_TO_F( code );
   x = DESC_TO_X( code );
   y = DESC_TO_Y( code );

   switch( f )
      {
      case 3 :
         return TYPE_SEQUENCE;
      break;
      case 2 :
         if (x == 5) 
            {
            *len = y;
             return TYPE_CCITT_IA5;
             }
         return TYPE_OPERATOR;
      break;
      case 0 :
         if (e == NULL) 
            e = bufr_fetch_tableB( tbls, code );
         if (e)
            {
            if (e->encoding.type == TYPE_CCITT_IA5)
               *len = (e->encoding.nbits / 8);
            return e->encoding.type;
            }
         return TYPE_UNDEFINED;
      break;
      case 1 :
      default :
         return TYPE_UNDEFINED;
      break;
      }
   }

/**
 * @english
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor tables
 */
int bufr_is_local_descriptor( int code )
   {
   int f, x, y;

   f = DESC_TO_F( code );
   x = DESC_TO_X( code );
   y = DESC_TO_Y( code );
   if ( ( x > 47 ) || (( y > 191 )&&( y <= 255)) ) return 1;

   return 0;
   }

/**
 * @english
 * encode F X Y into 16 bits code
 * @param     f : partie f du code
 * @param     x : partie x du code
 * @param     y : partie y du code
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 */
uint16_t bufr_fxy_to_descriptor_i16(int f,int x,int y)
   {
   uint16_t code;

   code = (((f)&3)<<14)|(((x)&63)<<8)|((y)&255) ;
   return code;
   }

/**
 * @english
 * @todo translate
 * @endenglish
 * @francais
 * encoder code descripteur entiere en 2 octets
 * @param     dcode : code en entier
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 * @todo codectomy
 */
uint16_t bufr_descriptor_i32_to_i16(int dcode)
   {
   int       f,x,y;
   uint16_t  code;

   f = dcode / 100000;
   x = (dcode / 1000)%100;
   y = dcode % 1000;
   code = bufr_fxy_to_descriptor_i16(f,x,y);
   return code;
   }

/**
 * @english
 * @todo translate
 * @endenglish
 * @francais
 * decoder code descripteur en F X Y
 * @param     code : code en entier
 * @param     f    : partie f du code
 * @param     x    : partie x du code
 * @param     y    : partie y du code
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 * @todo codectomy
 */
void bufr_descriptor_to_fxy(int code, int *f, int *x, int *y)
   {
   *f = code / 100000;
   *x = (code / 1000)%100;
   *y = code % 1000;
   }

/**
 * @english
 * @todo translate
 * @endenglish
 * @francais
 * encoder F X Y en code 2 octets
 * @param     f : partie f du code
 * @param     x : partie x du code
 * @param     y : partie y du code
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup descriptor
 * @todo codectomy
 */
int bufr_fxy_to_descriptor( int f, int x, int y )
   {
   int  code;

   code = f * 100000 + x * 1000 + y;
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
 * @ingroup tables
 */
int bufr_is_table_b( int code )
   {
   int f;

   f = code / 100000;
   if (f == 0) return 1;
   return 0;
   }

/**
 * @english
 *   check if a BUFR qualifier
 * @param desc descriptor to check
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Chris Beauregard
 * @ingroup tables
 */
int bufr_is_qualifier( int desc )
   {
   int f, x;
   f = desc / 100000;
	if( f!=0 ) return 0;
   x = (desc / 1000)%100;
	return x>=1 && x<=9;
   }

/**
 * @english
 *   check if a given descriptor is a valid 
 *   within acceptable defined parameters
 *   of FXXYYY
 * @param desc
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup tables
 */
int bufr_is_descriptor( int desc )
   {
   int f, y;

   f = DESC_TO_F( desc );
   switch( f )
      {
      case 0 :
      case 1 :
      case 2 :
      case 3 :
      break;
      default :
         return 0;
      break;
      }
   y = DESC_TO_Y( desc );
   if ( y >= 256 ) return 0;

   return 1;
   }

#if DEBUG
static void test_print_tableD( char *tabled )
   {
   int           i, count;
   EntryTableD  *r, **pe;
   char          buf[128];

   count = arr_count(tabled);
   for ( i = 0 ; i < count ; i++ )
      {
      pe = (EntryTableD **)arr_get( tabled, i );
      r = pe ? *pe : NULL;
      if ( r != NULL)
         {
         sprintf( buf, _("TableD : %d\n"), r->descriptor );
         bufr_print_debug( buf );
         }
      }
   }
#endif

/**
 * @english
 *    bufr_table_is_empty( tbls )
 *    (BUFR_Tables *tbls)
 * verify if the BUFR Tables is not empty
 * @return bool
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @ingroup tables
 */
int bufr_table_is_empty( BUFR_Tables *tbls )
   {
   int has_tblB = 0;

   if (tbls->master.tableB)
      has_tblB = arr_count( tbls->master.tableB );
   if (has_tblB <= 0)
      {
      if (tbls->local.tableB)
         has_tblB = arr_count( tbls->local.tableB );
      }
   if (has_tblB > 0) 
      return 0;
   else 
      return 1;
   }
