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
***/

#define _DEFAULT_SOURCE
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <regex.h>
#include "bufr_tables.h"
#include "bufr_linklist.h"
#include "bufr_i18n.h"
#include "bufr_io.h"
#include "bufr_api.h"


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

   env = getenv( "WMO_BUFR_TABLES" );
   if (env != NULL)
      {
      if (bufr_load_wmo_tables( tables ) > 0) return 1;
      }

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
   char         errmsg[1024];
   int          rtrnB, rtrnD;
   BUFR_Tables *tables, *ltables;
   int          version;
   LinkedList  *list;
   int          i, tb;
   struct       stat buf;

   list = lst_newlist();

   env = getenv( "WMO_BUFR_TABLES" );
   if (env != NULL)
      {
      sprintf( filename, "%s/fromWeb", env );
      tb = bufr_load_wmo_tables_list ( list, filename );
      }
/*
 * even if we have all the tables from WMO, we still need
 * to load what is found under BUFR_TABLES (need version 13 for tests)
 */
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
      rtrnB = rtrnD = -1;
      sprintf( filename, "%s/table_b_bufr-%d", path, tb );
      if( stat(filename,&buf) == 0 )
         if ( !S_ISDIR( buf.st_mode ) )
            rtrnB = bufr_load_m_tableB( tables, filename );
      sprintf( filename, "%s/table_d_bufr-%d", path, tb );
      if( stat(filename,&buf) == 0 )
         if ( !S_ISDIR( buf.st_mode ) )
            rtrnD = bufr_load_m_tableD( tables, filename );
      ltables = bufr_use_tables_list( list, version );
      if (ltables && (tables->master.version == ltables->master.version))
         {
         if (bufr_is_debug())
	    {
            sprintf( errmsg, _("Info: Skipping %s, version %d already loaded\n"), filename, tables->master.version );
            bufr_print_debug( errmsg );
	    }
         bufr_free_tables( tables );
	 }
      else
         {
         if ((rtrnD >= 0) && (rtrnB >= 0 ))
            lst_addlast( list, lst_newnode( tables ) );
         else
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

/**
 * @english
 *    bufr_load_wmo_tables( file_tables )
 *    (BUFR_Tables *tables)
 * This will load the WMO Table B and D into the BUFR tables object
 * that is created by bufr_create_tables. Table B and D should be located
 * in the directory pointed by the environment variable "WMO_BUFR_TABLES"
 * and the tables should be in the CSV table format as provided by WMO
 * @return int
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup tables
 * @todo should AFSISIO be supported as an env variable?
 */
int bufr_load_wmo_tables( BUFR_Tables *tables )
   {
   char      *env, *str;
   char       filename[512];
   char       path[512];
   char       errmsg[512];

   int        rtrnB, rtrnD;
   char       pattern[256];
   char       string[256];
   regmatch_t pmatch[20];
   regex_t    preg;

   if (tables == NULL) return -1;

   tables->master.version = -1;
   env = getenv( "WMO_BUFR_TABLES" );
   if (env)
      {
      if (bufr_is_verbose()||bufr_is_debug())
         {
         sprintf( errmsg, _("Info: WMO_BUFR_TABLES defined as %s\n"), env );
         bufr_print_debug( errmsg );
	 }
      sprintf( path, "%s", env );
      str = strrchr( path, '/' );
      if (str == NULL)
         str = path;
      else
         str += 1;
      /*
       * downloaded WMO BUFR Tables zip file would have version number as suffix
       */
      strcpy( pattern, "BUFR([0-9]+)-([0-9]+)" );
      if (regcomp(  &preg, pattern, REG_EXTENDED ) != 0 )
         perror( "Failed to compile regular expression\n" );
      if (regexec( &preg, path, 3, pmatch, 0 ) == 0)
         {
         int  len;
         len = pmatch[2].rm_eo - pmatch[2].rm_so;
         strncpy( string, path+pmatch[2].rm_so, len );
         string[len] = '\0';
         tables->master.version = atol( string );
         }
      else
         {
         char    cmd[256], rtrn[256];
         struct  stat buf;
	 FILE   *fp;
	 int     cgitrepo;

      /*
       * For gitlab WMO BUFR Tables, could get version number from tag
       * but is there any other way?
       */
         rtrn[0] = '\0';
	 sprintf( filename, "%s/.git", env );
         cgitrepo = ((stat(filename,&buf)==0)&&(S_ISDIR( buf.st_mode )));
         if (cgitrepo&&(stat(env,&buf)==0)&&(S_ISDIR( buf.st_mode )))
	    {
	    strcpy( cmd, "cd $WMO_BUFR_TABLES ; git describe --abbrev=0 --tags" );
            if (bufr_is_verbose()||bufr_is_debug())
               {
               sprintf( errmsg, _("Trying to obtain BUFR Tables version by doing : %s\n"), cmd );
               bufr_print_debug( errmsg );
	       }
	    fp = popen( cmd, "r" );
	    if (fgets( rtrn, 256, fp ) > 0 ) 
	       {
               strcpy( pattern, "(V|v)([0-9]+)(.([0-9]+))?" );
               if (regcomp(  &preg, pattern, REG_EXTENDED ) != 0 )
                  perror( "Failed to compile regular expression\n" );
               if (regexec( &preg, rtrn, 4, pmatch, 0 ) == 0)
                  {
                  int  len;
                  len = pmatch[2].rm_eo - pmatch[2].rm_so;
                  strncpy( string, rtrn+pmatch[2].rm_so, len );
                  string[len] = '\0';
                  tables->master.version = atol( string );
                  }
	       }
	    pclose( fp );
	    }
         else
            {
            sprintf( errmsg, _("Error: defined WMO_BUFR_TABLES is not valid %s\n"), env );
            bufr_print_debug( errmsg );
	    return -1;
	    }
         }
      if (tables->master.version <= 0)
         {
         sprintf( errmsg, _("Error: failed to obtain tables version\n") );
         bufr_print_debug( errmsg );
	 return -1;
	 }
      if (bufr_is_verbose()||bufr_is_debug())
         {
         sprintf( errmsg, _("Info: WMO_BUFR_TABLES version is %d\n"), tables->master.version );
         bufr_print_debug( errmsg );
	 }
      }
   else
      {
      sprintf( errmsg, _("Warning: env.var. WMO_BUFR_TABLES not defined\n") );
      bufr_print_debug( errmsg );
      return -1;
      }

   sprintf( filename, "%s/txt/BUFRCREX_TableB_en.txt", path );

   rtrnB = bufr_load_csv_tableB( tables, filename );

   sprintf( filename, "%s/txt/BUFR_TableD_en.txt", path );
   rtrnD = bufr_load_csv_tableD( tables, filename );

   return ( (rtrnD >= 0) && (rtrnB >= 0 ));
   }

/**
 * @english
 *    bufr_load_wmo_tables_list ( path )
 *    (char *path)
 * This will load the WMO Table B and D of all version available
 * into the BUFR_tables objects stored in a list
 * Table B and D should be named as table_b-XX table_d-XX located
 * in the directory pointed by the environment variable "WMO_BUFR_TABLES"
 * and the tables should be in the CMC table format.
 * @return LinkedList
 * @endenglish
 * @francais
 * @todo translate to French
 * @endfrancais
 * @author Vanh Souvanlasy
 * @ingroup tables
 */
int bufr_load_wmo_tables_list ( LinkedList *list, char *path )
   {
   int          rtrnB, rtrnD;
   BUFR_Tables *tables;
   BUFR_Tables *tbls;
   int          version;
   int          i, nb;
   struct       stat buf;
   struct       dirent **namelist;
   char         errmsg[512];

   regex_t      preg;
   char         filename[1024];
   char         pattern[256];
   char         string[256];
   char         verdir[256];
   char         verstmp[256];
   regmatch_t   pmatch[20];
   int          len;

   namelist = NULL;
   nb = scandir( path, &namelist, NULL, alphasort );
   for ( i= nb-1 ; i >= 0 ; i-- )
      {
      strcpy( pattern, "BUFRCREX_([0-9]+)_([0-9]+)_([0-9]+)" );
      if (regcomp(  &preg, pattern, REG_EXTENDED ) != 0 )
         perror( "Failed to compile regular expression\n" );
      if (regexec( &preg, namelist[i]->d_name, 4, pmatch, 0 ) == 0)
         {
         strcpy( verdir, namelist[i]->d_name );
         len = pmatch[1].rm_eo - pmatch[1].rm_so;
         strcpy( verstmp, verdir+pmatch[1].rm_so );
         strncpy( string, verdir+pmatch[1].rm_so, len );
         string[len] = '\0';
         version = atol( string );
	 tables = bufr_use_tables_list( list, version );
	 if (tables && (tables->master.version == version))
            {
            if (bufr_is_debug())
	       {
               sprintf( errmsg, _("Info: Skipping %s, version %d already loaded\n"), verdir, version );
               bufr_print_debug( errmsg );
	       }
	    continue;
	    }
	 else 
	    {
            tables = bufr_create_tables();
            tables->master.version = version;
            sprintf( filename, "%s/%s/BUFRCREX_%s_TableB_en.txt", path, verdir, verstmp );
            rtrnB = bufr_load_csv_tableB( tables, filename );

            sprintf( filename, "%s/%s/BUFR_%s_TableD_en.txt", path, verdir, verstmp );
            rtrnD = bufr_load_csv_tableD( tables, filename );

            if ((rtrnD >= 0) && (rtrnB >= 0 ))
               {
               if (bufr_is_debug())
	          {
                  sprintf( errmsg, _("Info: Adding Tables from %s, version %d\n"), verdir, version );
                  bufr_print_debug( errmsg );
		  }
               lst_addlast( list, lst_newnode( tables ) );
	       }
	    else
               {
               if (bufr_is_debug())
	          {
                  sprintf( errmsg, _("Warning: Rejected Tables from %s\n"), verdir );
                  bufr_print_debug( errmsg );
		  }
               bufr_free_tables( tables );
	       }
	    }
         }
      free(namelist[i]);
      }
   if (namelist)
      free(namelist);
   }
