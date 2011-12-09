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

static int set_value( DataSubset* dss, int desc, int start, int value )
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
	return n;
	}

static int isminute( void* i, BufrDescriptor* bv )
	{
	int m = (int)i;
	BufrDescriptor* minute;
	if( bv->meta == NULL ) return -1;
	minute = bufr_fetch_rtmd_qualifier( 4005, bv->meta );
	if( minute == NULL ) return -1;
	return bufr_descriptor_get_ivalue(minute) != m;
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
	bufr_set_debug_file( "test_find.DEBUG" );
	bufr_set_output_file( "test_find.OUTPUT" );

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

		/* lat/lon (location code) */
		bufr_init_DescValue( &bdv );
		bdv.descriptor = 301023;
		bufr_template_add_DescValue( tmpl, &bdv, 1 );

		/* lat increment */
		bufr_init_DescValue( &bdv );
		bdv.descriptor = 5012;
		bufr_template_add_DescValue( tmpl, &bdv, 1 );

		/* need replication to make TLC stuff "happen" */
		bufr_init_DescValue( &bdv );
		bdv.descriptor = 102003;
		bufr_template_add_DescValue( tmpl, &bdv, 1 );

		/* hour/minute/second */
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

		/* this is a bit of a hodge-podge... */
		n = set_value( dss, 4004, 0, 3 );
		n = set_value( dss, 4005, 0, 11 );
		n = set_value( dss, 4005, n+1, 12 );
		n = set_value( dss, 4005, n+1, 13 );
		n = set_value( dss, 4006, 0, 20 );
		n = set_value( dss, 5002, 0, 44 );
		n = set_value( dss, 6002, 0, -77 );
		n = set_value( dss, 5012, 0, 10 );
		n = set_value( dss, 12001, 0, 279 );
		n = set_value( dss, 12001, n+1, 291 );
		n = set_value( dss, 12001, n+1, 45 );

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
			BufrValue* bv;
			int k, n, len;
			const char* s;
			float          fvalues[5];
			int            ivalues[5];
			BufrDescValue  codes[10];

			/* need this to get at qualifier data */
			bufr_expand_datasubset( dts, i );

			dss = bufr_get_datasubset( dts, i );
			assert( dss != NULL );

			/* should match the second instance of 12001 */
			k = 0;

			bv = bufr_create_value(VALTYPE_INT32);
			bufr_value_set_int32(bv, 12);
			bufr_set_key_qualifier( &(codes[k++]), 4005, bv );
			bufr_free_value(bv);

			bufr_set_key_int32( &(codes[k++]), 12001, NULL, 0 );

			n = bufr_subset_find_values( dss, codes, k, 0 );
			assert( n >= 0 );
			bcv = bufr_datasubset_get_descriptor( dss, n );
			assert(bcv != NULL);
			assert( bcv->descriptor == 12001 );
			assert(bufr_descriptor_get_ivalue(bcv)==291);

			for ( i = 0; i < k ; i++ ) bufr_vfree_DescValue( &(codes[i]) );

			/* can't match */
			k = 0;

			bv = bufr_create_value(VALTYPE_INT32);
			bufr_value_set_int32(bv, 19);
			bufr_set_key_qualifier( &(codes[k++]), 4005, bv );
			bufr_free_value(bv);

			bufr_set_key_int32( &(codes[k++]), 12001, NULL, 0 );

			n = bufr_subset_find_values( dss, codes, k, 0 );
			assert( n < 0 );

			for ( i = 0; i < k ; i++ ) bufr_vfree_DescValue( &(codes[i]) );

			/* matches the first instance */
			k = 0;
			bufr_set_key_int32( &(codes[k++]), 12001, NULL, 0 );

			n = bufr_subset_find_values( dss, codes, k, 0 );
			assert( n >= 0 );
			bcv = bufr_datasubset_get_descriptor( dss, n );
			assert(bcv != NULL);
			assert( bcv->descriptor == 12001 );
			assert(bufr_descriptor_get_ivalue(bcv)==279);

			for ( i = 0; i < k ; i++ ) bufr_vfree_DescValue( &(codes[i]) );

			/* also can't match */
			k = 0;

			bv = bufr_create_value(VALTYPE_INT32);
			bufr_value_set_int32(bv, 12);
			bufr_set_key_qualifier( &(codes[k++]), 4005, bv );
			bufr_free_value(bv);

			ivalues[0] = 279;
			bufr_set_key_int32( &(codes[k++]), 12001, ivalues, 1 );

			n = bufr_subset_find_values( dss, codes, k, 0 );
			assert( n < 0 );

			for ( i = 0; i < k ; i++ ) bufr_vfree_DescValue( &(codes[i]) );

			/* should match the second instance of 12001, again, but
			 * this time with a callback-based test.
			 */
			k = 0;

			bufr_set_key_int32( &(codes[k++]), 12001, NULL, 0 );
			bufr_set_key_meta_callback( &(codes[k++]),isminute,(void*)12);

			n = bufr_subset_find_values( dss, codes, k, 0 );
			assert( n >= 0 );
			bcv = bufr_datasubset_get_descriptor( dss, n );
			assert(bcv != NULL);
			assert( bcv->descriptor == 12001 );
			assert(bufr_descriptor_get_ivalue(bcv)==291);

			for ( i = 0; i < k ; i++ ) bufr_vfree_DescValue( &(codes[i]) );

			}
		bufr_free_message( msg );
	}

   bufr_free_tables( tables );
	exit(0);
   }

