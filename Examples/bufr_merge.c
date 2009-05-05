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

#define SET_MSNG_BY_DEF_VALUE    0

static int   use_compress=0;
static int   save_local_table_B = 1;
static int   cmc_local_codes = 0;

static  char *def_str_obufr = "OUT.BUFR";

static  char *str_ltableb = NULL;
static  char *str_ltabled = NULL;
static  char *str_obufr = NULL;
static  char *str_error_logs = NULL;
static  int   tableD_code;
static  char *str_template= NULL;
static  char *str_output= NULL;
static  char *str_debug= "DEBUG.encoder";
static  int   nb_subset=1;
static  int   def_values=0;

static BUFR_Tables  *tables = NULL;
/*
 * data structures
 */
static int        edition=4;

static void abort_usage(char *pgrmname);
static void cleanup(void);
static int  read_cmdline( int argc, char *argv[] );

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
   fprintf( stderr, "BUFR Encoder Version %s\n", BUFR_API_VERSION );
   fprintf( stderr, "Usage: %s\n", pgrmname );
   fprintf( stderr, "          [-outbufr  <filename>] (default=OUT.bufr)\n" );
   fprintf( stderr, "          [-template <filename>]\n" );

   fprintf( stderr, "          [-ltableb  <filename>]\n" );
   fprintf( stderr, "          [-ltabled  <filename>]\n" );
   fprintf( stderr, "          [-cmclocalcodes] use CMC local codes in tables (default=dont use)\n" );
   fprintf( stderr, "          [-nolocal]  do not save local tables to file (default=save)\n" );
   fprintf( stderr, "          [-compress] compress datasubsets if possible\n" );
   fprintf( stderr, "          [-debug]\n" );
   fprintf( stderr, "  env. Variables:\n" );
   fprintf( stderr, "     BUFR_TEMPLATE : specify template file\n" );
   exit(5);
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

   tableD_code = -1;
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
     } else if (strcmp(argv[i],"-template")==0) {
       ++i; if (i >= argc) abort_usage(argv[0]);
       str_template = strdup(argv[i]);
     } else if (strcmp(argv[i],"-output")==0) {
       ++i; if (i >= argc) abort_usage(argv[0]);
       str_output = strdup(argv[i]);
     } else if (strcmp(argv[i],"-errlog")==0) {
       ++i; if (i >= argc) abort_usage(argv[0]);
       str_error_logs = strdup(argv[i]);
     } else if (strcmp(argv[i],"-edition")==0) {
       ++i; if (i >= argc) abort_usage(argv[0]);
       edition = atol( argv[i] );
       if ((edition < 2)||(edition > 4))
          edition = 4;
     } else if (strcmp(argv[i],"-nbsubset")==0) {
       ++i; if (i >= argc) abort_usage(argv[0]);
       nb_subset = atol( argv[i] );
       if (nb_subset <= 0) nb_subset = 1;
     } else if (strcmp(argv[i],"-debug")==0) {
       bufr_set_debug( 1 );
     } else if (strcmp(argv[i],"-compress")==0) {
       use_compress = 1;
     } else if (strcmp(argv[i],"-def_values")==0) {
       def_values = 1;
     } else if (strcmp(argv[i],"-verbose")==0) {
       bufr_set_verbose( 1 );
     } else if (strcmp(argv[i],"-nolocal")==0) {
       save_local_table_B = 0;
     } else if (strcmp(argv[i],"-cmclocalcodes")==0) {
       cmc_local_codes = 1;
     } else if (strcmp(argv[i],"-tableD")==0) {
       ++i; if (i >= argc) abort_usage(argv[0]);
       tableD_code = atoi(argv[i]);
     }
   }

   if (str_template == NULL)
      {
      env = getenv("BUFR_TEMPLATE");
      if (env != NULL) str_template = strdup(env);
      }

   if (str_error_logs == NULL) str_error_logs = strdup("ERRORS.encoder");

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
   if (str_error_logs != NULL) free( str_error_logs );
   if (str_template != NULL) free(str_template);
   if (str_output != NULL) free(str_output);

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
/*
 * redirect stderr to an error file
 */
