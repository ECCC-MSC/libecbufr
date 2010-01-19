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
#include <ctype.h>
#include "bufr_api.h"
#include "bufr_io.h"
#include "bufr_tables.h"
#include "bufr_dataset.h"
#include "bufr_local.h"

#define SET_MSNG_BY_DEF_VALUE    0

/* 
 * Data Compression:
 * -1 for unspecified (default to DATA_FLAG, or no compress if not defined
 *  0 for no compress
 *  1+ try to compress if possible
 */
static int   use_compress= -1;  
static int   save_local_table_B = 1;

static  char *def_str_obufr = "OUT.BUFR";

static  char *str_ltableb = NULL;
static  char *str_ltabled = NULL;
static  char *str_obufr = NULL;
static  int   sequ_desc;
static  char *str_template= NULL;
static  char *str_debug= "DEBUG.encoder";
static  int   nb_subset=1;
static  int   def_values=0;
static  char *str_datafile = NULL;

static BUFR_Tables  *tables = NULL;
/*
 * data structures
 */
static int        edition=4;

static void abort_usage(char *pgrmname);
static void cleanup(void);
static int  read_cmdline( int argc, char *argv[] );

static void run_tests(void);
static void make_test_dataset( BUFR_Dataset *dts );

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
   fprintf( stderr, "BUFR Encoder Version %s\n", BUFR_API_VERSION );
   fprintf( stderr, "Copyright Her Majesty The Queen in Right of Canada, Environment Canada, 2009\n" );
   fprintf( stderr, "Licence LGPLv3\n\n" );
   fprintf( stderr, "Usage: %s\n", pgrmname );
   fprintf( stderr, "          [-outbufr    <filename>]   encoded BUFR file (default=OUT.bufr)\n" );
   fprintf( stderr, "          [-datafile   <filename>]   data file from the dumped output of the decoder\n" );
   fprintf( stderr, "          [-template   <filename>]   template file to use for encoding\n" );
   fprintf( stderr, "          [-ltableb    <filename>]   local table B to use for encoding\n" );
   fprintf( stderr, "          [-ltabled    <filename>]   local table D to use for encoding\n" );
   fprintf( stderr, "          [-edition    <number>]     forcing the use of edition number (no template)\n" );
   fprintf( stderr, "          [-nbsubset   <number>]     specify the number of subset(s) of bogus data to generate\n" );
   fprintf( stderr, "          [-def_values]              use random default values\n" );
   fprintf( stderr, "          [-verbose]                 send more messages\n" );   
   fprintf( stderr, "          [-nolocal]                 do not save local tables to file (default=save)\n" );
   fprintf( stderr, "          [-compress]                compress datasubsets if possible\n" );
   fprintf( stderr, "          [-no_compress]             do not compress datasubsets\n" );
   fprintf( stderr, "          [-debug]                   debug mode (put the messages into file)\n" );
   fprintf( stderr, "          [-sequence   <descriptor>] sequence descriptor from table D\n" );
   fprintf( stderr, "\n  Env. Variables:\n" );
   fprintf( stderr, "     BUFR_TEMPLATE : specify template file\n" );
   fprintf( stderr, "     BUFR_TABLES :   path to BUFR tables in the CMC table format\n" );
   exit(1);
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
   char *env;

   sequ_desc = -1;
   for ( i = 1 ; i < argc ; i++ ) {
     if (strcmp(argv[i],"-ltableb")==0) {
       ++i; if (i >= argc) abort_usage(argv[0]);
       str_ltableb = strdup(argv[i]);
     } else if (strcmp(argv[i],"-ltabled")==0) {
       ++i; if (i >= argc) abort_usage(argv[0]);
       str_ltabled = strdup(argv[i]);
     } else if (strcmp(argv[i],"-outbufr")==0) {
       ++i; if (i >= argc) abort_usage(argv[0]);
       str_obufr = strdup(argv[i]);
     } else if (strcmp(argv[i],"-datafile")==0) {
       ++i; if (i >= argc) abort_usage(argv[0]);
       str_datafile = strdup(argv[i]);
     } else if (strcmp(argv[i],"-template")==0) {
       ++i; if (i >= argc) abort_usage(argv[0]);
       str_template = strdup(argv[i]);
     } else if (strcmp(argv[i],"-edition")==0) {
       ++i; if (i >= argc) abort_usage(argv[0]);
       edition = atoi( argv[i] );
       if ((edition < 2)||(edition > 4))
          edition = 4;
     } else if (strcmp(argv[i],"-nbsubset")==0) {
       ++i; if (i >= argc) abort_usage(argv[0]);
       nb_subset = atoi( argv[i] );
       if (nb_subset <= 0) nb_subset = 1;
     } else if (strcmp(argv[i],"-debug")==0) {
       bufr_set_debug( 1 );
     } else if (strcmp(argv[i],"-compress")==0) {
       use_compress = 1;
     } else if (strcmp(argv[i],"-no_compress")==0) {
       use_compress = 0;
     } else if (strcmp(argv[i],"-def_values")==0) {
       def_values = 1;
     } else if (strcmp(argv[i],"-verbose")==0) {
       bufr_set_verbose( 1 );
     } else if (strcmp(argv[i],"-nolocal")==0) {
       save_local_table_B = 0;
     } else if (strcmp(argv[i],"-sequ")==0) {
       /* -sequ will be deprecated eventually, in favor of -sequence */
       ++i; if (i >= argc) abort_usage(argv[0]);
       sequ_desc = atoi(argv[i]);
     } else if (strcmp(argv[i],"-sequence")==0) {
       ++i; if (i >= argc) abort_usage(argv[0]);
       sequ_desc = atoi(argv[i]);
     }
   }

   if (str_template == NULL)
      {
      env = getenv("BUFR_TEMPLATE");
      if (env != NULL) str_template = strdup(env);
      }

   if (str_obufr == NULL)
      str_obufr = def_str_obufr;
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
   if ((str_obufr!= NULL)&&(str_obufr!=def_str_obufr)) free(str_obufr);
   if (str_template != NULL) free(str_template);
   if (str_datafile != NULL) free( str_datafile );

   bufr_free_tables( tables );
}

