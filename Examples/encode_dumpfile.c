/**
@example encode_dumpfile.c
@english
Importing values from text files (dump file) to encode a BUFR message
@endenglish
@francais
@todo encode_dumpfile.c description should be translated
@endfrancais
*/

#include <stdio.h>
#include <stdlib.h>
#include "bufr_api.h"

/*
 * define these string and point them to a valid BUFR TABLE B file and TABLE D file
 * in order to load other local tables
 */
static char *str_ltableb=NULL;
static char *str_ltabled=NULL;

int main(int argc,char *argv[])
   {
   BUFR_Dataset  *dts;
   BUFR_Template *tmplt=NULL;
   FILE  *fpBufr;
   int           verbose;
   char          errmsg[256];
   BUFR_Message *msg;
   BUFR_Tables  *tables;

   if (argc != 3)
      {
      fprintf( stderr, "Usage: %s  template datafile\n", argv[0] );
      exit(-1);
      }
/*
 * load CMC Table B and D
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
 * create a template from a template file
 */
   tmplt = bufr_load_template( argv[1], tables );
   if (tmplt == NULL)
      {
      fprintf( stderr, "Template not defined properly\n" );
      exit(-1);
      }

/*
 * create a dataset from that template
 */
   dts = bufr_create_dataset( tmplt );
   if (dts == NULL)
      {
      fprintf( stderr, "Error: unable to create dataset, abort\n" );
      exit(-1);
      }
/*
 * load a file containing data that is made for that template
 */
   if ( bufr_load_dataset( dts, argv[2] ) <= 0)
      {
      fprintf( stderr, "Error: loading Data file: %s\n", argv[2] );
      exit(-1);
      }
/*
 * open a file for writing the BUFR message
 */
   fpBufr = fopen( "OUTPUT.BUFR", "w" );

/*
 * save the dataset to output file
 */
   msg = bufr_encode_message ( dts, 0 /* no compression */ );
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
   bufr_free_template( tmplt );
   bufr_free_tables( tables );
   }
