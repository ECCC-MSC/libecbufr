/**
@example decode_simple.c
@english
Single BUFR message in dataset and Dumping a message into
a text file.
@endenglish
@francais
@todo decode_simple.c description should be translated
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

int main(int argc, char *argv[])
   {
   BUFR_Dataset  *dts;
   FILE          *fp;
   BUFR_Tables   *tables=NULL;
   char           buf[256];
   BUFR_Message  *msg;
   int            rtrn;
   int            count;

/*
 * load CMC Table B and D
 * includes local descriptors
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
 * open a file for reading
 */
   fp = fopen( argv[1], "r" );
   if (fp == NULL)
      {
      bufr_free_tables( tables );
      fprintf( stderr, "Error: can't open file \"%s\"\n", argv[1] );
      exit(-1);
      }
/*
 * read a message from the input file
 */
   rtrn = bufr_read_message( fp, &msg );
   if (rtrn <= 0)
      {
      fprintf( stderr, "Warning: there are no message\n" );
      exit(0);
      }

/* 
 * BUFR_Message ==> BUFR_Dataset 
 * decode the message using the BUFR Tables
 */
   dts = bufr_decode_message( msg, tables ); 
   if (dts == NULL) 
      {
      fprintf( stderr, "Error: can't decode messages\n" );
      exit(-1);
      }
/*
 * dump the content of the Message into a file
 */
   bufr_dump_dataset( dts, "./OUTPUT.TXT" );

/*
 * discard the message
 */
   bufr_free_message( msg );
   bufr_free_dataset( dts );
/*
 * close all file and cleanup
 */
   fclose( fp );

   bufr_free_tables( tables );
   }

