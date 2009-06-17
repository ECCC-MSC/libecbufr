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
#include "private/bufr_util.h"
#include "bufr_io.h"
#include "bufr_ieee754.h"
#include "bufr_value.h"
#include "bufr_tables.h"
#include "bufr_sequence.h"

static int          bad_descriptor=0;
static BufrValueEncoding bufr_errtbe;
static int          bufr_errcode= BUFR_NOERROR;
static int          bufr_minimum_reference=0;
static int          bufr_minimum_nbits=0;


static int             bufr_load_tableB       ( BUFR_Tables *, BufrTablesSet *tbls, const char *filename, int local );
static int             bufr_load_tableD       ( BUFR_Tables *, BufrTablesSet *tbls, const char *filename );

static EntryTableD    *bufr_tabled_fetch_entry( EntryTableDArray addr_tabled, int code);

static EntryTableBArray        bufr_tableb_read       ( EntryTableBArray addr_tableb, const char *filename, int local,
                                                char *desc, int *cat, int *version );
static EntryTableDArray       bufr_tabled_read       ( EntryTableDArray addr_tabled, const char *filename);

static void            bufr_tabled_free       ( EntryTableDArray tabled );

static int             compare_tableb         ( const void *p1, const void *p2);
static int             compare_tabled         ( const void *p1, const void *p2);
static int             compare_tabled_seq     (const void *p1, const void *p2);

static int             bufr_check_code_tableD ( BUFR_Tables *tbls, int code , char *array );
static int             bufr_check_loop_tableD ( BUFR_Tables *tbls, BufrTablesSet *tbl );
static void            bufr_merge_tableB      ( EntryTableBArray table1, EntryTableBArray table2 );
static void            bufr_merge_tableD      ( EntryTableDArray table1, EntryTableDArray table2 );
static void            bufr_copy_EntryTableD  ( EntryTableD *r, const char *desc, int desclen,
                                                int *codes, int count);
static int             bufr_is_table_b        ( int code );
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
 * @param r a structure a detruire
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

   free( tbls );
   }

/**
 * bufr_set_tables_category
 * @english
 * set Tables category description
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: bufr_set_tables_category
 *
 * author:  Vanh Souvanlasy
 *
 * function: set Tables category description
 *
 * parameters:
 *
 *      tbls   : pointer to BUFR tables structure
 *      cat    : category number
 *      desc   : string describing the category (max 64 char.)

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
 * bufr_merge_TablesSet
 * @english
 * copy bufr tables from tbls2 into tbls1, merging and overwriting
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: bufr_merge_TablesSet
 *
 * author:  Vanh Souvanlasy
 *
 * function: copy bufr tables from tbls2 into tbls1, merging and overwriting
 *           duplicates
 *
 * parameters:
 *
 *      tbls1  : pointer to dest BufrTablesSet structure
 *      tbls2  : pointer to src  BufrTablesSet structure
 *

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
 * bufr_load_tableB
 * @english
 * load a table B file
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: bufr_load_tableB
 *
 * author:  Vanh Souvanlasy
 *
 * function: load a table B file
 *
 * parameters:
 *
 *        tbls      : the Set of tables to load into
 *        filename : string refering to table B
 *        local    : if local codes should be included

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
      }
   else
      {
      char *tableB;

      tableB = bufr_tableb_read( NULL, filename, local, data_cat_desc, 
                                 &data_cat, &version );
      bufr_merge_tableB( tbls->tableB, tableB );
      bufr_tableb_free( tableB );
      tbls->version = version;
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
 * bufr_load_tableD
 * @english
 * load table D from a file
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: bufr_load_tableD
 *
 * author:  Vanh Souvanlasy
 *
 * function: load table D from a file
 *
 * parameters:
 *
 *        tbls     : pointer to BUFR_Tables structure containing tables
 *        tbl      : the Set of tables to load into
 *        filename : string refering to table D

 */