/*
 *##############################################################################
 * programme principale
 *
 */
int main(int argc,char *argv[])
{
   

   if (argc == 1)
      {
      abort_usage( argv[0] );
      }

   read_cmdline( argc, argv );

   bufr_set_debug_file( str_debug );

   bufr_begin_api();
/*
 * charger les tables en memoire
 */
   run_tests();

   bufr_end_api();
   cleanup();
   exit(0);
}

static void run_tests(void)
   {
   BUFR_Dataset  *dts;
   BUFR_Template *tmplt=NULL;
   FILE  *fpBufr;
   int           verbose;
   char          errmsg[256];
   BUFR_Message *msg;

   verbose = bufr_is_verbose();
   if (verbose)
      {
      bufr_print_debug( "*\n" );
      sprintf( errmsg, "* BUFR encoder : API version %s\n", BUFR_API_VERSION );
      bufr_print_debug( errmsg );
      bufr_print_debug( "*\n" );
      }
/*
 * load CMC Table B
 */
   tables = bufr_create_tables();
   bufr_load_cmc_tables( tables );
/*
 * load local tables if any 
 */
   if (str_ltableb)
      bufr_load_l_tableB( tables, str_ltableb );
   if (str_ltabled)
      bufr_load_l_tableD( tables, str_ltabled );

/*
 * create a template
 */
   if (str_template)
      {
      tmplt = bufr_load_template( str_template, tables );
      }
   else if (sequ_desc > 0)
      {
      BufrDescValue           descs[2];

      descs[0].descriptor = sequ_desc;
      descs[0].values = NULL;
      descs[0].nbval = 0;
      tmplt = bufr_create_template( descs, 1, tables, edition );
      }

   if (tmplt == NULL)
      {
      fprintf( stderr, "Template not defined properly\n" );
      exit(1);
      }

/*
 * create a dataset
 */
   dts = bufr_create_dataset( tmplt );
   if (dts == NULL)
      {
      fprintf( stderr, "Error: unable to create dataset, abort\n" );
      exit(1);
      }

   if (str_datafile)
      {
      bufr_genmsgs_from_dump( tmplt, str_datafile, str_obufr, use_compress );
      }
   else
      {
      make_test_dataset( dts );

/*
 * open a file for writing
 */
      fpBufr = fopen( str_obufr, "w" );

/*
 * save local table to output BUFR file if requested
 */
      if (save_local_table_B)
         bufr_store_tables( fpBufr, dts );

/*
 * save the dataset to output file
 */
      msg = bufr_encode_message ( dts, use_compress );
      if (msg != NULL)
         {
         bufr_write_message( fpBufr, msg );
         bufr_free_message ( msg );
         }

/*
 * close all files and cleanup
 */
      fclose( fpBufr );
      }

   bufr_free_dataset( dts );
   bufr_free_template( tmplt );

   bufr_print_debug( NULL );
   }

