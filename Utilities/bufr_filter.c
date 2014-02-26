/**
@example bufr_filter.c
@english
filter a file containing 1 or several BUFR messages with subsets which match specified search key(s).
This generates multiple OUTPUT-n.bufr files

@endenglish
@francais
@endfrancais
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bufr_api.h"
#include "bufr_io.h"
#include <locale.h>
#include "bufr_i18n.h"
#include "gettext.h"
#include "bufr_util.h"

#define   EXIT_ERROR    5

static  char *str_ltableb = NULL;
static  char *str_ltabled = NULL;
static  char *str_ibufr   = NULL;
static  char *str_obufr   = NULL;

static  BufrDescValue srchkey[100];
static  int           nb_key=0;
static  int           use_compress=0;


static int  read_cmdline( int argc, const char *argv[] );
static void abort_usage(const char *pgrmname);
static int  filter_file (BufrDescValue *dvalues, int nbdv);
static int  match_search_pattern( DataSubset *subset, BufrDescValue *dvalues, int nb );
static int  resolve_search_values( BufrDescValue *dvalues, int nb, BUFR_Tables *tbls );


/*
 * nom: abort_usage
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: informer la facon d'utiliser le programme
 *
 * parametres:  
 *        pgrmname  : nom du programme
 */
static void abort_usage(const char *pgrmname)
{
   fprintf( stderr, _("BUFR Filter Version %s\n"), BUFR_API_VERSION );
   fprintf( stderr, _("Copyright Her Majesty The Queen in Right of Canada, Environment Canada, 2009\n") );
   fprintf( stderr, _("Licence LGPLv3\n\n") );
   fprintf( stderr, _("Usage: %s\n"), pgrmname );
   fprintf( stderr, _("          [-inbufr     <filename>]    input BUFR file to decode\n") );
   fprintf( stderr, _("          [-outbufr    <filename>]    outut BUFR file to encode\n") );
   fprintf( stderr, _("          [-ltableb    <filename>]    local table B to use for decoding\n") );
   fprintf( stderr, _("          [-ltabled    <filename>]    local table D to use for decoding\n") );
   fprintf( stderr, _("          [-compress] compress datasubsets if possible\n") );
   fprintf( stderr, _("          [-srchkey descriptor value] descriptor value pair(s) search key\n") );
   exit(EXIT_ERROR);
}

/*
 * nom: read_cmdline
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: lire la ligne de commande pour extraire les options
 *
 * parametres:  
 *        argc, argv
 */
static int read_cmdline( int argc, const char *argv[] )
{
   int i;

   for ( i = 1 ; i < argc ; i++ ) 
     {
     if (strcmp(argv[i],"-ltableb")==0) 
        {
        ++i; if (i >= argc) abort_usage(argv[0]);
        str_ltableb = strdup(argv[i]);
        } 
     else if (strcmp(argv[i],"-ltabled")==0)
        {
        ++i; if (i >= argc) abort_usage(argv[0]);
        str_ltabled = strdup(argv[i]);
        } 
     else if (strcmp(argv[i],"-inbufr")==0) 
        {
        ++i; if (i >= argc) abort_usage(argv[0]);
        str_ibufr = strdup(argv[i]);
        }
     else if (strcmp(argv[i],"-outbufr")==0) 
        {
        ++i; if (i >= argc) abort_usage(argv[0]);
        str_obufr = strdup(argv[i]);
        } 
     else if (strcmp(argv[i],"-compress")==0)
       {
       use_compress = 1;
       }
     else if (strcmp(argv[i],"-srchkey")==0)
       {
       int desc;
       ++i; if (i >= argc) abort_usage(argv[0]);
       bufr_init_DescValue( &(srchkey[nb_key]) );
       desc = atoi( argv[i] );
       ++i; if (i >= argc) abort_usage(argv[0]);
       bufr_set_key_string( &(srchkey[nb_key]), desc, &(argv[i]), 1 );
       ++nb_key; 
       }
   }

   if (str_obufr == NULL) str_obufr = strdup( "OUT.BUFR" );

   return 0;
}

/*
 * nom: cleanup
 *
 * auteur:  Vanh Souvanlasy
 *
 * fonction: nettoyer les allocations du programme
 *
 * parametres:  
 */
static void cleanup(void)
   {
   int i;

   if (str_ltableb != NULL) free(str_ltableb);
   if (str_ltabled != NULL) free(str_ltabled);
   if (str_ibufr!= NULL) free(str_ibufr);
   if (str_obufr!= NULL) free(str_obufr);

   for ( i = 0; i < nb_key ; i++ )
      bufr_vfree_DescValue( &(srchkey[i]) );
   }


int main(int argc, const char *argv[])
   {
   read_cmdline( argc, argv );

   //Setup for internationalization
   bufr_begin_api();
   setlocale (LC_ALL, "");
   bindtextdomain ("bufr_codec", LOCALEDIR);
   textdomain ("bufr_codec");

   if (argc == 1)
      abort_usage( argv[0] );

   filter_file( srchkey, nb_key );
   cleanup();
   }