static int bufr_load_tableD( BUFR_Tables *tbls, BufrTablesSet *tbl, const char *filename )
   {
   int  rtrn;

   tbl->tableDtype = TYPE_ALLOCATED;

   if (tbl->tableD == NULL)
      tbl->tableD = bufr_tabled_read( NULL, filename );
   else
      {
      EntryTableBArray tableD = bufr_tabled_read( NULL, filename );
      bufr_merge_tableD( tbl->tableD, tableD );
      bufr_tabled_free( tableD );
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
 * bufr_merge_tableD
 * @english
 * 
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: bufr_merge_tableD
 *
 * author:  Vanh Souvanlasy
 *
 * function:
 *
 * parameters:
 *

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
 * bufr_merge_tableB
 * @english
 * 
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: bufr_merge_tableB
 *
 * author:  Vanh Souvanlasy
 *
 * function:
 *
 * parameters:
 *

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
 * bufr_check_loop_tableD
 * @english
 * 
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: bufr_check_loop_tableD
 *
 * author:  Vanh Souvanlasy
 *
 * function:
 *
 * parameters:
 *

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
         if (bufr_check_code_tableD( tbls, etd->descriptors[i], array ) < 0)
            {
            has_error = -1;
            }
         }
      }

   arr_free( &array );
   return has_error;
   }

/**
 * bufr_check_code_tableD
 * @english
 * 
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: bufr_check_code_tableD
 *
 * author:  Vanh Souvanlasy
 *
 * function:
 *
 * parameters:
 *

 */
static int bufr_check_code_tableD( BUFR_Tables *tbls, int code , char *array )
   {
   int  f, x, y;
   EntryTableD  *etd;
   int  i;
   int  count;
   int  *pival, ival;
   char  errmsg[256];

   bufr_descriptor_to_fxy( code, &f, &x, &y );
   if (f == 3)
      {
      count = arr_count( array );
      for (i = 0; i < count ; i++ )
         {
         pival = (int *)arr_get( array, i );
         if (pival)
            {
            ival = *pival;
            if (ival == code)
               {
               sprintf( errmsg, "Warning: Table D code : %d is in a circular loop\n", code );
               bufr_print_debug( errmsg );
               return -1;
               }
            }
         }
      arr_add( array, (char *)&code );

      etd = bufr_fetch_tableD( tbls, code );
      if (etd == NULL)
         {
         sprintf( errmsg, "Warning: invalid Table D code : %d\n", code );
         bufr_print_debug( errmsg );
         return -1;
         }
      for (i = 0; i < etd->count ; i++ )
         {
         if (bufr_check_code_tableD( tbls, etd->descriptors[i], array ) < 0) return -1;
         }

      arr_del( array, 1 );
      }
   return 1;
   }

/**
 * bufr_fetch_tableB
 * @english
 * chercher et retourner une entree de la table B d'un code
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: bufr_fetch_tableB
 *
 * author:  Vanh Souvanlasy
 *
 * function: chercher et retourner une entree de la table B d'un code
 *
 * parameters:
 *        code : code a chercher

 */
EntryTableB *bufr_fetch_tableB(BUFR_Tables *tbls, int code)
   {
   int          f, x, y;
   EntryTableB *e;

	if( tbls == NULL ) return errno=EINVAL, NULL;

   bufr_descriptor_to_fxy( code, &f, &x, &y );
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
      e = bufr_tableb_fetch_entry( tbls->local.tableB, code );
   if (e == NULL)
      e = bufr_tableb_fetch_entry( tbls->master.tableB, code );
   if (e == NULL)
      {
      char buf[128];
      bufr_errcode = BUFR_TB_NOTFOUND;
      sprintf( buf, "Warning: Code BUFR unknown: %d\n", code );
      bufr_print_debug( buf );
      }
   return e;
   }

/**
 * bufr_fetch_tableD
 * @english
 * chercher et retourner une entree de la table D d'un code
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: bufr_fetch_tableD
 *
 * author:  Vanh Souvanlasy
 *
 * function: chercher et retourner une entree de la table D d'un code
 *
 * parameters:
 *        code : code a chercher

 */
EntryTableD *bufr_fetch_tableD(BUFR_Tables *tbls, int code)
   {
   EntryTableD *e=NULL;
   int   f, x, y;

	if( tbls == NULL ) return errno=EINVAL, NULL;

   bufr_descriptor_to_fxy( code, &f, &x, &y );
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
      e = bufr_tabled_fetch_entry( tbls->local.tableD, code );
   if (e == NULL)
      e = bufr_tabled_fetch_entry( tbls->master.tableD, code );
   if (e == NULL)
      {
      char buf[128];
      sprintf( buf, "Warning: Table D Code unknown: %d\n", code );
      bufr_print_debug( buf );
      }
   return e;
   }