static void make_test_dataset( BUFR_Dataset *dts )
   {
   DataSubset    *subset;
   int           sscount, cvcount;
   int           i, j;
   BufrDescriptor  *bdv;
   int           pos;
   double        flmin, flmax;
   int           verbose;
   char          errmsg[256];

   verbose = bufr_is_verbose();

   for (i = 0; i < nb_subset ; i++ )
      pos = bufr_create_datasubset( dts );
/*
 * fill datasubsets unset values with generated values =(min+max)/2
 */
   flmin = flmax = 0.0;
   sscount = bufr_count_datasubset( dts );
   for (i = 0; i < sscount ; i++)
      {
      subset = bufr_get_datasubset( dts, i );
      cvcount = bufr_datasubset_count_descriptor( subset );
      if (verbose)
         {
         sprintf( errmsg, "Subset #%d : %d descs\n", i+1, cvcount );
         bufr_print_debug( errmsg );
         }
      for (j = 0; j < cvcount ; j++)
         {
         bdv = bufr_datasubset_get_descriptor( subset, j );
         if (verbose)
            {
            if (bdv->flags & FLAG_SKIPPED)
               sprintf( errmsg, "  #%.6d ", bdv->descriptor );
            else
               sprintf( errmsg, "   %.6d ", bdv->descriptor );
            bufr_print_debug( errmsg );
            }

         if (verbose && bdv->meta)
            {
            bufr_print_rtmd_data( errmsg, bdv->meta );
            bufr_print_debug( errmsg );
            }


         if (bufr_descriptor_get_range( bdv, &flmin, &flmax ) > 0)
            {
            if (bdv->value->type == VALTYPE_INT32)
               {
               if (def_values)
                  {
                  int value = bufr_descriptor_get_ivalue( bdv );
                  if (value == -1)
                     {
                     if (bdv->flags & FLAG_CLASS31)
                        {
                        value = 1;
                        }
                     else
                        {
                        value = flmin + (flmax-flmin)/(sscount+2) * (i+1);
                        }
                     bufr_descriptor_set_ivalue( bdv, value );
                     }
                  }
               if (verbose)
                  {
                  if (bufr_print_dscptr_value( errmsg, bdv ))
                     bufr_print_debug( errmsg );
                  }
               if (verbose)
                  {
                  sprintf( errmsg, "[%d,%d] ", (int)flmin, (int)flmax );
                  bufr_print_debug( errmsg );
                  }
               }
            else if (bdv->value->type == VALTYPE_FLT32)
               {
               if (def_values)
                  {
                  float value = bufr_descriptor_get_fvalue( bdv );
                  if (bufr_is_missing_float(value))
                     {
                     value = flmin + (flmax-flmin)/(sscount+2) * (i+1);
                     bufr_descriptor_set_fvalue ( bdv, value );
                     }
                  }
               if (verbose)
                  {
                  if (bufr_print_dscptr_value( errmsg, bdv ))
                     bufr_print_debug( errmsg );
                  }
               if (verbose)
                  {
                  sprintf( errmsg, "[%E,%E] ", flmin, flmax );
                  bufr_print_debug( errmsg );
                  }
               }
            else
               {
               if (verbose)
                  {
                  if (bufr_print_dscptr_value( errmsg, bdv ))
                     bufr_print_debug( errmsg );
                  }
               }
            }
         else
            {
            if (verbose)
               {
               if (bufr_print_dscptr_value( errmsg, bdv ))
                  bufr_print_debug( errmsg );
               }
            }

         if (bdv->flags & FLAG_CLASS31)
            {
            if (bufr_expand_datasubset( dts, i ) >= 0)
               cvcount = bufr_datasubset_count_descriptor( subset );
            }

         if (verbose)
            bufr_print_debug( "\n" );
         }
      if (verbose)
         bufr_print_debug( "\n" );
      }
   }
