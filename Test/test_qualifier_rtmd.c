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
#include <assert.h>
#include <locale.h>

#include "bufr_api.h"

static void my_abort( const char* msg ) {
	fprintf(stderr,"%s\n", msg );
	exit(0);
}

static void set_value( DataSubset* dss, int desc, int start, int value )
	{
		int n = bufr_subset_find_descriptor( dss, desc, start );
		if ( n >= 0)
			{
			BufrDescriptor* bcv = bufr_datasubset_get_descriptor( dss, n );
			if( bcv )
				{
				bufr_descriptor_set_ivalue( bcv, value );
				}
			}
	}

static void cancel_value( DataSubset* dss, int desc, int start )
	{
		int n = bufr_subset_find_descriptor( dss, desc, start );
		if ( n >= 0)
			{
			BufrDescriptor* bcv = bufr_datasubset_get_descriptor( dss, n );
			if( bcv )
				{
				bufr_descriptor_set_ivalue( bcv, bufr_missing_int() );
				}
			}
	}

int main(int argc, char *argv[])
   {
	const char sect2[] = "Hi Yves!";
   BUFR_Tables   *tables=NULL;
	char msgstr[4096];
	ssize_t msglen = 0;
	int i;

   bufr_begin_api();
	bufr_set_verbose( 1 );
	bufr_set_debug( 1 );

	putenv("BUFR_TABLES=../Tables/");

   tables = bufr_create_tables();
   bufr_load_cmc_tables( tables );  

	bufr_set_abort( my_abort );
	bufr_set_debug_file( "test_qualifier_rtmd.DEBUG" );
	bufr_set_output_file( "test_qualifier_rtmd.OUTPUT" );

	/* encode a message with a section 2 content...
	 */
	{
		int n;
		BUFR_Message* msg;
		BUFR_Dataset* dts;
		BufrDescValue bdv;
		DataSubset    *dss;
		BUFR_Template* tmpl = bufr_create_template( NULL, 0, tables, 3 );
		assert( tmpl != NULL );
		
		/* we want to be able to see qualifiers, and to make sure they
		 * are cancelled properly.
		 */
		bufr_init_DescValue( &bdv );
		bdv.descriptor = 301013;
		bufr_template_add_DescValue( tmpl, &bdv, 1 );

		bufr_init_DescValue( &bdv );
		bdv.descriptor = 12001;
		bufr_template_add_DescValue( tmpl, &bdv, 1 );

		bufr_init_DescValue( &bdv );
		bdv.descriptor = 301013;
		bufr_template_add_DescValue( tmpl, &bdv, 1 );

		bufr_init_DescValue( &bdv );
		bdv.descriptor = 12001;
		bufr_template_add_DescValue( tmpl, &bdv, 1 );

		bufr_finalize_template( tmpl );

		dts = bufr_create_dataset(tmpl);
		assert(dts != NULL);

		n = bufr_create_datasubset(dts);
		assert( n >= 0 );
		bufr_expand_datasubset(dts,n);

		dss = bufr_get_datasubset( dts, n );
		assert( dss != NULL );

		set_value( dss, 4004, 0, 3 );
		set_value( dss, 4005, 0, 11 );
		set_value( dss, 4006, 0, 20 );
		set_value( dss, 12001, 0, 279 );
		cancel_value( dss, 4004, 5 );
		set_value( dss, 4005, 5, 10 );
		set_value( dss, 4006, 5, 1 );
		set_value( dss, 12001, 5, 291 );

		msg = bufr_encode_message(dts,0);
		assert( msg != NULL );

		msglen = bufr_memwrite_message( msgstr, sizeof(msgstr), msg);
		assert( msglen >= 0 );

		bufr_free_message( msg );
		bufr_free_dataset( dts );
		bufr_free_template( tmpl );
	}

	{
		BUFR_Message* msg = NULL;
		assert( bufr_memread_message(msgstr,msglen,&msg) > 0 );

		BUFR_Dataset* dts = bufr_decode_message(msg, tables);
		assert( dts != NULL );

		for( i=0; i<bufr_count_datasubset(dts); i++ )
			{
			DataSubset    *dss;
			BufrDescriptor* bcv;
			int n, len;
			const char* s;

			/* need this to get at qualifier data */
			bufr_expand_datasubset( dts, i );

			dss = bufr_get_datasubset( dts, i );
			assert( dss != NULL );

			/* first instance */
			n = bufr_subset_find_descriptor( dss, 12001, 0 );
			assert( n >= 0 );
			bcv = bufr_datasubset_get_descriptor( dss, n );
			assert(bcv != NULL);
			assert(bcv->meta != NULL);
			assert(bcv->meta->nb_qualifiers == 3);
			assert(bufr_descriptor_get_ivalue(bcv->meta->qualifiers[0])==3);
			assert(bufr_descriptor_get_ivalue(bcv->meta->qualifiers[1])==11);
			assert(bufr_descriptor_get_ivalue(bcv->meta->qualifiers[2])==20);

			/* second instance */
			n = bufr_subset_find_descriptor( dss, 12001, 6 );
			assert( n >= 0 );
			bcv = bufr_datasubset_get_descriptor( dss, n );
			assert(bcv != NULL);
			assert(bcv->meta != NULL);
			assert(bcv->meta->nb_qualifiers == 2);	/* 4004 was cancelled */

			assert(bufr_descriptor_get_ivalue(bcv->meta->qualifiers[0])==10);
			assert(bufr_descriptor_get_ivalue(bcv->meta->qualifiers[1])==1);
			}

		bufr_free_message( msg );
	}

   bufr_free_tables( tables );
	exit(0);
   }