/**
 * @english
 * @brief map a sequence of descriptors to a table D entry
 * @param tables tables to search
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
      bufr_print_debug( "Warning: Table D sequence not found\n" );
   return e;
   }

/**
 * strtlen
 * @english
 * find the length of a string without trailing white space
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Jean-Philippe Gauthier
 * @ingroup bufr_tables.c
  * name: strtlen
 *
 * author:  Jean-Philippe Gauthier
 *
 * function: find the length of a string without trailing white space
 *
 * parameters:
 *        Str : the string to evaluate

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
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: bufr_tableb_read
 *
 * author:  Vanh Souvanlasy
 *
 * changes:  
 *
 *    11/04/08 :  Jean-Pierre G. - truncate trailing white space from description
 *
 * function: read Table B from an ASCII file
 *
 * parameters:
 *      filename    : Table B file 
 *      addr_tableb : already allocated table b array to load in or merge with
 *      local       : if loading should restrict range to local descriptors only 
 *                    x=[1,47] y=[1,192]
 * return:
 *      desc        : string describing the local table in the file
 *      cat         : local table category description in the file
 *      version     : version of table B
 *      EntryTableBArray : A Table B array

 */
static EntryTableBArray bufr_tableb_read
   (EntryTableBArray addr_tableb, const char *filename, int local, 
    char *desc, int *cat, int *version)
   {
   FILE         *fp ;
   char          ligne[256] ;
   char          buf[256] ;
   EntryTableB  *etb;
   int           column[7], count;
   int           code;
   int           desclen, len;
   int           ver = -1;
   char          errmsg[256];

   if (filename == NULL) return NULL;
/*
 * on va lire la premiere ligne, car elle ne fait pas partie du fichier

*/

   fp = fopen ( filename, "r" ) ;
   if (fp == NULL)
      {
      sprintf( buf, "Warning: can't open Table B file %s\n", filename );
      bufr_print_debug( buf );
      return NULL;
      }

   if (addr_tableb == NULL)
      addr_tableb = (EntryTableBArray)arr_create( 100, sizeof(EntryTableB *), 100 );

   if ( fgets(ligne,256,fp) == NULL ) return NULL;
   count = bufr_parse_columns(ligne, column, 7);
   if (count < 6)
      {
      column[0] = 0; column[1] = 8; column[2] = 52;
      column[3] = 63; column[4] = 66; column[5] = 78;
      count = 6;
      }

   memset( ligne, (int)' ', 256 );
/**
 * on ne lit que les lignes ayant un descripteur commencant par 0
 * et ce, jusqu'au premier element local ou la fin du fichier.
 **/
   desclen = column[2] - column[1];
   while ( fgets(ligne,256,fp) != NULL )
      {
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
            strncpy ( desc,   &ligne[17],  len ) ;
         desc[len] = '\0';
         continue;
         }

      if ( ligne[0] == '#' ) continue ; /* comments */
      if ((ver < 0)&&(strncmp(ligne, "** VERSION", 10) == 0))
         ver = atoi( &ligne[11] ) ;
      if ( ligne[0] == '*' ) continue ; /* comments */

      if ( ligne[0] != '0' ) continue ; /* for table B, F=0  */


      code = atoi ( &ligne[0] ) ;

      if (!bufr_is_table_b( code )) continue;

      if (local == 1)
         {
         if ((count == 7)&&(ligne[column[6]] == '-')) continue;
         }
      else
         {
         if (bufr_is_local_descriptor( code )) continue;
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
         sprintf( errmsg, "Warning: error while loading Table B file: %s\n", 
            filename );
         bufr_print_debug( errmsg );
         sprintf( errmsg, "Error reading descriptor: %d unit=\"%s\"\n", 
                etb->descriptor, etb->unit );
         bufr_print_debug( errmsg );
         }

      arr_add( addr_tableb, (char *)&etb );
      }
   fclose ( fp ) ;

   *version = ver;
   return addr_tableb;
   }

/**
 * bufr_tableb_fetch_entry
 * @english
 * rechercher une entree dans la table B
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: bufr_tableb_fetch_entry
 *
 * author:  Vanh Souvanlasy
 *
 * function: rechercher une entree dans la table B
 *
 * parameters:
 *      addr_tabled  : une table B
 *      code : le code recherche

 */
