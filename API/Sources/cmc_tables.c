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
***/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "bufr_tables.h"
#include "bufr_linklist.h"
#include "bufr_i18n.h"

/**
 * @english
 *    bufr_load_cmc_tables( file_tables )
 *    (BUFR_Tables *tables)
 * This will load the CMC master Table B and D into the BUFR tables object
 * that is created by bufr_create_tables. Table B and D should be located
 * in the directory pointed by the environment variable "BUFR_TABLES"
 * and the tables should be in the CMC table format.
 * @warning This filename format is only related to the function
 * bufr_load_cmc_tables. This also uses an environment variable
 * "AFSISIO" which points to an operational directory that contains a
 * subdirectory "datafiles/constants" (caution: will not work in
 * Windows) beneath it where the Table D and Table B files reside. Caution:
 * The names of the tables should be in the format "table_b_bufr" and
 * "table_d_bufr". This does not support the current WinIDE, Met
 * manager, Codecon, NCP practice of TABB003.037 type names that allow for
 * concurrent versions to be in the directory that are versioned and also
 * permit the use of the DOS 8.3 filename format.
 * @return int
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup tables
 * @todo should AFSISIO be supported as an env variable?
 */
int bufr_load_cmc_tables( BUFR_Tables *tables )
   {
   char *env;
   char  filename[512];
   char  path[512];
   int   rtrnB, rtrnD;

   if (tables == NULL) return -1;

   env = getenv( "AFSISIO" );
   if (env) 
      {
      sprintf( path, "%s/datafiles/constants", env );
      }
   else 
      {
      env = getenv( "BUFR_TABLES" );
      if (env) 
         sprintf( path, "%s", env );
      else
         {
         char errmsg[256];

         sprintf( errmsg, _("Warning: env.var. BUFR_TABLES not defined\n") );
         bufr_print_output( errmsg );
         bufr_print_debug( errmsg );
         }
      }

   if (env == NULL) return -1;

   sprintf( filename, "%s/table_b_bufr", path );

   rtrnB = bufr_load_m_tableB( tables, filename );

   sprintf( filename, "%s/table_d_bufr", path );
   rtrnD = bufr_load_m_tableD( tables, filename );

   return ( (rtrnD >= 0) && (rtrnB >= 0 ));
   }

/**
 * @english
 *    bufr_load_tables_list ( path )
 *    (char *path)
 * This will load the CMC master Table B and D of version 13 and 14 
 * into the BUFR_tables objects stored in a list
 * Table B and D should be named as table_b-XX table_d-XX located
 * in the directory pointed by the environment variable "BUFR_TABLES"
 * and the tables should be in the CMC table format.
 * @return LinkedList
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup tables
 */
LinkedList *bufr_load_tables_list ( char *path, int tbnos[], int nb )
   {
   char        *env;
   char         filename[512];
   int          rtrnB, rtrnD;
   BUFR_Tables *tables;
   int          version;
   LinkedList  *list;
   int          i, tb;
   struct       stat buf;

   list = lst_newlist();

   if (path == NULL)
      {
      env = getenv( "BUFR_TABLES" );
      if (env == NULL) return list;
      path = env;
      }

   for (i = 0; i < nb ; i++)
      {
      tb = tbnos[i];
      tables = bufr_create_tables();
      sprintf( filename, "%s/table_b_bufr-%d", path, tb );
      if( stat(filename,&buf) == 0 )
         if ( !S_ISDIR( buf.st_mode ) )
            rtrnB = bufr_load_m_tableB( tables, filename );
      sprintf( filename, "%s/table_d_bufr-%d", path, tb );
      if( stat(filename,&buf) == 0 )
         if ( !S_ISDIR( buf.st_mode ) )
            rtrnD = bufr_load_m_tableD( tables, filename );
      if (arr_count(tables->master.tableB) > 0)
         {
         lst_addlast( list, lst_newnode( tables ) );
         }
      else
         {
         bufr_free_tables( tables );
         }
      }

   return list;
   }

/**
 * @english
 *    bufr_use_tables_list( list , version )
 *    (LinkedList *list, int version )
 * Find a table B of the same or compatible version in the list 
 * @return BUFR_Tables
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup tables
 */
BUFR_Tables *bufr_use_tables_list( LinkedList *list, int version )
   {
   ListNode  *node;
   BUFR_Tables  *tbls;
   BUFR_Tables  *btn = NULL;
   BUFR_Tables  *ltn = NULL;
   int           lv, bv;

   node = lst_firstnode( list );
   while ( node )
      {
      tbls = (BUFR_Tables *)node->data;
      if (tbls->master.version == version) return tbls;
      if (tbls->master.version > version)
         {
         if (btn == NULL)  
            {
            bv = tbls->master.version;
            btn = tbls;
            }
/* 
 * use the latest version 
 */
         else if (bv < tbls->master.version) 
            {
            bv = tbls->master.version;
            btn = tbls;
            }
         }
      else if ( ltn == NULL )
         {
         lv = tbls->master.version;
         ltn = tbls;
         }
      else if (lv < tbls->master.version)
         {
         lv = tbls->master.version;
         ltn = tbls;
         }
      node = node->next;
      }
/*
 * use latest version if no better version can be found
 */
   if (btn == NULL) btn = ltn;
   return btn;
   }

/**
 * @english
 *    bufr_free_tables_list( list )
 *    ( LinkedList *list )
 * destroy a linkedlist of BUFR_Tables 
 * @return None
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup tables
 */
void bufr_free_tables_list( LinkedList *list )
   {
   ListNode  *node;
   BUFR_Tables  *tbls;
   node = lst_firstnode( list );
   while ( node )
      {
      tbls = (BUFR_Tables *)node->data;
      bufr_free_tables( tbls );
      node = node->next;
      }

   lst_dellist( list );
   }

/**
 * @english
 *    bufr_tables_list_addlocal ( list , tableb_fn, tabled_fn )
 *    (LinkedList *list, char *tableb_fn, char *tabled_fn )
 * augment a list of tables with local table b and d from text file
 * @return None
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup tables
 */
void bufr_tables_list_addlocal
   ( LinkedList *tables_list, char *tableb_fn, char *tabled_fn )
   {
   ListNode  *node;
   BUFR_Tables *tbl;

   node = lst_firstnode( tables_list );
   while ( node )
      {
      tbl = (BUFR_Tables *)node->data;
      if (tableb_fn)
         bufr_load_l_tableB( tbl, tableb_fn );
      if (tabled_fn)
         bufr_load_l_tableD( tbl, tabled_fn );
      node = lst_nextnode( node );
      }
   }

/**
 * @english
 *    bufr_tables_list_merge( list , BUFR_Tables *tblm )
 *    (LinkedList *list, BUFR_Tables *tblm )
 * merge BUFR tables to a list of tables 
 * @return None
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup tables
 */
void bufr_tables_list_merge
   ( LinkedList *tables_list, BUFR_Tables *tblm )
   {
   ListNode  *node;
   BUFR_Tables *tbl;

   node = lst_firstnode( tables_list );
   while ( node )
      {
      tbl = (BUFR_Tables *)node->data;
      bufr_merge_tables( tbl, tblm );
      node = lst_nextnode( node );
      }
   }

