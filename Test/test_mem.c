#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "bufr_api.h"

/* snarfed from Utilities/bufr_decoder.c. Should be kept in sync */
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
            if ( bcv->meta )
               {
               bufr_print_rtmd_data( buf, bcv->meta );
               bufr_print_output( buf );
               }
            }
         else
            {
            sprintf( buf, "   %.6d ", bcv->descriptor );
            bufr_print_output( buf );

            if ( bcv->s_descriptor != 0 )
               {
               sprintf( buf, "{%.6d}", bcv->s_descriptor );
               bufr_print_output( buf );
               if ( bcv->meta )
                  bufr_print_output( " " );
               }
            if ( bcv->meta )
               {
               bufr_print_rtmd_data( buf, bcv->meta );
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
                     if (bcv->encoding.type == TYPE_FLAGTABLE)
                        {
                        char str[256];
                        bufr_print_binary( str, value, bcv->encoding.nbits );
                        sprintf( buf, "BITS=%s ", str );
                        }
                     else
                        {
                        sprintf( buf, "VALUE=%lld ", value );
                        }
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
                  if (str && !bufr_is_missing_string( str, len ) )
                     sprintf( buf, "VALUE=%s", str );
                  else
                     strcpy( buf, "VALUE=MSNG" );
                  bufr_print_output( buf );
                  }
					}
            }
         bufr_print_output( "\n" );
         }
      bufr_print_output( "\n" );
      }
   }

static void my_abort( const char* msg ) {
	fprintf(stderr,"%s\n", msg );
	exit(0);
}

int main(int argc, char *argv[])
   {
   BUFR_Dataset  *dts;
   FILE          *fp;
   BUFR_Tables   *tables=NULL;
   char           buf[256];
   BUFR_Message  *msg;
   ssize_t            rtrn;
   int            count;
	struct stat sbuf;
	char *mem = NULL;
	int n;
	int pos = 0;

	putenv("BUFR_TABLES=../Tables/");

   tables = bufr_create_tables();
   bufr_load_cmc_tables( tables );  
   bufr_load_l_tableB( tables, "./local_table_b" );

	bufr_set_abort( my_abort );
	bufr_set_debug_file( "test_mem.DEBUG" );
	bufr_set_output_file( "test_mem.OUTPUT" );

	for( n = 1; argv[n]; n ++ )
		{

		if( stat(argv[n], &sbuf ) )
			{
			perror(argv[n]);
			exit(1);
			}
		if( sbuf.st_size==0 || !S_ISREG(sbuf.st_mode) ) continue;
		
		mem = malloc(sbuf.st_size);
		if( mem == NULL )
			{
			perror(argv[n]);
			exit(1);
			}

		fprintf( stderr, "Decoding '%s'\n", argv[n]);

		fp = fopen( argv[n], "r" );
		if (fp == NULL)
			{
			free(mem);
			bufr_free_tables( tables );
			perror(argv[n]);
			exit(1);
			}

		if( sbuf.st_size != fread( mem, 1, sbuf.st_size, fp ) )
			{
			free(mem);
			bufr_free_tables( tables );
			perror(argv[1]);
			exit(1);
			}

		fclose(fp);

		{
		char out[PATH_MAX * 3];
		char* suffix = strrchr(argv[n],'.');
		if( suffix == NULL || strcmp(suffix,".bufr") ) continue;

		*suffix = 0;	/* NUL terminate the filename */

		snprintf( out, sizeof(out), "%s.memout", argv[n]);
		unlink(out);	/* clear it */
		bufr_set_output_file( out );
		}

		/*
		 * read a message from the input file
		 */
		pos = 0;
		while((rtrn = bufr_memread_message( mem+pos, sbuf.st_size-pos, &msg ))>0 )
			{
			pos += rtrn;

			/* 
			 * BUFR_Message ==> BUFR_Dataset 
			 * decode the message using the BUFR Tables
			 */
			dts = bufr_decode_message( msg, tables ); 
			if (dts == NULL) 
				{
				fprintf( stderr, "Error: can't decode messages\n" );
				bufr_free_message( msg );
				free( mem );
				continue;
				}

			bufr_print_message( msg, bufr_print_output );
			bufr_show_dataset( dts, tables );
			bufr_free_dataset( dts );
			}

		{
			int rc;
			char cmd[PATH_MAX*3];
			bufr_set_output_file( NULL );

			snprintf( cmd, sizeof(cmd), "diff -q -a %s.out %s.memout",
						 argv[n], argv[n]);
			rc = system( cmd );
			if( rc==-1 || WEXITSTATUS(rc) )
				{
				fprintf(stderr, "'%s' failed with code %d\n", cmd, WEXITSTATUS(rc));
				exit(1);
				}
		}

		free( mem );
		}

   bufr_free_tables( tables );
	exit(0);
   }

