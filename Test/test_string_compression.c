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

void add_string_to_dataset( BUFR_Dataset* dts, int dnum,
	int desc, const char* str)
	{
	DataSubset    *dss;
	BufrDescriptor* bcv;
	int n = dnum;

	dss = bufr_get_datasubset( dts, n );
	if( dss == NULL )
		{
		int n = bufr_create_datasubset(dts);
		assert( n >= 0 );
		assert( n == dnum );
		dss = bufr_get_datasubset( dts, n );
		}
	assert( dss != NULL );

	n = bufr_subset_find_descriptor( dss, desc, 0 );
	assert( n >= 0 );
	bcv = bufr_datasubset_get_descriptor( dss, n );
	assert(bcv != NULL);
	/* bufr_descriptor_set_svalue() has access to the descriptor
	 * encoding information and fill the string with extra spaces.
	 * bufr_value_set_string() doesn't, so it tests the bug
	 * better
	 */
	/* bufr_descriptor_set_svalue( bcv, str ); */
	bufr_value_set_string( bcv->value, str, strlen(str) );
	}

const char* rstring(int len)
	{
	static char buf[1024];
	int i;
	for (i=0; i<len; i++)
		{
		buf[i] = 'A' + rand() % ('Z'-'A');
		}
	buf[i] = 0;
	return buf;
	}

int main(int argc, char *argv[])
   {
	const char* message = "Hi Yves!";
   BUFR_Tables   *tables=NULL;
	char msgstr[4096];
	ssize_t msglen = 0;
	FILE* fp;
	int i;

	srand(901);

   bufr_begin_api();
	bufr_set_verbose( 1 );
	bufr_set_debug( 1 );

	putenv("BUFR_TABLES=../Tables/");

   tables = bufr_create_tables();
   bufr_load_cmc_tables( tables );  

	bufr_set_abort( my_abort );
	bufr_set_debug_file( "test_string_compression.DEBUG" );
	bufr_set_output_file( "test_string_compression.OUTPUT" );

	/* encode a message with a section 2 content...
	 */
	{
		BUFR_Message* msg;
		BUFR_Dataset* dts;
		BufrDescValue bdv;
		BUFR_Template* tmpl = bufr_create_template( NULL, 0, tables, 3 );
		assert( tmpl != NULL );
		
		bufr_init_DescValue( &bdv );
		bdv.descriptor = 1011;	/* 9 chars */
		bufr_template_add_DescValue( tmpl, &bdv, 1 );
		bdv.descriptor = 1015;	/* 20 chars */
		bufr_template_add_DescValue( tmpl, &bdv, 1 );
		bufr_finalize_template( tmpl );

		dts = bufr_create_dataset(tmpl);
		assert(dts != NULL);

		for (i=0; i<10; i++)
			{
			add_string_to_dataset(dts, i, 1011, message);
			add_string_to_dataset(dts, i, 1015, rstring(1+rand()%20));
			}

		msg = bufr_encode_message(dts,1);
		assert( msg != NULL );

		msglen = bufr_memwrite_message( msgstr, sizeof(msgstr), msg);
		assert( msglen >= 0 );

		/* keep a copy in a file... */
		fp = fopen("test_string_compression.BUFR", "w");
		bufr_write_message( fp, msg );
		fclose(fp);

		bufr_free_message( msg );
		bufr_free_dataset( dts );
		bufr_free_template( tmpl );
	}

	/* ...and then ensure we can decode it again. */
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

			dss = bufr_get_datasubset( dts, i );
			assert( dss != NULL );

			n = bufr_subset_find_descriptor( dss, 1011, 0 );
			assert( n >= 0 );
			bcv = bufr_datasubset_get_descriptor( dss, n );
			assert(bcv != NULL);
			s = bufr_value_get_string(bcv->value,&len);
			assert(s != NULL);
			assert(!strncmp(s,message,strlen(message)));
			assert(strspn(&s[strlen(message)]," ") == len-strlen(message));
			}

		bufr_free_message( msg );
	}

   bufr_free_tables( tables );
	exit(0);
   }