EntryTableB *bufr_tableb_fetch_entry(EntryTableBArray addr, int code)
   {
   EntryTableB *ptr1, tb, **ptr;

   tb.descriptor = code;
   ptr1 = &tb;
   ptr = (EntryTableB **)arr_search( addr, (char *)&ptr1, compare_tableb );
   if ( ptr != NULL)
      return *ptr;
   else
      return NULL;
   }

/**
 * compare_tableb
 * @english
 * comparaison d'ordre des entrees de table B
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: compare_tableb
 *
 * author:  Vanh Souvanlasy
 *
 * function: comparaison d'ordre des entrees de table B
 *
 * parameters:
 *      p1, p2 : l'addresse des 2 entrees

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
 * bufr_tableb_fetch_entry_desc
 * @english
 * chercher et retourner une entree de la table B d'une description
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Jean-Philippe Gauthier
 * @ingroup bufr_tables.c
  * name: bufr_tableb_fetch_entry_desc
 *
 * author:  Jean-Philippe Gauthier
 *
 * function: chercher et retourner une entree de la table B d'une description
 *
 * parameters:
 *        addr_tabled  : une table B
 *        desc : Description a chercher

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
 * bufr_new_EntryTableB
 * @english
 * creer une nouvelle entree de table B
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: bufr_new_EntryTableB
 *
 * author:  Vanh Souvanlasy
 *
 * function: creer une nouvelle entree de table B
 *
 * parameters:

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
 * bufr_free_EntryTableB
 * @english
 * liberer une entree de table B
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: bufr_free_EntryTableB
 *
 * author:  Vanh Souvanlasy
 *
 * function: liberer une entree de table B
 *
 * parameters:

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
 * bufr_copy_EntryTableB
 * @english
 * creer une nouvelle entree de table B
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: bufr_copy_EntryTableB
 *
 * author:  Vanh Souvanlasy
 *
 * function: creer une nouvelle entree de table B
 *
 * parameters:

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
 * bufr_tableb_free
 * @english
 * detruire une table B
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: bufr_tableb_free
 *
 * author:  Vanh Souvanlasy
 *
 * function: detruire une table B
 *
 * parameters:
 *      tableb : la table a detruire

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
 * bufr_tabled_read
 * @english
 * lire une table D d'un fichier
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: bufr_tabled_read
 *
 * author:  Vanh Souvanlasy
 *
 * function: lire une table D d'un fichier
 *
 * parameters:
 *      filename : nom du fichier
 *      addr_tabled  : une table D a incrementer (s'il y a lieu)

 */
static EntryTableDArray bufr_tabled_read (EntryTableDArray addr_tabled, const char *filename)
   {
   int desclen;
   FILE *fp ;
   char ligne[4096] ;
   EntryTableD  *etb;
   int  count;
   int  codes[1024];
   char *tok;
   int  column[20];

   if (filename == NULL) return NULL;


   fp = fopen ( filename, "r" ) ;
   if (fp == NULL)
		{
      sprintf( ligne, "Warning: can't open Table D file %s\n", filename );
      bufr_print_debug( ligne );
      return NULL;
      }

   if (addr_tabled == NULL)
      addr_tabled = (EntryTableDArray)arr_create( 50, sizeof(EntryTableD *), 10 );

   desclen = 0;
   while ( fgets(ligne,4096,fp) != NULL )
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
         continue ;
         }

      if ( ligne[0] == '#' ) continue ; /* comments */
      if ( ligne[0] == '*' ) continue ; /* comments */

      if ( ligne[0] != '3' ) continue ; /* for table D , F=3 */

      tok = strtok( ligne+desclen, " \t\n" );
      if (tok == NULL) continue;

      if ( tok[0] != '3') continue;
      count = 0;
      while ( tok )
         {
         codes[count++] = atoi(tok);
         tok = strtok( NULL, " \t\n" );
         }
      if (count > 1)
         {
         etb = bufr_new_EntryTableD( codes[0], ligne, desclen, codes+1, count-1 );
         arr_add( addr_tabled, (char *)&etb );
         }
      }
   fclose ( fp ) ;

   return addr_tabled;
   }

