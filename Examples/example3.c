#include <stdio.h>
#include <stdlib.h>
#include "bufr_api.h"
#include "bufr_local.h"

static void bufr_show_dataset( BUFR_Dataset *dts, char *filename );
/*
 * define these strings and point them to a valid BUFR TABLE B file and TABLE D file
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

      if (bufr_contains_tables( dts ))
         {
         BUFR_Tables   *tbls;

         tbls = bufr_extract_tables( dts );
         if (tbls)
            {
            bufr_merge_tables( tables, tbls );
            bufr_free_tables( tbls );
            }
         bufr_free_dataset( dts );
         bufr_free_message( msg );
         continue;
         }
/*
 * dump the content of the Message into a file
 */
      sprintf( filename, "./OUTPUT-%d.TXT", count );
      bufr_show_dataset( dts, filename );
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

/*
 * print out the content of a Dataset by going through each
 * DataSubset contained inside it.
 */
static void bufr_show_dataset( BUFR_Dataset *dts, char *filename )
   {
   DataSubset    *subset;
   int            sscount, cvcount;
   int            i, j;
   BufrCode      *bcv;
   char           buf[256];
   FILE          *fp;

   fp = fopen( filename, "w" );
   if (fp == NULL)
      {
      fprintf( stderr, "Error: can't open the output file: %s\n", filename );
      exit(-1);
      }
/*
 * see how many DataSubset are present in the Dataset, this is an extract from section3
 * but actually kept as an array size
 */
   sscount = bufr_count_datasubset( dts );
/*
 * loop through each of them
 */
   for (i = 0; i < sscount ; i++)
      {
/*
 * get a pointer of a DataSubset of each
 */
      subset = bufr_get_datasubset( dts, i );
/*
 * obtain the number of codes present in a Subset
 */
      cvcount = bufr_datasubset_count_code( subset );

      fprintf( fp, "## Subset %d : %d codes\n", i+1, cvcount );

      for (j = 0; j < cvcount ; )
         {
/*
 * obtain a pointer to a BufrCode structure which contains the descriptor and value
 */
         bcv = bufr_datasubset_next_code( subset, &j );
         if (bcv == NULL)
            break;
         if (bcv->flags & FLAG_SKIPPED)
            {
            fprintf( fp, "#  %.6d ", bcv->descriptor );
            }
         else
            {
               fprintf( fp, "   %.6d ", bcv->descriptor );
/*
 * if there is a alias descriptor (Data Present Bitmap Operator) 
 */
               if ( bcv->s_descriptor != 0 )
                  {
                  fprintf( fp, "{%.6d}", bcv->s_descriptor );
                  }
/*
 * if there are Meta Data defined
 */
            if ( bcv->meta )
               {
/*
 * print the Meta Data  associated with this BufrCode into a string (readable format)
 * and save it to output
 */
               bufr_print_metadata( buf, bcv->meta );
               fprintf( fp, "%s", buf );
               }

/*
 * some code has no value, 
 * those that have one, print it
 */
               if (bcv->value) /* If this code has a value */
                  {
/* 
 * If there are Associated Fields 
 */
                  if (bcv->value->af)  
                     {
                     BufrAF *af = bcv->value->af;
                     fprintf( fp, "(0x%llx:%d bits)", af->bits, af->nbits );
                     }
/*
 * depending on the datatype assigned to the value
 * obtain the value accordingly 
 */
                  if (bcv->value->type == VALTYPE_INT32)
                     {
                     int32_t value = bufr_code_get_ivalue( bcv );
                     fprintf( fp, "VALUE=%d ", value );
                     }
                  else if (bcv->value->type == VALTYPE_INT64)
                     {
                     int64_t value = bufr_code_get_ivalue( bcv );
                     fprintf( fp, "VALUE=%lld ", value );
                     }
                  else if (bcv->value->type == VALTYPE_FLT32)
                     {
                     float value = bufr_code_get_fvalue( bcv );
   
                     if (bufr_is_missing_float( value ))
                        fprintf( fp, "MSNG" );
                     else
                        {
                        fprintf( fp, "VALUE=%E", value );
                        }
                     }
                  else if (bcv->value->type == VALTYPE_FLT64)
                     {
                     double value = bufr_code_get_dvalue( bcv );
   
                     if (bufr_is_missing_double( value ))
                        fprintf( fp, "MSNG" );
                     else
                        {
                        fprintf( fp, "VALUE=%E", value );
                        }
                     }
                  else if (bcv->value->type == VALTYPE_STRING)
                     {
                     int   len;
   
                     char *str = bufr_code_get_svalue( bcv, &len );
                     fprintf( fp, "VALUE=%s", str );
                     }
                  }
               }
            fprintf( fp, "\n" );
            }
         fprintf( fp, "\n" );
         }
   fclose ( fp );
   }
