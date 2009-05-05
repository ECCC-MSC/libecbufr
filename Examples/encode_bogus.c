/**
@example encode_bogus.c
@english
Encoding a message from scratch whilst using a Template with bogus
values inserted

This differs from example 6 in the following. There are five data
subsets created, each by using ?input_bogus_data? using a random
number generator within the minima and maxima set.
@endenglish
@francais
@todo encode_bogus.c description should be translated
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

static void input_bogus_data( BUFR_Dataset *dts, int nbsubset  );

int main(int argc,char *argv[])
   {
   BUFR_Dataset  *dts;
   BUFR_Template *tmplt=NULL;
   FILE  *fpBufr;
   int           verbose;
   char          errmsg[256];
   BUFR_Message *msg;
   BUFR_Tables  *tables;

   if (argc != 2)
      {
      fprintf( stderr, "Usage: %s  template\n", argv[0] );
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
 * insert bogus data in the Dataset with only 5 Subsets.
 */
   input_bogus_data( dts, 5 );

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

static void input_bogus_data( BUFR_Dataset *dts, int nb_subset )
   {
   DataSubset     *subset;
   int             sscount, cvcount;
   int             i, j;
   BufrDescriptor *bcv;
   int             pos;
   double          flmin, flmax;
   int             verbose;
   char            errmsg[256];

/*
 * create as many datasubset as specified
 */
   for (i = 0; i < nb_subset ; i++ )
      bufr_create_datasubset( dts );
/*
 * fill all datasubsets unset values with generated values =(min+max)/2
 */
   flmin = flmax = 0.0;
   sscount = bufr_count_datasubset( dts );
   for (i = 0; i < sscount ; i++)
      {
      subset = bufr_get_datasubset( dts, i );
      cvcount = bufr_datasubset_count_descriptor( subset );
      for (j = 0; j < cvcount ; j++)
         {
         bcv = bufr_datasubset_get_descriptor( subset, j );

         if (bufr_descriptor_get_range( bcv, &flmin, &flmax ) > 0)
            {
            if ((bcv->value->type == VALTYPE_INT32)||(bcv->value->type == VALTYPE_INT8))
               {
               int value = bufr_descriptor_get_ivalue( bcv );
               if (value == -1)
                  {
                  if (bcv->flags & FLAG_CLASS31)
                     {
                     value = 1;
                     }
                  else
                     {
                     value = flmin + (flmax-flmin)/(sscount+2) * (i+1);
                     }
                  bufr_descriptor_set_ivalue( bcv, value );
                  }
               }
            else if ((bcv->value->type == VALTYPE_FLT32)||(bcv->value->type == VALTYPE_FLT64))
               {
               float value = bufr_descriptor_get_fvalue( bcv );
               if (bufr_is_missing_float(value))
                  {
                  value = flmin + (flmax-flmin)/(sscount+2) * (i+1);
                  bufr_descriptor_set_fvalue ( bcv, value );
                  }
               }
            }
/*
 * in case that code is flagged as a CLASS31
 * then it is associated with a delayed replication count which has just been set,
 * then the DataSubset need to be expanded properly
 */
         if (bcv->flags & FLAG_CLASS31)
            {
            bufr_expand_datasubset( dts, i );
            }
         }
      }
   }