/**
 * bufr_tabled_fetch_entry
 * @english
 * rechercher une entree dans la table D
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: bufr_tabled_fetch_entry
 *
 * author:  Vanh Souvanlasy
 *
 * function: rechercher une entree dans la table D
 *
 * parameters:
 *      addr_tabled  : une table D
 *      code : le code recherche

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
 * compare_tabled_seq
 * @english
 * compare the sequence of descriptors in two table D entries
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Christophe Beauregard
 * @ingroup bufr_tables.c
  * name: compare_tabled_seq
 *
 * author: Christophe Beauregard
 *
 * function: compare the sequence of descriptors in two table D entries
 *
 * parameters:
 *      p1, p2 : l'addresse des 2 entrees

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
 * compare_tabled
 * @english
 * comparaison d'ordre des entrees de table D
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: compare_tabled
 *
 * author:  Vanh Souvanlasy
 *
 * function: comparaison d'ordre des entrees de table D
 *
 * parameters:
 *      p1, p2 : l'addresse des 2 entrees

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
 * bufr_new_EntryTableD
 * @english
 * creer une nouvelle entree de table D
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: bufr_new_EntryTableD
 *
 * author:  Vanh Souvanlasy
 *
 * function: creer une nouvelle entree de table D
 *
 * parameters:
 *      codes : les codes a placer dans l'entree
 *      count : compte des codes

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
 * bufr_copy_EntryTableD
 * @english
 * detruire une table D
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: bufr_copy_EntryTableD
 *
 * author:  Vanh Souvanlasy
 *
 * function: detruire une table D
 *
 * parameters:
 *

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
 * bufr_tabled_free
 * @english
 * detruire une table D
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: bufr_tabled_free
 *
 * author:  Vanh Souvanlasy
 *
 * function: detruire une table D
 *
 * parameters:
 *      tabled : la table a detruire

 */
static void bufr_tabled_free(EntryTableDArray tabled)
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
         if (r->description != NULL)  free( r->description );
         if (r->descriptors != NULL)  free( r->descriptors );
         free( r );
         }
      }
   arr_free( &tabled );
   }

/**
 * bufr_parse_columns
 * @english
 * 
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: bufr_parse_columns
 *
 * author:  Vanh Souvanlasy
 *
 * function:
 *
 * parameters:

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
 * bufr_value_nbits
 * @english
 * identifie le nombre de bits minimum pour contenir une value
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: bufr_value_nbits
 *
 * author:  Vanh Souvanlasy
 *
 * function: identifie le nombre de bits minimum pour contenir une value
 *
 * parameters:
 *        ival : la valeur

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
 * bufr_leftest_bit
 * @english
 * identifie le bit le plus a gauche dans un mot
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: bufr_leftest_bit
 *
 * author:  Vanh Souvanlasy
 *
 * function: identifie le bit le plus a gauche dans un mot
 *
 * parameters:
 *        ival : la valeur

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
 * bufr_cvt_fval_to_i32
 * @english
 * convertir une valeur reel en entier avec l'entree
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: bufr_cvt_fval_to_i32
 *
 * author:  Vanh Souvanlasy
 *
 * function: convertir une valeur reel en entier avec l'entree
 *           de la table B
 *
 * parameters:
 *        e      : une entree de la table B
 *        fval   : la valeur a convertir
 *

 */
int32_t bufr_cvt_fval_to_i32(int code, BufrValueEncoding *be, float fval)
   {
   int64_t ival;
   uint64_t  maxval;
   float    val_pow;

   if (bufr_is_missing_float( fval )) return -1;

   maxval = (1ULL << be->nbits) - 1;

   val_pow = pow(10.0,(double)be->scale);

   ival = (int32_t)roundf(fval * val_pow);
   ival = ival - be->reference;
   if (ival < 0)
      {
      char buffer[128];
      int32_t  minval;
      minval = rint(fval * val_pow);
      bufr_minimum_reference = minval - 1;
      bufr_errtbe = *be;
      bad_descriptor = code;
      bufr_errcode = BUFR_TB_UNDERFLOW;
      sprintf( buffer, "Warning: UNDERFLOW with element %d : value = %e, giving %lld",
               code, fval, ival );
      bufr_print_debug( buffer );
      ival = -1;
      }
   if (ival >= maxval)
      {
      char buffer[128];

      bufr_minimum_nbits = bufr_value_nbits( ival );
      bufr_errtbe = *be;
      bad_descriptor = code;
      bufr_errcode = BUFR_TB_OVERFLOW;
      sprintf( buffer, "Warning: OVERFLOW with element %d (max=%lld) : value = %e, giving %lld",
               code, maxval, fval, ival );
      bufr_print_debug( buffer );
      ival = -1;
      }

   return ival;
   }