static int filter_file (BufrDescValue *dvalues, int nbdv)
   {
   BUFR_Dataset  *dts, *dts2;
   int           sscount;
   FILE          *fp, *fpO;
   char           buf[256];
   BUFR_Message  *msg;
   int            rtrn;
   int            count;
   int            pos;
   char           filename[512];
   char           prefix[512], *str;
   BUFR_Tables   *file_tables=NULL;
   BUFR_Tables   *useTables=NULL;
   LinkedList    *tables_list=NULL;
   int            tablenos[2];
   BUFR_Template *tmplt=NULL;
   DataSubset    *subset;
   int            i;
/*
 * load CMC Table B and D, currently version 14
 */
   file_tables = bufr_create_tables();
   bufr_load_cmc_tables( file_tables );

/*
 * load all tables into list
 */
   tablenos[0] = 13;
   tables_list = bufr_load_tables_list( getenv("BUFR_TABLES"), tablenos, 1 );
/* 
 * add version 14 to list 
 */
   lst_addfirst( tables_list, lst_newnode( file_tables ) );
/*
 * load local tables if any to list of tables
 */
   bufr_tables_list_addlocal( tables_list, str_ltableb, str_ltabled );
/*
 * open a file for reading
 */
   fp = fopen( str_ibufr, "rb" );
   if (fp == NULL)
      {
      fprintf( stderr, "Error: can't open file \"%s\"\n", str_ibufr );
      exit(-1);
      }

   fpO = fopen( str_obufr, "wb" );
   if (fpO == NULL)
      {
      fprintf( stderr, "Error: can't open file \"%s\"\n", str_obufr );
      exit(-1);
      }

   resolve_search_values( srchkey, nb_key, file_tables );
/*
 * read all messages from the input file
 */
   count = 0;
/*
 * a new instance of BUFR_Message is assigned to msg at each invocation
 */
   while ( (rtrn = bufr_read_message( fp, &msg )) > 0 )
      {
      ++count;
/*
 * fallback on default Tables first
 */
      useTables = file_tables;
/* 
 * try to find another if not compatible
 */
      if (useTables->master.version != msg->s1.master_table_version)
         useTables = bufr_use_tables_list( tables_list, msg->s1.master_table_version );

      if (useTables != NULL)
         dts = bufr_decode_message( msg, useTables );
      else 
         dts = NULL;

      if (dts != NULL)
         {
         tmplt = bufr_get_dataset_template( dts );
         dts2 = bufr_create_dataset( tmplt );
/*
 * transfer section 1 and flag from origin message
 */
         bufr_copy_sect1( &(dts2->s1), &(msg->s1) );
         dts2->data_flag |= msg->s3.flag;
         
         sscount = bufr_count_datasubset( dts );
         for (i = 0; i < sscount ; i++)
            {
            subset = bufr_get_datasubset( dts, i );
            if (match_search_pattern( subset, dvalues, nbdv ))
               {
               pos = bufr_create_datasubset( dts2 );
               bufr_merge_dataset( dts2, pos, dts, i, 1 );
               }
            }
/*
 * write the content of the Message into a file
 */
         if (bufr_count_datasubset( dts2 ) > 0)
            {
            bufr_free_message( msg );
            msg = bufr_encode_message ( dts2, use_compress );
            bufr_write_message( fpO, msg ); 
            }

         bufr_free_dataset( dts );
         bufr_free_dataset( dts2 );
         }
      
/*
 * discard the message
 */
      bufr_free_message( msg );
      }
/*
 * close all file and cleanup
 */
   fclose( fp );
   fclose( fpO );
   }

static int resolve_search_values( BufrDescValue *dvalues, int nb, BUFR_Tables *tbls )
   {
   int i;
   BufrDescValue *dv;
   EntryTableB   *e;
   char          *str = NULL, *ptr, *tok;
   ArrayPtr       iarr, farr;
   ValueType      vtype;
   int            len;
   long           ival;
   float          fval;

   
   if (nb <= 0) return 0;

   iarr = arr_create( 100, sizeof(int), 100 );
   farr = arr_create( 100, sizeof(float), 100 );
/*
 * convert search key type based descriptor
 */
   for (i = 0; i < nb ; i++)
      {
      dv = &(dvalues[i]);
      e = bufr_fetch_tableB( tbls, dv->descriptor );
      if (e != NULL)
         {
         vtype = bufr_encoding_to_valtype( &(e->encoding) );
         switch( vtype )
            {
            case VALTYPE_STRING : /* already a string */
            break;
            case VALTYPE_INT64 :
            case VALTYPE_INT32  :
               ptr = str = strdup( bufr_value_get_string( dv->values[0], &len ) );
               arr_del ( iarr, arr_count( iarr ) );
               tok = strtok_r( NULL, "\t\n,=", &ptr );
               while ( tok )
                  {
                  ival = atol( tok );
                  arr_add( iarr, (char *)&ival );
                  tok = strtok_r( NULL, "\t\n,", &ptr );
                  }
               bufr_set_key_int32( dv, dv->descriptor, arr_get( iarr, 0 ), arr_count( iarr ) );
               
               free( str );
            break;
            case VALTYPE_FLT64  :
            case VALTYPE_FLT32  :
               ptr = str = strdup( bufr_value_get_string( dv->values[0], &len ) );
               arr_del ( farr, arr_count( farr ) );
               tok = strtok_r( NULL, "\t\n,=", &ptr );
               while ( tok )
                  {
                  fval = atof( tok );
                  arr_add( farr, (char *)&fval );
                  tok = strtok_r( NULL, "\t\n,", &ptr );
                  }
               bufr_set_key_flt32( dv, dv->descriptor, arr_get( farr, 0 ), arr_count( farr ) );
               free( str );
            break;
            default :
            break;
            }
         }
      }
   arr_free( &iarr );
   arr_free( &farr );

   return nb;
   }

static int match_search_pattern ( DataSubset *subset, BufrDescValue *dvalues, int nb )
   {
   int pos;

   if (nb <= 0) return 1;
   pos = bufr_subset_find_values ( subset, dvalues, nb, 0 );
   return ((pos >= 0) ? 1 : 0);
   }
