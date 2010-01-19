/**
@example decode_multi_message.c
@english
Multiple BUFR messages in a file where one dataset equals one message

@verbatim
BUFR_TABLES=../Tables/ ./decode_multi_message ../Test/BUFR/iusd40_okli.bufr 
@endverbatim

This generates multiple OUTPUT-n.TXT files, each file for a separate BUFR message as found in
Test/BUFR/iusd40_okli.bufr.
@endenglish
@francais
@todo decode_multi_message.c description should be translated
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
   char           filename[256];

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
 * BUFR_Message ==> BUFR_Dataset 
 * decode the message using the BUFR Tables
 * the decoded message is stored in BUFR_Dataset *dts structure
 * a new instance of BUFR_Dataset is assigned to dts at each invocation
 */
      dts = bufr_decode_message( msg, tables ); 
      if (dts == NULL) 
         {
         fprintf( stderr, "Error: can't decode messages #%d\n", count );
         continue;
         }
/*
 * dump the content of the Message into a file
 */
      sprintf( filename, "./OUTPUT-%d.TXT", count );
      bufr_dump_dataset( dts, filename );
/*
 * discard the message
 */
      bufr_free_message( msg );
/*
 * discard an instance of the Decoded message pointed to by dts
 */
      bufr_free_dataset( dts );
      }
/*
 * close all file and cleanup
 */
   fclose( fp );

   bufr_free_tables( tables );
   }