/**
 * bufr_cvt_i64_to_dval
 * @english
 * convertir un entier en valeur reel avec l'entree
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: bufr_cvt_i64_to_dval
 *
 * author:  Vanh Souvanlasy
 *
 * function: convertir un entier en valeur reel avec l'entree
 *           de la table B
 *
 * parameters:
 *        e      : une entree de la table B
 *        ival   : la valeur a convertir
 *

 */
double bufr_cvt_i64_to_dval(BufrValueEncoding *be, int64_t ival)
   {
   double fval;
   double val_pow;

   if (ival < 0) return bufr_get_max_double();

   val_pow = pow(10.0,(double)be->scale);

   fval = (ival + (double)be->reference) / val_pow ;

   return fval;
   }

/**
 * bufr_cvt_i32_to_fval
 * @english
 * convertir un entier en valeur reel avec l'entree
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: bufr_cvt_i32_to_fval
 *
 * author:  Vanh Souvanlasy
 *
 * function: convertir un entier en valeur reel avec l'entree
 *           de la table B
 *
 * parameters:
 *        e      : une entree de la table B
 *        ival   : la valeur a convertir
 *

 */
float bufr_cvt_i32_to_fval(BufrValueEncoding *be, int32_t ival)
   {
   float fval;
   float val_pow;

   if (ival < 0) return bufr_get_max_float();

   val_pow = pow(10.0,(double)be->scale);

   fval = (ival + (float)be->reference) / val_pow ;

   return fval;
   }

/**
 * bufr_cvt_dval_to_i64
 * @english
 * convertir une valeur reel en entier avec l'entree
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: bufr_cvt_dval_to_i64
 *
 * author:  Vanh Souvanlasy
 *
 * function: convertir une valeur reel en entier avec l'entree
 *           de la table B
 *
 * parameters:  
 *        e      : une entree de la table B
 *        fval   : la valeur a convertir
 *

 */
int64_t bufr_cvt_dval_to_i64(int code, BufrValueEncoding *be, double dval)
   {
   int64_t  ival;
   int64_t  maxval;
   float val_pow=1.0;

   if (bufr_is_missing_double( dval )) return -1;

   maxval = (1ULL << be->nbits) - 1;

   val_pow = pow(10.0,(double)be->scale);

   ival = (int64_t)round(dval * val_pow);
   ival = ival - be->reference;
   if (ival < 0) {
      char buffer[128];
      int64_t  minval;
      minval = dval * val_pow;
      bufr_minimum_reference = minval - 1;
      bufr_errtbe = *be;
      bad_descriptor = code;
      bufr_errcode = BUFR_TB_UNDERFLOW;
      sprintf( buffer, "Warning: UNDERFLOW with element %d : value = %f, giving %lld",
               code, dval, ival );
      bufr_print_debug( buffer );
      ival = -1;
      }
   if (ival >= maxval) {
      char buffer[128];

      bufr_minimum_nbits = bufr_value_nbits( ival );
      bufr_errtbe = *be;
      bad_descriptor = code;
      bufr_errcode = BUFR_TB_OVERFLOW;
      sprintf( buffer, "Warning: OVERFLOW with element %d (max=%lld) : value = %e, giving %lld",
               code, maxval, dval, ival );
      bufr_print_debug( buffer );
      ival = -1;
      }

   return ival;
   }

/**
 * bufr_get_tberror
 * @english
 * retourne les conditions d'erreur de table B
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: bufr_get_tberror
 *
 * author:  Vanh Souvanlasy
 *
 * function: retourne les conditions d'erreur de table B
 *
 * parameters:

 */
int bufr_get_tberror(BufrValueEncoding *be, int *reference, int *nbits)
   {
   *reference = bufr_minimum_reference;
   *nbits = bufr_minimum_nbits;
   *be = bufr_errtbe;

   return bad_descriptor;
   }