/*   freopen ( str_error_logs, "w", stderr ) ; */
   bufr_set_debug_file( str_debug );

   if ( str_output )
      bufr_set_output_file( str_output );

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
   BUFR_Dataset  *dts2;
   DataSubset    *subset;
   int           sscount, cvcount;
   int           i, j;
   BufrDescriptor      *bcv;
   int           pos;
   BUFR_Template *tmplt=NULL;
   FILE  *fpBufr;
   double        flmin, flmax;
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
   else if (tableD_code > 0)
      {
      BufrDescValue           codes[2];

      codes[0].descriptor = tableD_code;
      codes[0].values = NULL;
      codes[0].nbval = 0;
      tmplt = bufr_create_template( codes, 1, tables, edition );
      }

   if (tmplt == NULL)
      {
      fprintf( stderr, "Template not defined\n" );
      exit(5);
      }

/*
 * create a dataset
 */
   dts = bufr_create_dataset( tmplt );
   if (dts == NULL)
      {
      fprintf( stderr, "Error: unable to create dataset, abort\n" );
      exit(5);
      }

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
         sprintf( errmsg, "Subset #%d : %d codes\n", i+1, cvcount );
         bufr_print_debug( errmsg );
         }
      for (j = 0; j < cvcount ; j++)
         {
         bcv = bufr_datasubset_get_descriptor( subset, j );
         if (verbose)
            {
            if (bcv->flags & FLAG_SKIPPED)
               sprintf( errmsg, "  #%.6d ", bcv->descriptor );
            else
               sprintf( errmsg, "   %.6d ", bcv->descriptor );
            bufr_print_debug( errmsg );
            }

         if (verbose && bcv->meta)
            {
            bufr_print_rtmd_data( errmsg, bcv->meta );
            bufr_print_debug( errmsg );
            }


         if (bufr_descriptor_get_range( bcv, &flmin, &flmax ) > 0)
            {
            if (bcv->value->type == VALTYPE_INT32)
               {
               if (def_values)
                  {
                  int value = bufr_descriptor_get_ivalue( bcv );
                  if (value == -1)
                     {
                     value = flmin + (flmax-flmin)/(sscount+2) * (i+1);
                     bufr_descriptor_set_ivalue( bcv, value );
                     }
                  }
               if (verbose)
                  {
                  if (bufr_print_value( errmsg, bcv->value ))
                     bufr_print_debug( errmsg );
                  }
               if (verbose)
                  {
                  sprintf( errmsg, "[%d,%d] ", (int)flmin, (int)flmax );
                  bufr_print_debug( errmsg );
                  }
               }
            else if (bcv->value->type == VALTYPE_FLT32)
               {
               if (def_values)
                  {
                  float value = bufr_descriptor_get_fvalue( bcv );
                  if (bufr_is_missing_float(value))
                     {
                     value = flmin + (flmax-flmin)/(sscount+2) * (i+1);
                     bufr_descriptor_set_fvalue ( bcv, value );
                     }
                  }
               if (verbose)
                  {
                  if (bufr_print_value( errmsg, bcv->value ))
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
                  if (bufr_print_value( errmsg, bcv->value ))
                     bufr_print_debug( errmsg );
                  }
               }
            }
         else
            {
            if (verbose)
               {
               if (bufr_print_value( errmsg, bcv->value ))
                  bufr_print_debug( errmsg );
               }
            }

         if (verbose)
            bufr_print_debug( "\n" );
         }
      if (verbose)
         bufr_print_debug( "\n" );
      }

   dts2 = bufr_create_dataset( tmplt );
   for (i = 0; i < nb_subset ; i++ )
      pos = bufr_create_datasubset( dts2 );

   bufr_merge_dataset( dts2, nb_subset-1, dts, 0, nb_subset );

/*
 * open a file for writing
 */
   fpBufr = fopen( str_obufr, "w" );
   BUFR_SET_ORIG_CENTRE( dts2, 54 ); /* Montreal */

/*
 * save local table to output BUFR file if requested
 */
   if (save_local_table_B)
      bufr_store_tables( fpBufr, dts2 );

/*
 * save the dataset to output file
 */
   msg = bufr_encode_message ( dts2, use_compress );
   if (msg != NULL)
      {
      bufr_write_message( fpBufr, msg );
      bufr_free_message ( msg );
      }

/*
 * close all file and cleanup
 */
   fclose( fpBufr );

   bufr_free_dataset( dts );
   bufr_free_dataset( dts2 );
   bufr_free_template( tmplt );

   bufr_print_debug( NULL );
   }

