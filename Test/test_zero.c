/*
Unit test for "what goes in must come out". The value chosen, 273.15, besides
being the Kelvin-to-Celsius conversion constant, is not directly representable
in 32 or 64 bit IEEE 754 floating point. This means any internal lossiness
(caused, say, by mixing 32 and 64-bit math operations) should be nicely
exposed.
*/

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

int main(int argc, char *argv[])
   {
   BUFR_Tables   *tables=NULL;
	char msgstr[4096];
	ssize_t msglen = 0;
	double kelvin = 273.15;

   bufr_begin_api();
	bufr_set_verbose( 1 );
	bufr_set_debug( 1 );

	putenv("BUFR_TABLES=../Tables/");

   tables = bufr_create_tables();
   bufr_load_cmc_tables( tables );  

	bufr_set_abort( my_abort );
	bufr_set_debug_file( "test_zero.DEBUG" );
	bufr_set_output_file( "test_zero.OUTPUT" );

	{
		double drybulb_c = 0.0;
		double drybulb_k = drybulb_c + kelvin;
		int n;
		BUFR_Message* msg;
		BUFR_Dataset* dts;
		BufrDescValue bdv;
		DataSubset    *dss;
		BUFR_Template* tmpl = bufr_create_template( NULL, 0, tables, 4 );
		assert( tmpl != NULL );
		
		bufr_init_DescValue( &bdv );
		bdv.descriptor = 12101;
		bufr_template_add_DescValue( tmpl, &bdv, 1 );
		bufr_finalize_template( tmpl );

		dts = bufr_create_dataset(tmpl);
		assert(dts != NULL);

		n = bufr_create_datasubset(dts);
		assert( n >= 0 );
		bufr_expand_datasubset(dts,n);

		dss = bufr_get_datasubset( dts, n );
		assert( dss != NULL );

		fprintf(stderr, "writing drybulb_k = %f\n", drybulb_k );

		n = bufr_subset_find_descriptor( dss, 12101, 0 );
		if ( n >= 0)
			{
			BufrDescriptor* bcv = bufr_datasubset_get_descriptor( dss, n );
			assert(bcv != NULL);
			/* zero Celsius */
			bufr_descriptor_set_dvalue( bcv, drybulb_k );
			}

		msg = bufr_encode_message(dts,0);
		assert( msg != NULL );

		msglen = bufr_memwrite_message( msgstr, sizeof(msgstr), msg);
		assert( msglen >= 0 );

		bufr_free_message( msg );
		bufr_free_dataset( dts );
		bufr_free_template( tmpl );
	}

	/* ...and then ensure we can decode it again. */
	{
		int n;
		BUFR_Message* msg = NULL;
		BUFR_Dataset* dts;
		BufrDescValue bdv;
		DataSubset    *dss;
		assert( bufr_memread_message(msgstr,msglen,&msg) > 0 );

		dts = bufr_decode_message( msg, tables ); 
		assert( dts != NULL );

		dss = bufr_get_datasubset( dts, 0 );
		assert( dss != NULL );

		n = bufr_subset_find_descriptor( dss, 12101, 0 );
		if ( n >= 0)
			{
			double drybulb_c, drybulb_k;
			BufrDescriptor* bcv = bufr_datasubset_get_descriptor( dss, n );
			assert(bcv != NULL);

			/* This is the important part. */
			drybulb_k = bufr_descriptor_get_dvalue(bcv);
			fprintf(stderr, "got drybulb_k = %f\n", drybulb_k );

			drybulb_c = drybulb_k - kelvin;

			fprintf(stderr, "ended with drybulb_c = %f\n", drybulb_c );

			assert( drybulb_c == 0.0 );

			}

		bufr_free_dataset( dts );

		bufr_free_message( msg );
	}

   bufr_free_tables( tables );
	exit(0);
   }