/**
 * bufr_errtype
 * @english
 * retourne le code d'erreur du logiciel
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: bufr_errtype
 *
 * author:  Vanh Souvanlasy
 *
 * function: retourne le code d'erreur du logiciel
 *
 * parameters:

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
 * @ingroup bufr_tables.c
  * name: bufr_unit_to_datatype
 *
 * author:  Vanh Souvanlasy
 *
 * function:
 *
 * parameters:
 *

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
 * bufr_descriptor_to_datatype
 * @english
 * 
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: bufr_descriptor_to_datatype
 *
 * author:  Vanh Souvanlasy
 *
 * function:
 *
 * parameters:

 */
BufrDataType bufr_descriptor_to_datatype   ( BUFR_Tables *tbls, EntryTableB *e, int code, int *len )
   {
   int f, x, y;
      
   if (e == NULL) 
      e = bufr_fetch_tableB( tbls, code );

   *len = 0;
   if (e)
      {
      if (e->encoding.type == TYPE_CCITT_IA5)
         *len = (e->encoding.nbits / 8);
      return e->encoding.type;
      }

   bufr_descriptor_to_fxy( code, &f, &x, &y );

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
      default :
         return TYPE_UNDEFINED;
      break;
      }
   }

/**
 * bufr_is_local_descriptor
 * @english
 * 
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: bufr_is_local_descriptor
 *
 * author:  Vanh Souvanlasy
 *
 * function:
 *
 * parameters:

 */
int bufr_is_local_descriptor( int code )
   {
   int f, x, y;

   bufr_descriptor_to_fxy( code, &f, &x, &y );
   if ( ( x > 47 ) || ( y > 191 ) ) return 1;

   return 0;
   }

/**
 * bufr_fxy_to_descriptor_i16
 * @english
 * encode F X Y into 16 bits code
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: bufr_fxy_to_descriptor_i16
 *
 * author:  Vanh Souvanlasy
 *
 * function: encode F X Y into 16 bits code
 *
 * parameters:
 *      f : partie f du code
 *      x : partie x du code
 *      y : partie y du code

 */
uint16_t bufr_fxy_to_descriptor_i16(int f,int x,int y)
   {
   uint16_t code;

   code = (((f)&3)<<14)|(((x)&63)<<8)|((y)&255) ;
   return code;
   }

/**
 * bufr_descriptor_i32_to_i16
 * @english
 * encoder code descripteur entiere en 2 octets
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: bufr_descriptor_i32_to_i16
 *
 * author:  Vanh Souvanlasy
 *
 * function: encoder code descripteur entiere en 2 octets
 *
 * parameters:
 *      dcode : code en entier

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
 * bufr_descriptor_to_fxy
 * @english
 * decoder code descripteur en F X Y
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: bufr_descriptor_to_fxy
 *
 * author:  Vanh Souvanlasy
 *
 * function: decoder code descripteur en F X Y
 *
 * parameters:
 *      code : code en entier
 *      f    : partie f du code
 *      x    : partie x du code
 *      y    : partie y du code

 */
void bufr_descriptor_to_fxy(int code, int *f, int *x, int *y)
   {
   *f = code / 100000;
   *x = (code / 1000)%100;
   *y = code % 1000;
   }

/**
 * bufr_fxy_to_descriptor
 * @english
 * encoder F X Y en code 2 octets
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: bufr_fxy_to_descriptor
 *
 * author:  Vanh Souvanlasy
 *
 * function: encoder F X Y en code 2 octets
 *
 * parameters:
 *      f : partie f du code
 *      x : partie x du code
 *      y : partie y du code

 */
int bufr_fxy_to_descriptor( int f, int x, int y )
   {
   int  code;

   code = f * 100000 + x * 1000 + y;
   return code;
   }

/**
 * bufr_is_table_b
 * @english
 * 
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup bufr_tables.c
  * name: bufr_is_table_b
 *
 * author:  Vanh Souvanlasy
 *
 * function:
 *
 * parameters:

 */
static int bufr_is_table_b( int code )
   {
   int f, x, y;

   bufr_descriptor_to_fxy( code, &f, &x, &y );
   if (f == 0) return 1;

   return 0;
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
         sprintf( buf, "TableD : %d\n", r->code );
         bufr_print_debug( buf );
         }
      }
   }
#endif
