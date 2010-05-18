/***
Copyright Her Majesty The Queen in Right of Canada, Environment Canada, 2009-2010.
Copyright Sa Majesté la Reine du Chef du Canada, Environnement Canada, 2009-2010.

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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <locale.h>

#include "bufr_i18n.h"
#include "bufr_api.h"

#define _(String) gettext(String)
#define _n(String1, String2, n) ngettext(String1, String2, n)
#define N_(String) gettext_noop (String)


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

      sprintf( buf, _n("## Subset %d : %d descriptor\n", "## Subset %d : %d descriptors\n", cvcount), i+1, cvcount );
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
                  sprintf( buf, _n("(0x%llx:%d bit)", "(0x%llx:%d bits)", af->nbits), af->bits, af->nbits );
                  bufr_print_output( buf );
                  }

               if (bcv->value->type == VALTYPE_INT32)
                  {
                  int32_t value = bufr_descriptor_get_ivalue( bcv );
                  if ( value == -1 )
                     bufr_print_output( _("MSNG") );
                  else
                     {
                     if (bcv->encoding.type == TYPE_FLAGTABLE)
                        {
                        char str[256];
                        bufr_print_binary( str, value, bcv->encoding.nbits );
                        sprintf( buf, _("BITS=%s "), str );
                        }
                     else
                        {
                        sprintf( buf, _("VALUE=%d "), value );
                        }
                     bufr_print_output( buf );
                     }
                  }
               else if (bcv->value->type == VALTYPE_INT64)
                  {
                  int64_t value = bufr_descriptor_get_ivalue( bcv );
                  if ( value == -1 )
                     bufr_print_output( _("MSNG") );
                  else
                     {
                     if (bcv->encoding.type == TYPE_FLAGTABLE)
                        {
                        char str[256];
                        bufr_print_binary( str, value, bcv->encoding.nbits );
                        sprintf( buf, _("BITS=%s "), str );
                        }
                     else
                        {
                        sprintf( buf, _("VALUE=%lld "), value );
                        }
                     bufr_print_output( buf );
                     }
                  }
               else if (bcv->value->type == VALTYPE_FLT32)
                  {
                  float value = bufr_descriptor_get_fvalue( bcv );

                  if (bufr_is_missing_float( value ))
                     bufr_print_output( _("MSNG") );
                  else
                     {
                     bufr_print_output( _("VALUE=") );
                     bufr_print_dscptr_value( buf, bcv );
                     bufr_print_output( buf );
                     }
                  }
               else if (bcv->value->type == VALTYPE_FLT64)
                  {
                  double value = bufr_descriptor_get_dvalue( bcv );

                  if (bufr_is_missing_double( value ))
                     bufr_print_output( _("MSNG") );
                  else
                     {
                     bufr_print_output( _("VALUE=") );
                     bufr_print_dscptr_value( buf, bcv );
                     bufr_print_output( buf );
                     }
                  }
               else if (bcv->value->type == VALTYPE_STRING)
                  {
                  int   len;

                  char *str = bufr_descriptor_get_svalue( bcv, &len );
                  if (str && !bufr_is_missing_string( str, len ) )
                     sprintf( buf, _("VALUE=%s"), str );
                  else
                     strcpy( buf, _("VALUE=MSNG") );
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

   //Setup for internationalization
   bufr_begin_api();
   setlocale (LC_ALL, "");
   bindtextdomain ("bufr_test", LOCALEDIR);
   textdomain ("bufr_test");


	putenv("BUFR_TABLES=../Tables/");

   tables = bufr_create_tables();
   bufr_load_cmc_tables( tables );  
   bufr_load_l_tableB( tables, "./local_table_b" );
   bufr_load_l_tableD( tables, "./local_table_d" );

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

		fprintf( stderr, _("Decoding '%s'\n"), argv[n]);

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
				fprintf( stderr, _("Error: can't decode messages\n") );
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
				fprintf(stderr, _("'%s' failed with code %d\n"), cmd, WEXITSTATUS(rc));
				exit(1);
				}
		}

		free( mem );
		}

   bufr_free_tables( tables );
	exit(0);
   }

