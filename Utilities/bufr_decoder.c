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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "bufr_api.h"
#include "bufr_io.h"
#include "bufr_tables.h"
#include "bufr_dataset.h"
#include "bufr_local.h"

#define   EXIT_ERROR    5


static int   save_local_table_B = 0;


static  char *str_ltableb = NULL;
static  char *str_ltabled = NULL;
static  char *str_ibufr = NULL;
static  char *str_output= NULL;
static  char *str_debug = "DEBUG.decoder";
static  char *str_template= NULL;

static int   stop_count=0;
static int   dumpmode=0;
static int   show_unitdesc=0;
static int   show_loctime=0;
static int   show_meta=1;
static int   show_locdesc=0;
static int   format_ouput=1;
/*
 * data structures
 */

static void abort_usage(char *pgrmname);
static void cleanup(void);
static int  read_cmdline( int argc, char *argv[] );
static void bufr_show_dataset( BUFR_Dataset *dts, BUFR_Tables * );
static void bufr_show_dataset_formatted( BUFR_Dataset *dts, BUFR_Tables * );

static void run_tests(void);

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
static void abort_usage(char *pgrmname)
{
   fprintf( stderr, "BUFR Decoder Version %s\n", BUFR_API_VERSION );
   fprintf( stderr, "Copyright Her Majesty The Queen in Right of Canada, Environment Canada, 2009\n" );
   fprintf( stderr, "Licence LGPLv3\n\n" );
   fprintf( stderr, "Usage: %s\n", pgrmname );
   fprintf( stderr, "          [-inbufr     <filename>]    BUFR file to decode\n" );
   fprintf( stderr, "          [-ltableb    <filename>]    local table B to use for decoding\n" );
   fprintf( stderr, "          [-ltabled    <filename>]    local table D to use for decoding\n" );
   fprintf( stderr, "          [-output     <filename>]    output file\n" );
   fprintf( stderr, "          [-otemplate  <filename>]    output template into file\n" );
   fprintf( stderr, "          [-debug]                    debug mode (put the messages into file) \n" );
   fprintf( stderr, "          [-nometa]                   dont show meta info\n" );
   fprintf( stderr, "          [-dump]                     dump template and data in ASCII file suitable for re-encoding\n" );
   fprintf( stderr, "          [-describe]                 show description and unit\n" );
   fprintf( stderr, "          [-location]                 show implicit location or time\n" );
   fprintf( stderr, "          [-locdesc    <descriptor>]  show RTMD of the location descriptors\n" );
   fprintf( stderr, "          [-no_format]                disable formatting\n" );
   fprintf( stderr, "          [-verbose]                  send more messages\n" );
   fprintf( stderr, "          [-local]                    save the local table B\n" );
   fprintf( stderr, "          [-stop       <nb_messages>] stops decoding after the specified number of messages\n" );
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
static int read_cmdline( int argc, char *argv[] )
{
   int i;

   for ( i = 1 ; i < argc ; i++ ) {
     if (strcmp(argv[i],"-ltableb")==0) {
       ++i; if (i >= argc) abort_usage(argv[0]);
       str_ltableb = strdup(argv[i]);
     } else if (strcmp(argv[i],"-ltabled")==0) {
       ++i; if (i >= argc) abort_usage(argv[0]);
       str_ltabled = strdup(argv[i]);
     } else if (strcmp(argv[i],"-inbufr")==0) {
       ++i; if (i >= argc) abort_usage(argv[0]);
       str_ibufr = strdup(argv[i]);
     } else if (strcmp(argv[i],"-output")==0) {
       ++i; if (i >= argc) abort_usage(argv[0]);
       str_output = strdup(argv[i]);
     } else if (strcmp(argv[i],"-otemplate")==0) {
       ++i; if (i >= argc) abort_usage(argv[0]);
       str_template = strdup(argv[i]);
     } else if (strcmp(argv[i],"-verbose")==0) {
       bufr_set_verbose( 1 );
     } else if (strcmp(argv[i],"-debug")==0) {
       bufr_set_debug( 1 );
     } else if (strcmp(argv[i],"-dump")==0) {
       dumpmode = 1;
     } else if (strcmp(argv[i],"-stop")==0) {
       ++i; if (i >= argc) abort_usage(argv[0]);
       stop_count = atoi(argv[i]);
     } else if (strcmp(argv[i],"-local")==0) {
       save_local_table_B = 1;
     } else if (strcmp(argv[i],"-describe")==0) {
       show_unitdesc = 1;
     } else if (strcmp(argv[i],"-location")==0) {
       show_loctime = 1;
     } else if (strcmp(argv[i],"-nometa")==0) {
       show_meta = 0;
     } else if (strcmp(argv[i],"-no_format")==0) {
       format_ouput = 0;
     } else if (strcmp(argv[i],"-locdesc")==0) {
       ++i; if (i >= argc) abort_usage(argv[0]);
       show_locdesc = atoi(argv[i]);
     }
   }

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

   if (str_ltableb != NULL) free(str_ltableb);
   if (str_ltabled != NULL) free(str_ltabled);
   if (str_ibufr!= NULL) free(str_ibufr);
   if (str_output != NULL) free(str_output);

}

/*
 *##############################################################################
 * programme principale
 *
 */
int main(int argc,char *argv[])
{
   
   if (argc == 1)
      abort_usage( argv[0] );

   read_cmdline( argc, argv );

   bufr_set_debug_file( str_debug );
   if (str_output && !dumpmode)
      bufr_set_output_file( str_output );
/*
 * charger les tables en memoire
 */
   run_tests();

   cleanup();
   exit(0);
}

static void run_tests(void)
   {
   BUFR_Dataset   *dts;
   FILE         *fpBufr;
   BUFR_Tables   *file_tables=NULL;
   char           buf[256];
   BUFR_Message  *msg;
   int            rtrn;
   int            count;
   FILE          *fp = NULL;

   if (str_ibufr == NULL)
      {
      fprintf( stderr, "Warning: No input file\n" );
      abort_usage( "bufr_decoder" );
      exit(EXIT_ERROR);
      }

   if (bufr_is_verbose())
      {
      char errmsg[256];

      bufr_print_debug( "*\n" );
      sprintf( errmsg, "* BUFR decoder : API version %s\n", BUFR_API_VERSION );
      bufr_print_debug( errmsg );
      bufr_print_debug( "*\n" );
      }

/*
 * load CMC Table B and D
 * includes local descriptors
 */
   file_tables = bufr_create_tables();
   bufr_load_cmc_tables( file_tables );  
/*
 * load local tables if any 
 */
   if (str_ltableb)
      bufr_load_l_tableB( file_tables, str_ltableb );
   if (str_ltabled)
      bufr_load_l_tableD( file_tables, str_ltabled );
/*
 * open a file for reading
 */
   fpBufr = fopen( str_ibufr, "r" );
   if (fpBufr == NULL)
      {
      bufr_free_tables( file_tables );
      sprintf( buf, "Error: can't open file \"%s\"\n", str_ibufr );
      bufr_print_debug( buf );
      exit(EXIT_ERROR);
      }

   if (str_output && dumpmode)
      {
      fp = fopen( str_output, "w" );
      if (fp == NULL)
         {
         sprintf( buf, "Error: can't open file \"%s\"\n", str_output );
         bufr_print_debug( buf );
         exit(EXIT_ERROR);
         }
      }
   else
      {
      fp = stdout;
      }

   count = 0;
   while ( (rtrn = bufr_read_message( fpBufr, &msg )) > 0 )
      {
      ++count;
      if (!dumpmode)
         bufr_print_message( msg, bufr_print_output );
/* 
 * BUFR_Message ==> BUFR_Dataset 
 */

      dts = bufr_decode_message( msg, file_tables ); 

      if (dts == NULL) 
         {
         strcpy( buf, "Error: can't decode messages\n" );
         bufr_print_output( buf );
         bufr_print_debug( buf );
         continue;
         }
/*
 * see if the Message contains Local Table Update
 */
      if (bufr_contains_tables( dts ))
         {
         BUFR_Tables   *tbls;

         tbls = bufr_extract_tables( dts );
         if (tbls)
            {
            bufr_merge_tables( file_tables, tbls );
            bufr_free_tables( tbls );
            }
         if (dumpmode)
            {
            bufr_free_dataset( dts );
            dts = NULL;
            continue;
            }
         }

      if (dumpmode && fp)
         {
         if (str_template)
            {
            bufr_save_template( str_template, bufr_get_dataset_template(dts) );
            }
         bufr_fdump_dataset( dts, fp );
         }
      else if (format_ouput)
         bufr_show_dataset_formatted( dts, file_tables );
      else
         bufr_show_dataset( dts, file_tables );

      bufr_free_dataset( dts );
      bufr_free_message( msg );
      if (count == stop_count) break;
      }
/*
 * close all file and cleanup
 */
   fclose( fpBufr );
   if (fp) fclose( fp );
   bufr_free_tables( file_tables );
   }

static void bufr_show_dataset( BUFR_Dataset *dts, BUFR_Tables *tables )
   {
   DataSubset    *subset;
   int            sscount, cvcount;
   int            i, j;
   BufrDescriptor      *bcv;
   char           buf[256];

   sscount = bufr_count_datasubset( dts );
   for (i = 0; i < sscount ; i++)
      {
      subset = bufr_get_datasubset( dts, i );
      cvcount = bufr_datasubset_count_descriptor( subset );

      sprintf( buf, "## Subset %d : %d descriptors\n", i+1, cvcount );
      bufr_print_output( buf );

      for (j = 0; j < cvcount ; j++)
         {
         bcv = bufr_datasubset_get_descriptor( subset, j );
         if (bcv->flags & FLAG_SKIPPED)
            {
            sprintf( buf, "#  %.6d ", bcv->descriptor );
            bufr_print_output( buf );
            if ( bcv->meta && show_meta )
               {
               if (show_locdesc)
                  {
                  bufr_print_rtmd_repl( buf, bcv->meta );
                  bufr_print_rtmd_location( buf, show_locdesc, bcv->meta );
                  }
               else if (show_loctime)
                  bufr_print_rtmd_data( buf, bcv->meta );
               else
                  bufr_print_rtmd_repl( buf, bcv->meta );
               bufr_print_output( buf );
               }
            }
         else
            {
            sprintf( buf, "   %.6d ", bcv->descriptor );
            bufr_print_output( buf );

            if ( bcv->s_descriptor != 0 )
               {
               sprintf( buf, "{%.6d} ", bcv->s_descriptor );
               bufr_print_output( buf );
               }
            if ( bcv->meta && show_meta )
               {
               if (show_locdesc)
                  {
                  bufr_print_rtmd_repl( buf, bcv->meta );
                  bufr_print_rtmd_location( buf, show_locdesc, bcv->meta );
                  }
               else if (show_loctime)
                  bufr_print_rtmd_data( buf, bcv->meta );
               else
                  bufr_print_rtmd_repl( buf, bcv->meta );
               bufr_print_output( buf );
               bufr_print_output( " " );
               }

            if (bcv->value) /* If this descriptor has a value */
               {
               if (bcv->value->af)  /* If there are Associated Fields */
                  {
                  BufrAF *af = bcv->value->af;
                  sprintf( buf, "(0x%llx:%d bits)", af->bits, af->nbits );
                  bufr_print_output( buf );
                  }

               if (bcv->value->type == VALTYPE_INT32)
                  {
                  int32_t value = bufr_descriptor_get_ivalue( bcv );
                  if ( value == -1 )
                     bufr_print_output( "MSNG" );
                  else
                     {
                     if (bcv->encoding.type == TYPE_FLAGTABLE)
                        {
                        char str[256];
                        bufr_print_binary( str, value, bcv->encoding.nbits );
                        sprintf( buf, "BITS=%s ", str );
                        }
                     else
                        {
                        sprintf( buf, "VALUE=%d ", value );
                        }
                     bufr_print_output( buf );
                     }
                  }
               else if (bcv->value->type == VALTYPE_INT64)
                  {
                  int64_t value = bufr_descriptor_get_ivalue( bcv );
                  if ( value == -1 )
                     bufr_print_output( "MSNG" );
                  else
                     {
                     sprintf( buf, "VALUE=%lld ", value );
                     bufr_print_output( buf );
                     }
                  }
               else if (bcv->value->type == VALTYPE_FLT32)
                  {
                  float value = bufr_descriptor_get_fvalue( bcv );

                  if (bufr_is_missing_float( value ))
                     bufr_print_output( "MSNG" );
                  else
                     {
                     bufr_print_output( "VALUE=" );
                     bufr_print_dscptr_value( buf, bcv );
                     bufr_print_output( buf );
                     }
                  }
               else if (bcv->value->type == VALTYPE_FLT64)
                  {
                  double value = bufr_descriptor_get_dvalue( bcv );

                  if (bufr_is_missing_double( value ))
                     bufr_print_output( "MSNG" );
                  else
                     {
                     bufr_print_output( "VALUE=" );
                     bufr_print_dscptr_value( buf, bcv );
                     bufr_print_output( buf );
                     }
                  }
               else if (bcv->value->type == VALTYPE_STRING)
                  {
                  int   len;

                  char *str = bufr_descriptor_get_svalue( bcv, &len );
                  sprintf( buf, "VALUE=%s", str );
                  bufr_print_output( buf );
                  }
               if (show_unitdesc)
                  {
                  int desc = ( bcv->s_descriptor != 0 ) ? bcv->s_descriptor : bcv->descriptor;
                  EntryTableB *tb = bufr_fetch_tableB( tables, desc );
                  if ( tb )
                     {
                     sprintf( buf, "(%s){%s}", tb->unit, tb->description );
                     bufr_print_output( buf );
                     }
                  }
               }
            }
         bufr_print_output( "\n" );
         }
      bufr_print_output( "\n" );
      }
   }

static void bufr_show_dataset_formatted( BUFR_Dataset *dts, BUFR_Tables *tables )
   {
   DataSubset     *subset;
   int             sscount, cvcount;
   int             i, j;
   char            buf[256];
   char            buf2[300];
   char            fmt[64];
   BufrDescriptor *bcv;
   int             maxlen, len;

   sscount = bufr_count_datasubset( dts );
   for (i = 0; i < sscount ; i++)
      {
      subset = bufr_get_datasubset( dts, i );
      cvcount = bufr_datasubset_count_descriptor( subset );

      sprintf( buf, "## Subset %d : %d descriptors\n", i+1, cvcount );
      bufr_print_output( buf );
/*
 * precalculates how long RTMD string will be
 */
      maxlen = 0;
      for (j = 0; j < cvcount ; j++)
         {
         bcv = bufr_datasubset_get_descriptor( subset, j );
         if ( bcv->meta && show_meta )
            {
            buf[0] = '\0';
            if (show_locdesc)
               {
               bufr_print_rtmd_repl( buf, bcv->meta );
               bufr_print_rtmd_location( buf, show_locdesc, bcv->meta );
               }
            else if (show_loctime)
               bufr_print_rtmd_data( buf, bcv->meta );
            else
               bufr_print_rtmd_repl( buf, bcv->meta );
            len = strlen(buf);
            if (len > maxlen) maxlen = len;
            }
         }
      fmt[0] = '%';
      fmt[1] = '-';
      sprintf( fmt+2, "%ds ", maxlen );

      for (j = 0; j < cvcount ; j++)
         {
         bcv = bufr_datasubset_get_descriptor( subset, j );
         if (bcv->flags & FLAG_SKIPPED)
            {
            sprintf( buf, "# %.6d ", bcv->descriptor );
            sprintf( buf2, "%-18s", buf );
            bufr_print_output( buf2 );
            if ( bcv->meta && show_meta )
               {
               if (show_locdesc)
                  {
                  bufr_print_rtmd_repl( buf, bcv->meta );
                  bufr_print_rtmd_location( buf, show_locdesc, bcv->meta );
                  }
               else if (show_loctime)
                  bufr_print_rtmd_data( buf, bcv->meta );
               else
                  bufr_print_rtmd_repl( buf, bcv->meta );
               sprintf( buf2, fmt, buf );
               bufr_print_output( buf2 );
               }
            }
         else
            {
            int desc;
            EntryTableB *tb;

            if ( bcv->s_descriptor != 0 )
               {
               sprintf( buf, "  %.6d {%.6d}", 
                     bcv->descriptor, bcv->s_descriptor );
               desc = bcv->s_descriptor;
               }
            else
               {
               sprintf( buf, "  %.6d", bcv->descriptor );
               desc = bcv->descriptor;
               }

            sprintf( buf2, "%-18s", buf );
            bufr_print_output( buf2 );

            if ( show_meta )
               {
               if ( bcv->meta )
                  {
                  if (show_locdesc)
                     {
                     bufr_print_rtmd_repl( buf, bcv->meta );
                     bufr_print_rtmd_location( buf, show_locdesc, bcv->meta );
                     }
                  else if (show_loctime)
                     bufr_print_rtmd_data( buf, bcv->meta );
                  else
                     bufr_print_rtmd_repl( buf, bcv->meta );
                  sprintf( buf2, fmt, buf );
                  bufr_print_output( buf2 );
                  }
               else
                  {
                  sprintf( buf2, fmt, " " );
                  bufr_print_output( buf2 );
                  }
               }

            if (bufr_is_table_b( desc ))
               {
               tb = bufr_fetch_tableB( tables, desc );
               if ( tb )
                  {  /* according to descriptor 13+14=>64 15=>24 */
                  sprintf( buf, "%-64s %-24s", tb->description, tb->unit );
                  bufr_print_output( buf );
                  }
               else
                  {
                  sprintf( buf, "Error: descriptor not found in table B {%d}", desc );
                  bufr_print_output( buf );
                  }
               }

            if (bcv->value) /* If this descriptor has a value */
               {
               if (bcv->value->af)  /* If there are Associated Fields */
                  {
                  BufrAF *af = bcv->value->af;
                  sprintf( buf, "(0x%llx:%d bits)", af->bits, af->nbits );
                  bufr_print_output( buf );
                  }

               if (bcv->value->type == VALTYPE_INT32)
                  {
                  int32_t value = bufr_descriptor_get_ivalue( bcv );
                  if ( value == -1 )
                     bufr_print_output( "MSNG" );
                  else
                     {
                     if (bcv->encoding.type == TYPE_FLAGTABLE)
                        {
                        char str[256];
                        bufr_print_binary( str, value, bcv->encoding.nbits );
                        sprintf( buf, "%-35s ", str );
                        }
                     else
                        {
                        sprintf( buf, "%d ", value );
                        }
                     bufr_print_output( buf );
                     }
                  }
               else if (bcv->value->type == VALTYPE_INT64)
                  {
                  int64_t value = bufr_descriptor_get_ivalue( bcv );
                  if ( value == -1 )
                     bufr_print_output( "MSNG" );
                  else
                     {
                     sprintf( buf, "%lld ", value );
                     bufr_print_output( buf );
                     }
                  }
               else if (bcv->value->type == VALTYPE_FLT32)
                  {
                  float value = bufr_descriptor_get_fvalue( bcv );

                  if (bufr_is_missing_float( value ))
                     bufr_print_output( "MSNG" );
                  else
                     {
                     bufr_print_dscptr_value( buf, bcv );
                     bufr_print_output( buf );
                     }
                  }
               else if (bcv->value->type == VALTYPE_FLT64)
                  {
                  double value = bufr_descriptor_get_dvalue( bcv );

                  if (bufr_is_missing_double( value ))
                     bufr_print_output( "MSNG" );
                  else
                     {
                     bufr_print_dscptr_value( buf, bcv );
                     bufr_print_output( buf );
                     }
                  }
               else if (bcv->value->type == VALTYPE_STRING)
                  {
                  int   len;

                  char *str = bufr_descriptor_get_svalue( bcv, &len );
                  sprintf( buf, "%s", str );
                  bufr_print_output( buf );
                  }
               }
            }
         bufr_print_output( "\n" );
         }
      bufr_print_output( "\n" );
      }
   }
