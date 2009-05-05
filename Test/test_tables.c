#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <assert.h>

#include "bufr_api.h"

static void my_abort( const char* msg ) {
	fprintf(stderr,"%s\n", msg );
	exit(0);
}

int main(int argc, char *argv[])
   {
   BUFR_Tables   *tables=NULL;
	EntryTableB   *tb = NULL;
	EntryTableD   *td = NULL;
	int            desc[4];

	putenv("BUFR_TABLES=../Tables/");

   tables = bufr_create_tables();
   bufr_load_cmc_tables( tables );  

	bufr_set_abort( my_abort );
	bufr_set_debug_file( "test_mem.DEBUG" );
	bufr_set_output_file( "test_mem.OUTPUT" );

	/* basic smoke testing */
	assert( bufr_is_local_descriptor( 12199 ) );
	assert( !bufr_is_local_descriptor( 12001 ) );
	assert( 3090 == bufr_fxy_to_descriptor( 0, 3, 90 ) );
	assert( 301008 == bufr_fxy_to_descriptor( 3, 1, 8 ) );
	
	/* some testing for Table B queries */
	assert( NULL == bufr_fetch_tableB( NULL, 3090 ) );
	assert( NULL == bufr_fetch_tableB( tables, 0 ) );
	assert( NULL == bufr_fetch_tableB( tables, 311001 ) );

	tb = bufr_fetch_tableB( tables, 1010 );
	assert( tb != NULL );
	assert( tb->descriptor == 1010 );
	assert( tb->description != NULL );
	assert( tb->encoding.type == TYPE_CCITT_IA5 );
	assert( tb->unit != NULL );

	tb = bufr_fetch_tableB( tables, 1001 );
	assert( tb != NULL );
	assert( tb->descriptor == 1001 );
	assert( tb->description != NULL );
	assert( tb->encoding.type == TYPE_NUMERIC );
	assert( tb->unit != NULL );

	/* some testing for Table D queries */
	assert( NULL == bufr_fetch_tableD( NULL, 311001 ) );
	assert( NULL == bufr_fetch_tableD( tables, 0 ) );
	assert( NULL == bufr_fetch_tableD( tables, 1001 ) );

	td = bufr_fetch_tableD( tables, 311001 );
	assert( td != NULL );
	assert( td->descriptor == 311001 );
	assert( td->count == 9 );
	assert( td->descriptors[0] == 301051 );
	assert( td->descriptors[3] == 11001 );
	assert( td->descriptors[8] == 20041 );

	td = bufr_fetch_tableD( tables, 310010 );
	assert( td != NULL );
	assert( td->descriptor == 310010 );
	assert( td->count == 3 );
	assert( td->descriptors[0] == 310011 );
	assert( td->descriptors[1] == 101005 );
	assert( td->descriptors[2] == 310012 );

	desc[0] = 310011;
	desc[1] = 101005;
	desc[2] = 310012;
	assert( NULL == bufr_match_tableD_sequence(NULL, 3, desc) );
	assert( NULL == bufr_match_tableD_sequence(tables, 0, desc) );
	assert( NULL == bufr_match_tableD_sequence(tables, 1, desc) );
	td = bufr_match_tableD_sequence(tables, 3, desc);
	assert( td != NULL );
	assert( td->descriptor == 310010 );
	assert( td->count == 3 );
	assert( td->descriptors[0] == 310011 );
	assert( td->descriptors[1] == 101005 );
	assert( td->descriptors[2] == 310012 );

	desc[0] = 101015;
	assert( NULL == bufr_match_tableD_sequence(tables, 3, desc) );

   bufr_free_tables( tables );
	exit(0);
   }

