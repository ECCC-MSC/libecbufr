/**
@example decode_random.c
@english
Non-sequential decode of a set BUFR messages (e.g. to decode some
elements)

@verbatim
BUFR_TABLES=../Tables ./decode_random ../Test/BUFR/iusd40_okli.bufr
@endverbatim

@todo need a BUFR example with the 33045 sequence actually being used

@endenglish
@francais
@todo decode_random.c description should be translated
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
   BUFR_Dataset   *dts;
   FILE           *fp;
   BUFR_Tables    *tables=NULL;
   char           buf[256];
   BUFR_Message   *msg;
   int            rtrn;
   int            count;
   BufrDescValue  codes[10];
   DataSubset     *subset;
   BufrDescriptor *bcv;
   int            i, k, pos;
   float          fvalues[5];
   int            ivalues[5];
   float          fval;
   double         dval;
   int            ival;
   int            startpos;

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
 * initializing search keys array
 */
   for ( i = 0; i < 10 ; i++ )
      bufr_init_DescValue( &(codes[i]) );
/*
 * read a message from the input file
 */
   while ( bufr_read_message( fp, &msg ) > 0 )
      {
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
    * suppose there is only 1 datasubset here, we get a pointer to it
    */
      subset = bufr_get_datasubset( dts, 0 );
   /*
    * we are fetching 11002 which is wind speed starting at beginning
    */
      k = 0;
      pos = bufr_subset_find_descriptor( subset, 11002, 0 );
      if ( pos >= 0)
         {
         bcv = bufr_datasubset_get_descriptor( subset, pos );
			if( bufr_value_is_missing( bcv->value ) )
				{
				fprintf( stdout, "wind speed=(missing)\n" );
				}
			else
				{
				dval = bufr_descriptor_get_dvalue( bcv );
				fprintf( stdout, "wind speed=%g\n", dval );
				}
			}

   /*
    * now we look for PROBABILITY OF FOLLOWING EVENT (33045)
    * which is a sequence of codes with particular values set
    * with whatever value it is "NULL" at the 3rd argument
    */
      k = 0;
      bufr_set_key_int32( &(codes[k++]), 33045, NULL, 0 );
      /* define an event, Table C ed. 4 */
      bufr_set_key_int32( &(codes[k++]), 241000, NULL, 0 ); 
      /* time period or displacement of 6 hour */
      fvalues[0] = 6;
      bufr_set_key_flt32( &(codes[k++]), 4024, fvalues, 1 ); 
      /* type of limit represented by following value, 1 is inclusive lower limit */
      ivalues[0] = 1;
      bufr_set_key_int32( &(codes[k++]), 33042, ivalues, 1 ); 
      /* witch an amount of 0.2 total precipitation */
      fvalues[0] = 0.2;
      bufr_set_key_flt32( &(codes[k++]), 13011, fvalues, 0 );
      /* Table C ed.4 : end of define event */
      bufr_set_key_int32( &(codes[k++]), 241255, NULL, 0 );
   
      startpos = 0;
      pos = bufr_subset_find_values( subset, codes, k, startpos );
   /*
    * the return position is the starting point of the sequence
    */
      if ( pos >= 0)
         {
         EntryTableB *tb;
         bcv = bufr_datasubset_get_descriptor( subset, pos );
         ival = bufr_descriptor_get_ivalue( bcv );
			if( bufr_value_is_missing( bcv->value) )
				{
				fprintf( stdout, "probability = (missing)\n");
				}
			else
				{
				tb = bufr_fetch_tableB( tables, bcv->descriptor );
				fprintf( stdout, "probability = %d %s\n", ival, tb->unit );
				}
         }
   
   /*
    * free all allocations of search keys' values 
    */
      for ( i = 0; i < 10 ; i++ )
         bufr_vfree_DescValue( &(codes[i]) );
   /*
    * discard the message
    */
      bufr_free_message( msg );
      bufr_free_dataset( dts );
   
      }
   
   /*
    * close all file and cleanup
    */
      fclose( fp );
   
      bufr_free_tables( tables );
      }
   
